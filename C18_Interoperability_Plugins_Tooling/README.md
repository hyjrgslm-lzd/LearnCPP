# C18：互操作、插件与语言工具

面向已有 C++ 编程经验的工程师，沿一个二进制记录处理案例，学习代码和对象跨过模块、语言、线程及编译工具边界时需要满足的协议。正文是主线，练习用于检验理解；能编译、观察运行成功、完成实现作业分别报告。

从[00：路线与先修](chapters/00-route.md)开始。先修只取 C01 的编译/ABI、C02 的生命周期、C03 的错误、C05 的字节表示、C07 的装载及 C08 的必要同步单元；Clang 路线衔接 C04，不要求先学完所有领域课程。

## 阅读路线

| 单元 | 连续正文 | 对应实践 |
|---|---|---|
| C ABI 基础 | [01 语言链接](chapters/01-c-abi.md) → [02 版本协商](chapters/02-versioned-contract.md) → [03 所有权与错误](chapters/03-ownership-errors.md) | [L01 C调用方](exercises/L01_c_linkage/README.md)、[L02 缓冲](exercises/L02_buffers_errors/README.md)、[L03 函数表](exercises/L03_versioned_api/README.md) |
| 动态插件 | [04 加载与回滚](chapters/04-loader-lifecycle.md) → [05 回调与排空](chapters/05-callback-drain.md) | [L04 关闭样章](exercises/L04_plugin_shutdown/README.md) |
| Python 原生边界 | [06 ctypes](chapters/06-ctypes.md) → [07 引用与错误](chapters/07-cpython-refs-errors.md) → [08 类型与buffer](chapters/08-types-buffers.md) → [09 嵌入与GIL](chapters/09-embedding-gil.md) | [L05](exercises/L05_ctypes/README.md)、[L06](exercises/L06_cpython/README.md)、[L07](exercises/L07_python_buffers/README.md)、[L08](exercises/L08_embedding/README.md) |
| Python 兼容与绑定 | [10 Limited API](chapters/10-limited-api.md) → [11 pybind11](chapters/11-pybind11.md) | [L09](exercises/L09_limited_api/README.md)、[L10](exercises/L10_pybind11/README.md) |
| Lua | [12 栈与引用](chapters/12-lua-stack.md) → [13 保护调用](chapters/13-lua-protected-errors.md) → [14 userdata与GC](chapters/14-lua-userdata-gc.md) → [15 协程与线程](chapters/15-lua-coroutines-threads.md) | [L11](exercises/L11_lua_stack/README.md)、[L12](exercises/L12_lua_protected/README.md)、[L13](exercises/L13_lua_userdata/README.md)、[L14](exercises/L14_lua_coroutines/README.md) |
| Clang | [16 编译语境与AST](chapters/16-compilation-database-ast.md) → [17 匹配](chapters/17-ast-matchers.md) → [18 静态检查](chapters/18-static-checks.md) → [19 重构](chapters/19-source-rewrite.md) → [20 生成](chapters/20-code-generation.md) | [L15](exercises/L15_ast/README.md)、[L16](exercises/L16_static_check/README.md)、[L17](exercises/L17_rewrite/README.md)、[L18](exercises/L18_codegen/README.md) |
| 集成、成本与前沿 | [21 集成责任](chapters/21-runtime-boundaries.md)、[22 成本](chapters/22-boundary-cost.md)、[23 新运行时与反射](chapters/23-frontier-bridges.md)、[24 源码回访](chapters/24-source-bridges.md) | [P1 工作台](exercises/P1_workbench/README.md)、[P2 ABI工具](exercises/P2_abi_tool/README.md)、[B01](exercises/B01_boundary_cost/README.md)、[F01](exercises/F01_runtime_frontier/README.md) |

## 共同实验契约

输入是显式长度的字节序列：将 ASCII a-z 转为 A-Z，其余字节包含 NUL/0xff 全部保留。输出由调用方持有；容量不足不写入输出；借用只覆盖调用期间。插件函数表经版本协商后才可使用，所有调用与跨语言对象结束后才能销毁状态并释放本次模块句柄。

Python 和 Lua 都是完整学习路线，各自保留对象、错误、线程与退出模型。C++ status、Python error indicator 和 Lua protected call 不能通过相同名字视为同一种协议。P2 使用真实 Clang AST/LibTooling，不将字符串替换或 AST dump 包装成同等实现。

## 构建与验证

见[构建指南](exercises/BUILD_GUIDE.md)。默认离线构建核心；Python Full C API、Limited API、pybind11、Lua、LibTooling分别显式开启，不自动下载依赖。各题支持单独构建，Student 与 Reference/checker控制组分开。

缺依赖时仍保留完整教材与实际实现。选项关闭记为 disabled，能力缺失的探针可以 SKIP；没有运行的主体是未验证，已经具备能力却构建或执行失败是 FAIL。ctypes 通过不能替代原生C API，AST观察通过不能替代P2的三个子命令。

覆盖、版本与源码入口见[覆盖表](references/coverage.md)、[规范与实现索引](references/standards-and-implementations.md)。运行日志、测量及审查记录只保存于忽略的 `build/`，教材不依赖某一次本机记录。
