# 06 allocator 与对象生命期

allocator 返回的是能放对象的 storage，不是已经构造好的对象。容器的核心不变量通常是：`[0, size)` 的元素对象生命期已经开始，`[size, capacity)` 只是可用槽位。访问未构造槽位不是“读取默认值”，而是越过对象生命期。

`allocator_traits` 是容器和 allocator 之间的薄适配层。它统一调用 `allocate/deallocate`，并在 `construct` 存在时使用 allocator 的构造入口，否则用 placement new / `construct_at`。构造失败时必须不提交 size；否则析构路径会销毁未构造对象。

最小单槽模型：

```cpp
template<class T>
T& construct(void* storage, auto&&... args) {
    return *std::construct_at(static_cast<T*>(storage),
        std::forward<decltype(args)>(args)...);
}
```

真实代码还要保证 `storage` 大小足够、对齐满足 `alignof(T)`、没有另一个活跃对象占用同一字节范围。C07 的页和映射只提供地址可访问性；对象生命期仍由这里的规则建立。

本章由 [L05_pmr](../exercises/L05_pmr/README.md) 的 raw storage Part 检查：构造成功后才提交，构造抛异常后不销毁假对象。
