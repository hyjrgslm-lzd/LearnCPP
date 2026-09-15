# L02 bits and endian

观察目标：位运算可以定义 wire order。`append_be` 总是写 big endian，`read_be` 总是按 big endian 读，和本机 `std::endian::native` 分开。

Part 1：运行 `L02_bits_endian_observation`，确认 `0x1234` 写成 `12 34`。这是格式规则，不是本机内存顺序。

Part 2：观察 `read_be` 和 `append_be` 往返。成功读取按类型宽度推进 cursor；输入不够时返回错误且不推进。

Part 3：把 `std::endian::native` 当成环境观察，不把它写进 wire format。解析重点：`read_be` 只接受 unsigned 整数且排除 `bool`，因为布尔值不是任意 octet 容器。

编辑位置：无，本题是观察题。检查目标：`L02_bits_endian_observation`。Reference：公共实现见 `../include/c05/bytes.hpp`；解析在本 README。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L02_bits_endian_observation`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`observation.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L02_bits_endian_observation。
修改后先重建 `L02_bits_endian_observation`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
