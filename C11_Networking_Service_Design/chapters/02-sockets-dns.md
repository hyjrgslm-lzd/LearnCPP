# 02 Socket 所有权、候选连接与 DNS

先读第 1 章。对应 [L03 的实现与观察题](../exercises/L03_transport/README.md)。本章既解释系统 resolver 的使用，也实现一个有界 DNS A 查询解析器；后者不是生产递归解析器。

## 1. 一个句柄必须有一个关闭责任

Windows 先用 WSAStartup 建立 Winsock 使用期，最后 WSACleanup；每个 socket 用 closesocket 释放。POSIX 使用文件描述符和 close。两种句柄不能按整数大小或用同一个无条件 close 封装。`socket.hpp` 的 `unique_socket` 禁止复制、支持移动；release 转移责任，reset 释放旧资源。socket runtime 要比所有 socket 活得久，连接池因此将 runtime 放进租约共享的池状态。

非阻塞标志不会让操作自动完成。recv 返回 would-block 说明当前没有可读结果；应等待就绪后再尝试。readiness 只是“现在尝试可能推进”，并不保证请求的全部长度可用。`wait_ready` 看到错误或 hangup 后仍交给 recv/send/SO_ERROR 取得实际结果，避免用就绪位臆测协议终态。

非阻塞 connect 的起始结果可能是进行中。等到可写后要读取 SO_ERROR：拒绝连接也会唤醒可写等待。`connect_address` 的退出顺序是：创建 RAII socket → nonblocking → connect → 按同一截止时间等候 → SO_ERROR → 移交句柄；任一步失败都沿 RAII 释放。

## 2. 名称解析返回候选集合

getaddrinfo 可能返回多种地址族和多个地址，结果链表用 freeaddrinfo 释放。程序应遍历匹配 socket 类型的候选，记录每次失败，最终返回成功连接或有意义的最终错误。顺序尝试是正确且简单的基线，但首个不可达候选可能消耗大部分预算。Happy Eyeballs 的交错候选尝试是延迟策略：稍后启动另一地址族，首个成功者获胜，关闭其余尝试。它需要限制并行连接数，不能对所有地址无限扇出。

总截止时间应在解析前建立，后续候选共享剩余预算。同步 getaddrinfo 本身没有本课 socket deadline 参数，不能假装外部计时检查能中断正在阻塞的 resolver。L03 的 localhost 解析是有限环境观察；生产异步 resolver 还需管理取消后返回的结果及其所属任务。不要把 system_clock 的校时跳变用于进程内剩余时间计算。

## 3. DNS 报文的边界

DNS 头固定 12 字节：事务 ID、flags、四个 section count。每个整数按大端解释。问题项包含名称、QTYPE、QCLASS；资源记录包含名称、TYPE、CLASS、TTL、RDLENGTH、RDATA。必须验证报文源与事务、QR/rcode、问题内容和所有 section 的结构，不能扫到四个像 IPv4 的字节就返回。测试用本地 UDP fixture，既不依赖公网记录，也不修改系统 DNS。

名称由长度字节和 label 构成，零长度 label 终止。高两位为 11 的两个字节是压缩指针，其余 14 位为报文内偏移。解析有两个位置：沿指针展开的位置，以及调用者在原字段后继续的位置。第一次跳转后，原消费位置固定在指针两字节之后；后续指针不能继续增加原字段消费长度。

例如问题名位于偏移 12，答案用 `c0 0c` 引用它。答案名称在原报文只占两字节，但展开结果可能长得多。若把展开长度加到原游标，后面的 TYPE 就会读错。自指针、前向指针、重复访问偏移、截断指针必须拒绝；本课要求指向之前出现的位置，并设置跳转/展开上限。[RFC1035 §4.1.4](https://www.rfc-editor.org/rfc/rfc1035.html#section-4.1.4)是压缩格式来源。

## 4. 从反例推导解析顺序

L03 的 bad 把循环引用错误变成合法空名；bad_nested 使嵌套指针的原消费位置错误；bad_length 漏掉展开名称总长度上限。检查器将这些输入交给当前选中的解码器，而不只判断“完成标记”。要修复它们，先画出 `cursor / consumed / jumped / visited / expanded` 五个量，再逐字节推进。

Reference `dns.hpp` 限定 ASCII LDH hostname：每 label 最多 63 字节，展开文本最多 253 字节，字母比较不区分大小写。DNS 原始名称可以包含更广泛字节；IDNA 转换、通用二进制名称和递归 CNAME 都超出这个 A-query 教学接口。字段不可支持应明确拒绝，不能悄悄解释成另一个名字。

解析答案时先保存候选 A 地址，再继续遍历权威和附加 sections 并检查报文尾部。否则首条合法 A 后面隐藏坏长度也会被提前成功绕过。RDLENGTH 控制每条资源记录的切片；未知类型可以按已校验长度跳过。这里验证其结构，并未实现每种 RR 的内部语义或 DNSSEC。DNS over UDP 响应被截断时，完整 resolver 可转 TCP；不能把 TC 位忽略并当完整答案。

## 5. 自测与解析

1. **可写 connect 是否就是连接成功？** 不是，必须读取 SO_ERROR，失败的连接也可能就绪。
2. **压缩指针占几字节？** 原字段两字节，展开长度另算；沿指针读取不移动调用者后继字段位置。
3. **为何找到 A 后还检查尾部？** API 承诺接受的是完整合法结构，提前返回会绕过尚未验证的计数和长度。
4. **TTL 等于连接最长寿命吗？** TTL 限制缓存记录使用，不关闭已建立 socket；池 TTL 是另一项服务策略。
5. **取消 resolver 后能马上析构所有上下文吗？** 只有取得完成通知或后端提供明确无回调保证后才行；取消请求不是完成事件。
