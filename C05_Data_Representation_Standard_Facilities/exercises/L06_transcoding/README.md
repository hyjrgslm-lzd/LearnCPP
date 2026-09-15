# L06 strict transcoding

目标：实现严格 UTF-8 和 UTF-16 转换。编辑 `src/student/utf_transcode.hpp`。

Part 1：`validate_utf8` 必须拒绝 overlong、代理项编码、超过 U+10FFFF、坏 continuation、截断序列。错误 offset 使用坏序列起始 byte。

Part 2：`utf8_to_utf16` 输出拥有型 `std::u16string`。NUL 是普通码点，BOM 是普通 U+FEFF，本函数不剥离。

Part 3：`utf16_to_utf8` 正确处理代理对。未配对 high surrogate 报 `incomplete_input` 或 `invalid_encoding`，offset 单位是 `utf16_code_unit`。

Part 4：实现对称预算。UTF-8 输入最多 `c05::max_package_bytes`；UTF-16 输入按字节预算，先检查 `size() <= max_package_bytes / sizeof(char16_t)`；UTF-8/UTF-16 输出追加前也检查 1MiB 上限。错误 offset 使用对应输入单位。

检查目标：`L06_transcoding_reference`、`L06_transcoding_validation_good`、`L06_transcoding_validation_bad_rejected`；开启 student preset 后还会运行学生占位。

Reference 可复用公共 `c05/utf.hpp`。`validation/good` 独立实现同一规则；`validation/bad` 漏掉 UTF-8 验证，检查器会拒绝。解析重点：NUL 和 BOM 是可保留的码点；非法编码不能替换成 U+FFFD 后继续成功。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L06_transcoding_student`。

本题是实现题。学习者只改 Student 入口；Reference、validation 和 checks 只用于对照与验证。
- Student 入口：`src/student/utf_transcode.hpp`。
- Checker 入口：`checks/transcode_checks.cpp`。
- Reference 对照：`src/reference/utf_transcode.hpp`。
- validation/good 对照：`validation/good/utf_transcode.hpp`。
- validation/bad 反例：`validation/bad/utf_transcode.hpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L06_transcoding_student。
修改后先重建 `L06_transcoding_student`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
