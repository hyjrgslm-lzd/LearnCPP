# 13：有界配置解析

C05 的结课项目需要两个外部设置：包文件名 `package_file` 和显示时区 `display_zone`。这还不值得引入 JSON、TOML 或覆盖层。一个小的 `key=value` 子集更适合教学：规则少，错误位置清楚，足够暴露文本边界、文件名边界和后续时间显示边界。

## 1. 格式先于业务

配置输入是 UTF-8 文本，最多 64KiB。解析器先做通用边界：

1. 总大小不超过 64KiB。
2. 严格 UTF-8。
3. 拒绝 NUL。
4. 每行最多 4096 bytes。

这些检查防止后续扫描在坏编码或无界输入上继续。首行 BOM 只在文件开头可选剥离；字符串中间的 `U+FEFF` 不会被通用 UTF 转换删掉。行结束支持 LF 和 CRLF，裸 CR 是坏行。

CRLF 只在“本行确实以 LF 结束，且 LF 前一个 byte 是 CR”时消费这个末尾 CR。随后仍要在剩余行内容里独立查找 CR。`display_zone=UT\rC\r\n` 的错误 offset 指向 `UT` 后面的那个 CR；`display_zone=UTC\r` 没有 LF，也必须作为裸 CR 拒绝。

每个有效行按第一个 `=` 分割。key 和 value 两侧只修剪 ASCII 空格和 tab；不做 Unicode 空白折叠。空行和整行 `#` 注释跳过。未知键、重复键、空值和坏行都返回 `DataError`，带 byte offset 和 1-based line。`DataError.field` 是拥有型字符串，不能指向输入视图。

## 2. 两个键有不同责任

`package_file` 是后续真实写文件会用到的 basename。它不能含目录分隔符、盘符、`.`、`..`、控制字符、Win32 保留设备名、非法字符，也不能以空格或点结尾。`CON.txt`、`NUL`、`COM1`、`LPT9`、`COM¹.txt` 这类名字在 Windows 有设备语义，必须在配置边界拒绝。设备名识别只做 ASCII 大小写折叠，不依赖全局 locale；stem 末尾的空格或点也按设备别名拒绝，例如 `NUL .txt`。

`display_zone` 只要求非空。`America/New_York` 是否存在，不在配置解析阶段判断；那需要 tzdb，是运行环境能力。解析器保存用户写的 zone 字符串，时间显示阶段再调用 `format_timestamp`，失败时把 field 标成 `display_zone`。

默认值是：

```text
package_file=manifest.c05m
display_zone=UTC
```

空配置合法，得到默认值。重复写同一键不做“后者覆盖前者”，因为覆盖会隐藏配置来源错误；本课程没有 include、环境变量替换、profile 叠加或命令行优先级。

## 3. 错误位置要能回到原文件

解析器扫描时维护两个坐标：byte offset 和 line。offset 以原始输入开头为 0，line 从 1 开始。首 BOM 被跳过后，第一条实际配置的 offset 仍按原始 byte 计数。这样编辑器或日志能定位到真实文件位置。

例如：

```text
package_file=a.c05m
package_file=b.c05m
```

第二行应返回 `duplicate_field`，field 是 `package_file`，line 是 2。它不是 `invalid_value`，因为值本身也许合法；真正错误是同一个键出现两次。

文件名 `dir/pack.c05m` 的错误 field 仍是 `package_file`。路径章节的 `validate_resource_path` 不能直接复用到这里：资源 path 允许 `assets/x.png` 这种相对多段路径，而配置的 `package_file` 只允许 basename。UTF-8非法和 NUL 这类全局文本错误也要按错误 offset 推导 line；只有“总大小超过 64KiB”这种尚无具体行位置的全局预算失败可以保留 line=0。

## 4. 练习入口与解析

运行 L13：

```powershell
cmake -S L13_configuration -B build/leaf-L13 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L13 --config Release --parallel 2
ctest --test-dir build/leaf-L13 -C Release --output-on-failure
```

学生只编辑：

```text
L13_configuration/src/student/config_parser.hpp
```

必须完成：

| Part | 要求 | 解析 |
|---|---|---|
| 行扫描 | LF/CRLF、首BOM、空行、整行注释、首个 `=` | offset 仍按原始byte；裸CR不是合法换行 |
| 键集合 | 固定两个键、默认值、重复拒绝 | 后者覆盖会隐藏配置错误 |
| 文件名 | basename、Win32设备名、非法尾部和控制字符 | P1会真实写文件，不能让设备名绕过临时目录 |
| zone | 非空即可，存在性延后 | tzdb是显示阶段能力，不是配置语法 |
| 错误 | `DataError` 带 code/offset/field/line | field 必须拥有，输入视图失效后仍可报告 |

`validation/bad` 故意漏掉重复 `package_file` 拒绝；检查器用 `duplicate package_file` 证明坏实现会被抓到。Reference 位于 `src/reference/config_parser.hpp`，公共项目入口是 `c05/config.hpp` 的 `parse_config`。

自测问题：

**问：为什么不用 JSON/TOML？**

答：本项目只有两个键。引入完整格式会把教学重点从边界和诊断移到库规则、schema和依赖部署。需要嵌套结构时再换格式。

**问：为什么 `display_zone=Bad/Zone` 不在 parse 阶段失败？**

答：它是语法合法、运行时可能无效的值。时间显示阶段能区分“配置格式坏”和“当前环境没有这个 zone”，诊断更准。
