# 可重放probe的位置

r5c原始命令使用`build-doc-probe`作为源目录名，该名字被仓库已有`build-*`规则忽略。集成时仅将原始`CMakeLists.txt`与`g2_doc_probe.cpp`移入`doc-probe/`，未改文件字节，原始日志不覆盖。后续独立复验使用新源目录和另一个忽略的build目录。

重放时从仓库根运行：

```powershell
cmake -S C06_Ranges/references/validation/ranges-protocols-r5/doc-probe -B C06_Ranges/references/validation/ranges-protocols-r5/build-replay -G "Visual Studio 18 2026"
cmake --build C06_Ranges/references/validation/ranges-protocols-r5/build-replay --config Release
ctest --test-dir C06_Ranges/references/validation/ranges-protocols-r5/build-replay -C Release --output-on-failure
```
