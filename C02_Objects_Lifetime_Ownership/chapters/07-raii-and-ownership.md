# 07 RAII 与所有权

前面几章已经把对象生命期、构造顺序、析构顺序和 move 语义拆开看过。本章把这些规则合在一个工程问题里：一个对象要同时持有两个资源，如果第二个资源获取失败，第一个资源会不会被释放？

这个问题看起来小，但它正是 RAII 的核心。RAII 不是“析构函数里写清理”这么简单。RAII 的意思是：资源的占有状态必须由已经构造完成的对象表示；对象析构时释放它已经拥有的资源；构造失败时，语言只会析构那些已经构造完成的子对象。只要资源没有进入一个已经完成构造的子对象，它就不在自动清理链上。

## 资源不是对象

C++ 对象有生命期，资源通常没有 C++ 语言层面的生命期。文件句柄、socket、锁、GPU buffer、数据库事务、临时目录和本章用的整数资源 id，都是外部状态。语言知道 `int`、`std::string`、类成员什么时候构造和析构，却不知道一个 `int fd` 代表打开的文件。

下面是一个正确但脆弱的手工基线：

```cpp
int first = acquire_first();
int second = acquire_second();

release_second(second);
release_first(first);
```

只看成功路径，它没有问题。两个资源按顺序获取，再按反顺序释放。反顺序释放不是仪式感，而是依赖关系的保守默认：后获取的资源可能依赖先获取的资源，例如映射内存依赖文件句柄，注册回调依赖事件循环。

问题出在新失败路径。假设 `acquire_second()` 抛异常：

```cpp
int first = acquire_first();
int second = acquire_second(); // 抛异常

release_second(second);
release_first(first);
```

控制流不会继续走到两个 `release`。`first` 只是一个整数变量，语言不会猜它需要调用 `release_first(first)`。所以第一个资源遗留了。

这不是未定义行为。本章默认实验不用真实悬空指针或双释放制造崩溃，而是用有界计数器记录 acquire/release 事件。计数器只能证明这个模型里的资源没有归零，不能证明所有真实系统都会以同样方式失败。真实资源的后果可能是文件没关、锁没放、句柄耗尽、事务没回滚，也可能暂时看不出来。

## 构造失败时会析构什么

现在把手工代码放进构造函数：

```cpp
struct manual_pair {
    int first{};
    int second{};

    manual_pair() {
        first = acquire_first();
        second = acquire_second(); // 抛异常
    }

    ~manual_pair() {
        release_second(second);
        release_first(first);
    }
};
```

如果 `second = acquire_second()` 抛异常，`manual_pair` 这个完整对象没有构造成功。C++ 不会调用 `manual_pair::~manual_pair()`。原因很直接：析构函数负责销毁一个已经完成构造的对象；这里这个对象从来没有完成构造。

但这不代表什么都不会清理。构造函数体抛异常时，C++ 会销毁已经构造完成的基类和成员。上面的 `first` 和 `second` 都是 `int` 成员，销毁它们不会释放外部资源。资源还没有被交给一个会在析构时释放它的成员对象。

所以部分构造失败的规则可以记成两句：

1. 最外层对象构造失败，它自己的析构函数不运行。
2. 已经构造完成的成员会按逆序析构。

RAII 修复依赖第二句。我们把每个资源放进自己的成员对象中，让“已经获取资源”与“已经构造成员”同步。

## 单资源 owner

最小 owner 只需要四件事：默认空状态、接管资源、析构释放、禁止复制。资源 id 是外部系统给的句柄，复制一个 owner 会让两个对象都以为自己拥有同一个资源，最后重复释放。

```cpp
class first_resource {
public:
    first_resource() = default;
    explicit first_resource(int id) noexcept : id_(id) {}
    ~first_resource() { reset(); }

    first_resource(const first_resource&) = delete;
    first_resource& operator=(const first_resource&) = delete;

    first_resource(first_resource&& other) noexcept
        : id_(std::exchange(other.id_, empty)) {}

    first_resource& operator=(first_resource&& other) noexcept {
        if (this != &other) {
            reset();
            id_ = std::exchange(other.id_, empty);
        }
        return *this;
    }

    void reset() noexcept {
        if (id_ != empty) {
            release_first(id_);
            id_ = empty;
        }
    }

private:
    static constexpr int empty = -1;
    int id_ = empty;
};
```

这段代码里最重要的不是 `std::exchange`，而是状态转移：

- `empty` 表示不拥有资源。
- 析构只释放非空资源，所以空 owner 可以安全析构。
- move 构造把资源 id 交给新对象，并把源对象改为空。
- move 赋值先释放目标已有资源，再接管源对象资源。
- self move 不做事，避免先把自己清空再从自己接管。

这就是独占所有权。一个时刻只有一个 owner 负责释放资源。借用可以存在，例如读取 `id()` 或把句柄临时传给不保存它的函数，但借用方不能释放，也不能比 owner 活得更久。

## 两个资源的 RAII 修复

现在组合两个单资源 owner：

```cpp
struct raii_pair {
    first_resource first;
    second_resource second;

    raii_pair()
        : first(acquire_first()),
          second(acquire_second()) {}
};
```

如果 `acquire_first()` 抛异常，`first` 成员没有构造完成，没有资源需要释放。如果 `acquire_second()` 抛异常，`first` 成员已经构造完成，`second` 成员没有构造完成；语言会析构已经完成的 `first`，于是 `release_first` 自动执行。

这正好覆盖手工代码漏掉的路径。不是因为 `raii_pair::~raii_pair()` 运行了，而是因为 `first_resource::~first_resource()` 运行了。外层对象失败时，清理责任必须放在已经构造完成的成员里。

成功路径也变简单：

```cpp
{
    raii_pair pair;
    use(pair);
} // second 先析构，first 后析构
```

成员按声明顺序构造，按声明逆序析构。初始化列表的书写顺序不改变成员构造顺序。需要 `second` 依赖 `first` 时，成员声明顺序应该先写 `first`，再写 `second`。

## 与 `std::unique_ptr` 的关系

`std::unique_ptr<T>` 是标准库提供的独占 owner。它把“指针为空或拥有一个对象”这个状态模型做好了：析构自动 delete，禁止复制，允许 move，`reset` 释放旧对象，`release` 放弃所有权并把裸指针交出去。

自定义资源不一定能直接用默认 `delete`。这时可以用自定义 deleter：

```cpp
using file_owner = std::unique_ptr<FILE, int(*)(FILE*)>;
file_owner f(std::fopen("data.txt", "r"), &std::fclose);
```

本章练习仍然手写一个最小 owner，因为你需要看清楚 `unique_ptr` 替你维护了哪些状态。但工程代码里，资源形状能被 `unique_ptr`、`std::vector`、`std::lock_guard`、`std::jthread`、`std::fstream` 表达时，优先使用标准设施。手写 owner 的理由通常是资源释放 API 不是 `delete`、需要成对资源、需要记录额外状态，或者教学中要暴露所有权转移。

## 本章实验闭环

练习 L07 使用一个安全资源模型。模型放在公共 checker/support 中，不放在 Student 实现里，所以学生代码不能通过伪造计数器或事件绕过检查：

- `acquire_first` 和 `acquire_second` 只改变计数器并返回 id。
- 失败点可配置为 first acquire 或 second acquire 时抛异常。
- 事件记录使用固定容量缓冲，不在 `noexcept` release 路径分配内存。
- release 会校验资源种类、id 和重复释放，错误释放会记为检查失败。
- checker 比较 alive 计数、事件顺序和 move/reset 后状态。

实验顺序是：

1. 跑观察程序，确认手工基线在成功路径正确。
2. 让第二个 acquire 抛异常，观察手工模型遗留第一个资源。
3. 用构造失败规则解释为什么外层析构函数不运行。
4. 跑 Reference 的 RAII 版本，确认同一个失败点下资源归零。
5. 跑 Student，占位会安全失败；完成 `src/student/owner.hpp` 后再打开学生测试。
6. 跑公开 good/bad 变体，确认 checker 能接受正确实现并拒绝 no-op 假实现。

读完本章后，你应该能判断一个类型是否真的表达所有权：析构是否释放、复制是否会重复释放、move 是否转交、失败路径是否进入已完成成员的自动清理链、借用是否没有越过 owner 的生命期。
