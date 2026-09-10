# 20. 显式对象参数：把四个成员重载合成一个正确转发点

先修：[转发与 CTAD](03-forwarding-ctad.md)说明了 `T&&` 怎样保留实参类别；[约束](06-constraints.md)说明了接口什么时候应该拒绝调用。本章看 C++23 的显式对象参数，也常叫 deducing this：成员函数可以把对象本身写成第一个参数，让 `this` 参与模板推导。

最小正确基线是四个重载：

```cpp
template<class T>
class slot {
public:
    T& value() & noexcept { return value_; }
    const T& value() const& noexcept { return value_; }
    T&& value() && noexcept { return std::move(value_); }
    const T&& value() const&& noexcept { return std::move(value_); }

private:
    T value_;
};
```

这段代码啰嗦，但语义清楚：左值对象返回左值引用，右值对象返回右值引用，`const` 对象返回 `const` 引用，`noexcept` 在每条路径都保留。很多错误实现会把它简化成 `T value() const`，看似调用方便，实际复制了成员，地址变了，写回也断了。

## `this auto&& self` 推导对象类别

C++23 可以把对象参数显式写出来：

```cpp
template<class T>
class slot {
public:
    constexpr decltype(auto) value(this auto&& self) noexcept {
        return forward_like<decltype(self)>(self.value_);
    }

private:
    T value_;
};
```

调用 `s.value()` 时，`self` 表示 `s`；调用 `std::move(s).value()` 时，`self` 表示右值对象。`decltype(self)` 携带 cv/ref 信息，`forward_like` 把对象的 cv/ref 投射到成员上。

四类结果可以手推：

| 调用表达式 | `decltype(self)` | 返回表达式 |
| --- | --- | --- |
| `s.value()` | `slot<T>&` | `T&` |
| `std::as_const(s).value()` | `const slot<T>&` | `const T&` |
| `std::move(s).value()` | `slot<T>&&` | `T&&` |
| `std::move(std::as_const(s)).value()` | `const slot<T>&&` | `const T&&` |

`std::forward_like` 是标准库工具；练习同时提供了等价的小实现，避免不同库版本对 `<utility>` 支持不一致时挡住本章主题。核心规则只有两步：先看 `Like` 是左值还是右值，再看 `Like` 是否 `const`。

## 返回类型、地址和 noexcept 是同一个契约

检查 `decltype(auto)` 不是形式主义。若写成 `auto value(this auto&& self) noexcept`，返回值会复制成员，`slot<int>` 上能读到同样的数字，但 `&result != &self.value_`，给 result 赋值也不会写回对象。若漏掉 `noexcept`，调用方的异常规格和约束也会变化。

所以正确性同时包括三件事：

```cpp
slot<int> s{7};
decltype(auto) r = s.value();
static_assert(std::same_as<decltype(r), int&>);
static_assert(noexcept(s.value()));
assert(&r == s.address());
```

值相等只是最低层检查。成员访问器的真正契约是表达式类别和对象身份。

## move-only 成员不能从 const 右值偷走

`value()` 可以返回 `const T&&`，因为它只是暴露表达式类别；但“消费对象”的接口必须更严格：

```cpp
T take(this slot&& self) noexcept {
    return std::move(self.value_);
}

T take(this slot&) = delete;
T take(this const slot&) = delete;
T take(this const slot&&) = delete;
```

`slot<std::unique_ptr<int>>` 只能从 mutable rvalue 调用 `take()`。`const slot&&` 不能 move，因为 move constructor 通常需要修改源对象；从 `const` 对象上 `std::move` 只得到 `const T&&`，不是可移动所有权。把 `const&& take()` 放开，会让接口承诺一件类型系统不支持的事。

显式对象参数让这种限制写得很直接：允许哪类对象，就把对象参数写成那类；不允许的类别删除。

## 递归 lambda 也能显式接收自己

显式对象参数不只属于成员函数。lambda 也可以把自己作为第一个参数：

```cpp
struct chain {
    int value{};
    std::unique_ptr<chain> next;

    int sum(this const chain& self) noexcept {
        auto walk = [](this auto const& again, const chain* node) noexcept -> int {
            return node == nullptr ? 0 : node->value + again(node->next.get());
        };
        return walk(&self);
    }
};
```

传统递归 lambda 常需要 `std::function` 或把 lambda 自己作为普通参数传入。这里 `again` 就是 lambda 对象本身，调用 `again(...)` 会递归进入同一个闭包类型。它没有类型擦除，也不分配。

## 借用不会延长外部对象生命周期

显式对象参数保留对象类别，不改变生命周期规则：

```cpp
class name_holder {
public:
    decltype(auto) name(this auto&& self) noexcept {
        return forward_like<decltype(self)>(self.name_);
    }
private:
    std::string name_;
};
```

`name_holder h{"outer"}; auto& n = h.name();` 中 `n` 借用 `h` 内部的字符串。这个引用不拥有字符串，也不会延长 `h` 的生命。如果从临时对象取引用并保存，完整表达式结束后引用就悬垂。显式对象参数让返回类别更精确，但不会把借用变成所有权。

## 先保留正确基线，再消重

迁移顺序应该是先写四重载基线并锁住行为，再合成一个显式对象参数版本。不要直接从“少写代码”出发。少写一行但丢失引用、地址或异常规格，就是错误接口。

[A03练习](../exercises/A03_explicit_object/README.md)按这个顺序验证：Student 初态能构建但运行失败；Reference 与 good 保留四类 cv/ref、地址、`noexcept`、move-only 限制、递归 lambda 和借用边界；bad 返回副本并只计算链表头，检查器必须拒绝。负编译用例证明 `const&& take()` 被删除。

自测：

1. 为什么 `decltype(auto)` 必要？它保留引用和值类别；`auto` 会复制成员。
2. 为什么 `forward_like<decltype(self)>` 比 `std::forward<T>` 更适合成员？我们要转发的是成员，但类别来自对象。
3. 为什么 `value()` 可以暴露 `const T&&`，而 `take()` 不能允许 `const&&`？前者只是访问表达式；后者要移动所有权，`const` 源对象不能被消费。
4. 递归 lambda 的 `this auto const& again` 解决了什么？它让闭包对象本身可递归调用，避免 `std::function` 类型擦除。
