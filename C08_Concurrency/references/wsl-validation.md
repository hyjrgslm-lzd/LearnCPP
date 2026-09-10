# C08 的 WSL2 验证环境

本轮沿用Windows主工作区与Linux ext4快照两条路径，不迁移主仓库。

| 用途 | 路径 |
|---|---|
| Windows编辑/Git/MSVC | `F:\CPPTrain\LearnCPP` |
| 专用发行版 | `LearnCPP-C08-Ubuntu-24.04` |
| VHDX所在目录 | `H:\wsl\LearnCPP-C08-Ubuntu-24.04` |
| Linux源码快照 | `/root/learncpp-c08-src/<run-id>` |
| Linux构建 | `/root/learncpp-c08-builds/<run-id>` |
| 固定依赖 | `/root/learncpp-c08-deps` |

镜像使用Ubuntu官方24.04.4 AMD64 WSL版本。镜像SHA、安装命令、包版本与初始控制见[环境记录](validation/c08-revision/wsl-setup/setup-record-20260910.md)。`findmnt`确认guest工作目录是ext4，而`/mnt/f`为Windows挂载访问；H盘存放VHDX不意味着guest的每个文件操作都走Windows挂载协议。

## 进入与构建

```powershell
wsl -d LearnCPP-C08-Ubuntu-24.04 --user root --cd /root -- bash
```

进入已生成的、具有manifest的源码快照后：

```bash
cd /root/learncpp-c08-src/<run-id>/C08_Concurrency/exercises
cmake --preset linux-core
cmake --build --preset linux-core --parallel 4
ctest --preset linux-core
```

`linux-debug`、`linux-asan`、`linux-tsan`各用独立构建目录。后两项使用Clang18，address模式包含ASan/UBSan，thread模式单独使用TSan。日志扩展需显式打开SPDLOG，并指定guest内固定fmt/spdlog源码目录；具体参数见[构建指南](../exercises/BUILD_GUIDE.md)。

源码从当前工作树筛选，包含本轮未提交及新增文件；排除隐藏状态、build、依赖缓存、二进制和归档。跨课链接所需文档一并进入快照。每批记录文件manifest、编译器/标准库、CMake参数与测试输出；既有快照和失败证据不覆盖。传回仓库的是纯文本/JSON/JUnit证据，构建产物留在guest。脚本须显式设guest工作目录并回读`pwd`，不能误把WSL继承的`/mnt/f`目录当作ext4工作目录。

## 检测器校准与边界

初始C++23控制、TSan正常/已知竞争校准代码见[控制脚本](validation/c08-revision/wsl-setup/run-probes.sh)。运行脚本之前进入guest工作目录；标准控制通过和已知竞争被检出只证明该工具入口能够运行。

本轮额外定位了Clang18、libstdc++13头文件与TSan组合下的边界：`call_once`异常重试、fence发布关系、全局new/delete注入冲突，以及异常状态由worker最后释放时的消息读取报告。见[专项诊断](validation/c08-revision/tsan-diagnosis/diagnosis-20260910.md)和[M1纯标准库控制](validation/c08-revision/tsan-diagnosis/m1-20260910-1815/diagnosis.md)。受影响部分明确SKIP/PARTIAL_SKIP，其他协议仍检查；非TSan保留完整验证。没有增加同步、suppression或全局安全设置来隐藏报告。

WSL不提供真实多NUMA节点时，N1相关分支仍跳过。原生C++26/C++29按最小编译/链接能力与主体分别记录；模型通过不替代标准主体。最新矩阵、精确PASS/FAIL/SKIP及已验证范围统一见[本轮质量报告](revision-quality-report-20260910.md)，历史初步矩阵不冒充当前代码证据。
