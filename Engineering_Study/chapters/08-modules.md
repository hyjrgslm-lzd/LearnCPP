# 08：C++ Modules、BMI 与包消费

头文件靠预处理复制文本。每个包含它的翻译单元都会重新解析文本，并可能生成自己的定义。Modules 把接口提升为语言单元：构建系统可以扫描 `export module` 和 `import`，编译器可以先编译接口，再让 importer 引用生成的 BMI/IFC。

## 基本文件角色

Primary module interface 是模块的主接口单元：

```cpp
export module geometry;
export int square_area(int side);
```

每个 named module 必须有且只有一个不带 partition 的 primary module interface。消费者写 `import geometry;` 后，能命名 primary interface 直接或间接导出的声明。

Implementation unit 属于同一 named module，但不导出新接口：

```cpp
module geometry;
int square_area(int side) { return side * side; }
```

它能定义 interface 中声明的函数，也能访问同 module 内部实体。它不是头文件替代品；它是一个正常翻译单元，仍会生成 object 并参与链接。

Partition 把一个 module 拆成多个内部单元：

```cpp
export module geometry:units;
export int unit_scale();
```

primary module 可写：

```cpp
export module geometry;
export import :units;
```

`export import :units;` 会把 partition 的导出声明继续暴露给导入 `geometry` 的消费者。若只写 `import :units;`，partition 的声明只供当前 module 内部使用。外部翻译单元不能直接 `import geometry:units;`；partition 只能被同一 named module 的 module unit 导入。

## Global module fragment

Global module fragment 写在 module declaration 前：

```cpp
module;
#define NOMINMAX
#include <windows.h>
export module platform;
```

它的作用是给 legacy header 和宏配置留一个入口。根据模块语法，`module;` 后到 `export module ...;` 前只能放预处理指令。它不把 header 变成 named module，也不自动导出宏。宏仍属于预处理世界；导出边界仍由 module interface 中的声明决定。

## Private module fragment

Private module fragment 只能出现在 primary module interface 尾部：

```cpp
export module single_file;
export int answer();

module : private;
int answer() { return 42; }
```

它把“会影响 importer 的接口部分”和“只给本模块实现看的部分”分开。限制也很硬：private module fragment 只能出现在 primary module interface。标准还要求含 private fragment 的 named module 只有这一份 module unit；这类违规允许 no diagnostic required，所以不要把“所有工具链都必须报错”设计成门禁。工程上它适合单文件小模块，不适合再配多个 implementation unit 或 partition。

课程的 `H1_modules` 主验证使用 primary interface + implementation unit + partition，因为这更接近工程组织；同一练习也提供 `fragments.ixx` 单文件正例，实际编译运行 global module fragment + private module fragment 的组合。它不和多单元 geometry 混用。

## 可见、可达、名字查找

`export` 先影响名字查找：importer 能直接写出的名字，必须来自被导出的声明。没有 export 的名字不会因为出现在某个导出函数签名里就变成可查找名字。

但“名字不可见”和“声明/定义不可达”不是一回事。下面这个接口是合法的：

```cpp
export module visibility_boundary;

struct hidden_state { int value; };      // 名字没有 export

export hidden_state make_hidden() {      // 导出函数返回该类型
    return hidden_state{42};
}

export int read_hidden(hidden_state const& state) {
    return state.value;
}
```

消费者不能直接命名 `hidden_state`：

```cpp
import visibility_boundary;

int main() {
    hidden_state state = make_hidden();  // 编译失败：名字查找找不到 hidden_state
    return read_hidden(state);
}
```

但消费者可以通过导出函数间接得到这个类型：

```cpp
import visibility_boundary;

int main() {
    auto state = make_hidden();          // OK：不直接命名 hidden_state
    return state.value + read_hidden(state);
}
```

这里 `hidden_state` 的名字不可见，所以 `hidden_state state` 失败；它的定义对 `auto` 推导、成员访问和调用 `read_hidden(state)` 又必须足够可达，否则编译器无法检查对象布局、成员和参数匹配。`H1_modules` 的 `visibility_boundary` 正例验证 `auto state = make_hidden(); state.value; read_hidden(state)` 可编译运行；`H1_hidden_type_negative` 单独验证直接命名 `hidden_state` 的负例在编译阶段失败。

这个例子适合讲语义边界，不适合作为长期库 API 风格。让使用者拿到不可命名类型会降低可读性，也会让显式变量声明、函数签名、容器类型等普通写法受限。工程接口通常把隐藏类型包在已导出的类型、概念、迭代器模式或纯值返回里，避免把“只能用 `auto` 接住”变成 API 要求。

可达性还用于解释构建证据。某个定义或声明必须通过 import 图对当前翻译单元可达，编译器才能检查类型、内联定义、模板定义等。链接库只能在链接阶段解决符号，不能在编译阶段补出声明或 BMI。于是 `CXXModules.json`、`.ddi` 和编译命令中的 IFC/BMI 引用，是比“最终 exe 运行了”更早的一层证据。

## Module ownership、linkage 与 ODR

出现在 module declaration 之后的声明通常附着到该 named module。附着到 named module 的实体只能在同一 module 中定义；不同 module 里同名的非导出实体不会像头文件文本展开那样挤进同一个全局声明空间。下面只讨论本例中的命名空间作用域非 `static`、未导出函数：它们具有 module linkage，属于各自 named module。这比头文件时代“每个 TU 文本展开后靠 inline/ODR 维持一致”的模型更严格。

例子：

```cpp
export module a;
int helper() { return 1; }     // module linkage，属于 a
export int value_a() { return helper(); }

export module b;
int helper() { return 2; }     // 另一个实体，属于 b
export int value_b() { return helper(); }
```

这两个 `helper` 不冲突，因为它们属于不同 named module。相反，如果你把一个全局 module 中声明的实体又在 named module 中定义，可能形成跨 ownership 的 ill-formed 程序；有些情况标准允许 no diagnostic required，所以课程练习不把这种边界写成“必然有清晰报错”。

Primary module interface 被 import 时，它 export-import 的 partition 也会对 importer 可见。普通非 partition implementation unit 属于同一 named module，并隐式导入 primary module interface，因此可以定义 primary interface 中声明的实体；但它自己的非导出声明不会自动给 importer。partition 的导入关系仍要显式写在同 module 的 module unit 中。这个边界是 modules 解决 ODR 和可见性污染的重要部分。

## Header unit、宏和 include
Named module、header unit、普通 include 是三条不同路径。普通 named module 不导出宏；header unit 是把 header 作为可导入单元。直接 import 一个 header unit 会让该 header unit 的宏集合对 importer 可见；这和 named module 不导出普通宏不同。`#include` 则仍是预处理复制文本。

因此不能把：

```cpp
import geometry;
```

解释成“等价 include 了 geometry 的头”。也不能因为某个宏在 module interface 的 global fragment 里存在，就认为 importer 自动得到该宏。公开 API 应靠导出声明，不靠宏侧漏。

## 扫描、BMI 与 DAG

构建系统不能只按文件名猜 module 依赖。它要扫描源码，发现 `export module`、`import`、partition，再生成动态依赖图。CMake + Ninja 的模块构建通常出现这些阶段：

1. compiler scan dependencies，生成 `.ddi`。
2. `cmake_ninja_dyndep` 合并 module map。
3. 编译 module interface，生成 object 和 BMI/IFC。
4. 编译 importer，命令行引用 BMI/IFC。
5. 链接 object/library。

BMI/IFC 是编译器产物，不是稳定分发格式。不同编译器、版本、标准库、flags 之间不能默认复用。包安装应分发 module interface source 和 CMake metadata，让 consumer 在自己的工具链下重建 BMI。

## 安装导出与独立 consumer

本机验证通过的最小包形态使用 `FILE_SET CXX_MODULES`：

```cmake
target_sources(h1_geometry_package PUBLIC
  FILE_SET CXX_MODULES
  BASE_DIRS library
  FILES library/h1_geometry_package.ixx)

install(TARGETS h1_geometry_package EXPORT h1_geometry_package_targets
  ARCHIVE DESTINATION lib
  FILE_SET CXX_MODULES DESTINATION cxx-modules)

install(EXPORT h1_geometry_package_targets
  NAMESPACE H1::
  DESTINATION lib/cmake/h1_geometry_package
  CXX_MODULES_DIRECTORY cxx-modules)
```

独立 consumer 用 `find_package(h1_geometry_package CONFIG REQUIRED)` 导入 target。构建时 CMake 从安装 prefix 找到 module source 和 export metadata，consumer 在自己的 build dir 重新生成 BMI，再链接安装的 `.lib`。这证明包 metadata、module source 安装和 consumer 重定位有效；它不证明 BMI 可跨编译器或跨 flags 复制。

`H1_modules` 同时提供 header baseline、module 变体和 package consumer。header baseline 没有扫描/BMI；module 变体需要构建 DAG；package consumer 证明安装后的 prefix 足够独立。

## 已核对的规范边界

本章机制与 C++20 modules 语法一致：module unit、primary interface、partition、global module fragment、private module fragment、module ownership 和 module linkage 的边界已按标准语义核对。工程实现还受编译器和构建系统限制；本课程只把本机 CMake 4.2.3 + MSVC 19.51 + VS Ninja 1.13.2 验证通过的路径写成“已实测”。其他编译器保留规格说明，不冒称通过。

## 自测

- `export import :part;` 和 `import :part;` 对消费者有什么区别？
- 外部 TU 为什么不能直接 import partition？
- hidden type 可以怎样通过导出 API 间接使用，直接命名为什么失败？
- 为什么安装包不应该承诺 BMI 跨编译器可用？
- private module fragment 为什么不适合 H1 的多单元 geometry 组织？

答案：前者把 partition 导出接口继续暴露，后者只供当前 module 内部；partition 只对同 named module 可导入；隐藏类型可以由导出 factory 返回，消费者用 `auto` 接住后访问可达定义并传给导出函数，但 `hidden_state state` 这种直接命名失败，因为名字没有 export；BMI 依赖编译器、标准库和 flags；private module fragment 只能在 primary interface，且标准要求该 named module 只有这一份 module unit，违规不一定有诊断。
