# 04. 构造、析构与异常展开

一个对象不是“构造函数体开始执行”就完整存在。C++ 会先构造基类，再构造成员，最后执行最外层构造函数体。只有这些步骤全部成功，最外层对象才算构造完成。这个规则决定了析构顺序，也决定了构造失败时谁负责清理。

07 章的 RAII 会依赖本章结论：如果对象有两个资源，第二个资源获取失败时，第一个资源必须被释放；但最外层对象的析构函数不会运行。因此资源不能只靠最外层析构函数兜底，已经完成的成员自己必须能清理自己。

## 一个完整对象的构造顺序

看这个类：

```cpp
struct Base {
    Base();
    ~Base();
};

struct Member {
    Member(char const* name);
    ~Member();
};

struct Derived : Base {
    Member first;
    Member second;

    Derived()
        : second("second"), first("first") {
    }

    ~Derived();
};
```

`Derived` 的构造顺序是：

1. 先构造直接基类 `Base`。
2. 再按成员声明顺序构造成员：`first`，然后 `second`。
3. 最后执行 `Derived` 构造函数体。

注意初始化列表写了 `second` 再写 `first`，但成员仍按声明顺序构造。初始化列表的文字顺序不能改变对象布局里的成员顺序。实际工程里应让初始化列表顺序和声明顺序一致，避免读者误判，也避免编译器 warning。

如果构造成功，析构顺序正好反过来：

1. 先执行 `Derived` 析构函数体。
2. 再按成员构造的逆序析构：`second`，然后 `first`。
3. 最后析构直接基类 `Base`。

析构函数体运行时，成员和基类仍然活着；析构函数体结束后，成员才开始逆序析构。

## 委托构造函数

委托构造函数把一个构造函数的工作交给同一个类的另一个构造函数：

```cpp
struct Port {
    int value;

    Port() : Port(80) {}

    explicit Port(int v) : value(v) {
        if (v <= 0) {
            throw std::invalid_argument("port");
        }
    }
};
```

`Port()` 不会先构造成员再调用 `Port(int)`。它委托给目标构造函数，目标构造函数负责完整构造这个对象。目标构造函数成功后，委托构造函数体才执行。目标构造函数抛异常时，对象没有构造完成，委托构造函数体不会执行。

还有一个容易漏掉的分支：目标构造函数已经成功，委托构造函数体随后抛异常。此时目标构造函数已经完成了完整对象的构造，C++ 会调用这个对象的析构函数。它不同于“某个成员构造到一半失败”的情况；后者最外层对象从未完成构造，所以最外层析构函数不运行。

委托构造适合把检查和归一化集中到一个入口。不要让多个构造函数分别维护同一组不变量，那会让异常路径和默认值很快分叉。

## new 和 delete 的最小模型

`new T(args...)` 做两件事：先取得足够且对齐正确的动态存储，再在那段存储中构造 `T` 对象。构造成功后返回 `T*`。`delete p` 做反向动作：先析构 `*p`，再释放那段动态存储。

```cpp
auto* p = new std::string("core");
delete p;
```

如果 `new` 已经取得存储，但 `std::string` 构造抛异常，C++ 会释放刚才为这个对象取得的存储。它不会返回一个半成品指针给你。

数组形式 `new T[n]` 和 `delete[]` 成对使用。不要混用 `new[]` 与 `delete`，也不要混用 `new` 与 `delete[]`。本课后续会优先使用标准库拥有者，手写 `new`/`delete` 只在需要观察机制或实现底层容器时出现。

## 构造失败时，未完成对象没有析构函数

最容易写错的是这个版本：

```cpp
struct RawPair {
    Resource* first = nullptr;
    Resource* second = nullptr;

    RawPair() {
        first = acquire("first");
        second = acquire("second"); // 这里可能抛异常
    }

    ~RawPair() {
        release(second);
        release(first);
    }
};
```

如果 `acquire("second")` 抛异常，`RawPair` 没有构造完成，所以 `~RawPair()` 不会运行。`first` 已经获取，却没有被成员对象接管，也没有局部 guard 清理，资源就泄漏了。

很多人会误以为“构造函数失败也会调用析构函数”。准确规则是：最外层对象未完成构造，它自己的析构函数不运行；已经完成构造的基类和成员会按逆序析构。

把资源放进成员对象后，规则变成可依赖的清理机制：

```cpp
struct Handle {
    Resource* p = nullptr;

    explicit Handle(char const* name)
        : p(acquire(name)) {}

    ~Handle() {
        release(p);
    }

    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;
};

struct Pair {
    Handle first;
    Handle second;

    Pair()
        : first("first"), second("second") {}
};
```

如果 `second` 构造失败，`Pair` 仍然没有构造完成，`~Pair()` 不运行。但 `first` 是已经完成构造的成员，所以 `~Handle()` 会运行并释放第一个资源。`second` 没有完成构造，所以 `~Handle()` 不会为它运行。这正是我们想要的行为。

这个设计不需要在 `Pair` 构造函数里写 `try`/`catch`。清理职责落在每个已经完成的成员上，异常展开自动调用这些成员的析构函数。

## 异常展开做什么

异常抛出后，控制流会离开当前表达式和作用域，寻找匹配的 `catch`。这个过程叫 stack unwinding。展开经过的每个已经构造完成的自动对象都会析构。

```cpp
void f() {
    std::string a = "a";
    std::string b = "b";
    throw std::runtime_error("stop");
}
```

抛异常时，`b` 先析构，`a` 后析构。析构顺序仍然是构造逆序。这个规则让普通局部对象也能承担清理职责：

```cpp
void write(std::filesystem::path const& path) {
    std::ofstream out(path);
    may_throw();
    out << "done\n";
}
```

如果 `may_throw()` 抛异常，并且程序沿正常异常处理机制找到匹配的 `catch`，展开经过 `write` 的栈帧时，`out` 会析构并关闭文件。若异常最终未被捕获，程序会调用 `std::terminate`；标准不要求这种情况下仍然有同样可观察的完整展开过程。因此 RAII 能覆盖正常异常传播和捕获路径，不应被表述成“无论是否捕获都保证清理”。

析构函数通常不应该让异常逃出。异常展开期间如果另一个析构函数又抛出未捕获异常，程序会调用 `std::terminate`。资源释放函数如果可能失败，应在析构函数内记录、吞掉或转成显式 `close()` 返回值；不要把失败清理留给析构外的偶然路径。

## 局部 guard 能修正手写构造

有些底层代码暂时不能把资源直接做成成员，比如需要先调用 C API 获取多个句柄，再统一提交到对象状态。此时也要让已经获取的资源立刻进入某个局部 owner：

```cpp
struct CloseFile {
    void operator()(FILE* f) const noexcept {
        if (f) {
            std::fclose(f);
        }
    }
};

using FileOwner = std::unique_ptr<FILE, CloseFile>;

FileOwner open_checked(char const* path) {
    FileOwner f(std::fopen(path, "w"));
    if (!f) {
        throw std::runtime_error("open failed");
    }
    may_throw();
    return f;
}
```

`FileOwner` 是局部对象。`may_throw()` 失败时，它的析构函数关闭文件。成功返回时，所有权随返回值移动出去。这个模型和 `Pair` 的成员 owner 是同一个原则：每一次 acquire 后，尽快交给一个已经构造完成、析构可释放的对象。

## 正反代码对照

错误版本把释放写在最外层析构函数里：

```cpp
struct BadTwoHandles {
    HandleId first{};
    HandleId second{};

    BadTwoHandles() {
        first = acquire("first");
        second = acquire("second"); // 抛异常时 first 泄漏
    }

    ~BadTwoHandles() {
        release(second);
        release(first);
    }
};
```

正确版本让每个资源独立成为成员：

```cpp
struct HandleOwner {
    HandleId id{};

    explicit HandleOwner(char const* name)
        : id(acquire(name)) {}

    ~HandleOwner() {
        release(id);
    }

    HandleOwner(HandleOwner const&) = delete;
    HandleOwner& operator=(HandleOwner const&) = delete;
};

struct GoodTwoHandles {
    HandleOwner first;
    HandleOwner second;

    GoodTwoHandles()
        : first("first"), second("second") {}
};
```

当 `second` 获取失败：

| 版本 | 已构造完成的对象 | 自动清理 | 结果 |
|---|---|---|---|
| `BadTwoHandles` | 最外层对象未完成；没有成员 owner | `~BadTwoHandles()` 不运行 | `first` 泄漏 |
| `GoodTwoHandles` | `first` 这个成员 owner 已完成 | `first.~HandleOwner()` 运行 | `first` 释放 |

07 章会把这个模式推进到真实 RAII：资源获取写在构造或工厂里，释放写在析构里，复制被禁止或定义为深复制，移动负责转交所有权。

## 练习入口

完成 [L04_construction](../exercises/L04_construction/README.md)。它会记录基类、成员和最外层析构顺序，并用受控的“第二次 acquire 抛异常”验证：构造失败时，未完成的最外层对象不析构，已经完成的成员会析构并释放资源。练习里的资源是有界安全模型，只证明资源计数、事件顺序和异常展开路径；真实堆对象、文件句柄或操作系统句柄还需要各自的释放 API 和错误边界。
