#include <condition_variable>
#include <string_view>
#include <sys/sysinfo.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <numbers>
#include <string>
#include <thread>

[[nodiscard]] constexpr char xtoupper(char in)
{
    if (in >= 'a' && in <= 'z') {
        return in - ('a' - 'A');
    }
    return in;
}
static_assert(xtoupper('a') == 'A');
static_assert(xtoupper('A') == 'A');
static_assert(xtoupper('b') == 'B');

[[nodiscard]] inline int parse_int(const std::string_view& sv)
{
#if 0
    char* endptr = nullptr;
    const auto ret = strtol(sv.data(), &endptr, 10);
    if (endptr != sv.end()) {
        throw std::runtime_error(std::string("int parsing of ") + std::string(sv) +
                                 " failed");
    }
    return ret;
#elif 0
    int ret = 0;
    for (auto ch : sv) {
        if (ch < '0' || ch > '9') {
            throw std::runtime_error(std::string("int parsing of ") + std::string(sv) +
                                     " failed");
        }
        ret = 10 * ret + ch - '0';
    }
    return sign * ret;
#else
    int ret = 0;
    auto cur = sv.begin();
    if (cur == sv.end()) {
        throw std::runtime_error("parse_int on empty string");
    }
    int sign = 1;
    if (*cur == '-') {
        sign = -1;
        cur++;
    }
    for (; cur != sv.end(); cur++) {
        auto& ch = *cur;
        if (ch < '0' || ch > '9') {
            throw std::runtime_error(std::string("int parsing of ") + std::string(sv) +
                                     " failed");
        }
        ret = 10 * ret + ch - '0';
    }
    return sign * ret;
#endif
}

[[nodiscard]] constexpr int maidenhead_to_index(std::string_view sv)
{
    assert(sv.size() == 4);
    const auto a = xtoupper(sv[0]);
    const auto b = xtoupper(sv[1]);
    const auto c = sv[2];
    const auto d = sv[3];
    return ((a - 'A') * 10 + c - '0') * 180 + (b - 'A') * 10 + d - '0';
}
static_assert(maidenhead_to_index("AA00") == 0);

[[nodiscard]] std::string maidenhead_from_index(int i);

[[nodiscard]] constexpr std::pair<double, double> decode_maidenhead(std::string_view sv)
{
    const auto mida = ('m' - 97) / 24 + 1 / 48 - 90;
    const auto midb = ('m' - 97) / 12 + 1 / 24 - 180;
    return { 10 * (sv[1] - 'A') + (sv[3] - '0') + mida,
             20 * (sv[0] - 'A') + (sv[2] - 0'0) * 2 + midb };
}

using coord_t = std::pair<double, double>;
[[nodiscard]] inline double dist(coord_t c1, coord_t c2)
{
    constexpr double R = 6371000;
    const auto lat1 = c1.first * std::numbers::pi / 180;
    const auto lon1 = c1.second * std::numbers::pi / 180;
    const auto lat2 = c2.first * std::numbers::pi / 180;
    const auto lon2 = c2.second * std::numbers::pi / 180;
    const auto dlat = lat2 - lat1;
    const auto dlon = lon2 - lon1;
    const auto asinlat = sin(dlat / 2);
    const auto asinlon = sin(dlon / 2);
    const auto a = asinlat * asinlat + cos(lat1) * cos(lat2) * asinlon * asinlon;
    const auto c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}
// Math functions are not constexpr by standard. It works in GCC, but not clang. :-(
// static_assert(dist(decode_maidenhead("IO91"), decode_maidenhead("IO91")) == 0);
constexpr auto maiden_count = maidenhead_to_index("RR99") + 1;

inline void setaffinity(int i)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(i, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask)) {
        throw std::runtime_error(std::string("setting affinity: ") + strerror(errno));
    }
}

inline void setprio(bool high)
{
    struct sched_param param {
    };
    param.sched_priority = high ? 10 : 0;
    if (sched_setscheduler(0, high ? SCHED_FIFO : SCHED_OTHER, &param)) {
        std::cerr << "Setting priority: " << strerror(errno) << "\n";
    }
}

class ThreadPool
{
public:
    using func_t = std::function<void()>;
    ThreadPool(int workers) : workers_(workers)
    {
        for (int i = 0; i < workers_; i++) {
            threads_.emplace_back([this, i] { thread_main(i); });
        }
    }

    // No copy or move.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void thread_main(int i)
    {
        setaffinity(i % get_nprocs());
        setprio(false);
        for (;;) {
            func_t fn;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [this] { return !work_.empty() || done_; });
                if (work_.empty() && done_) {
                    return;
                }
                fn = work_.front();
                work_.pop_front();
            }
            fn();
        }
    }

    void add(func_t fn)
    {
        std::unique_lock<std::mutex> lk(mu_);
        work_.push_back(std::move(fn));
        cv_.notify_one();
    }

    ~ThreadPool()
    {
        std::unique_lock<std::mutex> lk(mu_);
        done_ = true;
        cv_.notify_all();
    }

private:
    std::mutex mu_;
    bool done_ = false;
    std::condition_variable cv_;
    std::deque<func_t> work_;
    std::vector<std::jthread> threads_;
    const int workers_;
};
