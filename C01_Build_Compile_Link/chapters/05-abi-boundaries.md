# 05：ABI 边界、所有权与错误通道

动态库让代码可以在运行时进入进程，但“能装载”不等于“接口能长期兼容”。ABI 讨论的是二进制调用边界：调用方按什么名字找函数，参数放在哪里，返回值怎样传回，对象布局是否一致，谁分配谁释放，错误怎样穿过边界。

源码级接口可以靠重新编译消化很多变化。ABI 边界不一定有这个机会。consumer 可能只拿到头文件、导入库和 DLL；它按旧约定编译好以后，再换一个新 DLL。如果函数签名、结构体布局、调用约定、运行库所有权或异常边界变了，程序可能编译通过但运行错误。

本章用一个故意保守的小 API：

```c
typedef struct lesson_engine lesson_engine;

lesson_status lesson_create(unsigned requested_abi, int seed, lesson_engine** out_engine);
lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value);
void lesson_destroy(lesson_engine* engine);
```

实际规则固定：ABI 版本是 1；`seed` 和 `input` 必须在 `[-1000, 1000]`；成功时 `eval = seed + input`。这个函数本身没有业务价值，价值在于它把跨边界风险缩到能验证的范围。

## 为什么用 C ABI

C++ 名字会被编码，重载、命名空间、类成员、异常规格和模板都会影响符号形状。不同编译器和版本的编码规则不保证互通。`extern "C"` 关闭 C++ name mangling，让导出名稳定成 C 函数名。这只解决名字和 C 调用形状，不会让 C++ 对象、STL、异常自动跨边界安全。

配套练习有一个真实 C consumer。它包含同一个 `lesson_api.h`，链接 reference shared library，然后调用 create/eval/destroy。这个程序证明 C 端可以消费当前 C ABI；它不证明 C 能消费 C++ 类，也不证明不同 CRT 可以混合释放对象。

## opaque handle 隐藏布局

`typedef struct lesson_engine lesson_engine;` 只声明一个不完整类型。consumer 知道它是一个句柄类型，但不知道对象内部字段。这样实现方可以在 DLL 内部放 `std::string`、vector、mutex 或别的私有状态，只要不让这些类型出现在 ABI 参数和返回值里。

opaque handle 也把所有权说清楚：`lesson_create()` 在库内创建对象，`lesson_destroy()` 在同一个库内销毁对象。consumer 不能 `delete` 它，也不能按结构体大小分配它。指针在 destroy 后失效；重复 destroy 同一个已释放指针是调用方违反契约，本课不运行这种 UB。`lesson_destroy(nullptr)` 被允许，便于调用方清理失败路径。

## create 的失败路径

`lesson_create()` 的第一个动作是检查 `out_engine`。如果它非空，函数先把 `*out_engine` 置为 null，再继续检查 ABI 版本和 `seed` 范围。这样调用方不会在失败后误用旧指针。

顺序很重要：

1. `out_engine == nullptr` 时返回 `LESSON_INVALID_ARGUMENT`，没有可写位置。
2. `*out_engine = nullptr`，清掉调用方输出槽。
3. `requested_abi != 1` 时返回 `LESSON_UNSUPPORTED_ABI`。
4. `seed` 越界时返回 `LESSON_OUT_OF_RANGE`。
5. 分配并构造对象，成功后写入真实 handle。

内部任何 C++ 异常都被转换成 `lesson_status`。配套 private test variant 用编译期私有定义让实现受控抛出 `std::bad_alloc`，验证它返回 `LESSON_INTERNAL_ERROR` 且不写出 handle。它不耗尽系统内存，也不是生产 API 的故障注入开关。

## eval 不破坏旧输出

`lesson_eval()` 成功时写 `out_value = seed + input`。失败时不改 `*out_value`。检查器会先放一个哨兵值，再传入空 handle、空输出指针和越界输入，确认失败路径没有复用上一次正确结果，也没有把输出写成看似合理的值。

这个规则比“失败时写 0”更适合教学，因为它能明确区分成功写入和失败无写入。真实项目也常这样设计：只有返回 OK 时输出参数有效。

## status 和静态字符串

错误通道用 `lesson_status`，不是异常。`lesson_status_message()` 返回静态借用字符串，调用方不能释放它，也不能长期假设字符串地址有业务意义。C consumer 只能把它当只读消息打印或比较非空。

跨 DLL 抛 C++ 异常会把异常对象布局、运行库、展开表和编译器实现一起放进 ABI 契约。除非整个边界由同一工具链、同一运行库和同一发布策略锁定，否则更小的做法是把异常留在库内，边界上返回 status。

## 调用约定、布局和配置

即使函数名稳定，调用约定也要一致。Windows 上 `__cdecl`、`__stdcall` 等约定会影响参数清理和符号装饰。x64 Windows 的普通 C 调用约定更统一，但这不是跨平台、跨架构的通用免检理由。本课 API 不额外切换调用约定，避免把教学重点变成平台汇编。

结构体布局同样不能随便跨边界。字段顺序、对齐、填充、枚举大小、编译选项、标准库实现都会影响二进制布局。配套练习只在 observation 中打印安全的布局事实，并说明“本机看到这个大小”不是兼容承诺。公共 ABI 不暴露结构体定义，所以布局变化留在库内部。

## STL、异常和所有权的取舍

不要把 `std::string`、`std::vector` 或 C++ 类对象直接放进这个边界。它们的布局和分配策略属于实现细节。即使 producer 和 consumer 都是 C++，只要它们可能用不同编译器、不同标准库或不同 CRT，跨边界构造、析构和释放就会变成隐形契约。

保守边界的代价是写起来啰嗦：要传输出参数，要检查 status，要手动 destroy。收益是故障能被定位到明确阶段：版本不匹配被拒绝，参数非法被拒绝，内部异常被转换，资源在创建它的库内释放。对插件、语言绑定、长期发布的 DLL 来说，这个代价通常比追查 ABI 崩溃小得多。

## 自测

1. `extern "C"` 解决了 ABI 的全部问题吗？

   **解析：** 没有。它主要稳定 C 形状函数名和语言链接。对象布局、调用约定、运行库、所有权、异常、线程和版本兼容仍要单独设计和验证。

2. 为什么 `lesson_create()` 失败前要把 `*out_engine` 置 null？

   **解析：** 这样调用方在失败后不会误用旧 handle。检查 ABI 或范围之前清空输出槽，可以让所有失败路径保持同一个可解释状态。

3. 为什么 `eval` 失败时不改输出？

   **解析：** 输出参数只在 `LESSON_OK` 时有效。失败不改输出让检查器能用哨兵值证明实现没有在错误路径写入假结果，也防止调用方误读失败值。

4. 为什么不运行跨 CRT 释放来证明危险？

   **解析：** 那是真实未定义行为或平台相关崩溃，不保证稳定复现。课程用契约和安全检查证明正确边界：谁创建谁销毁，consumer 不释放库内对象。
