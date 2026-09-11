# 标准、实现与工具链

核对日期：2026-09-11。编译器模式、特性实现、标准库和运行后端是不同证据轴。本页记录输入，不把尚未执行的主体写成通过。

## 固定规范

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：C++26固定草案；其DIS关系见[N5051编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5051.html)。
- [N5054](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5054.pdf)：C++29工作草案；与已采纳增量/编辑修正的关系见[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)。
- [execution条款导航](https://eel.is/c++draft/exec)仅用于跳转；动态网页不替代固定草案版本。按[exec.async.ops]、[exec.opstate]、[exec.connect]、[exec.getcomplsigs]、[exec.then]、[exec.run.loop]、[exec.task]、[exec.scope]追踪本课论证。

## 本课代码输入

stdexec固定tag `nvhpc-26.05`，SHA `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。默认只使用显式本地checkout，配置不调用其会下载rapids-cmake/ICM的上游CMake；需要编译的parallel_scheduler后端单独构建。

| 代码层 | 身份与验证口径 |
|---|---|
| std::execution | 所选工具链的原生标准库；逐设施probe后编译真正的标准接口主体。 |
| stdexec:: | 固定上游参考实现；与规范逐协议对照，不能因命名相似直接判定相同。 |
| exec:: | 固定上游扩展；例如async_scope、实验task必须和标准counting scope/task分开说明。 |
| nvexec:: | NVIDIA异构扩展；需实际兼容的nvc++、设备与运行时。 |
| c10教学实现 | 每单元声明支持的类型、通道、执行模型和资源上限，不宣称实现全部标准。 |

源码基地址：[固定提交](https://github.com/NVIDIA/stdexec/tree/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43)。G1对照`examples/algorithms/then.hpp`及`include/stdexec/__detail/__then.hpp`；运行时对照`include/stdexec/__detail/__run_loop.hpp`；task/scope从`include/stdexec/__detail/__task.hpp`、`__counting_scopes.hpp`进入，扩展另读`include/exec/task.hpp`、`async_scope.hpp`；设备从`include/nvexec/stream_context.cuh`进入。各正文须展开对象与退出路径，源码链接不能替代讲解。

## 语言与平台基线

- Windows：MSVC19.51、CMake4.2.3；C++26请求映射/std:c++latest。映射只选择模式，各设施仍需真实实例化和必要链接。
- WSL：GCC13.3接受C++23，不接受-std=c++26；Clang18.1接受C++26，宿主标准库能力单独检查。预处理已观察到Clang18配当前libstdc++时expected宏缺失，因此不能把接受c++26当作完整C++23标准库支持。
- [HPC SDK26.5参考指南](https://docs.nvidia.com/hpc-sdk/archive/26.5/compilers/hpc-compilers-ref-guide/index.html)：nvc++接受C++23/26，但6.4列明C++26语言支持子集，6.5说明多数标准库依赖GCC。未来GPU目标同时检查language/host library/device compilation。
- 现有CUDA Toolkit13.2.1、NVCC13.2.78和RTX4500 Ada不等于具备nvc++。固定stdexec README及[当前官方要求](https://github.com/NVIDIA/stdexec#compiler-support)要求nvexec使用nvc++。本机HPC SDK缺失记`NVEXEC_NO_COMPILER`，不是GPU程序运行PASS。

liburing复用2.15固定SHA `d41bf9220ec39277ff235379e9089d9e0fd6c2a5`。io_uring必须区分库准备、kernel/opcode能力与实际请求完成；IOCP取消提交也不等于目标操作已经完成。

## 状态规则

选项关闭为NOT_ENABLED；最小独立probe确认能力缺失为SKIP；探针通过后的主体失败是FAIL；准备/权限基础设施/清理问题是FAIL或BLOCKED。参考实现、模型、原生标准与实际GPU分别统计。本机验证记录需绑定命令、源码、二进制和数据版本后才登记已验证结果。
