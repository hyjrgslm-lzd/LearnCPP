# L12 filesystem paths

目标：区分 `std::filesystem::path` 的 native表示、generic表示、词法规范化和文件身份。编辑入口无；本题是观察型。

Part 1：观察 `generic_string()`。Windows native分隔符可显示为反斜杠，generic格式固定用 `/`，适合作为包内元数据。

Part 2：观察 `lexically_normal()`。它只重写字符串，不打开文件，也不证明两个路径指向同一对象。`canonical`/`equivalent` 需要访问文件系统，本课资源清单 path 不做这种检查。

Part 3：检查 `validate_resource_path`。资源 path 必须是严格 UTF-8、非空、无 NUL 的 generic相对路径；拒绝根、盘符、反斜杠、空段、`.` 和 `..`。比较按 byte，不做 NFC 或大小写折叠。

运行：

```powershell
cmake -S L12_paths -B build/leaf-L12 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L12 --config Release --parallel 2
ctest --test-dir build/leaf-L12 -C Release --output-on-failure
```

解析：这不是 sandbox 校验。它只保证包内元数据 path 是可移植的相对 generic路径；真正写文件时仍要把目标限定在自己创建的临时目录，并处理 `error_code` 或异常。
