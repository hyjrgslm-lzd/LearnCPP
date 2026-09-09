# 13 别名、类型访问与指针来源

`reinterpret_cast` 能把很多指针写成另一个指针类型，但转换表达式成功不代表访问合法。底层 C++ 要同时满足三件事：目标存储里有活跃对象；通过该指针类型访问这个对象被允许；指针仍然来自一个可用对象或分配。少一个条件，优化器就可以按标准边界重排或删除你的代码。

本章把“看字节”“复制位模式”“复用存储”“指针来源”分开。这样第 15 章的 `object_buffer<T>` 不会用 `reinterpret_cast` 去弥补没有开始生命期的对象。

## 合法类型访问

一个对象通常只能通过以下类型访问：

- 对象自己的动态类型。
- 对应 cv 限定版本。
- 与它类型相似的有符号/无符号整数类型。
- 在规则允许时，包含该对象的聚合或联合类型。
- `char`、`unsigned char` 或 `std::byte`，用于观察对象表示。

```cpp
float f = 1.0f;
auto* p = reinterpret_cast<int*>(&f);
// int bits = *p; // UB: int 不是 float 对象的合法访问类型
```

这个限制服务于优化。编译器可以假设 `float*` 和不相关的 `int*` 不指向同一个活跃对象，于是缓存加载结果、合并写入或删除看似重复的内存访问。违反类型访问规则后，一次运行得到“想要的 bit”没有证明程序合法。

合法整数相似类型的例子：

```cpp
unsigned int u = 1;
int* signed_view = reinterpret_cast<int*>(&u);
// 通过对应 signed 类型访问属于标准允许集合；结果值仍受表示影响。
```

这不是任意整数互通。`std::uint32_t*` 访问 `float`、`std::uint64_t*` 访问两个 `std::uint32_t`、结构体指针跨类型访问，都需要分别证明类型规则和生命期。

## 字节观察不是对象改名

`char`、`unsigned char` 和 `std::byte` 可以观察任意对象的对象表示：

```cpp
#include <array>
#include <cstddef>
#include <cstring>

std::array<std::byte, sizeof(double)> bytes_of(double x) {
    std::array<std::byte, sizeof(double)> out{};
    std::memcpy(out.data(), &x, sizeof x);
    return out;
}
```

这个函数复制字节，不通过错误类型访问 `double`。复制出的 `std::byte` 数组有自己的对象生命期和值。若你之后把这些字节解释成另一个类型，还要重新满足目标类型的创建和值约束。

错误模型是：

```cpp
alignas(float) std::byte raw[sizeof(float)]{};
auto* fp = reinterpret_cast<float*>(raw.data());
// *fp = 1.0f; // 不能只靠 reinterpret_cast 证明 raw 中已有 float 对象
```

第 12 章已经说明，某些 implicit-lifetime 类型可以由特定操作隐式创建，C++23 还有 `start_lifetime_as`。但判断依据是对象创建规则，不是 `reinterpret_cast` 本身。

## `bit_cast` 与 `memcpy`

`std::bit_cast<To>(from)` 创建一个新的 `To` 值。要求 `sizeof(To) == sizeof(From)`，两端都是 trivially copyable。它适合表达“我要按位得到另一个值”：

```cpp
#include <bit>
#include <cstdint>

std::uint32_t float_bits(float f) {
    return std::bit_cast<std::uint32_t>(f);
}
```

`bit_cast` 不让原来的 `float` 同时成为 `std::uint32_t` 对象。返回值是一个独立整数对象。

`std::memcpy` 有两类常见用法：

```cpp
struct Plain { int x; double y; };
static_assert(std::is_trivially_copyable_v<Plain>);

Plain a{1, 2.0};
Plain b{};
std::memcpy(&b, &a, sizeof a); // OK: b 已经是 Plain 对象
```

以及 C++20 低层隐式创建场景：

```cpp
alignas(Plain) std::byte raw[sizeof(Plain)];
std::memcpy(raw, &a, sizeof a);
// 对 implicit-lifetime Plain，相关规则可让 raw 中的 Plain 对象生命期开始。
```

第二种用法不能推广到所有类型。若 `Plain` 换成需要构造函数登记资源的类型，复制字节不会调用构造函数，也不会建立资源所有权。

## 存储复用与 `std::launder`

销毁对象后在同一存储中构造新对象，是 `optional`、variant、对象池和本课 `storage_slot<T>` 的基础。旧指针何时还能用，取决于透明替换条件。

透明替换的大意是：新旧对象类型相同，覆盖同一完整对象存储，新旧对象不是 const 完整对象，也不是某些基类子对象或带 `[[no_unique_address]]` 的特殊子对象；在这些条件下，旧指针、引用或对象名可以自动指向新对象。

```cpp
#include <memory>
#include <new>

struct Node { int value; };

void transparent() {
    Node n{1};
    Node* p = &n;
    std::destroy_at(p);
    std::construct_at(p, Node{2});
    int x = p->value; // 同类型完整对象透明替换，通常可用
    (void)x;
}
```

`std::launder` 用于不满足自动透明替换、但你已经通过正确方式创建了新对象的场景。它告诉优化器重新取得这段存储里的对象指针：

```cpp
struct Base { int b; };
struct Derived : Base { int d; };

alignas(Derived) std::byte raw[sizeof(Derived)];
auto* d = std::construct_at(reinterpret_cast<Derived*>(raw), Derived{{1}, 2});
Base* base = d;
std::destroy_at(d);
auto* d2 = std::construct_at(reinterpret_cast<Derived*>(raw), Derived{{3}, 4});
Base* refreshed = std::launder(reinterpret_cast<Base*>(d2));
(void)base;
(void)refreshed;
```

这个例子里要点不是具体是否必须 launder 某个表达式，而是边界：`launder` 不开始生命期，不修复未对齐，不修复错类型访问，不让释放后的存储重新有效。它只在“新对象已经合法存在”之后刷新可用指针。

## 指针来源和整数往返

指针不是裸地址整数。标准 wording 用对象、存储实例和可达字节描述一个指针能访问什么。把指针转成整数、做运算、再转回指针，即使数值碰巧等于某个对象地址，也不自动恢复访问权。

```cpp
#include <cstdint>

int a = 1;
int b = 2;
auto ia = reinterpret_cast<std::uintptr_t>(&a);
auto ib = reinterpret_cast<std::uintptr_t>(&b);
(void)ia;
(void)ib;
// 不能通过拼整数、猜地址、或跨 allocation 运算制造可访问指针。
```

这类规则在真实优化里很重要：allocator 释放一块内存后，之后另一次 allocation 可能返回相同数值地址，但旧指针的生命期和来源已经结束。旧指针不能因为“地址又一样”而复活。

## C++29 provenance / invalid pointer / lifetime-end 相关 DR

N5055 把 pointer provenance、invalid pointer、lifetime-end 等 DR 列入 C++29 工作底稿相关变更。课程采用保守可移植模型：

- 指针只访问其来源对象或 allocation 的可达范围。
- 对象生命期结束后，旧指针不能继续用于访问；即使地址被复用，也要从新对象取得新指针。
- 把指针变成整数只适合记录、哈希或诊断，不作为跨对象寻址机制。
- 当前工具链没有可靠运行时 probe 能证明这些 DR 的完整语义；本课把它们作为标准 wording 趋势和代码审查规则，不把宏存在当成行为支持。

这不是“未来才需要关心”。它解释了为什么本章从现在开始禁止用整数地址或错类型指针绕过 owner、view 和 lifetime 边界。

## 正反例汇总

| 目标 | 推荐写法 | 常见错误 |
| --- | --- | --- |
| 查看浮点 bit | `std::bit_cast<std::uint32_t>(f)` | `*reinterpret_cast<std::uint32_t*>(&f)` |
| 保存同类型平凡对象 | 已存在目标对象上 `std::memcpy` | 字节复制资源 owner |
| 复用存储 | `construct_at` / `destroy_at`，必要时 `launder` | 只改指针类型就访问 |
| 暂存借用 | 保存 `span<const T>` 并约束 owner 生命期 | owner 扩容/销毁后继续读 span |
| 地址诊断 | 打印 `static_cast<const void*>(p)` | 整数拼接制造新指针 |

## 本章实验怎么读

`L13_aliasing` observation 只运行合法路径：`bit_cast`、`memcpy`、字节观察、同存储重建和需要刷新指针的模型。错类型访问、释放后复用旧指针、整数伪造指针作为正文反例，不默认运行真实 UB。checker 证明的是“学生能分类并使用安全替代写法”，不是证明某个 UB 在当前机器一定崩溃。
