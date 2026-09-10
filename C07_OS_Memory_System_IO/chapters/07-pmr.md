# 07 polymorphic memory resource

`std::pmr` 把“容器元素类型”与“从哪里分配 storage”分开。`std::pmr::vector<T>` 使用 `std::pmr::polymorphic_allocator<T>`，allocator 内部只保存一个 `memory_resource*`。真正分配时调用虚函数 `do_allocate(bytes, alignment)`，释放时调用 `do_deallocate`，资源相等由 `do_is_equal` 决定。

这带来两个直接后果。第一，resource 必须比所有使用它的容器和嵌套元素活得更久。第二，嵌套 pmr 类型需要拿到同一个 resource；否则外层 vector 用 arena，内层 string 又跑回 default resource，计数和寿命都会错。

最小计数资源：

```cpp
class counting_resource : public std::pmr::memory_resource {
    void* do_allocate(std::size_t n, std::size_t a) override {
        ++allocations;
        return std::pmr::new_delete_resource()->allocate(n, a);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        ++deallocations;
        std::pmr::new_delete_resource()->deallocate(p, n, a);
    }
    bool do_is_equal(const memory_resource& other) const noexcept override {
        return this == &other;
    }
};
```

`monotonic_buffer_resource` 的核心是单次 deallocate 不回收，直到 `release()` 或析构才整体释放。`unsynchronized_pool_resource` 会维护尺寸池，但名字已经说明它不提供跨线程同步。跨线程资源共享进入 C08；本章只讲单线程寿命、传播和失败边界。

[L05_pmr](../exercises/L05_pmr/README.md) 用 `std::pmr::vector<std::pmr::string>` 检查嵌套传播，用 `is_equal` 检查资源身份。
