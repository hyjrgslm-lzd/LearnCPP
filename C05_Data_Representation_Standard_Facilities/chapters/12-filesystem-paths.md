# 12：filesystem path、词法规则与资源元数据

资源清单要保存资源路径，但它保存的是元数据，不打开资源文件。这个边界很关键：`std::filesystem` 可以访问真实文件系统，解析 symbolic link、大小写规则和当前目录；Manifest 的 path 字段只需要一个可移植、可比较的 generic相对字符串。

## 1. native与generic不是同一层

`std::filesystem::path` 在 Windows 上可以用反斜杠和盘符，在 POSIX 上根路径以 `/` 开始。`path::generic_string()` 固定使用 `/` 作为分隔符，适合写进跨平台数据格式。

这不表示所有 generic 字符串都安全。`../secret.txt` 也是 generic 字符串，但它会逃出资源根目录。`C:/x` 在 Windows 是带盘符的路径；`/x` 是根路径；`a//b` 有空段；这些都不应该成为包内资源标识。

C05 公共入口 `validate_resource_path` 因此只接受：

- 严格 UTF-8。
- 非空，最多 4096 bytes。
- 无 NUL。
- generic相对路径，分隔符只用 `/`。
- 无根、无盘符、无反斜杠、无空段、无 `.` 或 `..`。

它不做 NFC 规范化，也不做大小写折叠。`"A.txt"` 和 `"a.txt"` 是否同一文件是文件系统问题；包格式按 byte 比较，避免不同平台给出不同身份判断。

## 2. lexically_normal不访问文件系统

`std::filesystem::path{"alpha/./beta/../beta.txt"}.lexically_normal()` 可以得到 `alpha/beta.txt`。这是字符串层面的变换，不检查 `alpha` 是否存在，也不检查 `beta` 是否是 symlink。

如果真实文件系统中 `alpha/beta` 是 symbolic link，那么词法消除 `..` 可能改变含义。`canonical` 和 `equivalent` 会访问文件系统并处理身份，但它们依赖权限、存在性、symlink能力和平台规则。本章观察题只在安全范围内读取 `current_path(error_code)`，展示 error_code 通道；它不创建或删除用户文件。

C05 不把 `validate_resource_path` 叫 sandbox。它只是包内元数据过滤器。后续项目写输出文件时，还必须使用自己创建的临时目录，组合路径后再处理文件系统错误。

## 3. 编码边界

`std::filesystem::path` 的 native编码由平台决定。Windows native路径倾向宽字符，POSIX常把路径当 bytes。C++ 标准库提供 `u8path`、`u8string`、`generic_u8string` 等接口，但实现细节和异常路径仍与平台相关。

Manifest 选择普通 `std::string` 保存 UTF-8 generic路径，是为了让 wire 规则独立于宿主 native路径。进入 Manifest 前先用 `validate_utf8` 验证；需要写入本机路径时，再由 I/O 层把 generic相对路径拼到临时目录下。这个转换属于 I/O 阶段，不属于字段解码阶段。

不要把 `std::string_view` 指向 path 的临时字符串结果。例如：

```cpp
std::string_view bad = fs::path{"a/b"}.generic_string();
```

这会悬垂，因为 `generic_string()` 返回临时 `std::string`。本课公共模型保存拥有型 `std::string`，输入视图只在调用期间借用。

## 4. 练习入口与解析

运行 L12：

```powershell
cmake -S L12_paths -B build/leaf-L12 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L12 --config Release --parallel 2
ctest --test-dir build/leaf-L12 -C Release --output-on-failure
```

自测问题：

**问：为什么 `assets/../secret.txt` 被拒绝，而不是先 `lexically_normal()`？**

答：拒绝比改写更清楚。调用者给的是资源标识，不是让解析器猜路径意图。自动规范化还会让两个不同输入变成同一输出，掩盖上游错误。

**问：为什么不检查文件存在？**

答：Manifest path 是元数据。读取包不应因为本机没有资源文件就失败，也不应打开攻击者指定路径。真实资源文件操作属于后续受控 I/O 阶段。

**问：为什么按 byte 比较，不按大小写或 Unicode 规范化比较？**

答：大小写和规范化规则随文件系统和区域变化。wire 格式需要跨平台稳定规则；byte 比较最明确。需要平台身份判断时，用文件系统 API，在 I/O 层另写规则和错误处理。
