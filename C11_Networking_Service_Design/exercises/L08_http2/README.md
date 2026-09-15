# L08 HTTP/2 的双流与窗口

正文：[HTTP/2](../../chapters/08-http2.md)。使用本地 nghttp2v1.70.0，配置 C11_ENABLE_HTTP2=ON 与 C11_NGHTTP2_SOURCE。完整观察驱动 [main.cpp](main.cpp)作为 Reference。

## Part A：先预测再运行

客户端声明流窗口 1024，关闭自动归还；大响应 8192，小响应 `ok`。预测第一次 drive 后大流长度、小流状态与两个 HEADERS 大小，再构建 C11_L08_http2 并运行。

```powershell
cmake --build build/c11-asio --config Release --target C11_L08_http2
ctest --test-dir build/c11-asio -C Release -R '^C11_L08_http2$' --output-on-failure
```

解析：大流停在 1024，小流有自己的窗口所以可完成；连接总窗口尚有信用。重复字段让固定实现复用 HPACK 表，第二块较小。TCP 收到字节后另按 7 字节切片喂 parser，验证分片而不依赖网络碰巧切段。

## Part B：归还与错误范围

检查每轮 consumed 如何更新，将副本中的初始窗口改为 2048，同时将相应预测改为 2048；完整 body 仍必须精确相等。随后跟踪 RST_STREAM 后的新小请求，以及增量为 0 的连接 WINDOW_UPDATE。

解析：consume 归还流与连接信用，不能重复归还；reset 清理单流，连接保持可用；非法连接控制帧产生 GOAWAY。data provider 的 payload 保留到 stream close。C callback 返回库错误码，不能跨 C ABI 抛异常。

## Part C：源码

从 mem_recv2 跟踪 header、stream 与 window，从 submit_response2 跟踪 provider、DATA、END_STREAM。正文和来源索引给出固定源码入口。本驱动是有限少量请求的协议实验；SETTINGS 的字段列表建议值不等于完整不可信连接的内存硬限制，TLS/ALPN 不在此目标。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L08_http2`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
