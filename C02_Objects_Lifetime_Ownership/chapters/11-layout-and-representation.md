# 11 布局、对齐与对象表示

本章解决三个问题：对象在内存中占据哪段存储，起始地址必须满足什么对齐，哪些字节能代表值。后面的存储槽、别名规则、UB 诊断和 `object_buffer<T>` 都依赖这三个问题。这里要养成一个习惯：把“标准保证”和“本机观察”分开写。

C++ 标准规定对象占据一段存储，对象有类型、生命期、地址和值。实现负责选择 ABI 细节，例如 `int` 的大小、结构体填充策略、过对齐对象的分配方式。课程实验会打印当前 MSVC/Clang 环境的数值，但 checker 不把这些数值当成跨平台保证。

## 对象、字节与地址

`sizeof(T)` 给出对象大小，单位是 `char` 字节。标准保证 `sizeof(char) == 1`，但这个 1 是 C++ byte，不是承诺硬件 byte 一定等于 8 bit。现代常见平台上 `CHAR_BIT == 8`，课程可以记录它，但协议代码不能只靠直觉。

`alignof(T)` 给出 `T` 对象起始地址必须满足的对齐。若一个 `double` 要求 8 字节对齐，那么通过普通 `double*` 访问一个未按 8 对齐的地址是未定义行为。这个规则与 CPU 是否“碰巧能读”无关；标准层面已经越界。

```cpp
#include <cstddef>
#include <new>

struct alignas(32) Wide {
    double lane[4];
};

void ok() {
    void* raw = ::operator new(sizeof(Wide), std::align_val_t{alignof(Wide)});
    auto* p = ::new (raw) Wide{};
    p->lane[0] = 1.0;
    p->~Wide();
    ::operator delete(raw, std::align_val_t{alignof(Wide)});
}
```

这个例子同时展示两个边界：过对齐类型要使用匹配的 aligned allocation/deallocation；placement new 开始对象生命期。若省掉 aligned 版本，某些平台仍可能返回足够对齐的地址，但那不是你能写进 portable 代码的不变量。

## 成员顺序、填充与尾部填充

看一个常见结构体：

```cpp
struct Packet {
    char tag;
    int value;
};
```

非静态数据成员按声明顺序分配。实现不能为了省空间把 `value` 放到 `tag` 前面。实现可以在 `tag` 后插入填充，让 `value` 满足 `int` 对齐；也可以在对象尾部插入填充，让数组中的下一个 `Packet` 仍满足 `Packet` 对齐。因此 `sizeof(Packet)` 常见为 8，而不是 5。

这类尺寸是本机事实，不是标准保证。标准不保证 `sizeof(int) == 4`，也不保证 `Packet` 的填充字节数量。你可以用 `static_assert(alignof(Packet) >= alignof(int));` 表达标准层面的关系，但不能对所有平台断言 `sizeof(Packet) == 8`。

尾部填充会影响二进制协议。下面的写法错误地把 C++ 对象布局当成 wire format：

```cpp
Packet p{'A', 42};
// 错误协议模型：把 sizeof(Packet) 个字节直接写到网络。
// 填充字节内容不稳定，int 字节序也未由 C++ 标准固定。
```

正确做法是显式编码字段：写 1 字节 tag，再按协议规定的字节序写整数。布局知识能帮助你理解成本，不能替代协议定义。

## 数组连续性与指针边界

数组元素连续存放。对 `T a[N]`，`a + i` 在 `0 <= i <= N` 时可以形成指针；`a + N` 是 one-past 指针，可以比较、相减，不能解引用。形成 `a + N + 1` 已经越过允许范围。

```cpp
int a[3]{1, 2, 3};
int* begin = a;
int* end = a + 3;       // OK: one-past
auto count = end - begin; // OK: 3
// *end;                // UB: one-past 不是对象
// int* bad = a + 4;    // UB: 超出允许形成的范围
```

这个规则直接约束 `object_buffer<T>`。即使容器分配了 `capacity_` 个槽，只有 `[0, size_)` 是活跃对象。`data_ + capacity_` 可以作为分配数组边界的 one-past；`data_ + size_` 是已构造元素范围的 one-past。`view()` 必须返回 `std::span<const T>{data_, size_}`，不能把未构造槽暴露成元素。

## 对象表示和值表示

对象表示是对象占用的全部字节，包括填充。值表示是参与表达值的那部分 bit。任何对象的对象表示都可以通过 `char`、`unsigned char` 或 `std::byte` 观察；这个访问特权只允许看字节，不允许把对象生命期或类型身份改成别的东西。

```cpp
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <type_traits>

struct Padded {
    char c;
    int i;
};

static_assert(std::is_trivially_copyable_v<Padded>);

std::array<std::byte, sizeof(Padded)> snapshot(Padded p) {
    std::array<std::byte, sizeof(Padded)> bytes{};
    std::memcpy(bytes.data(), &p, sizeof p);
    return bytes;
}
```

`Padded` 是 trivially copyable，所以可以用 `memcpy` 保存并恢复同类型对象的对象表示。它仍可能有填充字节。两个 `Padded` 值相等，不要求它们的填充字节相等；两个对象表示完全相等，通常能推出同类型值相等，但反向不成立。`std::has_unique_object_representations_v<T>` 可以帮助判断“每个值是否只有唯一对象表示”，但很多合法类型不满足它。

`std::bit_cast<To>(from)` 创建一个新的 `To` 值，要求两端大小相同且都是 trivially copyable。它不是 `reinterpret_cast` 的安全包装，也不会让源对象同时具有目标类型。第 13 章会继续区分“复制位模式”和“通过某类型访问同一对象”。

## standard-layout 的用途和边界

`std::is_standard_layout_v<T>` 表示类型满足一组布局限制，适合 C ABI、硬件寄存器描述或协议 header 的一部分需求。它给你的不是“没有填充”，也不是“任何编译器都同样打包”。它主要让一些规则可用，例如 standard-layout 类对象地址与第一个非静态数据成员地址之间的关系。

```cpp
struct Header {
    std::uint16_t kind;
    std::uint16_t size;
};

static_assert(std::is_standard_layout_v<Header>);
```

即便 `Header` 是 standard-layout，你仍要处理整数宽度、字节序和对齐。若对外二进制格式要求固定 layout，通常还需要显式序列化，或者在受控平台上配合 `static_assert(sizeof(Header) == expected)`，并把这个断言写成本项目 ABI 约束，而不是 C++ 标准保证。

## trivially copyable 的用途和边界

`std::is_trivially_copyable_v<T>` 表示同类型对象的字节复制有标准支持。它常用于缓存、序列化中间缓冲、无锁结构中的快照。但它不是“这个类型没有不变量”，也不是“可以任意别名访问”。

```cpp
struct Plain {
    int x;
    double y;
};

static_assert(std::is_trivially_copyable_v<Plain>);

Plain a{1, 2.0};
Plain b{};
std::memcpy(&b, &a, sizeof a); // OK: b 是已存在 Plain 对象，得到 a 的值
```

反例：

```cpp
struct Owner {
    Owner();
    ~Owner();
    int* p;
};

// 不能通过 memcpy 复制 Owner 来制造第二个 owner。
```

资源拥有者通常不是 trivially copyable；即使某个类型技术上满足 trait，也要问清它的语义是否允许 bitwise snapshot。trait 是标准性质，不自动等于业务契约。

## EBO 与 `[[no_unique_address]]`

空类对象本身也有非零大小，因为两个同类型完整对象必须能有不同地址。作为基类时，empty base optimization 允许实现把空基类放进派生对象已有存储中：

```cpp
struct Tag {};
struct Holder : Tag {
    int value;
};
```

常见 ABI 下 `sizeof(Holder) == sizeof(int)`。这是允许优化，不是二进制协议保证。C++20 的 `[[no_unique_address]]` 把类似能力给了成员：

```cpp
struct StatelessDeleter {};

struct PtrBox {
    int* ptr;
    [[no_unique_address]] StatelessDeleter deleter;
};
```

带 `[[no_unique_address]]` 的成员可能与其他成员共享地址。这会影响第 13 章的透明替换和 `std::launder` 条件：有些子对象身份不适合用旧指针继续访问。不要把“地址数值相等”误当成“同一个可访问对象”。

## 本章实验怎么读

`L11_layout` observation 会输出本机 `sizeof/alignof`、数组步长、standard-layout/trivially-copyable、EBO/`[[no_unique_address]]` 观察。通过条件只检查标准保证，例如数组步长等于 `sizeof(T)`、one-past 不被解引用、trait 与代码示例一致。本机尺寸作为记录项，用于让你看到 ABI 选择，不用于跨平台断言。

学习本章后，你应该能给每个底层判断标注类别：标准保证、本项目 ABI 约束、本机观测值，或根本没有保证。后面所有手写存储代码都按这个标注工作。
