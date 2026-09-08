# 规范、工具链与实现观察

核对日期：2026-09-08。课程以 C++23 既有语言/库能力作为普通示例基线；Modules 语言能力与标准库模块、CMake 构建支持分别记录。当前机器的编译器、生成器或动态库格式不是 C++ 标准规定。

## 证据分层

| 问题 | 应核对的依据 | 本课实际观察入口 |
|---|---|---|
| 声明、定义、ODR、linkage | 语言规范；[basic.def.odr](https://eel.is/c++draft/basic.def.odr)、[basic.link](https://eel.is/c++draft/basic.link)是滚动导航 | B1/C1：预处理、两个对象的符号及两种修复 |
| inline允许什么、优化做了什么 | 规范规则与编译器优化是不同证据；[dcl.inline](https://eel.is/c++draft/dcl.inline) | C1/G2，不从是否出现call指令反推ODR合法性 |
| 哪些符号导出、怎样找到DLL | 平台工具和装载器文档，不是语言保证 | D1的显式/隐式消费与失败对照 |
| 不同MSVC版本能否混用 | [MSVC兼容性说明](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017?view=msvc-170)有链接器、运行库及LTO限制，不能简化成“任意14.x都兼容” | E1/J1固定当前工具链的接口与包消费，不外推所有版本 |
| 谁能释放跨DLL对象 | [CRT边界说明](https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries?view=msvc-170)；不同堆和CRT状态需要区分 | E1在同一模块创建/销毁，不运行错误跨堆释放 |
| exported targets如何重定位 | [CMake导入导出指南](https://cmake.org/cmake/help/v4.2/guide/importing-exporting/index.html)；consumer只能使用安装后的接口 | F2/J1配置文件、使用要求与复制prefix验证 |
| Modules怎样进入构建图 | [CMake modules手册](https://cmake.org/cmake/help/v4.2/manual/cmake-cxxmodules.7.html)；编译器扫描、生成器与file set分别约束 | H1扫描/BMI/consumer及独立模块包 |
| import std怎样启用 | [v4.2.3固定实验说明](https://raw.githubusercontent.com/Kitware/CMake/v4.2.3/Help/dev/experimental.rst)定义该版本gate且要求工具链发现前启用 | I1逐层探测与实际构建；不能拿另一版本的UUID当常量通用接口 |

滚动语言链接用于定位，新的草案变化不能无条件回写成 C++23 规则。官方4.2系列网页可能显示后续补丁版本；涉及实验行为时以本次固定4.2.3源码和实际命令交叉检查，不把网页支持表当作本机构建已经成功。

## 本轮环境和源码阅读

当前可用性在[质量报告](quality-report.md)登记。每个实验保存实际编译链接参数，分别标记选项关闭、探测失败、真实通过、运行库装载失败及未验证平台。

工具链源码阅读从固定版本 CMake 的 `Modules/Compiler/MSVC-CXX-CXXImportStd.cmake` 开始：先找 `_cmake_cxx_import_std` 的输入（标准级别），再追 `modules.json` 的发现、版本/库身份检查、模块源码列表，以及最后产生的编译目标。退出路径包括缺metadata、未知schema/库身份和正常生成目标。用 I1 的实际诊断对照这些分支，而不是只记一个 gate 字符串。此处是工具链源码导读；不代表已运行 CMake 所有编译器分支。

编译产物阅读同时贯穿 B1/C1/C2/D1：先提出具体问题，再查看预处理文本、对象、符号、依赖和导出。ELF与Windows的不同输出格式、命令和链接规则分别解释；未在Linux实测的分支明确保留未验证状态。
