# L10 mp-units 实际单位转换

先读 [量纲与单位](../../chapters/04-units.md)。这是观察型 consumer，使用 mp-units 2.5.0、gsl-lite 合约和标准格式设施，不使用模块或上游测试工程。

从 `exercises` 的开发终端执行 `cmake --preset full-windows`、`cmake --build --preset full-windows --target c13_units`，再运行 `../build/full-windows/L10_units/c13_units.exe`。可用 `ctest --preset full-windows -R '^c13_units$'` 检查。

单题构建：`cmake -S L10_units -B ../build/units -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DC13_ENABLE_THIRD_PARTY=ON -DC13_FETCH_DEPS=ON`，随后 `cmake --build ../build/units`。预填 `FETCHCONTENT_SOURCE_DIR_*` 并关闭获取可使用本地副本，来源责任见依赖索引。

## 观察任务与解析

1. 先预测 16 ms 用秒表示的数值：`0.016`。类型仍表示时间，改变的是单位与数值表示。
2. 120 m / 10 s 得到 12 m/s，乘 16 ms 得到 0.192 m。运行检查消费真实库结果，不能用自己打印的预期常量代替。
3. 复制示例到自己的编辑位置，将 `speed` 的时间参数换成长度，解释 Concepts 诊断。原始文件中的 `static_assert` 已验证时间不是长度；故意不能编译的调用不要直接留在默认正常目标中。
4. 思考整数表示为何不适合这个亚秒换算。答案是单位正确不保证目标表示能保存小数；显式强制转换也需要明确精度政策。

`main.cpp` 完整可运行，所以通过只表示这个 consumer 正确，并不代表已经完成上述预测与解释。不要把命名空间改成 `std` 来模拟标准库。
