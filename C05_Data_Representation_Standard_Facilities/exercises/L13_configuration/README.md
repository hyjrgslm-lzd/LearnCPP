# L13 configuration

目标：实现 C05 的小配置格式，不引入 JSON/TOML。配置只接收 UTF-8 `key=value` 行，固定键为 `package_file` 和 `display_zone`。

Part 1：按行扫描。支持 LF、CRLF、首行 BOM、空行和整行 `#` 注释；遇到首个 `=` 后分成 key/value，并修剪两侧 ASCII 空格和 tab。CRLF 只能消费 LF 前的末尾 CR；行内 CR 和裸末尾 CR 都要拒绝，offset 指向实际 CR。坏行、UTF-8非法和 NUL 要返回 byte offset 和 1-based line。

Part 2：实现键规则。未知键拒绝；重复键返回 `duplicate_field`；空值拒绝。默认值是 `manifest.c05m` 和 `UTC`。

Part 3：实现 `package_file` 规则。它只能是 basename：不能含 `/`、`\`、盘符、`.`、`..`、控制字符、Win32 保留设备名、非法字符或末尾空格/点。设备名识别用 ASCII fold，包含 `COM¹`、`COM²`、`COM³`、`LPT¹`、`LPT²`、`LPT³`，并拒绝 stem 末尾空格/点形成的别名。这个规则服务于后续真实写文件，避免 `CON.txt`、`COM1` 这类名字绕过临时目录边界。

Part 4：实现 `display_zone` 规则。本层只要求非空 UTF-8 字符串；真正时区是否存在，由时间显示阶段用 tzdb 验证。本解析器不读环境、不改全局 locale 或时区。

Part 5：比较 `validation/bad`。它只取第一个 `=` 后面的值，既不识别重复键，也不校验文件名；检查器用 `duplicate package_file` 拒绝它。

运行：

```powershell
cmake -S L13_configuration -B build/leaf-L13 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L13 --config Release --parallel 2
ctest --test-dir build/leaf-L13 -C Release --output-on-failure
```

Reference 在 `src/reference/config_parser.hpp`。学生只编辑 `src/student/config_parser.hpp`。公共可复用入口是 `c05/config.hpp` 的 `parse_config`，结课项目会直接使用它。
