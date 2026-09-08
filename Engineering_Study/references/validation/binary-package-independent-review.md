# Binary package 独立审查

审查日期：2026-09-08。结论：APPROVE。

审查身份：非作者 review。范围只读核对 `chapters/04-dynamic-libraries-and-runtime.md`、`05-abi-boundaries.md`、`10-packaging-and-compatibility.md`、`exercises/D1_shared_library`、`exercises/E1_abi`、`exercises/J1_package` 和 `references/validation/binary-package`。未修改 D/E/J 源码；临时验证只写入 `Engineering_Study/exercises/build/binary-package-independent-review/`。

## 依据

- 根指南要求课程正文不能靠外链替代推导，练习要能遮住答案完成，验证要区分规范、实现和实验边界。
- D1 正文和练习区分 static library、Windows import library、DLL、隐式装载和显式装载；`loader_check.cpp` 用 `LoadLibraryA/GetProcAddress/FreeLibrary` 或 `dlopen/dlsym/dlclose`，并用退出文件观察模块卸载。
- E1 正文和练习把 ABI 边界收束到 C ABI、opaque handle、同库 create/destroy、status 错误通道；检查器覆盖 ABI 版本、空指针、范围、输出不变、异常转换、真实 C consumer。
- J1 正文和练习把安装包和 build tree 分开；脚本安装到 `prefix_A`，复制到 `prefix_B`，consumer 只设置 `CMAKE_PREFIX_PATH=prefix_B` 并禁用 package registry/system package registry；负例分别检查版本拒绝、缺 exported target 和缺 DLL 运行失败。

## 代码审查结果

### D1

- `D1_runtime_static` 和 `D1_runtime_shared` 分开建库；shared producer 私有定义 `D1_RUNTIME_BUILD_SHARED`，shared consumer 定义 `D1_RUNTIME_USE_SHARED`。
- `D1_loader_reference` 显式装载真实 shared library，查找 `lesson_runtime_value`、`lesson_runtime_init_count`、`lesson_runtime_record_exit_to`，并在释放模块后检查退出文件。
- `D1_negative_missing_dll` 只复制 exe 到 build tree 下隔离目录，不复制 DLL；脚本拒绝删除 build tree 外路径，并拒绝“跑到 linked check passed”的伪负例。
- `D1_negative_missing_export` 用真实 DLL 查找 `lesson_missing_export`，匹配 `missing export`，不是任意失败。

### E1

- `lesson_api.h` 的 `LESSON_STATIC`、`LESSON_BUILD_SHARED`、Windows shared consumer `dllimport` 分支清晰；C ABI 导出函数没有 STL、异常或 C++ 类穿边界。
- Reference `lesson_create()` 先处理 `out_engine == nullptr`，再清空 `*out_engine`，再检查 ABI 和 seed；成功后同库 `new`，`lesson_destroy()` 同库 `delete`。
- Reference `lesson_eval()` 对空 handle、空输出、越界输入返回 status，失败不改输出；内部异常转换为 `LESSON_INTERNAL_ERROR`。
- `E1_abi_c_consumer` 是真实 C 源文件，包含同一个 `lesson_api.h`，调用 create/eval/destroy 得到 `42`。
- `check.hpp` 使用抛异常的 `check()`，Release 下不会像 `assert` 一样消失。

### J1

- 包只安装并导出 `LessonPackage::lesson_static` 和 `LessonPackage::lesson_shared`；没有 `LessonPackage::lesson` alias。缺 `LessonPackage::lesson` 负例是故意的 consumer 名字错误，用于验证目标集合，不是损坏导出配置。
- `lesson_static` PUBLIC 传播 `LESSON_STATIC`；安装后的 `LessonPackageTargets.cmake` 中 `LessonPackage::lesson_static` 带有 `INTERFACE_COMPILE_DEFINITIONS "LESSON_STATIC"`。
- `run_package_roundtrip.cmake` 读取 `prefix_B/lib/cmake/LessonPackage/*.cmake`，拒绝导出文件包含 source/build 路径；独立复验的 `prefix_B` package cmake 文件未命中源码或 build 路径。
- consumer cache 显示 `CMAKE_PREFIX_PATH` 和 `LessonPackage_DIR` 均指向 `prefix_B`；registry/system package registry 被 consumer CMake 和 roundtrip common args 双重禁用。
- shared consumer 运行只通过子进程 `PATH=${prefix_b}/bin` 找 DLL；缺 DLL 负例只删除 `prefix_missing` 下 DLL 并运行已构建 consumer。

## 独立复验

原始日志：`Engineering_Study/exercises/build/binary-package-independent-review/logs/`。

### Visual Studio generator，多配置 Debug/Release

使用 `Visual Studio 18 2026` generator。MSBuild build 步骤使用保守参数：

```powershell
/m:1 /nr:false /p:UseMultiToolTask=false /p:CL_MPCount=1
```

结果摘要：

| 步骤 | 结果 |
| --- | --- |
| `01-configure-D1-vs` | exit 0 |
| `02-build-D1-Debug-vs` | exit 0 |
| `03-ctest-D1-Debug-vs` | exit 0，5/5 passed |
| `04-build-D1-Release-vs` | exit 0 |
| `05-ctest-D1-Release-vs` | exit 0，5/5 passed |
| `06-configure-E1-vs` | exit 0 |
| `07-build-E1-Debug-vs` | exit 0 |
| `08-ctest-E1-Debug-reference-vs` | exit 0，5/5 passed |
| `09-ctest-E1-Debug-student-vs` | exit 8，starter 明确失败 |
| `10-build-E1-Release-vs` | exit 0 |
| `11-ctest-E1-Release-reference-vs` | exit 0，5/5 passed |
| `12-ctest-E1-Release-student-vs` | exit 8，starter 明确失败 |
| `13-configure-J1-vs` | exit 0 |
| `14-build-J1-Debug-vs` | exit 0 |
| `15-ctest-J1-Debug-vs` | exit 0，1/1 passed |
| `16-ctest-J1-Debug-verbose-vs` | exit 0，roundtrip 内部步骤全 passed |
| `17-build-J1-Release-vs` | exit 0 |
| `18-ctest-J1-Release-vs` | exit 0，1/1 passed |
| `19-ctest-J1-Release-verbose-vs` | exit 0，roundtrip 内部步骤全 passed |

J1 verbose 输出确认内部步骤：

- install package passed
- configure/build/run static consumer passed
- configure/build/run shared consumer passed
- reject version 2.0.0 failed as expected
- reject missing exported target failed as expected
- missing DLL run failed as expected

### E1 student 实际路径正反变体

临时源码只在 `Engineering_Study/exercises/build/binary-package-independent-review/e1-student-variants-src/`，不进入课程源码。它复用真实 `E1_abi/checks/abi_contract_check.cpp` 和公开 `lesson_api.h`，不调用 Reference。

有效 rerun：

| 步骤 | 结果 |
| --- | --- |
| `24-rebuild-E1-student-variants-Release-vs` | exit 0 |
| `25-ctest-E1-good-student-Release-vs` | exit 0，`good_student_check` passed |
| `26-ctest-E1-bad-students-Release-vs` | exit 8，4/4 bad variants failed |

bad variants 被拒原因：

- `bad_no_version_reject_check`：`create must reject unsupported ABI`
- `bad_no_clear_on_failure_check`：`create must clear out pointer before ABI rejection`
- `bad_writes_output_on_error_check`：`eval must not modify output on null handle`
- `bad_wrong_sum_check`：`eval must compute seed + input`

这证明学生实际实现路径可以独立完成并通过，也证明 Release 下检查器能拒绝关键错误。

## 指纹

`Engineering_Study/references/validation/binary-package/source-fingerprints.sha256` 与当前工作树对应文件全部匹配。

## 已测 / 未测

已测：

- D1 Debug/Release：static、shared、显式 loader、缺 DLL、缺导出。
- E1 Debug/Release：static/shared Reference、bad_alloc 私有变体、真实 C consumer、layout observation、student stub 清晰失败。
- E1 Release 临时 student 正反变体。
- J1 Debug/Release：安装到 `prefix_A`，复制 `prefix_B`，static/shared consumer，版本拒绝，缺 target，缺 DLL。
- VS 多配置 generator 路径；作者 Ninja 证据另见 `references/validation/binary-package/author-validation-summary.md`。

未测：

- ELF/Linux loader、rpath、`LD_LIBRARY_PATH` 分支。
- ASan/UBSan。
- 跨 CRT 释放 UB；正文明确不运行此类 UB。

## 结论

APPROVE。未发现需要作者修改的阻塞项。现有 D/E/J 正文、练习、Reference、student gate、J1 packaging roundtrip 和作者证据满足本轮教学/技术/实验审查要求。
