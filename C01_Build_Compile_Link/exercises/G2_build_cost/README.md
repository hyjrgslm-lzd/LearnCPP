# 练习 G2：控制输入后测量构建成本

先读[07 诊断与成本](../../chapters/07-diagnostics-and-build-cost.md)。本题是可运行实验驱动，不要求修改Reference来完成作业。所有改变发生在driver创建的独立源码副本中；原fixture保持不变。

## Part 1：先证明三种配置做同一件事

baseline、PCH、LTO各自独立编译`alpha.cpp`、`beta.cpp`、`gamma.cpp`、`common.cpp`、`main.cpp`。这些TU都实际包含同一heavy public header；没有共享一个已编译库来偷换工作量。PCH作用于真正重复包含的头；Release LTO覆盖fixture所有TU。

在LearnCPP根目录的x64 Native Tools环境运行：

```powershell
cmake -S C01_Build_Compile_Link/exercises/G2_build_cost -B C01_Build_Compile_Link/exercises/build/learner-g2 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build C01_Build_Compile_Link/exercises/build/learner-g2 --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-g2 --output-on-failure
```

**解析：** 三个程序都必须计算42并通过行为检查。PCH和LTO是候选构建配置，不是“更快”的同义词；先确认源码、宏、语言与结果相同，才讨论它们改变了哪些阶段成本。MSVC日志中的`/Yc`/`/Yu`和`/GL`/`/LTCG`可佐证实际采用的路径。

## Part 2：预测四种输入的工作量

| 场景 | 计时前准备 | 计时部分 | 应检查的证据 |
|---|---|---|---|
| clean | 新源码副本、空build目录 | configure + build | 分开的配置/构建命令及全量编译记录 |
| noop | 已完成并验证一次构建 | 无改动build | 无源码重编，通常`ninja: no work to do` |
| implementation | 已完成基线，在副本common.cpp追加无语义影响注释 | build | 对应TU重编和重新链接 |
| public-header | 已完成基线，在副本heavy.hpp追加同类注释 | build | 五个TU重编；PCH配置还可能重建PCH |

先写预测，再核对actual compile/link/no-op记录。变化不应改变程序结果；运行正确性检查在计时外。clean不是“清空了操作系统缓存”，本实验不修改机器缓存策略或关闭后台服务。

正式`elapsed_seconds`取计时命令各自`process_seconds`之和：包含进程启动、执行及输出收集，不包含driver随后把日志/JSON写盘的时间。clean相加configure与build，其他场景只有build。verbose输出收集仍有观测开销，所有配置保持相同协议；它不是纯编译器CPU时间。失败轮的时长用null记录，不作为数值样本。

**解析：** 实现和公共头变化的区别来自依赖扇出；PCH自身也是受头影响的产物。一次墙钟差异不足以证明解析或链接就是瓶颈，要结合实际重新执行的命令和阶段记录。某个场景更慢不代表这个方案在所有场景都无效。

## Part 3：先自检，再正式采样

工具要求Python3.11+、已安装CMake/Ninja/MSVC。选择实际解释器和工具路径，不依赖作者D盘路径。

```powershell
$StudyPython = '你的Python3.11以上解释器完整路径'
$StudyCMake = (Get-Command cmake).Source
$StudyNinja = (Get-Command ninja).Source
& $StudyPython C01_Build_Compile_Link/exercises/G2_build_cost/scripts/measure_build.py --self-test
& $StudyPython C01_Build_Compile_Link/exercises/G2_build_cost/scripts/measure_build.py --output C01_Build_Compile_Link/exercises/build/learner-g2-smoke-report --work-root C01_Build_Compile_Link/exercises/build/learner-g2-smoke-work --cmake $StudyCMake --ninja $StudyNinja --samples 1 --warmups 1 --variant baseline --scenario noop
```

自检拒绝已有output、非零命令和timeout；subset smoke只应汇总请求的baseline/noop，不能凭空把未请求组标INVALID。这个小检查不产生正式性能结论。

停止本任务其他构建和测量后，使用新目录执行完整矩阵：

```powershell
& $StudyPython C01_Build_Compile_Link/exercises/G2_build_cost/scripts/measure_build.py --output C01_Build_Compile_Link/exercises/build/learner-g2-formal-report --work-root C01_Build_Compile_Link/exercises/build/learner-g2-formal-work --cmake $StudyCMake --ninja $StudyNinja --samples 5 --warmups 1 --seed 20260908 --parallel 1 --timeout 180
```

完整矩阵是3 variants × 4场景 ×（1预热 + 5正式）=72轮；预热不进入正式统计。每条外部命令有限时，Windows只终止自己启动的进程树，无法确认清理就是失败。不要重复使用output来覆盖失败样本；参数`--work-root`隔离大量构建产物。

## Part 4：解释结果而不是排名

`report.json`保存环境、参数、随机顺序、fixture与公共runner指纹、每轮source/exe指纹、prepare/timed命令、退出/timeout/cleanup、原始输出及编译观察。summary只汇总本次请求组的有效正式样本，保留median、min/max/range及原始值；失败回合仍在rounds里，样本不足整体不能PASS。

**解析：** 比较同一场景/同一工作量的三个配置，不把clean与noop相除当优化倍数。PCH可能省去重复解析，也增加生成/失效成本；LTO可能增加链接工作，运行期收益需要另一套公平运行基准，本题未宣称。结合五TU trace和真实命令分析，不从一次总耗时直接推测cache miss或CPU瓶颈。

当前作者1+1记录只作driver预检；正式结果与限制从[课程质量报告](../../references/quality-report.md)进入。源码里对性能没有预填“应越来越快”的答案。
