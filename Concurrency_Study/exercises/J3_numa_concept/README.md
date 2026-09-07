# J3：拓扑、允许 CPU 与实际执行位置

完整正文是 [NUMA 拓扑](../../topics/numa/01-topology.md) 和 [线程亲和](../../topics/numa/02-affinity.md)。[main.cpp](main.cpp) 探测并验证一个允许 CPU，[solution.cpp](solution.cpp) 检查列表解析和至多四个 CPU 的 before/after 样本；平台实现复用 [numa.hpp](../include/concurrency_study/numa.hpp)。

入口类型：main 是已实现的平台诊断探针，不是待填 C++ 算法 Starter。启动时输出 PLATFORM PROBE ONLY，0 仅表示实际执行的拓扑/单 CPU 绑定检查通过；本题各 Part 要求解释探测证据。页面能力及完整放置 Part 必须另跑 N1，不能由这个退出码推断。

## 构建

从 `Concurrency_Study/exercises`：

```powershell
cmake -S J3_numa_concept -B build/j3 -G "Visual Studio 18 2026" -A x64
cmake --build build/j3 --config Release
ctest --test-dir build/j3 -C Release -V
```

Windows 由统一配置链接 Psapi。Linux 分支使用 sched_getaffinity、sysfs、/proc 和内核查询接口，不链接 libnuma；作者当前环境没有 Linux 运行证据。API 不可用或范围无法可靠确定时返回 77 并说明原因。

## Part 1：读懂探测表

记录 group/cpu/socket/core/node、allowed memory nodes 和页大小。回答 socket 是否等于 node、相邻 CPU ID 是否一定是两个独立 core。

答案：都不是必然关系。以完整拓扑键判断 SMT 兄弟；hardware_concurrency 不是允许 CPU 列表。Windows 仅枚举调用者 primary group 的候选 mask/CPU set 交集，不由 GetThreadGroupAffinity 推断原线程已限制单组，也不能用该范围推断整个服务器拓扑。Linux 的 Mems_allowed_list 与 CPU allowed 集合分别记录。

## Part 2：请求与观测

用表中允许的 CPU 建立 affinity，在 worker 的操作前后各查询 actual CPU，检查与请求一致。Reference 使用同一 on_cpus 路径；没有 sleep，也不靠任务管理器肉眼观察取代检查。

答案：成功请求只是设置被接收，实际采样是另一份证据。两次 CPU 样本不能证明中间从未抢占，也不能证明页面位于该 CPU 的 node。on_cpus 只在专用新线程中永久绑定，操作结束或抛出后让它退出并 join，不修改调用者 affinity；它不提供复用 worker 的通用恢复 guard。Reference 检查正常/异常路径均在不同于调用者的线程中执行，异常回传。单个旧 GROUP_AFFINITY 不足以恢复 Win11 默认跨组状态。

## Part 3：单节点也能完成什么

作者本次探测得到 group 0、32 个逻辑 CPU、16 组 SMT 核心关系、一个 node、4096 字节基础页。J3 的四个目标 CPU 均通过前后观测。这是本次证据，不是所有桌面的固定结果。

答案：单节点仍能验证拓扑过滤、亲和、异常回传和页查询；它不能给出 remote 时间。J3 不把 CPU 检查冒充页面检查，后者在 [N1](../N1_numa_placement/README.md) 完成。后续改变 reader 数时优先明确选择不同 core 还是 SMT 兄弟，再解释结果。
