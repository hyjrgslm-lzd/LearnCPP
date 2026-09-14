# L09 Limited API

本题构建一个 Python 3.8 floor 的 Limited API 扩展，并用同一个 `.pyd` 在 Python 3.8.10 和 3.10.11 中导入运行。目标是区分“当前能 import”和“二进制确实走 Stable ABI”。

Student 编辑文件：`student/solution.cpp`。骨架导出 `abi_tag()` 和 `transform()`，但 `transform()` 抛 `NotImplementedError`。

## Part 1：固定 floor

构建必须定义：

```text
Py_LIMITED_API=0x03080000
```

这表示源码承诺只使用 Python 3.8 及以前进入 Limited API 的符号。CMake helper 用 Python 3.8.10 的 `Include` 和 Windows `libs/python3.lib` 编译 Limited 目标。

## Part 2：实现最小模块

模块导出：

- `abi_tag()`：返回 `"limited-0x03080000"`，这是教学 checker 的第一层承诺检查。
- `transform(bytes)`：返回同长 bytes，ASCII `a-z` 转 `A-Z`，其他字节不变。

`abi_tag()` 只是描述性标签，不参与 ABI 证明。真正证明来自四件事合在一起：构建宏、链接库、真实 PE imports、同一二进制双解释器导入和实际行为检查。

Windows 测试会用 CMake 找到已有 `dumpbin.exe` 或 `objdump`，并把它作为 `--imports-tool` 传给 `checks.py`。checker 在加载模块前读取 `.pyd` imports，只允许 `python3.dll`。如果看到 `python38.dll` 或 `python310.dll`，测试直接失败。

## Part 3：同一二进制双解释器

CTest 不重建模块，而是把同一个 `$<TARGET_FILE:c18_l09_reference>` 分别传给：

- `py -3.8 .../checks.py module.pyd`
- Python 3.10.11 `.../checks.py module.pyd`

两个解释器都通过，才说明这个练习的跨版本导入路径成立。加载后 checker 会验证：

- 空 bytes。
- 包含 `NUL` 和 `0xff` 的 bytes。
- `bytes(range(256))` 全字节输入。
- 重复长输入。
- 非 bytes 参数必须抛 `TypeError`。

## Part 4：bad 的意义

`validation/bad/solution.cpp` 代表 Full API/版本专属构建混进 Limited API 教材。Windows 下它链接 `python38.lib`，真实 PE imports 会暴露版本专属 DLL 依赖。checker 诊断 `extension depends on a version-specific Python DLL`，并且在加载模块前拒绝。非 Windows 不注册这个特定 bad，因为它不是 PE imports 场景。

## Reference / good / bad

- `reference/solution.cpp`：完整答案。
- `validation/good/solution.cpp`：独立好实现，调用 `bytes.upper` 内建 descriptor，不复用 Reference 的手写循环。
- `validation/bad/solution.cpp`：Windows 版本专属 DLL 依赖反例。
- `checks.py`：先查 imports，再导入同一模块，检查多组 bytes 契约和 TypeError。

## 构建与运行

Windows Release：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L09_limited_api -B C18_Interoperability_Plugins_Tooling/build/l09 -G "Visual Studio 18 2026" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release -DC18_ENABLE_LIMITED_API=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/l09 --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l09 -C Release --output-on-failure
```

期望：`c18_l09_reference_py38`、`c18_l09_reference_py310`、`c18_l09_good_py38`、`c18_l09_good_py310` 通过；Windows 上 `c18_l09_bad_rejected` 通过，表示版本专属 DLL 依赖被精确拒绝。

Student 验证：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises -B C18_Interoperability_Plugins_Tooling/build/python-student -DC18_BUILD_REFERENCE=OFF -DC18_ENABLE_LIMITED_API=ON -DC18_TEST_STUDENTS=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/python-student --config Release --target c18_l09_student
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/python-student -C Release -R "c18_l09_student" --output-on-failure
```

期望：Student 当前失败，说明 starter 只提供接口壳，没有实现 Limited API transform。

本题的通过范围有限：它证明当前这个扩展二进制在 Python 3.8.10 和 3.10.11 上满足 imports 与行为契约，不证明所有 Stable ABI 函数，也不承诺未来版本。
