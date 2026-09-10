# C04 B01 编译成本实验结果

模式：`check-only`。记录时间：`2026-09-09T16:10:31.608538+00:00`。随机种子：`40412025`。

## 环境

- 编译器：`D:\VisualStudio2026\Installed\VC\Tools\Llvm\x64\bin\clang-cl.exe`，sha256 `ec094d5f8f37c106eefcfc47c9997c261a06e3bd8eaaa7255835983a58524156`
- clang 版本：`clang version 22.1.3 (https://github.com/llvm/llvm-project e9846648fd6183ee6d8cbdb4502213fcf902a211)`
- CPU：`{"ProcessorIdentifier":"AMD64 Family 26 Model 68 Stepping 0, AuthenticAMD","NumberOfLogicalProcessors":"24"}`
- OS：`Windows 10 10.0.26100`，machine `AMD64`。
- 标准与选项：`/std:c++latest /O2 /EHsc /GR-`；trace 额外使用 `-ftime-trace` 和 `-ftime-trace-granularity=0`。
- 进程空闲检测：按进程名/PID/CPU 观察 `MSBuild.exe, cl.exe, clang-cl.exe, cmake.exe, link.exe, lld-link.exe, ninja.exe`；不读取完整 command line。
- 多 TU 计时责任：implicit 为 4 个调用方编译进程；explicit 为 4 个调用方加 1 个 provider/instantiation 编译进程。main 编译与 link 只进入 correctness/section 绑定，不进入正式编译时间样本。
- 产物 SHA-256 在编译/链接命令结束后读取，不计入 `process_seconds`。

## Check-only 结果

- 本模式只声明 correctness、trace 和 parser 检查通过；不产生正式 timing 结论。
- detector 控制：`PASS`。
- `type_query` 覆盖 found、not-found、empty 边界；`multi_tu` 检查 implicit/explicit stdout 相等。

## 多 TU section 证据

| 版本 | 调用方 .obj .text raw 合计 | 全部 .obj .text raw 合计 | exe .text virtual |
|---|---:|---:|---:|
| implicit | 336 | 705 | 90861 |
| explicit | 28 | 474 | 90861 |
