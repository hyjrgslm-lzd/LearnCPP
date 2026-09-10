# L01 bytes and object representation

观察目标：对象有值，也有 object representation。`std::byte` 表示原始字节，`std::bit_cast` 在 trivially copyable 类型之间复制表示，不通过别名规则偷看对象。

Part 1：运行 `L01_bytes_object_representation`，观察 `sizeof(PacketWord)` 决定 `std::array<std::byte, N>` 的长度。

Part 2：观察 `bit_cast` 往返后值保持。它复制表示，不建立指向原对象的别名。

Part 3：修改 byte array 后再 `bit_cast` 回对象，值可能变化。解析重点：这只说明本类型在本机的对象表示会影响值；不要把内存 dump 当成跨机器协议格式，布局、填充和字节序都不是 wire contract。

编辑位置：无，本题是观察题。检查目标：`L01_bytes_object_representation`。Reference：观察题没有独立 Reference；解析在本 README。
