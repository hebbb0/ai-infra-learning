#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class SafeCounter {
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++value_;
    }

    long long value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

private:
    mutable std::mutex mutex_;
    long long value_{0};
};

int main() {
    SafeCounter counter;

    constexpr int thread_count = 4;
    constexpr int increments_per_thread = 250000;

    std::vector<std::thread> workers;

    for (int i = 0; i < thread_count; ++i) {
        workers.emplace_back([&counter] {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.increment();
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    const long long expected =
        static_cast<long long>(thread_count) * increments_per_thread;

    assert(counter.value() == expected);
    std::cout << "counter = " << counter.value() << '\n';
}