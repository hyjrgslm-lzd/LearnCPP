# C18 规范与实现索引

核对日期：2026-09-14。规范地位、固定实现和实验状态分开。此处列学习基准与能力边界，不记录某次运行的PASS数量。

| 主题 | 地位与采用版本 | 检查边界/一手入口 |
|---|---|---|
| C/C++语言链接 | C11公共头、C++23课程实现；平台ABI仍由工具链定义 | [C++ dcl.link](https://eel.is/c++draft/dcl.link)，L01 C消费者 |
| Windows DLL/CRT | Windows实现机制，不是ISO C++插件协议 | [跨DLL CRT责任](https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries?view=msvc-170)，显式loader/创建销毁 |
| Linux动态装载 | Linux/POSIX实现接口 | [dlopen/dlclose](https://man7.org/linux/man-pages/man3/dlopen.3.html)，释放本次handle不等于全局卸载 |
| CPython Full C API | CPython实现API；主线固定3.10.11，子解释器观察另含3.12.3 | [引用与错误](https://docs.python.org/3.10/c-api/intro.html)、[固定源码](https://github.com/python/cpython/tree/v3.10.11) |
| CPython Limited/Stable ABI | 明确子集，不等同任意Full API | [Stable ABI](https://docs.python.org/3.10/c-api/stable.html)；L09以3.8.10构建，同一pyd验证3.8.10/3.10.11 |
| pybind11 | 第三方绑定库，固定3.0.4 | [固定发布](https://github.com/pybind/pybind11/releases/tag/v3.0.4)、[源码导读](python-source-reading.md)；不是abi3的自动证明 |
| Lua | 第三方嵌入式语言，固定5.4.9、C编译longjmp模型 | [5.4手册](https://www.lua.org/manual/5.4/manual.html)、[5.4.9源码头](https://www.lua.org/source/5.4/lua.h.html)、[源码导读](lua-source-reading.md) |
| LLVM/Clang LibTooling | 编译器实现API，固定llvmorg-18.1.3 | [固定源码](https://github.com/llvm/llvm-project/tree/llvmorg-18.1.3/clang)、[源码导读](clang-source-reading.md)；CLI与开发库分别探测 |
| free-threaded C API | CPython3.13实现路线，独立构建/ABI；与Limited路径分开 | [官方移植指引](https://docs.python.org/3.13/howto/free-threading-extensions.html)、F01真实原生主体 |
| C++26/29反射及生成演进 | 规范/提案归属由C04主讲 | [C04规范索引](../../C04_Generic_CompileTime_Reflection/references/standards-and-implementations.md)；C18不以宏包装冒充语言反射 |

源码阅读使用上述tag/release。滚动文档只辅助查找，若行为与固定源码不一致，注明版本差异，不静默切换实现。pybind11、Lua、LLVM及CPython的许可随上游源码说明核对；本课程不包含下载或重新分发整套依赖的动作。

能力分层：命令存在→版本匹配→头及库匹配→最小编译/链接→真实主体→边界/负例→完整集成。任何前一层的成功都不能替代后层。未安装依赖仍保留实际实现及实验规格；不以模拟接口、假头文件或其他后端充数。
