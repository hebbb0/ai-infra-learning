#include "thread_pool.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr int task_count = 64;
constexpr int repetitions = 5;
constexpr int queue_capacity = 16;
constexpr int cpu_iterations = 3000000;
constexpr std::array<int, 3> worker_counts{1, 2, 4};

// 无符号运算允许回绕。依赖输入并检查返回值，防止工作变成无用计算。
int cpu_work(int id) {
    std::uint64_t value = static_cast<std::uint64_t>(id) + 1;
    for (int i = 0; i < cpu_iterations; ++i) {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;
    }
    return static_cast<int>(value & 0x7fffffffULL);
}

int wait_work(int id) {
    // 仅模拟等待，不代表真实磁盘/网络 I/O 性能。
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return id;
}

struct Measurement {
    double milliseconds;
    std::int64_t checksum;
};

Measurement run_batch(bool cpu, int workers, const std::vector<int>& expected) {
    ThreadPool pool(workers, queue_capacity);
    std::vector<std::future<int>> futures;
    futures.reserve(task_count);

    // 不计构造线程池的时间；计提交、排队、执行、结果收集。
    const auto start = std::chrono::steady_clock::now();
    for (int id = 0; id < task_count; ++id) {
        futures.push_back(pool.submit([cpu, id] {
            return cpu ? cpu_work(id) : wait_work(id);
        }));
    }

    std::int64_t checksum = 0;
    for (int id = 0; id < task_count; ++id) {
        const int result = futures[id].get();
        if (result != expected[id]) {
            throw std::runtime_error("incorrect task result");
        }
        checksum += result;
    }
    const auto stop = std::chrono::steady_clock::now();
    // 析构 close/join 不在计时内。
    return {std::chrono::duration<double, std::milli>(stop - start).count(), checksum};
}

int main() {
    try {
        std::cout << std::fixed << std::setprecision(3);
        std::cerr << std::fixed << std::setprecision(3);
        std::cout << "workload,workers,repeat,tasks,elapsed_ms,tasks_per_second,checksum\n";
        std::cerr << "tasks=64,queue_capacity=16,repetitions=5,cpu_iterations=3000000,wait_ms=10\n";
        std::cerr << "timing includes submission, queueing, execution, result collection\n";
        std::cerr << "timing excludes pool construction/destruction; one warmup per configuration\n";
        std::cerr << "wait_simulated uses sleep_for, not real disk or network IO\n";
        std::cerr << "workload,workers,median_ms,tasks_per_second,speedup_vs_1_worker\n";

        for (bool cpu : {true, false}) {
            const std::string workload = cpu ? "cpu" : "wait_simulated";
            std::vector<int> expected;
            for (int id = 0; id < task_count; ++id) {
                expected.push_back(cpu ? cpu_work(id) : id);
            }
            std::array<std::vector<double>, 3> samples;
            for (int workers : worker_counts) {
                (void)run_batch(cpu, workers, expected); // 预热，不写入测量结果。
            }
            for (int repeat = 0; repeat < repetitions; ++repeat) {
                // 轮换测量顺序，降低固定顺序带来的偏差。
                for (int offset = 0; offset < 3; ++offset) {
                    const int index = (repeat + offset) % 3;
                    const int workers = worker_counts[index];
                    const auto result = run_batch(cpu, workers, expected);
                    samples[index].push_back(result.milliseconds);
                    std::cout << workload << ',' << workers << ',' << repeat + 1 << ','
                              << task_count << ',' << result.milliseconds << ','
                              << task_count * 1000.0 / result.milliseconds << ','
                              << result.checksum << '\n';
                }
            }
            double baseline = 0;
            for (int index = 0; index < 3; ++index) {
                auto& values = samples[index];
                std::sort(values.begin(), values.end());
                const double median = values[repetitions / 2];
                if (index == 0) { baseline = median; }
                std::cerr << workload << ',' << worker_counts[index] << ',' << median << ','
                          << task_count * 1000.0 / median << ',' << baseline / median << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "BENCHMARK FAILED: " << error.what() << '\n';
        return 1;
    }
}
