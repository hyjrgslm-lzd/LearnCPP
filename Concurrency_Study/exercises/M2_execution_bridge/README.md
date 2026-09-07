# M2：执行管线的 value、error 和 stopped

完整正文：[执行桥接](../../topics/scheduling/03-execution-bridge.md)。[main.cpp](main.cpp) 运行 schedule/then 的值管线，[solution.cpp](solution.cpp) 是包含所有 Part 的独立 Reference。

实际代码使用 NVIDIA stdexec nvhpc-26.05 固定 commit `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。标准层对照 C++26 N5050；默认编译标准仍为 C++23。使用 `#if CS_HAS_STDEXEC`，缺依赖时 main 和 Reference 均说明原因并返回 77。

入口类型：main 是 OBSERVATION 驱动，只运行并检查独立的值管线 42，没有直接 include solution.cpp。0 仅表示这条管线通过，不完成 when_all/error/stopped 等其他 Part；相应实现练习按下文重写并另跑独立 Reference。缺依赖返回 77。保留 baseline 观察与预测任务，不把完整答案执行伪装成未填学生实现，也不另建评分框架。

## 构建

从 `Concurrency_Study/exercises` 使用已经下载的依赖：

```powershell
cmake -S M2_execution_bridge -B build/m2 -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_STDEXEC=ON -DCONCURRENCY_STUDY_STDEXEC_SOURCE_DIR="$PWD/build/full-windows/_deps/stdexec-src"
cmake --build build/m2 --config Release
ctest --test-dir build/m2 -C Release --output-on-failure
```

不开启依赖的默认配置用于验证 SKIP 分支，不是完整 execution 实验。统一 CMake 提供 MSVC 必要的预处理器设置，不修改下载源码。

## Part 1：成功值与惰性描述

遮住 main 的两个 then，重写无参产出 21、接收 int 并翻倍的管线。Reference 检查 optional 有值且 tuple 中为 42。

答案：schedule 产出调度完成，then 消费成功值；管线描述本身不等于已经起线程。sync_wait 负责连接、启动并等完成。不要假设 optional 总是有值，也不要把 just 当成新建线程。

## Part 2：汇合和执行资源

组合两个 schedule/then 分支产出 100 与 23，when_all 后相加，Reference 同时检查 123 与两个业务函数的完成计数。

答案：when_all 组合全部子操作的完成；执行资源决定它们是否并行。这个任务汇合不关闭 pool。所有 sync_wait 都放在外部 main，pool 存活到全部管线完成以后，才由作用域析构回收资源。

## Part 3：error

Reference 的一个 then 抛出 `pipeline-error`，与另一个分支 when_all 汇合，main 检查 sync_wait 重抛的明确消息。

答案：错误沿 error 通道传播，普通 then 不处理它。兄弟分支可能收到停止请求，但不会被强杀；已经运行且不响应停止的业务代码仍要自行结束。错误路径不能要求兄弟 then 一定执行，可能在 schedule 阶段已经停止。

## Part 4：stopped 与显式恢复

Reference 的 stopped_int 声明一个 int 成功签名及 stopped 签名，在 operation.start 中真实调用 set_stopped。依次检查：下游 then 未执行、sync_wait 返回空 optional、when_all 汇合后仍为停止、upon_stopped 显式恢复成成功值 7。

答案：运行时的停止不等于错误，也不是默认值 0。声明可能的成功签名使 sync_wait 可以形成结果类型，即使此次只发送 stopped。这个测试没有伪造“模拟取消”输出；它真实经过 stdexec 的 connect/start/receiver 协议。停止请求与停止完成仍是不同概念，本例直接制造后者来检查消费行为。

## 必须说清的标准边界

N5050 的标准 consumer 是 `std::this_thread::sync_wait`，不是 `std::execution::sync_wait`。本题运行的是 `stdexec::sync_wait`；`exec::static_thread_pool` 是具体库资源。二者不能仅凭相似名字认定逐字等价。源码与正文参考链接均固定版本，滚动 eel 不作为 C++26 定版证据。

作者已运行固定依赖的 Release Reference，value/error/stopped/join/recovery 全部检查通过；依赖头自身有 MSVC 对齐及局部名称遮蔽警告。最新验证和不可测项见[记录](../../topics/scheduling/verification.md)。
