# F01：新容器与新Ranges能力

对应[新容器](../../chapters/08-new-containers.md)、[前沿Ranges](../../chapters/09-frontier-ranges.md)和[标准索引](../../references/standards.md)。这是观察/能力实验，不是Student完成度判定；各Part的预测和解析必须结合正文核对。

## 默认与显式前沿入口

默认构建始终包含`F01_flat_containers`，真实检查flat_map的顺序/更新、proxy迭代、extract/replace和flat_set去重。普通核心基线要求这些本机已具备的能力。

```powershell
cmake -S C06_Ranges/exercises/F01_frontier -B C06_Ranges/exercises/F01_frontier/build-local -G "Visual Studio 18 2026" -DRANGES_ENABLE_FRONTIER=ON
cmake --build C06_Ranges/exercises/F01_frontier/build-local --config Release --parallel 2
ctest --test-dir C06_Ranges/exercises/F01_frontier/build-local -C Release --output-on-failure
```

| 目标 | 实际主体 | 前提与检查 |
|---|---|---|
| F01_inplace_vector | 当前optional引用try API、满容量、异常、N=0 | 头文件及202603L；不静默兼容旧指针签名 |
| F01_hive | 插入/擦除后的保留对象稳定性、遍历、clear | 头文件及202502L |
| F01_concat | 空段、跨段、别名及临时owner | 202403L |
| F01_cache_latest | 重复解引用/推进的计算次数与input能力 | 202411L |
| F01_as_input | 真实值与input/borrowed能力 | 当前名macro 202502L |
| F01_const_input_filter | P3725受约束const filter | as_input宏后另测const范围约束 |
| F01_reserve_hint | 8的提示与3个实际元素分离、真实to收束 | 202502L |
| F01_optional_range | 0/1元素、transform/to和右值dangling | optional_range_support 202406L |
| F01_view_interface_at | C++29受约束at、负/上界拒绝、const元素区分 | view_interface 202606L |
| F01_map_lookup | C++29三个map族的optional引用、不插入缺失键 | map_lookup 202606L |

宏值按[固定草案源码快照](../../references/validation/standard-snapshot.json)记录。每个`.cpp`保留完整主体；条件不满足时只运行说明缺失的驱动，返回77由CTest标SKIP。条件满足后会真正编译、链接和运行主体，任何错误都是失败，不转换成SKIP。未实现分支不能宣称“已经编译了所有前沿代码”。

## Part与解析

两项C++29宏的同一固定源码证据见[补充快照](../../references/validation/standard-snapshot-cpp29.json)。追加主体后的作者复验见`frontier-author-build-r2.json`和`frontier-author-ctest-r2.json`：1 PASS、10 SKIP、0 FAIL；初次8项SKIP记录仍保留，不覆盖成新结果。

- Part A：先预测flat_map重复键、两列存储、迭代能力，再运行。解析：默认存储连续不等于逻辑pair连续，更新同键不增加size。
- Part B：固定容量、稳定对象地址分别选inplace_vector/hive；运行支持的主体。解析：两者解决不同保证，inplace容量不能长大，hive不保证插入顺序或随机访问。
- Part C：逐个解释concat、cache_latest、as_input与reserve_hint改变哪条契约。解析见对应正文，不把惰性当缓存、容量提示当size或input适配当实体化。
- Part D：记录每个缺失项及下一环境所需的真实标准库版本；不把另一个库或自己实现的类型重命名为std接口。

本批作者原始证据在`../../references/validation/frontier-author-{configure,build,ctest}.json`：MSVC19.51 / STL145、202604 / CMake4.2.3，flat观察1 PASS、前沿8 SKIP、0 FAIL。CTest输出的“100% tests passed”包含SKIP，不能改写成9个主体通过。其他平台和前沿主体运行仍未验证，最终状态以全课质量报告为准。
