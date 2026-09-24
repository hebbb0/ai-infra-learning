#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include<future>
// 学习版：先只存 int，集中理解等待、唤醒与关闭。
// 使用者必须在销毁队列前，结束并 join 所有访问队列的线程。
using Task = std::packaged_task<int()>;

class TaskQueue {
public:
    explicit TaskQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("queue capacity must be positive");
        }
    }

    bool push(Task task) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this] {
                return closed_ || queue_.size() < capacity_;
            });
            if (closed_) {
                return false;
            }
            queue_.push(std::move(task));
        }
        not_empty_.notify_one();
        return true;
    }

    bool pop(Task& out) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_.wait(lock, [this] {
                return closed_ || !queue_.empty();
            });
            if (queue_.empty()) {
                return false;
            }
            out = std::move(queue_.front());
            queue_.pop();
        }
        not_full_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    const std::size_t capacity_;
    std::mutex mutex_;
    std::queue<Task> queue_;
    bool closed_ = false;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};
