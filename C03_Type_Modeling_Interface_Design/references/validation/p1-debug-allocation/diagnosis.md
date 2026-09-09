# P1 Debug 故障注入误伤 STL 代理分配

2026-09-09，本轮初版P1的Release检查通过，Debug的Reference/good却触发Visual C++ Runtime Library `abort() has been called`。用户反馈的程序为 `build/leader-document/Debug/P1_document_validation_good.exe`。这是本轮检查器的实验边界缺陷，不能当作用户环境故障或Document提交算法失败。

## 复现、定位与对照

初版在全部P1目标中替换全局operator new，在apply_batch整个动态调用范围内逐次抛bad_alloc。先加进程内terminate诊断，避免交互式CRT对话框；这只改变错误报告方式，不修复原因。原问题仍可复现为exit 86，日志显示 `phase=inject batch allocation failure fail_at=5 allocations=6`，见[诊断记录](../p1-debug-terminate-diagnostic-r2.json)。

随后缩小到[最小程序](noexcept_string_move.cpp)：构造一个256字符的std::string，只在其移动构造期间令下一次普通分配失败。相同源码、相同Debug CRT做两组：

| 分支 | 结果 | 证据 |
|---|---|---|
| 默认MSVC Debug iterator diagnostics | 16字节分配被注入，noexcept string move进入terminate；受控exit86，无弹窗 | [IDL默认](debug-iterator-on-r1.json) |
| 仅此对照目标设_ITERATOR_DEBUG_LEVEL=0 | 不发生这次代理分配，移动成功，长度256，exit0 | [IDL0](debug-iterator-off-r1.json) |

本机MSVC 14.51.36231/STL202604的源码链：`xstring:1077-1080` 的string移动构造虽为noexcept，仍在Debug路径创建container proxy；`xmemory:1231-1232` 的_Alloc_proxy调用代理allocator的allocate。vector扩容移动含string的Element时进入这条路径。人为让全局new失败，就可能从这个noexcept内部抛出bad_alloc并终止。`xstring:1274-1287`也记录了Debug代理相关的noexcept OOM边界。此定位经非作者源码核对，不能推广为所有标准库都会采用同样实现。

vector::swap的对应实现仅交换代理归属和指针，不进行这次分配；故障不应归因到事务的最终swap提交。

## 修复边界

- 普通P1 Debug目标继续使用默认迭代器检查；不编译全局operator new/delete替换，也不运行全局分配注入。
- 新增两个明确命名的allocation目标，对Reference和独立good分别注入；仅这些单翻译单元实验定义C03_ALLOCATION_INJECTION及MSVC的_ITERATOR_DEBUG_LEVEL=0，不与IDL2对象混链。
- 注入目标仍运行真实Document实现、实际测得的普通分配点、失败回滚与失败后复用检查；没有静默跳过或把异常当成功。
- 测试进程本地设置CRT报告至stderr，unexpected terminate仍非零退出86。没有修改机器全局设置。

修复后原P1 Debug程序及新增注入目标[5/5通过](../p1-ctest-debug-r3.json)，ASan分支[5/5通过](../p1-ctest-asan-r3.json)。生成的普通Reference项目无IDL覆盖，allocation项目单独包含IDL0宏。两条路径分别证明正常Debug行为与受限分配注入实验，不能合并声称验证了所有Debug STL内部OOM路径。原失败记录保留。

复现命令从课程exercises目录执行：

```powershell
cmake -S ../references/validation/p1-debug-allocation -B build/p1-debug-allocation -G "Visual Studio 18 2026" -A x64
cmake --build build/p1-debug-allocation --config Debug
```

两个程序通过C02 record_process.py带30秒外部超时运行；第一个期望exit86与terminate诊断，第二个期望exit0与长度输出。configure/build/run完整命令已保存于本目录JSON。这个复现是显式诊断材料，不进入普通课程自动成功测试。
