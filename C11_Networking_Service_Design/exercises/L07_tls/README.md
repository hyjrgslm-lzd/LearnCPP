# L07 TLS 身份与关闭

正文：[TLS](../../chapters/06-tls.md)。完整观察型驱动，依赖显式私有 OpenSSL3.5.8；TCP 分支还需 Boost。配置方法见 [构建指南](../BUILD_GUIDE.md)。

## Part A：可确定的协议驱动

[main.cpp](main.cpp)将双方 SSL 的 memory BIO 字节互相搬运。先预测可信 CA/localhost 成功，错误 hostname、未知 CA、过期证书与 mTLS 缺少客户端证书失败，再运行 C11_L07_tls。区分证书生成 fixture 与待验证握手；不会向系统安装根证书。

## Part B：真实 TCP

[tcp.cpp](tcp.cpp)把 SSL stream 接到实际 loopback，完成握手、数据和 TLS shutdown。运行 C11_L07_tcp。任务是在副本里改变应用消息分片，保留完整消息与关闭判断。不能使用 verify_none 让反例通过。

```powershell
cmake --build build/c11-asio --config Release --target C11_L07_tls C11_L07_tcp
ctest --test-dir build/c11-asio -C Release -R 'C11_L07_' --output-on-failure
```

## 解析

信任链有效、主机名匹配、有效期/用途正确是独立条件；SNI 选站点但不执行身份校验。mTLS 增加客户端身份，应用授权另做。memory BIO 检查密码协议推进，TCP 检查后端接线；前者不代表真实网络。close_notify 与裸 TCP EOF 不同，失败关闭不能统一转成功。

Reference 为两份原驱动及 [certificate_fixture.hpp](../include/c11/certificate_fixture.hpp)。ALPN、session ticket、0-RTT 在正文推导，不属于这两个驱动的已执行功能。不要从 no-asm 教学依赖的耗时推导生产密码性能。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L07_tls`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
