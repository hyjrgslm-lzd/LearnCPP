# 迁移入口：线程生命周期与 jthread

线程与异常的完整正文已迁入 [03 线程与执行方式](chapters/03-threads-and-execution.md)。先修内容见 [00 对象](chapters/00-execution-and-objects.md)与 [02 结果通道](chapters/02-result-channels.md)。

- [A1 生命周期](exercises/A1_jthread_lifecycle/README.md)：手动 join、自动请求停止与 join、移动、参数所有权。
- [A3 异常传播](exercises/A3_thread_exception/README.md)：worker 捕获、join 后读 exception_ptr、promise 结果通道。
- [A2 协作取消](exercises/A2_stop_token_cancellation/README.md)：原练习入口保留，取消协议由该专题继续维护；包含 stop_source/token/callback、回调线程、可取消等待和检查点设计。

原 A1/A3 的 sleep 时间线已改为同步关系和可运行检查。显式 join 两次是错误；第一次 join 后 jthread 析构安全，是因为析构检查 joinable 而不会再调用 join。错误程序仅作为阅读材料，不进入默认实验。
