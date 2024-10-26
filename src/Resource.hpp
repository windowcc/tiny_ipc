#ifndef _IPC_RESOURCE_H_
#define _IPC_RESOURCE_H_

#include <map>
#include <unordered_map>
#include <string>
#include <limits>
#include <system_error>
#include <sys/time.h>
#include <time.h>

namespace ipc {

namespace {

constexpr char const * pf(int)                { return "%d"  ; }
constexpr char const * pf(long)               { return "%ld" ; }
constexpr char const * pf(long long)          { return "%lld"; }
constexpr char const * pf(unsigned int)       { return "%u"  ; }
constexpr char const * pf(unsigned long)      { return "%lu" ; }
constexpr char const * pf(unsigned long long) { return "%llu"; }
constexpr char const * pf(float)              { return "%f"  ; }
constexpr char const * pf(double)             { return "%f"  ; }
constexpr char const * pf(long double)        { return "%Lf" ; }

} // internal-linkage
template <typename T>
struct hash : public std::hash<T> {};

template <typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T, ipc::hash<Key>, std::equal_to<Key>, std::allocator<std::pair<Key const, T>>
>;

template <typename Key, typename T>
using map = std::map<
    Key, T, std::less<Key>, std::allocator<std::pair<Key const, T>>
>;

template <typename Char>
using basic_string = std::basic_string<
    Char, std::char_traits<Char>, std::allocator<Char>
>;

using string  = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <> struct hash<string>
{
    std::size_t operator()(string const &val) const noexcept
    {
        return std::hash<char const *>{}(val.c_str());
    }
};

template <> struct hash<wstring>
{
    std::size_t operator()(wstring const &val) const noexcept
    {
        return std::hash<wchar_t const *>{}(val.c_str());
    }
};

template <typename T>
ipc::string to_string(T val)
{
    char buf[std::numeric_limits<T>::digits10 + 1] {};
    if (std::snprintf(buf, sizeof(buf), pf(val), val) > 0)
    {
        return buf;
    }
    return {};
}

/// \brief Check string validity.
constexpr bool is_valid_string(char const *str) noexcept
{
    return (str != nullptr) && (str[0] != '\0');
}

/// \brief Make a valid string.
inline ipc::string make_string(char const *str)
{
    return is_valid_string(str) ? ipc::string{str} : ipc::string{};
}

/// \brief Combine prefix from a list of strings.
inline ipc::string make_prefix(ipc::string prefix, std::initializer_list<ipc::string> args)
{
    prefix += "__IPC_SHM__";
    for (auto const &txt: args) {
        if (txt.empty()) continue;
        prefix += txt;
    }
    return prefix;
}

inline timespec make_timespec(std::uint64_t tm /*ms*/) noexcept(false)
{
    timespec ts {};
    timeval now;
    if (::gettimeofday(&now, NULL) != 0)
    {
        throw std::system_error{static_cast<int>(errno), std::system_category()};
    }
    ts.tv_nsec = (now.tv_usec + (tm % 1000) * 1000) * 1000;
    ts.tv_sec  =  now.tv_sec  + (tm / 1000) + (ts.tv_nsec / 1000000000l);
    ts.tv_nsec %= 1000000000l;

    return ts;
}

inline std::size_t align_size(const std::size_t &size, const std::size_t &align)
{
    return (size + align - 1) & ~(align - 1);
}

} // namespace ipc

#endif // ! _IPC_RESOURCE_H_