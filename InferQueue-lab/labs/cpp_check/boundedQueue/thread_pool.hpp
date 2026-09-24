#pragma once
#include"bounded_queue.hpp"
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

// 教学版只接收可复制、无参数、返回 int 的函数对象。
// 主线程拥有线程池；任务应在有限时间内结束。
// 不支持任务内部递归提交/等待同一线程池的任务，或销毁线程池。
// 析构前应停止其他外部线程对线程池的访问。


class ThreadPool {
public:
    ThreadPool(std::size_t worker_count, std::size_t queue_capacity)
        : queue_(queue_capacity) {
        if (worker_count == 0) {
            throw std::invalid_argument("worker count must be positive");
        }
        workers_.reserve(worker_count);
        try {
            for (std::size_t i = 0; i < worker_count; ++i) {
                workers_.emplace_back([this] {
                    for (;;) {
                        Task task;
                        if (!queue_.pop(task)) {
                            break;
                        }
                        // pop 已释放队列锁。任务的返回值/异常进入关联 future。
                        task();
                    }
                });
            }
        } catch (...) {
            // 若创建部分线程后失败，也必须回收已经创建的线程。
            queue_.close();
            for (auto& worker : workers_) {
                worker.join();
            }
            throw;
        }
    }

    std::future<int> submit(std::function<int()> function) {
        Task task(std::move(function));
        std::future<int> result = task.get_future();
        if (!queue_.push(std::move(task))) {
            throw std::runtime_error("submit to closed thread pool");
        }
        return result;
    }

    // 停止接收任务，唤醒等待者；已有任务仍会继续执行。
    // close 本身不等待任务全部完成。
    void close() {
        queue_.close();
    }

    ~ThreadPool() {
        queue_.close();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    TaskQueue queue_;
    std::vector<std::thread> workers_;
};
