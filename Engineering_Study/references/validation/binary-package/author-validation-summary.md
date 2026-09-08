# Binary package 作者验证摘要

日期：2026-09-08。

## 环境

- CMake：4.2.3
- 编译器：MSVC 19.51.36256.0
- VS：`D:\VisualStudio2026\Installed`
- Ninja：`D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe`

## 已验证命令

以下命令都在 `VsDevCmd.bat -arch=x64 -host_arch=x64` 子进程环境中执行；不修改系统 PATH。

### D1

```powershell
cmake -S Engineering_Study\exercises\D1_shared_library -B Engineering_Study\exercises\build\binary-package-author-ninja\D1-Release -G Ninja -DCMAKE_MAKE_PROGRAM=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_BUILD_TYPE=Release
cmake --build Engineering_Study\exercises\build\binary-package-author-ninja\D1-Release --parallel 4
ctest --test-dir Engineering_Study\exercises\build\binary-package-author-ninja\D1-Release --output-on-failure
```

结果：5/5 PASS，含 `D1_static_reference`、`D1_shared_reference`、`D1_loader_reference`、`D1_negative_missing_dll`、`D1_negative_missing_export`。

Debug 同构命令结果：5/5 PASS。

### E1

```powershell
cmake -S Engineering_Study\exercises\E1_abi -B Engineering_Study\exercises\build\binary-package-author-ninja\E1-Release -G Ninja -DCMAKE_MAKE_PROGRAM=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build Engineering_Study\exercises\build\binary-package-author-ninja\E1-Release --parallel 4
ctest --test-dir Engineering_Study\exercises\build\binary-package-author-ninja\E1-Release -LE student --output-on-failure
ctest --test-dir Engineering_Study\exercises\build\binary-package-author-ninja\E1-Release -L student --output-on-failure
```

结果：非 student 5/5 PASS，含 static/shared reference、bad_alloc 私有变体、真实 C consumer、layout observation。Student starter 预期失败，输出 `check failed: create must reject unsupported ABI`。

Debug 同构命令结果：非 student 5/5 PASS；Student starter 预期失败，同样输出 `check failed: create must reject unsupported ABI`。

### J1

```powershell
cmake -S Engineering_Study\exercises\J1_package -B Engineering_Study\exercises\build\binary-package-author-ninja\J1-Release -G Ninja -DCMAKE_MAKE_PROGRAM=D:\VisualStudio2026\Installed\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_BUILD_TYPE=Release
cmake --build Engineering_Study\exercises\build\binary-package-author-ninja\J1-Release --parallel 4
ctest --test-dir Engineering_Study\exercises\build\binary-package-author-ninja\J1-Release --output-on-failure
```

结果：1/1 PASS。`J1_package_roundtrip` 完成安装到 `prefix_A`、复制到 `prefix_B`、静态 consumer、动态 consumer、版本 2.0.0 拒绝、缺 `LessonPackage::lesson` 拒绝、缺 DLL 隔离运行拒绝；导出 include 不指向源码/build，static target 传播 `LESSON_STATIC`。

Debug 同构命令结果：1/1 PASS。

## 额外边界

- 最初尝试过 Visual Studio generator 并发构建，MSBuild 在 `ALL_BUILD` 上返回失败但报告 `0 个错误`；未作为代码失败证据。最终验收采用 VS Ninja 正常链路。
- ELF 条件路径和说明已保留，但本次未实测。
- 未改 `Concurrency_Study`，未改公共 helper、导航或顶层注册。
- 源文件指纹见 `source-fingerprints.sha256`。
- 最终 CTest 原始输出见 `final-D1-*-ctest.stdout.txt`、`final-E1-*-ctest.stdout.txt`、`final-J1-*-ctest.stdout.txt`；J1 的内部 install/consumer/负例步骤见 `final-J1-*-ctest-verbose.stdout.txt`；E1 student 的 stderr 只含 CTest 失败提示，stdout 记录明确失败原因。
