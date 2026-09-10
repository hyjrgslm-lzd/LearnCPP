# F01：C++29 线程属性

本题对应 [线程属性正文](../../topics/frontier/01-thread-attributes.md)。

## Part

| Part | 任务 | 检查 |
|---|---|---|
| A | 判断属性必须位于可调用对象之前 | 模型拒绝“callable 后再放属性” |
| B | 区分名字借用和线程对象持有 | 构造期读取后销毁属性，线程仍正常 join |
| C | 区分 `stack_size_hint{0}` 与非零建议 | 0 被忽略，非零只记录为 hint |
| D | 核对 `jthread` 属性不改变 stop token 语义 | stop 请求仍由函数检查 |

`main.cpp` 是 OBSERVATION，只运行教学模型。`solution.cpp` 是标准库主体：缺 `CS_HAS_STD_THREAD_ATTRIBUTES` 返回 77；能力存在后实例化 `std::thread` 与 `std::jthread` 属性构造。

## 运行

```powershell
cmake -S C08_Concurrency/exercises/F01_thread_attributes -B C08_Concurrency/exercises/build/c08-frontier-author/f01 -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX29=ON
cmake --build C08_Concurrency/exercises/build/c08-frontier-author/f01 --config Release
ctest --test-dir C08_Concurrency/exercises/build/c08-frontier-author/f01 -C Release --output-on-failure
```

## 答案要点

属性是构造函数参数前缀；第一个非属性参数才是可调用对象。同一属性类型不能重复。`name_hint<char>` 借用字符序列，不把名字存在 `thread` 对象里；临时 string 只保证完整表达式内构造可读，跨语句保存 hint/view 会悬垂。`stack_size_hint` 是平台建议，0 表示忽略；创建失败仍是 `system_error`。`jthread` 的 stop token、异常和自动 join 语义不因属性改变。

一手来源：[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)、[P2019R9](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p2019r9.pdf)。
