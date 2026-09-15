# L02 sequence containers

观察顺序容器的存储与失效边界。程序只比较合法地址和容器状态，不解引用已经失效的指针、引用、迭代器或 view。

构建入口：

```powershell
cmake -S C06_Ranges\exercises\L02_sequence_containers -B C06_Ranges\exercises\L02_sequence_containers\build-author
cmake --build C06_Ranges\exercises\L02_sequence_containers\build-author --config Release --parallel 2
ctest --test-dir C06_Ranges\exercises\L02_sequence_containers\build-author -C Release --output-on-failure
```
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `L02_sequence_containers`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`main.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target L02_sequence_containers。
修改后先重建 `L02_sequence_containers`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
