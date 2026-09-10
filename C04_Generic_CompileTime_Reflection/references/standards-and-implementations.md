# C04 规范、实现与源码输入

核对日期：2026-09-09。状态分两轴：规范是已发布标准/指定草案/DR/未采纳提案；实验是本机实际通过/能力缺失/未启用/未验证/失败。浏览支持表不等于运行过具体接口。

## 固定规范

- C++23使用[N4950](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf)。模板规则按`temp`、`temp.deduct`、`temp.res`、`temp.constr`、`expr.const`等稳定条款名导航，不将工具的一条诊断当作规范全文。
- C++26使用[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)；[N5051](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)确认其为C++26 DIS基础。
- C++29使用[N5054](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5054.pdf)与[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)。P3670R4模板名pack indexing为新增条目；P4101R1 consteval-only值按C++26 DR记录，不因编辑报告年份误归新语言功能。
- [P3385R8](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3385r8.html)是目标C++29的属性反射提案；不能与C++26 annotations混为一谈。

## 前沿能力逐项核对

| 设施 | 规范线索 | 实验边界 |
|---|---|---|
| 反射、查询、splicing | P2996R13及N5050后续整合 | 真实`<meta>`与语法；手工描述器只是C++23对照 |
| annotations | P3394R4 | 与编译器attributes不同；名称映射/格式化投影分别检查 |
| 展开语句 | P1306R5 | 独立检查，不能从反射宏推断所有展开形式可用 |
| 静态生成与存储 | P3491R3、反射查询/生成条款 | `define_static_*`和`define_aggregate`按实际表达式分别验证 |
| pack indexing | P2662R3 | C++26类型/值包，与C++29模板名包分开 |
| 折叠约束、常量求值演进 | P2963R3、P2747R2、P3068R6等对应条款 | 标准模式差异单列，不能用C++23旧规则验证C++26新行为 |
| 模板名包索引、conditional noexcept复合要求与consteval-only DR | N5054/N5055、P3670R4、P3822R2 | 资料与编译器实现可能不同步，未测项保留 |

[GCC官方支持表](https://gcc.gnu.org/projects/cxx-status.html)列出GCC16的反射入口需要`-std=c++26 -freflection`及独立展开支持；仍有后续修正未全部实现。[Clang支持表](https://clang.llvm.org/cxx_status.html)需按每项能力判断，不因`-std=c++2c`可选就推断反射可用。本任务不安装工具链、不上传课程源码到远端编译器。提供实验源码、条件与命令，并按本机实际能力报告。

## 本机输入与源码导读

MSVC工具集目录`D:/VisualStudio2026/Installed/VC/Tools/MSVC/14.51.36231`，cl文件版本19.51.36256.0，STL `_MSVC_STL_UPDATE=202604L`。本次只读确认include下没有`meta`。编译器预定义的语言特性宏须由真实编译探测读取，不能只搜索STL头文件。

本机STL `type_traits`的`_Invoker_*`和`invoke`、`xutility`的`ranges::_Begin`是源码导读入口。它们是安装版本实现，文件SHA绑定具体输入，不伪称是某个上游Git提交；教学API与内部实现的命名及优化策略分别说明。

clang-cl22.1.3的上游版本标识为`e9846648fd6183ee6d8cbdb4502213fcf902a211`。用[官方时间trace说明](https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-ftime-trace)解释采样字段及插桩成本。本机附带llvm-nm、llvm-readobj、llvm-size。它们的存在只证明工具入口，具体实验结果另记。

C10的当前repo源码及练习用于回访；其FetchContent滚动`main`不作为本课固定源码版本证据，也不为本课验证触发下载。
