
#include <vector>
#include <iostream>
#include <mutex>
#include <atomic>
#include <cstring>
#include <random.hpp>
#include <Resource.hpp>
#include <ipc/Ipc.h>
#include <ipc/Buffer.h>

#include "test.h"

using namespace ipc;

namespace {

constexpr int LoopCount   = 10000;
constexpr int MultiMax    = 8;
constexpr int TestBuffMax = 65536;

struct msg_head {
    int id_;
};

class RandBuf : public Buffer
{
public:
    RandBuf() {
        int size = random<>{(int)sizeof(msg_head), TestBuffMax}();
        *this = Buffer(new char[size], size, [](void * p, std::size_t) {
            delete [] static_cast<char *>(p);
        });
    }

    RandBuf(msg_head const &msg) {
        *this = Buffer(new msg_head{msg}, sizeof(msg), [](void * p, std::size_t) {
            delete static_cast<msg_head *>(p);
        });
    }

    RandBuf(RandBuf &&) = default;
    RandBuf(RandBuf const & rhs) {
        if (rhs.empty()) return;
        void * mem = new char[rhs.size()];
        std::memcpy(mem, rhs.data(), rhs.size());
        *this = Buffer(mem, rhs.size(), [](void * p, std::size_t) {
            delete [] static_cast<char *>(p);
        });
    }

    RandBuf(Buffer && rhs)
        : Buffer(std::move(rhs)) {
    }

    void set_id(int k) noexcept {
        get<msg_head *>()->id_ = k;
    }

    int get_id() const noexcept {
        return get<msg_head *>()->id_;
    }

    using Buffer::operator=;
};

class data_set {
    std::vector<RandBuf> datas_;

public:
    data_set() {
        datas_.resize(LoopCount);
        for (int i = 0; i < LoopCount; ++i) {
            datas_[i].set_id(i);
        }
    }

    std::vector<RandBuf> const &get() const noexcept {
        return datas_;
    }
} const data_set__;

template <Relation Rp, Relation Rc, Transmission Ts, typename Que = Ipc<Wr<Rp, Rc, Ts>> >
void test_sr(char const * name, int s_cnt, int r_cnt) {
    ipc_ut::sender().start(static_cast<std::size_t>(s_cnt));
    ipc_ut::reader().start(static_cast<std::size_t>(r_cnt));

    std::atomic_thread_fence(std::memory_order_seq_cst);
    ipc_ut::test_stopwatch sw;

    for (int k = 0; k < s_cnt; ++k) {
        ipc_ut::sender() << [name, &sw, r_cnt, k] {
            Que que { name, ipc::SENDER };
            sw.start();
            for (int i = 0; i < (int)data_set__.get().size(); ++i) {
                ASSERT_TRUE(que.write(data_set__.get()[i]));
            }
        };
    }

    for (int k = 0; k < r_cnt; ++k) {
        ipc_ut::reader() << [name] {
            Que que { name, ipc::RECEIVER };
            for (;;) {
                RandBuf got { que.read() };
                ASSERT_FALSE(got.empty());
                int i = got.get_id();
                if (i == -1) {
                    return;
                }
                ASSERT_TRUE((i >= 0) && (i < (int)data_set__.get().size()));
                auto const &data_set = data_set__.get()[i];
                if (data_set != got) {
                    printf("data_set__.get()[%d] != got, size = %zd/%zd\n", 
                            i, data_set.size(), got.size());
                    ASSERT_TRUE(false);
                }
            }
        };
    }

    ipc_ut::sender().wait_for_done();
    Que que { name };
    for (int k = 0; k < r_cnt; ++k) {
        que.write(RandBuf{msg_head{-1}});
    }
    ipc_ut::reader().wait_for_done();
    sw.print_elapsed<std::chrono::microseconds>(s_cnt, r_cnt, (int)data_set__.get().size(), name);
}

} // internal-linkage


TEST(IPC, UNICAST) {
    test_sr<Relation::SINGLE, Relation::SINGLE, Transmission::UNICAST  >("ssu", 1, 1);
    test_sr<Relation::MULTI, Relation::SINGLE , Transmission::UNICAST  >("msu", 1, 1);
}

TEST(IPC, BROADCAST) {
    test_sr<Relation::SINGLE , Relation::MULTI , Transmission::BROADCAST  >("smb", MultiMax, MultiMax);
    test_sr<Relation::MULTI , Relation::MULTI , Transmission::BROADCAST>("mmb", MultiMax, MultiMax);
}
