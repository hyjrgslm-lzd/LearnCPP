# L01 bytes and object representation

观察目标：对象有值，也有 object representation。`std::byte` 表示原始字节，`std::bit_cast` 在 trivially copyable 类型之间复制表示，不通过别名规则偷看对象。

Part 1：运行 `L01_bytes_object_representation`，观察 `sizeof(PacketWord)` 决定 `std::array<std::byte, N>` 的长度。

Part 2：观察 `bit_cast` 往返后值保持。它复制表示，不建立指向原对象的别名。

Part 3：修改 byte array 后再 `bit_cast` 回对象，值可能变化。解析重点：这只说明本类型在本机的对象表示会影响值；不要把内存 dump 当成跨机器协议格式，布局、填充和字节序都不是 wire contract。

编辑位置：无，本题是观察题。检查目标：`L01_bytes_object_representation`。Reference：观察题没有独立 Reference；解析在本 README。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L01_bytes_object_representation`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`observation.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L01_bytes_object_representation。
修改后先重建 `L01_bytes_object_representation`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
