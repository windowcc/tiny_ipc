#ifndef _IPC_MEMORY_ALLOC_H_
#define _IPC_MEMORY_ALLOC_H_

#include <algorithm>
#include <utility>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <cassert>

namespace ipc
{
namespace detail
{

class StaticAlloc {
public:
    static void* alloc(std::size_t size)
    {
        return size ? std::malloc(size) : nullptr;
    }

    static void free(void* p, std::size_t size = 0)
    {
        (void)size;
        std::free(p);
    }
};

////////////////////////////////////////////////////////////////
/// Scope allocation -- The destructor will release all allocated blocks.
////////////////////////////////////////////////////////////////

namespace detail {

constexpr static std::size_t aligned(std::size_t size, size_t alignment) noexcept
{
    return ( (size - 1) & ~(alignment - 1) ) + alignment;
}

template <typename T>                                                  
class has_take
{                                                           
private:                                                               
    template <typename Type>                                           
    static std::true_type check(decltype(std::declval<Type>().take(std::move(std::declval<Type>())))*); 
    template <typename Type>                                           
    static std::false_type check(...);                                 
public:                                                                
    using type = decltype(check<T>(nullptr));                          
    constexpr static auto value = type::value;                         
};

class ScopeAllocBase
{
protected:
    struct block_t
    {
        std::size_t size_;
        block_t   * next_;
    } * head_ = nullptr, * tail_ = nullptr;

    enum : std::size_t
    {
        aligned_block_size = aligned(sizeof(block_t), alignof(std::max_align_t))
    };

public:
    void swap(ScopeAllocBase & rhs)
    {
        std::swap(head_, rhs.head_);
        std::swap(tail_, rhs.tail_);
    }

    bool empty() const noexcept
    {
        return head_ == nullptr;
    }

    void take(ScopeAllocBase && rhs)
    {
        if (rhs.empty())
        {
            return;
        }
        
        if (empty())
        {
            swap(rhs);
        }
        else
        {
            std::swap(tail_->next_, rhs.head_);
            // rhs.head_ should be nullptr here
            tail_ = rhs.tail_;
            rhs.tail_ = nullptr;
        }
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <typename AllocP = StaticAlloc>
class ScopeAlloc : public detail::ScopeAllocBase
{
public:
    using base_t = detail::ScopeAllocBase;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    void free_all() {
        while (!empty()) {
            auto curr = head_;
            head_ = head_->next_;
            alloc_.free(curr, curr->size_);
        }
        // now head_ is nullptr
    }

public:
    ScopeAlloc() = default;

    ScopeAlloc(ScopeAlloc && rhs)         { swap(rhs); }
    ScopeAlloc& operator=(ScopeAlloc rhs) { swap(rhs); return (*this); }

    ~ScopeAlloc() { free_all(); }

    void swap(ScopeAlloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    template <typename A = AllocP>
    auto take(ScopeAlloc && rhs) -> typename std::enable_if<detail::has_take<A>::value, void>::type
    {
        base_t::take(std::move(rhs));
        alloc_.take(std::move(rhs.alloc_));
    }

    template <typename A = AllocP>
    auto take(ScopeAlloc && rhs) -> typename std::enable_if<!detail::has_take<A>::value, void>::type
    {
        base_t::take(std::move(rhs));
    }

    void* alloc(std::size_t size) {
        std::size_t real_size = aligned_block_size + size;
        auto curr = static_cast<block_t*>(alloc_.alloc(real_size));
        curr->size_ = real_size;
        curr->next_ = head_;
        head_ = curr;
        if (tail_ == nullptr) {
            tail_ = curr;
        }
        return (reinterpret_cast<uint8_t*>(curr) + aligned_block_size);
    }
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_MEMORY_ALLOC_H_