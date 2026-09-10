# L01 complexity

观察 worst / average / amortized 三种成本口径。运行程序后对照 `chapters/01-complexity-contracts.md`，确认每个计数对应哪个公开操作，不用总耗时代替复杂度归因。

构建入口：

```powershell
cmake -S C06_Ranges\exercises\L01_complexity -B C06_Ranges\exercises\L01_complexity\build-author
cmake --build C06_Ranges\exercises\L01_complexity\build-author --config Release --parallel 2
ctest --test-dir C06_Ranges\exercises\L01_complexity\build-author -C Release --output-on-failure
```
