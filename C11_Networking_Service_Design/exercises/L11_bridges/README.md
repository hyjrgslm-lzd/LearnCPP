# L11 C09/C10 的真实网络桥接

正文：[桥接与源码](../../chapters/12-bridges-source-performance.md)。两题均提供完整观察 Reference，保持原课程 Student/API 不变。

## Part A：旧 RPC 互操作

配置 C11_ASIO_SOURCE 指向固定 standalone Asio。目标 C11_L11_legacy 编译 C09 原 good/client.cpp 与 protocol.cpp；[legacy.cpp](legacy.cpp)的服务端只用 C11 socket/frame 检查预期 wire。预测 add、unknown_method、error、timeout 以及 `C|4` 后运行。

解析：客户端仍发送八位十进制帧，req_id 每次变化；C11 peer 不借用同一序列化函数，所以能发现两端一起犯同一错误的情况。peer 只有观察到取消帧才关闭，客户端读循环消费真实 EOF 后自然退出，没有用固定 sleep 猜发送完成。这里是受限脚本 peer，不是通用 RPC 服务。

## Part B：sender 三条完成通道

配置 C11_ENABLE_BOOST/STDEXEC 与对应路径。[network_sender.hpp](network_sender.hpp)把实际 async_read_some 接入固定 stdexec 协议；[sender.cpp](sender.cpp)运行有数据、EOF、活动停止和提前停止。

```powershell
cmake --build build/c11-bridges --config Release --target C11_L11_legacy C11_L11_sender
ctest --test-dir build/c11-bridges -C Release --output-on-failure
```

任务：遮住 begin/finish，依据正文先写出 read/timer/stop_callback 的拥有关系；然后在独立副本将数组改为 64 字节，保持接口仍为 receive_some。不要把它宣称为 read_frame。

解析：connect 拥有 operation，start 接上 executor，内部 shared state 活到所有 completion 收束；value 给拥有的 string，普通错误进 error，只有停止导致的实际 canceled completion 进 stopped。晚到停止不能把 EOF/普通错误改成 stopped。stop callback 仅设原子标记，owner timer 才调用 socket.cancel；该操作独占 socket 的取消责任，并要求单个 io.run owner 或 strand 串行执行 timer/read handler。最终信号前取消/消费 timer、注销 stop callback，最终信号之后不访问 operation。

本桥接不复用 C10 的文件 read_at 后端，不混用 Boost 和 standalone Asio 类型，也不声称 stdexec 扩展是标准网络 API。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L11_sender`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
