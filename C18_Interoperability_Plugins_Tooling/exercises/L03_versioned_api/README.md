# L03：先协商，再创建状态

先读[版本契约](../../chapters/02-versioned-contract.md)。本题用真实 C11 consumer 调用真实共享库的 `c18_get_api`；作业是补齐 `student/solution.hpp` 的 host 函数表验证。公共 ABI 和插件业务实现已提供，不修改 checker。

## Part 1：完整表的边界

空表返回 BAD_ARGUMENT；版本不匹配返回 UNSUPPORTED_VERSION；结构过小或必需函数为空返回 BAD_ARGUMENT。全部满足才返回 OK。检查器传入的是一个实际完整大小的本地对象，其中 struct_size 字段可能声明短前缀；如果直接读取任意外来短对象，在访问成员之前还需要外部可读长度，本题没有宣称这个函数能探测内存可访问性。

**解析：** 版本描述语义，size 描述可用前缀，函数指针描述实际提供的操作，三者不能互相替代。bad 只检查版本，因此“缺 destroy”仍被接受，必须由 `required function table` 拒绝。

## Part 2：失败不能提交半份输出

观察公共 query 面对未知版本和过小输出表时是否保持原始哨兵。读取构建日志，确认 `checks.c` 确实按 C11 编译。再观察真实 create 的失败输出是否清空；检查器保留原有效 context 的独立所有者并完成销毁，不丢掉原资源。

**解析：** 协商失败尚无业务资源，所以最容易回滚；create 失败不应交付一个无法判定是否有效的旧指针。公共模块对这两类 out 参数采用不同契约：表失败保持不变，create 失败清空输出。区别必须显式说明，不能一律推断为“所有失败都写零”。

## Part 3：兼容边界

将本题与 L01 对照：现在状态码表示固定为 uint32_t，长度使用本进程的 size_t，函数表也属于当前平台 ABI。运行得到的 sizeof 只证明该构建环境，不允许把它硬编码成跨平台序列化格式。

**解析：** 用另一操作系统/架构运行需要重编译匹配二进制；同一功能名字或版本号不能替代平台兼容。C11/C++ 消费者是局部证据，Python结构体还需与本机布局对照。

```sh
cmake -S exercises/L03_versioned_api -B build/l03
cmake --build build/l03 --config Release
ctest --test-dir build/l03 -C Release --output-on-failure
```

Student 初始对有效表也返回错误，应真实失败；不能通过改版本常量或调用 Reference 完成作业。
