# C18 知识覆盖与反向追踪

此表记录教材与源码的固定对应关系；某次构建、测量或审查的状态另存本机忽略目录。每个练习README列出学生编辑位置、Reference、独立good/bad及Part解析。观察型入口明确自己的证明范围，不当作学生实现完成。

| 下游问题与能力 | 主讲位置 | 练习/项目与解析 | 必要先修 |
|---|---|---|---|
| C调用C++、符号与语言链接、二进制NUL | [01](../chapters/01-c-abi.md) | [L01](../exercises/L01_c_linkage/README.md) | C01翻译单元/符号、C05字节 |
| ABI版本、结构前缀、必需函数、失败初始化 | [02](../chapters/02-versioned-contract.md) | [L03](../exercises/L03_versioned_api/README.md) | C01 ABI、C03提交点 |
| 缓冲长度/重叠/错误、不跨模块free | [03](../chapters/03-ownership-errors.md) | [L02](../exercises/L02_buffers_errors/README.md) | C02借用、C03异常安全 |
| 装载路径、符号、create/destroy回滚 | [04](../chapters/04-loader-lifecycle.md) | [L04](../exercises/L04_plugin_shutdown/README.md)、[P1](../exercises/P1_workbench/README.md) | C07动态装载 |
| 回调重入、active lease、drain、失败重试 | [05](../chapters/05-callback-drain.md) | [L04](../exercises/L04_plugin_shutdown/README.md) | C08锁/同步、C02对象存活 |
| Python ctypes结构/函数原型/回调owner | [06](../chapters/06-ctypes.md) | [L05](../exercises/L05_ctypes/README.md) | C ABI前三章 |
| new/borrowed/stolen、error indicator | [07](../chapters/07-cpython-refs-errors.md) | [L06](../exercises/L06_cpython/README.md) | C02所有权、C03错误 |
| 原生类型、buffer exporter/consumer、存活 | [08](../chapters/08-types-buffers.md) | [L07](../exercises/L07_python_buffers/README.md) | Python引用协议、C05长度 |
| 嵌入、GIL、外部线程和finalize | [09](../chapters/09-embedding-gil.md) | [L08](../exercises/L08_embedding/README.md)、P1 Python | C08同步、解释器引用 |
| Limited API与同一二进制跨minor | [10](../chapters/10-limited-api.md) | [L09](../exercises/L09_limited_api/README.md) | ABI版本、Full C API |
| pybind11 holder/policy/keep_alive、继承回调 | [11](../chapters/11-pybind11.md) | [L10](../exercises/L10_pybind11/README.md) | C02借用、C03多态、Python GIL |
| Lua虚拟栈、registry、upvalue、模块 | [12](../chapters/12-lua-stack.md) | [L11](../exercises/L11_lua_stack/README.md) | 指针与显式长度 |
| Lua protected error、longjmp与RAII | [13](../chapters/13-lua-protected-errors.md) | [L12](../exercises/L12_lua_protected/README.md) | C++析构、错误通道 |
| userdata/metatable、GC、所有权配对 | [14](../chapters/14-lua-userdata-gc.md) | [L13](../exercises/L13_lua_userdata/README.md) | Lua栈/registry、C02 |
| continuation、owner线程、预算与关闭 | [15](../chapters/15-lua-coroutines-threads.md) | [L14](../exercises/L14_lua_coroutines/README.md)、P1 Lua | Lua错误/对象、C08 |
| 编译数据库、AST与隐式节点 | [16](../chapters/16-compilation-database-ast.md) | [L15](../exercises/L15_ast/README.md) | C01编译选项、C04模板 |
| RAV、ASTMatchers、SourceManager | [17](../chapters/17-ast-matchers.md) | [L16](../exercises/L16_static_check/README.md) | AST上下文与类型 |
| 静态ABI危险签名诊断 | [18](../chapters/18-static-checks.md) | [L16](../exercises/L16_static_check/README.md)、P2 check | C ABI/AST |
| token范围、宏/模板拒绝、跨TU冲突、幂等 | [19](../chapters/19-source-rewrite.md) | [L17](../exercises/L17_rewrite/README.md)、P2 rewrite | SourceManager/解析语境 |
| 导出清单、生成代码、真实消费方 | [20](../chapters/20-code-generation.md) | [L18](../exercises/L18_codegen/README.md)、[P2](../exercises/P2_abi_tool/README.md) | ABI契约、C01编译 |
| 端到端相同字节契约与独立后端 | [21](../chapters/21-runtime-boundaries.md) | [P1](../exercises/P1_workbench/README.md) | 对应后端完整路线 |
| 调用粒度/复制成本、测量归因和代价 | [22](../chapters/22-boundary-cost.md) | [B01](../exercises/B01_boundary_cost/README.md) | C13测量、转换正确性 |
| 子解释器/free-threaded、静态反射桥接 | [23](../chapters/23-frontier-bridges.md) | [F01](../exercises/F01_runtime_frontier/README.md) | Python生命周期、C04 |
| 从源码回访协议与退出路径 | [24](../chapters/24-source-bridges.md) | [Python](python-source-reading.md)、[Lua](lua-source-reading.md)、[Clang](clang-source-reading.md) | 各路线对应机制 |

## 结课反向检查

P1使用的每个owner、错误转换和线程责任必须回到前五章及相应语言章节；P2使用的每个类型判定、源位置和替换决策必须回到16—20章。若实际代码引入前文没有展开的机制，应回补主讲和检查，不能用“参见答案”补足先修。

本课程为新增C18，不迁移或删除既有课程。C01/C07保留基础ABI和装载，C04保留语言反射，C08保留通用同步；C18负责这些机制在跨边界应用中的额外责任。
