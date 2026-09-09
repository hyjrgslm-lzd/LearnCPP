# E3：CAS 回填、重算与 min/max

完整正文见[CAS 回填、重算与 min/max](../../topics/atomics/02-cas-and-minmax.md)。[main.cpp](main.cpp) 是安全、有限结束的 Starter；[solution.cpp](solution.cpp) 是含运行检查的 Reference。默认 C++23。

## Part 1：失败回填与最大值的两种契约

先保留 Starter 的失败 CAS 检查，再检查成功时 expected 不变。实现 `raise_max`：候选不大于当前观察值时可直接返回；否则 weak CAS，失败用回填值重新判断。四个 worker 的候选覆盖 0..3999，最终检查 3999。

再实现 `fetch_max_rmw`，即使候选较小也完成一次成功 CAS，返回旧值。答案：两者可能有同样的数值结果，但 shortcut 可以只有 load，不能冒充发布用的 release RMW。Reference 检查目标 9、候选 8 时的返回和目标值，内存序差异由正文证明，不能由相同输出证明等价。

## Part 2：乘法每次重试都重算

实现 `fetch_multiply`，输入 unsigned，发现乘积不可表示时抛 overflow_error。先判断 factor 是否为 0，再用 max/factor 判溢出；检查必须位于乘法求值前。四个线程各乘四次 2，初值 1，最终 65536；另检查溢出不修改目标及乘以 0。

答案：CAS 失败后 old 可能改变，desired 不能沿用上一轮计算。weak 可以伪失败，strong 不可以；不能用 weak 失败次数推断竞争次数。worker 抛出的异常由 get 返回主线程。

## Part 3：C++26 原生专项

在 `#if CS_HAS_ATOMIC_MIN_MAX` 下使用 fetch_max 与 fetch_min，检查旧值和最终值。缺能力时只跳过该专项，前两部分仍完整执行并决定测试成功与否。

答案：原生接口提供 RMW 契约，不能把 `if(candidate<=old) return;` 的纯读取分支当成完全等价的通用替换。标准版本、库能力探测与 CAS 回退必须分开陈述；不要求本机一定提供 C++26 接口。

普通 atomic 的失败 CAS 序不能为 release/acq_rel；单参数 release 会将失败序推导成 relaxed。需要解引用失败回填的已发布指针时，应重新证明 acquire 与寿命，而不是机械套用纯整数循环的 relaxed。

## 构建与验收

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S E3_cas_loop -B build/E3_cas_loop -G "Visual Studio 18 2026" -A x64
cmake --build build/E3_cas_loop --config Release
./build/E3_cas_loop/Release/E3_cas_loop.exe
ctest --test-dir build/E3_cas_loop -C Release --output-on-failure
```

VS2026 生成器需要 CMake 4.2 或更新版。CTest 运行 `E3_cas_loop_reference` 并设进程超时；cs::check 在 Release 仍有效。通过表示本次检查成功，不替代正文中的协议证明。规范链接与版本说明见对应正文。

Part 3 的原生接口需要显式启用探测。仍在 `C08_Concurrency/exercises`，使用独立构建目录：

```powershell
cmake -S E3_cas_loop -B build/E3-native -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX26=ON
cmake --build build/E3-native --config Release
ctest --test-dir build/E3-native -C Release --output-on-failure
```

默认 OFF 时，能力宏为 0 表示本次构建没有请求原生探测，不能据此判断标准库是否支持。ON 时会实际编译和链接指定接口；若宏仍为 0，查看 `build/E3-native/capabilities/CS_HAS_ATOMIC_MIN_MAX.log` 中的原因。Reference 的 SKIP 信息只说明该专项没有启用已验证的原生路径，前两部分的 CAS 检查仍执行。缺少预览语言选项与缺少库接口也应根据配置/探测日志分别判断。
