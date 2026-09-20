## 所有权

- first 创建 Resource，最初拥有对象。
- std::move 后所有权转移给 second。
- first 仍然是合法对象，但内部指针为空。
- consume 接管所有权，函数结束时 Resource 自动析构。
## 并发
std::lock_guard<std::mutex> lock(mutex_);进入作用域时加锁，离开作用域时自动解锁
value() const 表示这个函数不能修改对象的普通成员。但加锁会改变 mutex_ 内部状态，因此锁需要声明成 mutable
long long value() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

mutable std::mutex mutex_;
