
#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>
#include <deque>
#include <array>
#include <cstdio>
#include "test.h"
#include <sync/Mutex.h>

// TEST(Sync, Mutex) {
//     ipc::detail::Mutex lock;
//     EXPECT_TRUE(lock.open("test-mutex-robust"));
//     std::thread{[] {
//         ipc::detail::Mutex lock {"test-mutex-robust"};
//         EXPECT_TRUE(lock.valid());
//         EXPECT_TRUE(lock.lock());
//     }}.join();

//     EXPECT_THROW(lock.try_lock(), std::system_error);

//     // int i = 0;
//     // EXPECT_TRUE(lock.lock());
//     // i = 100;
//     // auto t2 = std::thread{[&i] {
//     //     ipc::detail::Mutex lock {"test-mutex-robust"};
//     //     EXPECT_TRUE(lock.valid());
//     //     EXPECT_FALSE(lock.try_lock());
//     //     EXPECT_TRUE(lock.lock());
//     //     i += i;
//     //     EXPECT_TRUE(lock.unlock());
//     // }};
//     // std::this_thread::sleep_for(std::chrono::seconds(1));
//     // EXPECT_EQ(i, 100);
//     // EXPECT_TRUE(lock.unlock());
//     // t2.join();
//     // EXPECT_EQ(i, 200);
// }

// #include <sync/semaphore.h>

// TEST(Sync, Semaphore) {
//     ipc::detail::semaphore sem;
//     EXPECT_TRUE(sem.open("test-sem"));
//     std::thread{[] {
//         ipc::detail::semaphore sem {"test-sem"};
//         EXPECT_TRUE(sem.post(1000));
//     }}.join();

//     for (int i = 0; i < 1000; ++i) {
//         EXPECT_TRUE(sem.wait(0));
//     }
//     EXPECT_FALSE(sem.wait(0));
// }

#include <sync/Condition.h>

// TEST(Sync, Condition) {
//     ipc::detail::Condition cond;
//     EXPECT_TRUE(cond.open("test-cond"));
//     ipc::detail::Mutex lock;
//     EXPECT_TRUE(lock.open("test-mutex"));
//     std::deque<int> que;

//     auto job = [&que](int num) {
//         ipc::detail::Condition cond {"test-cond"};
//         ipc::detail::Mutex lock {"test-mutex"};
//         for (int i = 0; i < 10; ++i) {
//             int val = 0;
//             {
//                 std::lock_guard<ipc::detail::Mutex> guard {lock};
//                 while (que.empty()) {
//                     ASSERT_TRUE(cond.wait(lock));
//                 }
//                 val = que.front();
//                 que.pop_front();
//             }
//             EXPECT_NE(val, 0);
//             std::printf("test-cond-%d: %d\n", num, val);
//         }
//         for (;;) {
//             int val = 0;
//             {
//                 std::lock_guard<ipc::detail::Mutex> guard {lock};
//                 while (que.empty()) {
//                     ASSERT_TRUE(cond.wait(lock, 1000));
//                 }
//                 val = que.front();
//                 que.pop_front();
//             }
//             if (val == 0) {
//                 std::printf("test-cond-%d: exit.\n", num);
//                 return;
//             }
//             std::printf("test-cond-%d: %d\n", num, val);
//         }
//     };
//     std::array<std::thread, 10> test_conds;
//     for (int i = 0; i < (int)test_conds.size(); ++i) {
//         test_conds[i] = std::thread{job, i};
//     }

//     for (int i = 1; i < 100; ++i) {
//         {
//             std::lock_guard<ipc::detail::Mutex> guard {lock};
//             que.push_back(i);
//             ASSERT_TRUE(cond.notify(lock));
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//     }
//     for (int i = 1; i < 100; ++i) {
//         {
//             std::lock_guard<ipc::detail::Mutex> guard {lock};
//             que.push_back(i);
//             ASSERT_TRUE(cond.broadcast(lock));
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//     }
//     {
//         std::lock_guard<ipc::detail::Mutex> guard {lock};
//         for (int i = 0; i < (int)test_conds.size(); ++i) {
//             que.push_back(0);
//         }
//         ASSERT_TRUE(cond.broadcast(lock));
//     }

//     for (auto &t : test_conds) t.join();
// }

/**
 * https://stackoverflow.com/questions/51730660/is-this-a-bug-in-glibc-pthread
*/
// TEST(Sync, ConditionRobust) {
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 1\n");
//     ipc::detail::Condition cond {"test-cond"};
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 2\n");
//     ipc::detail::Mutex lock {"test-mutex"};
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 3\n");
//     ASSERT_TRUE(lock.lock());
//     std::thread unlock {[] {
//         printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW 1\n");
//         ipc::detail::Condition cond {"test-cond"};
//         printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW 2\n");
//         ipc::detail::Mutex lock {"test-mutex"};
//         printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW 3\n");
//         {
//             std::lock_guard<ipc::detail::Mutex> guard {lock};
//         }
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW 4\n");
//         ASSERT_TRUE(cond.broadcast(lock));
//         printf("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW 5\n");
//     }};
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 4\n");
//     ASSERT_TRUE(cond.wait(lock));
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 5\n");
//     ASSERT_TRUE(lock.unlock());
//     printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX 6\n");
//     unlock.join();
// }