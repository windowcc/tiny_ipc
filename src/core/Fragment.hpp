#ifndef _IPC_CORE_FRAGMENT_H_
#define _IPC_CORE_FRAGMENT_H_

#include <thread>
#include <sstream>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <memory_resource>
#include <ipc/def.h>
#include <sync/RwLock.h>
#include <Handle.h>

namespace ipc
{
namespace detail
{

static constexpr uint32_t DEFAULT_WRITE_CNT = 1;
static constexpr std::size_t DEFAULT_CACHE_SIZE = 1024 * 1024 * 1024; // 1G
static const std::string DEFAULT_SHM_NAME = "tiny_ipc_";

// Unified release after the application ends
static std::unordered_map<std::string, std::shared_ptr<SpinLock>> locks;
    

static std::string thread_id_to_string(const std::thread::id &id)
{
    std::ostringstream oss;
    oss << id;
    return oss.str();
}

class FragmentBase
{
public:
    FragmentBase()
    {
    }

    virtual ~FragmentBase()
    {
    };

public:
    virtual bool init() = 0;
    // SENDER
    virtual Descriptor write(void const *data, const std::size_t &size,const uint32_t &cnt = DEFAULT_WRITE_CNT)
    {
        return {};
    }

    // RECEIVER
    virtual Buffer read(const Descriptor &desc)
    {
        return {};
    }
};

template<unsigned>
class Fragment{};


// Just need to add a lock on the writer end
// When releasing memory, it will first determine whether the usage count of the current memory segment has been reset.
// Therefore, there is no need to perform a locking operation
template<>
class Fragment<SENDER> : public FragmentBase
{
public:
    Fragment<SENDER>()
        : FragmentBase()
        , id_{std::this_thread::get_id()}
        , handle_ {}
        , pool_ {nullptr}
    {
        
    }

    virtual ~Fragment<SENDER>()
    {
        // free all apply memory
        // There may be some memory that has not been read properly
        for (auto &it : map_)
        {
            pool_->deallocate(it.first,it.second);
        }
        map_.clear();
    }

public:
    virtual bool init() final
    {
        // Apply for shared memory space
        auto name = DEFAULT_SHM_NAME + thread_id_to_string(id_);
        if (!handle_.acquire(name.c_str(), DEFAULT_CACHE_SIZE))
        {
            return false;
        }

        // Create a new lock, if it does not exist
        if(locks.find(std::string{"/"} + name) == locks.end())
        {
            locks.insert(
                {std::string{"/"} + name,std::make_shared<SpinLock>()}
            );
        }

        // std::pmr 
        pool_ = std::make_shared<std::pmr::monotonic_buffer_resource>(handle_.get(),handle_.size(),
                    std::pmr::null_memory_resource());

        return true;
    }

    virtual Descriptor write(void const *data, const std::size_t &size,const uint32_t &cnt = DEFAULT_WRITE_CNT) final
    {
        recyle_memory();

        auto pool_size = align_size(size + sizeof(uint32_t), alignof(std::max_align_t));
        void *pool_data = nullptr;

        auto it = locks.find(std::string(handle_.name()));
        if ( it != locks.end())
        {
            std::lock_guard<SpinLock> l(*(it->second));
            pool_data = pool_->allocate(pool_size);
        }

        if(!pool_data)
        {
            return {};
        }

        std::atomic<uint32_t> *count = static_cast<std::atomic<uint32_t>*>(pool_data);
        count->store(cnt,std::memory_order_relaxed);
        memcpy(static_cast<char*>(pool_data) + sizeof(uint32_t),data,size);
        
        map_.insert(
            {pool_data,pool_size}
        );

        return
        {
            id_,
            reinterpret_cast<std::size_t>(pool_data) - reinterpret_cast<std::size_t>(handle_.get()),
            pool_size
        };
    }

private:

    void recyle_memory()
    {
        if(map_.empty())
        {
            return;
        }
        for (auto it = map_.begin(); it != map_.end();)
        {
            std::atomic<uint32_t> *count = static_cast<std::atomic<uint32_t>*>(it->first);
            if(!count->load())
            {
                pool_->deallocate(it->first,it->second);
                it = map_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    // thread id
    std::thread::id id_;
    // shm handle
    Handle handle_;
    // shared memory manager
    std::shared_ptr<std::pmr::monotonic_buffer_resource> pool_;
    // already memory using map
    std::unordered_map<void*,std::size_t> map_;
};

template<>
class Fragment<RECEIVER> : public FragmentBase
{
public:
    Fragment<RECEIVER>()
        : FragmentBase()
        , handles_()
    {
        
    }

    virtual ~Fragment<RECEIVER>()
    {
        handles_.clear();
    }
public:
    virtual bool init() final
    {
        return true;
    }

    virtual Buffer read(const Descriptor &desc) final
    {
        Handle *handle = get_handle(desc.id());
        if(handle == nullptr)
        {
            return Buffer{};
        }
        void *pool_data = static_cast<char*>(handle->get()) + desc.offset();

        return std::move(Buffer(static_cast<char*>(pool_data) + sizeof(uint32_t), 
            desc.length() - sizeof(uint32_t), [](void *p, std::size_t size) -> void
        {
            (void)size;
            std::atomic<uint32_t> *cnt  =  static_cast<std::atomic<uint32_t>*>(
                            static_cast<void*>(static_cast<char*>(p) - sizeof(uint32_t)));

            cnt->fetch_sub(1, std::memory_order_relaxed);
        }));
    }
private:

    Handle *get_handle(const std::thread::id &id)
    {
        auto it = handles_.find(id);
        if(it == handles_.end())
        {
            Handle handle;
            auto name = DEFAULT_SHM_NAME + thread_id_to_string(id);
            if (!handle.acquire(name.c_str(), DEFAULT_CACHE_SIZE, open))
            {
                return nullptr;
            }
            else
            {
                handles_.insert(
                    {id, std::move(handle)}
                );
                return &(handles_[id]);
            }
        }
        else
        {
            return &(it->second);
        }
    }

private:
    std::unordered_map<std::thread::id,Handle> handles_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_CORE_FRAGMENT_H_