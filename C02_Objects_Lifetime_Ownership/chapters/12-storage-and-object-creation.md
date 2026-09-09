# 12 存储与对象创建

上一章讲对象布局时，我们一直默认对象已经存在。本章把这件事拆开：程序先取得一段存储，然后某些规则让对象生命期开始。这个拆分是 `storage_slot<T>`、`std::vector`、`std::optional` 和本课 `object_buffer<T>` 的基础。

要避免一个常见错误：不能简单说“`malloc`/`operator new` 只给存储，那里绝对没有对象”。C++20 以后，低层存储操作可以隐式创建一部分类型的对象。正确说法是：

- 分配函数不调用 `T` 的构造函数。
- 对 implicit-lifetime 类型，某些操作可以隐式开始对象生命期，让低层 C 风格代码有定义行为。
- 对需要构造函数建立不变量的普通类，仍然要显式构造。
- `std::allocator<T>::allocate(n)` 会开始一个 `T[n]` 数组对象的生命期，但不会开始每个元素对象的生命期。

本章先用必须显式构造的类型建立主模型，再单独讲 C++20 的例外。这样你不会把例外当成任意字节复活机制。

## 分配、数组对象和元素对象

看一个有析构副作用的类型：

```cpp
struct Handle {
    explicit Handle(int fd) : fd(fd) {
        if (fd < 0) throw std::runtime_error("bad fd");
    }
    ~Handle() noexcept { close(fd); }
    int fd;
};
```

如果你调用普通分配函数取得 `sizeof(Handle)` 字节的存储，这不会调用 `Handle(int)`，也不会建立 `fd >= 0` 这个不变量。此时把地址转成 `Handle*` 并读写成员不是“省掉构造函数”，而是在没有活跃 `Handle` 对象和不变量的前提下访问。

显式构造的最小模型是：

```cpp
void* raw = ::operator new(sizeof(Handle), std::align_val_t{alignof(Handle)});
try {
    auto* p = ::new (raw) Handle(3);   // placement new：开始 Handle 生命期
    std::destroy_at(p);                // 调用析构并结束生命期
    ::operator delete(raw, std::align_val_t{alignof(Handle)});
} catch (...) {
    ::operator delete(raw, std::align_val_t{alignof(Handle)});
    throw;
}
```

`std::construct_at(p, args...)` 是现代代码里更常用的写法，它表达“在 `p` 指向的存储中构造 `T`”。`std::destroy_at(p)` 表达“结束这个对象生命期”。这两个函数仍然要求：`p` 指向的存储大小足够、对齐正确、没有另一个不可重叠的活跃对象占用同一存储。

`std::allocator<T>` 多了一层容易混淆的规则。`allocator<T>::allocate(n)` 返回能放下 `n` 个 `T` 的存储，并按标准要求开始一个 `T[n]` 数组对象的生命期。这个数组对象是为了让指针算术和容器内部模型有对象边界；它不表示 `n` 个元素都已经构造完成。

因此 `object_buffer<T>` 的不变量必须写成：

```text
data_ 指向 allocator<T>::allocate(capacity_) 返回的存储。
数组对象 T[capacity_] 的边界存在。
只有 [0, size_) 中的元素对象生命期已经开始。
[size_, capacity_) 是可用于构造 T 的元素槽，不是可读取的 T 值。
```

这就是为什么 `view()` 返回 `span<const T>{data_, size_}`，不能返回 `{data_, capacity_}`。

## `storage_slot<T>` 的提交顺序

一个单槽对象看似简单，但它能暴露所有关键顺序：

```cpp
template<class T>
class storage_slot {
public:
    template<class... Args>
    T& construct(Args&&... args) {
        if (engaged_) throw std::logic_error("occupied");
        T* p = std::construct_at(ptr(), std::forward<Args>(args)...);
        engaged_ = true;
        return *p;
    }

    void destroy() noexcept {
        if (!engaged_) return;
        std::destroy_at(ptr());
        engaged_ = false;
    }

private:
    T* ptr() noexcept { return reinterpret_cast<T*>(&storage_); }
    alignas(T) unsigned char storage_[sizeof(T)]{};
    bool engaged_ = false;
};
```

构造路径必须先 `construct_at`，后 `engaged_ = true`。构造函数可能抛异常；如果先提交 `engaged_`，失败后状态会声称有对象，析构时就可能销毁未构造对象。

销毁路径可以先销毁再清标志，因为本课要求 `T` nothrow destructible。如果析构可能抛，状态恢复、异常传播和资源释放会互相冲突；这不是本章练习要教的复杂容器问题。

## C++20 implicit object creation

P0593R6 解决的是低层代码长期存在的一类问题：程序拿到字节存储后，想在里面使用“足够平凡”的对象。标准没有把所有类型都纳入这个模型，只覆盖 implicit-lifetime types。

一个类型是 implicit-lifetime type，大致包含：

- scalar types，例如 `int*`、`int`、枚举、浮点。
- array types，数组本身是 implicit-lifetime type，元素类型不要求都是 implicit-lifetime。
- implicit-lifetime class types。
- 上述类型的 cv 限定版本。

implicit-lifetime class type 的条件要精确。它不是“没有复杂不变量”的口语判断，而是标准定义的类型性质：

- 析构函数不是 user-provided 的 aggregate；或
- 至少有一个 trivial eligible constructor，并且析构函数 trivial 且未删除。

这意味着一个类即使有某个非平凡构造函数，只要仍有符合条件的 trivial eligible constructor，并且析构条件满足，也可能是 implicit-lifetime class。反过来，一个带 user-provided 析构的类通常不满足，即使成员都是整数。

以下类型适合用来理解差异：

```cpp
struct Plain {
    int x;
    int y;
}; // aggregate，析构不是 user-provided，可隐式生命期

struct NeedsCtor {
    NeedsCtor(int value) : x(value) {}
    ~NeedsCtor() noexcept {}          // user-provided 析构
    int x;
}; // 不能靠隐式创建跳过构造/析构责任

struct HasNonTrivialCtorButTrivialDefault {
    HasNonTrivialCtorButTrivialDefault() = default;
    explicit HasNonTrivialCtorButTrivialDefault(int value) : x(value) {}
    int x = 0;
}; // 仍可能满足 implicit-lifetime class 条件，不能用“有非平凡构造”简单排除
```

哪些操作会隐式创建对象，也不是“任意 reinterpret_cast 访问”。C++20 的模型把对象创建挂在特定操作上，例如：

- 创建 `char`、`unsigned char` 或 `std::byte` 数组。
- `malloc`、`calloc`、`realloc` 和名为 `operator new`/`operator new[]` 的分配函数。
- `std::allocator<T>::allocate(n)`，但它创建的是 `T[n]` 数组对象边界，不是 `n` 个元素值。
- `std::memcpy`、`std::memmove` 在目标区域按规则隐式创建对象，再复制表示。
- `std::bit_cast` 为结果对象处理表示。

这些规则的目的，是让一组 implicit-lifetime 对象的创建能使程序有定义行为时，由抽象机选择这样的对象集合。若没有任何对象集合能让执行有定义行为，程序仍然是 UB。

因此以下示例在 C++20 后可以作为 Plain 的低层模型讲：

```cpp
void* raw = std::malloc(sizeof(Plain));
auto* p = static_cast<Plain*>(raw);
p->x = 1;
p->y = 2;
std::free(raw);
```

它依赖 Plain 是 implicit-lifetime type，且写成员没有跳过必须运行的构造代码。把 `Plain` 换成 `NeedsCtor`，这个推理就不成立。

## `start_lifetime_as`

C++23 的 `std::start_lifetime_as<T>(p)` 是显式请求：在 `p` 指向的存储中开始 implicit-lifetime 类型 `T` 的生命期，同时保留原对象表示。它不是构造函数调用，不执行初始化表达式，也不是别名规则豁免。

使用前提包括：

- `T` 必须是 implicit-lifetime type。
- `[p, p + sizeof(T))` 必须表示一段已分配存储，且这段存储能通过 `p` 到达。
- `p` 必须满足 `alignof(T)`。
- 创建的对象表示来自调用前的存储内容；对象值是否有效取决于 `T` 对这些位模式的要求。
- 对 `start_lifetime_as_array<T>(p, n)`，区域大小要能容纳 `n` 个 `T`，并满足数组边界和可达性要求。

一个安全观察例子：

```cpp
alignas(std::uint32_t) std::array<std::byte, 4> bytes{
    std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}
};
auto* p = std::start_lifetime_as<std::uint32_t>(bytes.data());
```

这只说明本机小端环境下观察到的整数值可能是 `0x12345678`。标准保证的是对象生命期与表示保留，不保证所有平台字节序相同。

本仓库能力探测记录了实际工具链状态：MSVC 19.51 + 当前 STL 定义 `__cpp_lib_start_lifetime_as=202207` 并能编译运行；Clang 22.1.3 使用当前 MSVC STL 时没有该 feature-test macro，`start_lifetime_as` 探针编译失败。教学材料按“规范存在”和“本机实现支持”分两轴记录。

## union 活跃成员

`union` 多个成员共享同一段存储，但同一时刻通常只有一个活跃成员。初始化哪个成员，哪个成员生命期开始；给另一个成员赋值可能开始新成员生命期并结束旧成员生命期，但这个“可能”有条件。

对平凡成员，直接赋值常能切换活跃成员：

```cpp
union Value {
    int i;
    double d;
};

Value v{.i = 1}; // i 活跃
v.d = 3.5;       // d 活跃，i 生命期结束
```

对非平凡成员，必须显式构造和销毁：

```cpp
union Cell {
    Cell() {}
    ~Cell() {}
    std::string text;
    int number;
};

Cell c;
std::construct_at(&c.text, "hello");
std::destroy_at(&c.text);
std::construct_at(&c.number, 7);
std::destroy_at(&c.number);
```

这里自定义空构造/析构只是阻止编译器自动处理 union 成员。真正开始和结束成员生命期的是 `construct_at` 和 `destroy_at`。

公共初始序列是另一个窄规则。若 union 中两个 standard-layout struct 共享相同开头成员类型和顺序，可以通过非活跃成员读取公共初始部分，用于判别 tag。它不允许读取任意后续成员，也不让两个完整对象同时活跃。

```cpp
struct A { int tag; int x; };
struct B { int tag; double y; };
union U { A a; B b; };

U u{.a = {1, 2}};
int tag = u.b.tag; // 公共初始序列内 OK
// u.b.y;          // 不在公共初始序列，不能读
```

所以 union 不是“安全 type punning 工具”。它表达存储复用；类型访问、对象生命期和活跃成员仍要分别满足。

## 本章练习

`L12_storage` 要实现一个有界 `storage_slot<T>`。checker 使用外部受信计数器验证：

- 新槽为空，空 `destroy()` 不销毁对象。
- 构造成功后才报告 engaged。
- 重复构造抛 `logic_error` 且保留原对象。
- 构造函数抛异常时槽仍为空，不能销毁未构造对象。
- 槽析构会销毁仍活跃的对象。

这个练习故意使用需要显式构造/析构的非 implicit-lifetime 类，避免把 C++20 隐式创建例外当成普通资源类型的通用规则。
