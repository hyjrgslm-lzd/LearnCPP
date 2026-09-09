# L04：variant 有限状态集合观察

先读 `../../chapters/04-variant-and-state-space.md`。本练习是观察型程序，不要求编辑 Student 文件。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L04_variant -B build/c03-l04 -G "Visual Studio 18 2026" -A x64
cmake --build build/c03-l04 --config Debug
ctest --test-dir build/c03-l04 -C Debug --output-on-failure
```

Part 1：观察 `monostate` 和默认构造。第 0 个替代项决定默认状态。

Part 2：观察访问前提。`get_if` 返回空指针，`get` 在状态不匹配时抛 `std::bad_variant_access`。

Part 3：观察重复类型。`variant<int, int>` 必须按索引访问；业务模型应优先用强类型区分含义。

Part 4：观察 `visit`。单个 `variant` 要覆盖每个替代项；两个 `variant` 会按组合分发。

Part 5：观察异常边界。`emplace` 构造新替代项失败时，当前实现会留下 `valueless_by_exception`；这是允许行为，不能推广成所有操作都会这样。

解析：`variant` 适合封闭状态集合。它不是手写 tag 的语法糖，而是把非法组合从类型空间里删除。开放扩展、插件和跨 ABI 多态另由后续章节讲。
