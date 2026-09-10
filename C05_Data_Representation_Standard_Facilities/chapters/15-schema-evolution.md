# 15：schema 演进不是多加一个字段

L14 已经把一个字段读安全：长度有界、payload 存在、UTF-8 严格、失败不推进游标。完整资源清单还要解决另一个问题：内存对象、wire bytes 和未来版本不是同一种东西。`ResourceRecord` 在内存里有 `id/name/path/byte_size/modified_at_ms/note`；wire 格式只看 tag、length、payload；兼容规则决定旧读者遇到新字段该跳过还是拒绝。

本章只做基础 schema 演进。C12 会深入通用序列化格式、索引、WAL、崩溃恢复和存储成本；这里不抢那些内容。这里的目标是让读者能解释一个小二进制格式怎样稳定读写，怎样拒绝坏输入，怎样让 v1 和 v2 有清楚边界。

## 1. 三个版本轴

第一轴是内存模型。`Manifest` 拥有 `vector<ResourceRecord>`，每条记录拥有自己的字符串。输入 `span<const byte>` 只在调用期间借用，返回对象不指向输入缓冲。这样 decoder 成功后，调用者释放文件缓冲也不会让清单悬垂。

第二轴是 wire schema。本课固定 magic 为 `C05M`，后面是 `major:u16`、`minor:u16`、`record_count:u32`。每条记录先写 `body_length:u32`，再写多个字段：`tag:u16 | length:u32 | payload`。所有整数都用 big endian；不能 dump struct。

第三轴是兼容策略。major 表示不兼容语义，minor 表示同一 major 下的可忽略增量。读者接受 `major == 1 && minor >= 1`；拒绝未知 major 和 minor 0。v2 新增可选 `note`，旧 v1 读者不理解 tag 6 时跳过它。重编码不会保留未知字段，这是有意的信息丢失，必须写进契约。

| 版本 | writer 写什么 | reader 行为 | 兼容含义 |
|---|---|---|---|
| v1 `(1,1)` | tags 1-5 | 需要 id/name/path/size/mtime | 没有 note；writer 遇到 present note 必须拒绝 |
| v2 `(1,2)` | tags 1-6，note 可选 | v2 读者保留 note；v1 读者跳过 tag 6 | 同 major 可忽略新增字段 |
| future minor `(1,n)` | 可含未知 tag | 当前读者有界 skip 未知 tag | re-encode 丢弃未知字段 |
| unknown major | 未知语义 | 拒绝 | 不能假装兼容 |

## 2. 必需、可选和未知不是一类字段

id、name、path、size、mtime 是必需字段。缺任意一个都不能发布部分对象，因为后续代码会以为记录满足不变量。note 是可选字段：缺失表示没有 note；存在但长度 0 表示空 note。这两个状态在内存中分别是 `nullopt` 和 `string{}`，不能混淆。

未知 tag 只能在同 major 的 future minor 中作为可忽略字段跳过。跳过不等于信任：必须先检查 `length` 在当前 record body 内，不能按长度越界移动 cursor。未知字段也要拒绝重复，因为同一个扩展字段出现两次时，当前读者无法知道它的合并语义。

## 3. 输入验证先于对象发布

`validate_manifest` 先检查内存对象：最多 1024 条记录，id 非零且唯一，timestamp 在公历 0001-9999 对应的 Unix 毫秒范围内，name/path 非空且是严格 UTF-8，不含 NUL；note 可空但同样必须是严格 UTF-8 且不含 NUL。path 规则由公共 `validate_resource_path` 负责；本章不复制另一套路径判断。

decode 时也执行同一组规则。wire 文本先按 length 取出拥有型 `string`，再严格 UTF-8 验证，再检查 NUL/空值。错误 offset 用整个 packet 的 byte 坐标。UTF-16 code unit offset 只属于 UTF-16 输入转换，不属于本章 wire decoder。

## 4. v1/v2 兼容实验

练习使用手写 `fixtures/golden.hpp` 作为黄金输入，不用 writer 生成 expected。这样能发现 writer 把 tag 顺序、length 或 endian 写错的问题。检查器覆盖：

- v1/v2 黄金字节写出和读入；
- 每个截断点；
- 空 manifest；
- record 数量、字符串长度、包大小上限；
- known/unknown tag 重复、必需字段缺失、标量宽度错误、尾随包数据；
- minor 0、未知 major、future minor unknown skip；
- v1 reader 读 v2 note，v2 reader 读 v1 no note；
- present empty note 与 absent note 的区别；
- v1 writer 遇到 present note 拒绝。

bad 变体只保留一个真实错误：把 present note 静默丢弃后写 v1。它会通过很多普通读写检查，但在“v1 writer rejects note”处被拒绝。这比故意崩溃或随便返回常量更有教学价值：schema bug 常常不是字节循环不会写，而是兼容边界被悄悄改宽。

## 5. 本章边界

本章不做压缩、索引、零拷贝、checksum、加密、数据库事务或崩溃恢复。那些都需要新的场景、成本模型和故障证据。当前最小闭环是：明确字段、长度、版本、兼容和失败语义；用独立 golden bytes 与坏包证明 reader/writer 真在执行这些规则。
