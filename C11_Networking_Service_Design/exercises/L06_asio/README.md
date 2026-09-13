# L06 Asio 协程、取消与工作收束

正文：[Asio 生命周期](../../chapters/05-asio-lifetime.md)。观察型单元，依赖本地固定 Boost；按 [BUILD_GUIDE](../BUILD_GUIDE.md)启用 C11_ENABLE_BOOST 和 C11_BOOST_ROOT。

## Part A：控制流

[main.cpp](main.cpp)提供真实 awaitable echo、strand 更新和取消槽。先在纸面标注每个 co_await 挂起时哪些对象仍被借用、哪个 completion 收集异常，再运行 C11_L06_asio。修改独立副本的 read buffer 长度，结果仍应按字节流语义推进，不能依赖一次 read 收齐。

## Part B：关闭（第 10、11 章后回读）

[runtime.cpp](runtime.cpp)使用明确 gate 把工作停在已进入阶段，检查期限内与期限外的收束。[startup.cpp](startup.cpp)观察启动失败重试及 owner 延迟。运行目标 C11_L06_runtime、C11_L06_startup；它们包含约 5 秒的有意预算越界场景，程序正确识别越界才是检查成功。

```powershell
cmake --build build/c11-asio --config Release --target C11_L06_asio C11_L06_runtime C11_L06_startup
ctest --test-dir build/c11-asio -C Release -R 'C11_L06_' --output-on-failure
```

## 解析

strand 串行化 handler，不固定线程，也不保护绕过 executor 的直接调用。协程帧保留局部变量，但外部取消仍需等待完成才能销毁帧。固定两个 slot 使工作接纳后转交和结果回传不依赖再次分配；启动部分失败必须停止并 join 已创建线程，重试时重新设置 joined 状态。owner 迟到后发现 live 已空仍须检查原 drain deadline，不能把超时抹掉。

所有 Part 的 Reference 为对应完整源码；观察通过不代表完成书面的拥有关系与失败路径解释。真实服务接线见 P1/P2。
