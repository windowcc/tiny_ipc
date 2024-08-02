#ifndef _IPC_FRAGMENT_FRAGMENT_H_
#define _IPC_FRAGMENT_FRAGMENT_H_

#include <thread>
#include <sstream>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <memory_resource>
#include <ipc/def.h>
#include <Handle.h>

namespace ipc
{
namespace detail
{

static constexpr uint32_t DEFAULT_WRITE_CNT = 1;
static constexpr std::size_t DEFAULT_CACHE_SIZE = 1024 * 1024 * 1024; // 1G
static const std::string DEFAULT_SHM_NAME = "tiny_ipc_";
    

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
    virtual BufferDesc write(void const *data, const std::size_t &size,const uint32_t &cnt = DEFAULT_WRITE_CNT)
    {
        return {};
    }

    // RECEIVER
    virtual Buffer read(const BufferDesc &desc)
    {
        return {};
    }
};

template<unsigned>
class Fragment{};

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
        for (auto &it : map_)
        {
            pool_->deallocate(it.first,it.second);
        }
        map_.clear();
    }

public:
    virtual bool init() final
    {
        auto name = DEFAULT_SHM_NAME + thread_id_to_string(id_);
        if (!handle_.acquire(name.c_str(), DEFAULT_CACHE_SIZE))
        {
            return false;
        }
        pool_ = std::make_shared<std::pmr::monotonic_buffer_resource>(handle_.get(),handle_.size(),
                    std::pmr::null_memory_resource());

        return true;
    }

    virtual BufferDesc write(void const *data, const std::size_t &size,const uint32_t &cnt = DEFAULT_WRITE_CNT) final
    {
        remove_pool();

        auto pool_size = align_size(size,alignof(std::max_align_t)) + alignof(std::max_align_t);
        void *pool_data = pool_->allocate(pool_size);

        alignas(alignof(std::max_align_t)) std::atomic<uint32_t> *count = static_cast<std::atomic<uint32_t>*>(pool_data);
        count->store(cnt,std::memory_order_acquire);
        memcpy(static_cast<char*>(pool_data) + alignof(std::max_align_t),data,size);
        
        map_.insert({pool_data,pool_size});
        return {id_, reinterpret_cast<std::size_t>(pool_data) - reinterpret_cast<std::size_t>(handle_.get()), pool_size};
    }

private:

    void remove_pool()
    {
        if(map_.empty())
        {
            return;
        }
        for (auto it = map_.begin(); it != map_.end(); ++it)
        {
            alignas(alignof(std::max_align_t)) std::atomic<uint32_t> *count = static_cast<std::atomic<uint32_t>*>(it->first);
            if(count->load(std::memory_order_relaxed) == 0)
            {
                pool_->deallocate(it->first,it->second);
                map_.erase(it);
            }
        }
    }

private:
    std::thread::id id_;            // thread id
    Handle handle_;                 // 根据线程id创建的共享内存段 (!!!! )
    std::shared_ptr<std::pmr::monotonic_buffer_resource> pool_;     // 用于接管共享内存段

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

    virtual Buffer read(const BufferDesc &desc) final
    {
        Handle *handle = get_handle(desc.id);
        if(handle == nullptr)
        {
            return Buffer{};
        }
        //通过偏移量计算地址
        void *pool_data = static_cast<char*>(handle->get()) + desc.offset;

        return std::move(Buffer(static_cast<char*>(pool_data) + alignof(std::max_align_t), 
            desc.len - alignof(std::max_align_t), [](void *p, std::size_t size) -> void
        {
            (void)size;
            // 字符串向前偏移 alignof(std::max_align_t) 长度是计数器的首地址
            alignas(alignof(std::max_align_t)) std::atomic<uint32_t> *cnt  = 
                    static_cast<std::atomic<uint32_t>*>(static_cast<void*>(static_cast<char*>(p) - alignof(std::max_align_t)));
            cnt->fetch_sub(1,std::memory_order_release);
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
                handles_.insert({id, std::move(handle)});
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

#endif // ! _IPC_FRAGMENT_FRAGMENT_H_