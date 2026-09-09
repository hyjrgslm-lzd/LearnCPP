# 01：类不变量

类不变量是在每次公开成员函数返回给调用者时必须成立的条件。它不是调试断言，也不是注释愿望。构造函数必须建立不变量，公开操作必须在成功和失败路径都保持不变量，析构函数要能处理任何已构造完成的合法对象。

不变量要能被一句话检查。`min_ <= value_ && value_ <= max_` 是不变量；“这个对象应该合理”不是。不变量也不能依赖调用者的记忆。若每次调用 `render()` 前都要求调用者先调用 `prepare()`，类型其实有一个未建模状态：unprepared/prepared。要么把状态写入类型，要么把 `prepare()` 放进构造或工厂流程。

例子：命中率 `Rate` 的值必须在 `[0, 1]`。如果类型允许 `Rate{-0.2}` 构造成功，再要求每个调用者自己检查，错误已经从类型内部泄漏到所有使用处。更小的设计是：构造和 `set()` 拒绝非法值，`value()` 永远返回合法值。

不变量要写在类型边界，而不是写在每个调用处：

```cpp
class Rate {
public:
    explicit Rate(double value) { set(value); }
    void set(double value) {
        if (value < 0.0 || value > 1.0) {
            throw std::out_of_range("rate outside [0, 1]");
        }
        value_ = value;
    }
    double value() const noexcept { return value_; }

private:
    double value_ = 0.0;
};
```

这个类型的调用者不用再判断负数或大于 1 的情况，因为那些对象不存在。这里的异常不是控制流炫技，而是拒绝构造非法对象的错误通道。若业务希望“夹紧到边界”，那是不变量不同：输入 `1.2` 被定义为 `1.0`，不是同一个契约。

公有接口必须逐个过不变量门。下面这个反例把字段设成 private，但 `set_raw()` 仍允许坏状态进入：

```cpp
class Rate {
public:
    explicit Rate(double value) { set(value); }
    void set(double value) {
        if (value < 0.0 || value > 1.0) {
            throw std::out_of_range("rate outside [0, 1]");
        }
        value_ = value;
    }
    void set_raw(double value) noexcept { value_ = value; } // breaks invariant

private:
    double value_ = 0.0;
};
```

如果 `set_raw()` 只供内部测试使用，也不能留在生产公有接口。更小的做法是把测试放到独立 fixture，或者让测试走同一个公开契约。

构造失败和工厂失败也要分清阶段。构造函数抛出时，完整对象不存在：

```cpp
Rate bad{-0.2}; // throws; no Rate object is produced
```

命名工厂适合输入解析和错误信息更复杂的场景：

```cpp
class Port {
public:
    static Port from_number(int value) {
        if (value < 0 || value > 65535) {
            throw std::out_of_range("port outside TCP range");
        }
        return Port(value);
    }

    int value() const noexcept { return value_; }

private:
    explicit Port(int value) : value_(value) {}
    int value_ = 0;
};
```

私有构造函数保证调用者不能绕开 `from_number()`。如果以后改成返回错误对象而非异常，仍应保持同一事实：失败时没有产生非法 `Port`。

练习 L01 要实现一个带上下界的 `BoundedInt`。它检查构造、修改、边界包含和非法输入失败后旧值不变。坏变体会跳过范围检查，checker 应拒绝它。
