#include <thread>
#include <iostream>

#include <sync/Waiter.h>


#include "test.h"

namespace {

// TEST(Waiter, broadcast) {
//     for (int i = 0; i < 10; ++i) {
//         ipc::detail::Waiter waiter;
//         std::thread ts[10];

//         int k = 0;
//         for (auto& t : ts) {
//             t = std::thread([&k] {
//                 ipc::detail::Waiter waiter {"test-ipc-waiter"};
//                 EXPECT_TRUE(waiter.valid());
//                 for (int i = 0; i < 9; ++i) {
//                     while (!waiter.wait_for([&k, &i] { return k == i; })) ;
//                 }
//             });
//         }

//         EXPECT_TRUE(waiter.open("test-ipc-waiter"));
//         std::cout << "waiting for broadcast...\n";
//         for (k = 1; k < 10; ++k) {
//             std::cout << "broadcast: " << k << "\n";
//             ASSERT_TRUE(waiter.broadcast());
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         }
//         for (auto& t : ts) t.join();
//         std::cout << "quit... " << i << "\n";
//     }
// }

// TEST(Waiter, quit_waiting) {
//     ipc::detail::Waiter waiter;
//     EXPECT_TRUE(waiter.open("test-ipc-waiter"));

//     std::thread t1 {
//         [&waiter] {
//             EXPECT_TRUE(waiter.wait_for([] { return true; }));
//         }
//     };

//     bool quit = false;
//     std::thread t2 {
//         [&quit] {
//             ipc::detail::Waiter waiter {"test-ipc-waiter"};
//             EXPECT_TRUE(waiter.wait_for([&quit] { return !quit; }));
//         }
//     };

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     EXPECT_TRUE(waiter.quit());
//     t1.join();
//     ASSERT_TRUE(t2.joinable());

//     EXPECT_TRUE(waiter.open("test-ipc-waiter"));
//     std::cout << "nofify quit...\n";
//     quit = true;
//     EXPECT_TRUE(waiter.notify());
//     t2.join();
//     std::cout << "quit... \n";
// }

} // internal-linkage
