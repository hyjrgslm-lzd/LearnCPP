# L03 Socket、UDP 与 DNS 名称解压

正文：[传输契约](../../chapters/01-transport.md)、[Socket/DNS](../../chapters/02-sockets-dns.md)。先修是字节序、span 边界、expected 和 RAII。默认离线，不查询公网 DNS。

## Part A：实现名称解压

已提供 DNS 类型、报文读取设施和 checks.cpp。只编辑 [student/solution.hpp](student/solution.hpp) 的 decode_name(span,offset)，返回名称和原字段消费位置，失败返回对应错误。不得直接转发 c11::dns::read_name 或包含 Reference；它们是答案依据。未完成会抛 UNFINISHED，检查器有限退出。

从普通 label 开始，再加入零终止和压缩指针。记录原位置与跳转位置，先检查切片，再读长度；限制 label、展开名称与跳转，拒绝越界、自指、前向引用、环和不可支持的名称字节。重点输入是答案名称的两字节指针，以及指针指向另一个压缩名称。

```powershell
cmake -S C11_Networking_Service_Design/exercises/L03_transport -B build/c11-l03 -DC11_TEST_STUDENTS=ON
cmake --build build/c11-l03 --config Release --target C11_L03_student
ctest --test-dir build/c11-l03 -C Release -R '^C11_L03_student$' --output-on-failure
```

Reference：[reference/solution.hpp](reference/solution.hpp)接入公共正确解码器 [dns.hpp](../include/c11/dns.hpp)；good 是不同实现，bad 家族是有限错误控制。正常矩阵要求 good/reference 成功，并确实拒绝 bad，Student 初始失败不反转为成功。

## Part B：观察真实网络

构建 C11_L03_sockets 并运行对应 CTest。观察 IPv4、可用时 IPv6、localhost 地址候选、UDP 空报文与截断，以及本地 UDP DNS 问答。系统 API 的地址候选与自行 DNS 解析是两条不同路径。

修改实验副本使 A 答案后跟随一条截断附加记录，预测应拒绝整个报文；不得改变 parser 让坏输入通过。正常问题大小写变化应保持相同主机名语义。

## 解析

原字段只消费到第一次指针后两字节，展开路径继续追踪指向位置；嵌套引用不额外吃掉答案 TYPE 字节。长度验证应先于构造输出，环检查和长度上限解决不同问题。parse_a 保存首个候选后仍遍历剩余 sections，因此坏尾部不会被成功 A 掩盖。zero UDP datagram 与 TCP EOF 不共享判断。源码里用于说明交付缺口的序号模型，不证明真实网络随机丢包。

本课只支持有界 ASCII hostname A 查询；IDNA、递归 CNAME、DNSSEC 不是隐藏在测试通过后的保证。
