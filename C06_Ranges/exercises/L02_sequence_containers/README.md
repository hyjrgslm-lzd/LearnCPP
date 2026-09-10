# L02 sequence containers

观察顺序容器的存储与失效边界。程序只比较合法地址和容器状态，不解引用已经失效的指针、引用、迭代器或 view。

构建入口：

```powershell
cmake -S C06_Ranges\exercises\L02_sequence_containers -B C06_Ranges\exercises\L02_sequence_containers\build-author
cmake --build C06_Ranges\exercises\L02_sequence_containers\build-author --config Release --parallel 2
ctest --test-dir C06_Ranges\exercises\L02_sequence_containers\build-author -C Release --output-on-failure
```
