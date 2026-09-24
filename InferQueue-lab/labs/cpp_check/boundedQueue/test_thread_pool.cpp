#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

void check(bool ok, const char* description) {
    if (!ok) {
        std::cerr << "FAIL: " << description << std::endl;
        std::abort();
    }
}

void test_invalid_configuration() {
    bool bad_workers = false;
    bool bad_capacity = false;
    try { ThreadPool pool(0, 8); }
    catch (const std::invalid_argument&) { bad_workers = true; }
    try { ThreadPool pool(2, 0); }
    catch (const std::invalid_argument&) { bad_capacity = true; }
    check(bad_workers && bad_capacity, "reject zero worker/capacity");
    std::cout << "PASS invalid configuration\n";
}

void test_numbered_tasks() {
    constexpr int count = 1000;
    std::vector<std::atomic<int>> hits(count);
    for (auto& hit : hits) { hit.store(0); }
    ThreadPool pool(4, 8);
    std::vector<std::future<int>> results;
    for (int id = 0; id < count; ++id) {
        // id 按值捕获：每个任务保存自己的编号。
        results.push_back(pool.submit([id, &hits] {
            hits[id].fetch_add(1);
            return id * id;
        }));
    }
    for (int id = 0; id < count; ++id) {
        check(results[id].get() == id * id, "correct result for each ID");
    }
    pool.close();
    std::cout << "PASS 1000 numbered task results\n";
    // 在函数末尾析构时还会 join。完整的不重检查见下面独立作用域。
    // 此处每个任务只有一次计数并在返回前完成。
    for (const auto& hit : hits) {
        check(hit.load() == 1, "each ID executes once");
    }
}

void test_exception_and_recovery() {
    ThreadPool pool(1, 2);
    auto failed = pool.submit([]() -> int {
        throw std::runtime_error("task failed on purpose");
    });
    auto succeeded = pool.submit([] { return 42; });
    bool caught = false;
    try { (void)failed.get(); }
    catch (const std::runtime_error& error) {
        caught = std::string(error.what()) == "task failed on purpose";
    }
    check(caught, "exception reaches future.get");
    check(succeeded.get() == 42, "same worker continues after task exception");
    std::cout << "PASS exception propagation and worker recovery\n";
}

void test_close_rejects_new_tasks() {
    ThreadPool pool(2, 4);
    pool.close();
    pool.close();
    bool rejected = false;
    try { (void)pool.submit([] { return 1; }); }
    catch (const std::runtime_error&) { rejected = true; }
    check(rejected, "submit after close must throw");
    std::cout << "PASS repeated close and rejected submission\n";
}

void test_destructor_drains_and_joins() {
    constexpr int count = 1000;
    std::vector<std::atomic<int>> hits(count);
    for (auto& hit : hits) { hit.store(0); }
    std::vector<std::future<int>> results;
    {
        ThreadPool pool(4, 8);
        for (int id = 0; id < count; ++id) {
            results.push_back(pool.submit([id, &hits] {
                hits[id].fetch_add(1);
                return id;
            }));
        }
        // 没有手动 close，也没有先 get。离开作用域必须执行完所有任务。
    }
    for (int id = 0; id < count; ++id) {
        check(hits[id].load() == 1, "destructor drains each ID exactly once");
        check(results[id].wait_for(std::chrono::seconds(0)) ==
                  std::future_status::ready, "result ready after destructor");
        check(results[id].get() == id, "result survives pool destruction");
    }
    std::cout << "PASS destructor drains 1000 tasks and joins workers\n";
}

void test_close_wakes_blocked_submitter() {
    ThreadPool pool(1, 1);
    std::promise<void> release;
    auto gate = release.get_future();
    std::promise<void> running;
    auto entered = running.get_future();
    auto first = pool.submit([&] {
        running.set_value();
        gate.wait(); // 只用于测试：把唯一的 worker 暂时停在这里。
        return 1;
    });
    entered.get();
    auto second = pool.submit([] { return 2; }); // 填满唯一的队列位置。
    std::promise<void> submitting;
    auto started = submitting.get_future();
    auto third = std::async(std::launch::async, [&] {
        submitting.set_value();
        try { (void)pool.submit([] { return 3; }); }
        catch (const std::runtime_error&) { return true; }
        return false;
    });
    started.get();
    check(third.wait_for(std::chrono::milliseconds(100)) ==
              std::future_status::timeout, "third submit is blocked while full");
    pool.close();
    check(third.wait_for(std::chrono::seconds(2)) ==
              std::future_status::ready, "close wakes blocked submitter");
    check(third.get(), "blocked submitter receives rejection");
    release.set_value();
    check(first.get() == 1 && second.get() == 2, "accepted tasks still drain");
    std::cout << "PASS close wakes blocked submitter and drains accepted tasks\n";
}

int main() {
    test_invalid_configuration();
    test_numbered_tasks();
    test_exception_and_recovery();
    test_close_rejects_new_tasks();
    test_destructor_drains_and_joins();
    test_close_wakes_blocked_submitter();
    std::cout << "ALL 6 TESTS PASSED\n";
}
