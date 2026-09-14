# F01：解释器能力与原生主体分开验证

先读[前沿与反射桥接](../../chapters/23-frontier-bridges.md)。本单元是观察/源码实验，不设置假 Student。支持状态必须按各入口分别报告。

## Part 1：子解释器的命名空间

```sh
python exercises/F01_runtime_frontier/run_subject.py subinterpreters
```

probe 限定 CPython 3.10/3.12 的 `_xxsubinterpreters` 私有接口；不存在时 SKIP。存在后创建两个真实解释器，分别保存不同 marker，再逐个验证，最终逆序销毁；主体失败/清理失败均 FAIL。

**解析：** 这证明本实验中解释器命名空间与清理流程，不证明某个 C 扩展没有共享全局变量，也不证明多个解释器拥有独立GIL。`_xxsubinterpreters` 是固定 CPython 的教学入口，不作为稳定公共 API 推荐给应用。

## Part 2：free-threaded 原生扩展

`free_native.cpp` 使用 CPython 3.13 Full C API、多阶段初始化和 GIL/多解释器支持槽。输入只接受 exact immutable bytes，输出在发布前独占构造；没有可变进程全局业务状态。它不是 abi3 扩展。

在已有 CPython3.13 free-threaded 开发环境及 CMake>=3.30 的机器，使用独立构建目录：

```sh
cmake -S exercises/F01_runtime_frontier -B build/f01-free -DC18_ENABLE_FREE_THREADED_CAPI=ON -DPython3_EXECUTABLE=/absolute/path/to/python3.13t
cmake --build build/f01-free --config Release
ctest --test-dir build/f01-free -C Release --output-on-failure
```

没有环境时不执行安装。开启选项却缺正确解释器或开发库，配置失败；选项关闭时主体为 disabled/未验证。默认 capability 检查只说明运行时构建能力。

**解析：** capability 成功后，扩展导入、GIL状态、四线程屏障、逐字节结果、可变输入拒绝与线程退出全部属于主体检查。扩展不存在或错误不能再变成 SKIP。导入后若GIL被重新启用，则检查失败。

## Part 3：反射与工具生成的适用面

比较 C04 的静态反射字段访问与 P2 从现有源码提取 ABI 声明：前者属于编译期语言机制，后者需要真实编译语境和源位置。沿[既有 C04 实验](../../../C04_Generic_CompileTime_Reflection/exercises/F01_frontier/README.md)检查能力，不复制一套假反射宏来冒充标准特性。

**解析：** AST 能看到源码位置、宏展开和编译命令；语言反射可以参与编译期生成。二者都不能自动证明跨语言对象存活，生成代码仍需消费方编译和行为检查。
