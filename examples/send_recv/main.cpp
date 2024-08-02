
#include <signal.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <array>
#include <string.h>
#include <memory_resource>
#include <unordered_map>
#include <ipc/Ipc.h>

namespace {

std::atomic<bool> IsQuit { false };
ipc::Channel *Ipc = nullptr;
uint16_t Count = 0;

void do_send()
{
    ipc::Channel ipc {"ipc", ipc::SENDER};
    Ipc = &ipc;
    const std::string buffer {"Hello,World!!!"};
    while (!IsQuit.load(std::memory_order_acquire))
    {
        if(Count++ >= 10)
        {
            break;
        }
        std::cout << "write : " << buffer << "\n";
        ipc.write(buffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void do_recv()
{
    std::cout << "Please Enter Ctrl + C to End." << std::endl;
    ipc::Channel ipc {"ipc", ipc::RECEIVER};
    Ipc = &ipc;
    while (!IsQuit.load(std::memory_order_acquire))
    {
        ipc::Buffer recv;
        for (int k = 1; recv.empty(); ++k)
        {
            recv = ipc.read();
            if (IsQuit.load(std::memory_order_acquire))
            {
                return;
            }
        }
        std::cout << "read size: " << std::string((char*)recv.data(),recv.size()) << "\n";
    }
}

} // namespace

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        return -1;
    }

    auto exit = [](int)
    {
        IsQuit.store(true, std::memory_order_release);
        if (Ipc)
        {
            Ipc->disconnect();
        }
    };
    ::signal(SIGINT  , exit);
    ::signal(SIGABRT , exit);
    ::signal(SIGSEGV , exit);
    ::signal(SIGTERM , exit);
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
    ::signal(SIGBREAK, exit);
#else
    ::signal(SIGHUP  , exit);
#endif

    std::string mode {argv[1]};
    if (mode == "send")
    {
        do_send();
    }
    else if (mode == "recv")
    {
        do_recv();
    }
    return 0;
}

// bool isAddressInBuff(void *addr, void *start, std::size_t size);
 
// void printResult(const std::pmr::string &name, void *addr, bool isInBuff);

// template<int N>
// class alignas(alignof(std::max_align_t)) CacheTest
// {
// public:
//     CacheTest(const char *data, std::size_t size)
//         : count_ {0}
//     {
//         // memcpy(msg_.data(),data,size);
//         std::cout << "construct" << std::endl;
//     }
//     ~CacheTest()
//     {
//         std::cout << "destruct" << std::endl;
//     }
// private:
//     std::atomic<uint16_t> count_;
//     std::array<char,N> msg_;
// };
  
// void foo2() {
//     unsigned char buff[1024]{};
//     std::pmr::monotonic_buffer_resource pool(buff, 1024,
//                                              std::pmr::null_memory_resource());
    
//     std::string value = "Hello,World!!!";
//     CacheTest<100> *ca = std::pmr::polymorphic_allocator<CacheTest<100>>(&pool).allocate(1);
//     std::pmr::polymorphic_allocator<CacheTest<100>>(&pool).construct(ca, value.c_str(),value.size());

//      std::pmr::polymorphic_allocator<CacheTest<100>>(&pool).destroy(ca);
//      std::pmr::polymorphic_allocator<CacheTest<100>>(&pool).deallocate(ca, 1);

//     // std::pmr::monotonic_buffer_resource pool1(buff, 1024,
//     //                                          std::pmr::null_memory_resource());
//     // {
//     // std::pmr::string s1{"1111 ", &pool};
//     //     printResult("string s1", &s1, isAddressInBuff(&s1, buff, sizeof(buff)));
//     // }
//     // std::pmr::string s2{"2222 ", &pool1};
//     // printResult("string s2", &s2, isAddressInBuff(&s2, buff, sizeof(buff)));


//     // std::pmr::string s3{"3333 ", &pool};
//     // std::pmr::string &s4 = *new std::pmr::string{"4444 ", &pool};
//     // std::pmr::string &s5 = *new std::pmr::string{"5555 ", &pool};
//     // std::pmr::string &s6 = *new std::pmr::string{"6666 ", &pool};
//     // std::pmr::vector<std::pmr::string> vec{&pool};
 
//     // std::pmr::unordered_map<int, std::pmr::string> map{&pool};
//     // vec.push_back(s1);
//     // vec.push_back(s2);
//     // vec.push_back(s3);
//     // map.insert({1, s1});
//     // map.insert({2, s2});
//     // map.insert({3, s3});
 
//     // printResult("string s1", &s1, isAddressInBuff(&s1, buff, sizeof(buff)));
//     // printResult("string s2", &s2, isAddressInBuff(&s2, buff, sizeof(buff)));
//     // printResult("string s3", &s3, isAddressInBuff(&s3, buff, sizeof(buff)));
//     // printResult("string s4", &s4, isAddressInBuff(&s4, buff, sizeof(buff)));
//     // printResult("string s5", &s5, isAddressInBuff(&s5, buff, sizeof(buff)));
//     // printResult("string s6", &s6, isAddressInBuff(&s6, buff, sizeof(buff)));
 
//     // printResult("s1.c_str()", (void *) s1.c_str(), isAddressInBuff((void *) s1.c_str(), buff, sizeof(buff)));
//     // printResult("s2.c_str()", (void *) s2.c_str(), isAddressInBuff((void *) s2.c_str(), buff, sizeof(buff)));
//     // printResult("s3.c_str()", (void *) s3.c_str(), isAddressInBuff((void *) s3.c_str(), buff, sizeof(buff)));
//     // printResult("s4.c_str()", (void *) s4.c_str(), isAddressInBuff((void *) s4.c_str(), buff, sizeof(buff)));
//     // printResult("s5.c_str()", (void *) s5.c_str(), isAddressInBuff((void *) s5.c_str(), buff, sizeof(buff)));
//     // printResult("s6.c_str()", (void *) s6.c_str(), isAddressInBuff((void *) s6.c_str(), buff, sizeof(buff)));
 
//     // printResult("vector vec", &vec, isAddressInBuff(&vec, buff, sizeof(buff)));
//     // printResult("unordered_map map", &map, isAddressInBuff(&map, buff, sizeof(buff)));
 
//     // printResult("vec[0]", (void *) vec[0].c_str(), isAddressInBuff((void *) vec[0].c_str(), buff, sizeof(buff)));
//     // printResult("vec[1]", (void *) vec[1].c_str(), isAddressInBuff((void *) vec[1].c_str(), buff, sizeof(buff)));
//     // printResult("vec[2]", (void *) vec[2].c_str(), isAddressInBuff((void *) vec[2].c_str(), buff, sizeof(buff)));
//     // printResult("map[0]", (void *) map[0].c_str(), isAddressInBuff((void *) map[0].c_str(), buff, sizeof(buff)));
//     // printResult("map[1]", (void *) map[1].c_str(), isAddressInBuff((void *) map[1].c_str(), buff, sizeof(buff)));
//     // printResult("map[2]", (void *) map[2].c_str(), isAddressInBuff((void *) map[2].c_str(), buff, sizeof(buff)));
// }
 
// bool isAddressInBuff(void *addr, void *start, std::size_t size) {
//     auto start_ptr = reinterpret_cast<const std::uint8_t *>(start);
//     auto addr_ptr = reinterpret_cast<const std::uint8_t *>(addr);
//     return (addr_ptr >= start_ptr) && (addr_ptr < (start_ptr + size));
// }
 
// void printResult(const std::pmr::string &name, void *addr, bool isInBuff) {
//     std::cout << name << ": " << ((isInBuff) ? "yes" : "no") << " - " << addr << std::endl;
// }

// int main() {
//     foo2();
// }