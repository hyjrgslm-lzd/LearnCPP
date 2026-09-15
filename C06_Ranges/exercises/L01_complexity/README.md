# L01 complexity

观察 worst / average / amortized 三种成本口径。运行程序后对照 `chapters/01-complexity-contracts.md`，确认每个计数对应哪个公开操作，不用总耗时代替复杂度归因。

构建入口：

```powershell
cmake -S C06_Ranges\exercises\L01_complexity -B C06_Ranges\exercises\L01_complexity\build-author
cmake --build C06_Ranges\exercises\L01_complexity\build-author --config Release --parallel 2
ctest --test-dir C06_Ranges\exercises\L01_complexity\build-author -C Release --output-on-failure
```
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L01_complexity`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`main.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L01_complexity。
修改后先重建 `L01_complexity`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
