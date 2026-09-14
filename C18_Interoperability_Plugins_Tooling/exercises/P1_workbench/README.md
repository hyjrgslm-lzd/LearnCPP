# P1：可扩展二进制处理 CLI

这是集成观察与扩展项目。先完成 L01/L02、插件生命周期及所选语言练习。四个后端接受同一字节契约；测试必须明确选择后端，不能把 native 的通过当成 Python/Lua 的通过。

## 契约与入口

`--hex` 输入为偶数位十六进制，解码后最多1 MiB。输出为转换后的十六进制及换行；工具不写输入文件、不覆盖输出文件。ASCII a-z 转大写，其余全部保留。`--backend plugin` 额外需要 `--plugin` 的绝对路径。未知/未编入后端返回非零错误，不静默回退。

```sh
cmake -S exercises/P1_workbench -B build/p1
cmake --build build/p1 --config Release
# Windows 多配置生成器将程序放到 build/p1/Release。
build/p1/c18_workbench --backend native --hex 61007aff
ctest --test-dir build/p1 -C Release --output-on-failure
```

输出应为 `41005aff`。真实插件路径由 CTest 的 TARGET_FILE 传入，不靠当前目录搜 DLL。Python/Lua 需要对应构建选项及现有开发依赖，见总构建指南。

## Part 1：反向追踪完整调用

从 `main.cpp` 的输入验证到所选后端，再返回结果。对 plugin 路径，画出 `SerialPlugin`、模块句柄、函数表、context、输入及输出的存活区间；对脚本路径，再补解释器与脚本对象。

**解析：** P1 的串行基线不保留回调，也不允许调用方逃逸函数表。process 返回后先显式 close 并检查结果，再结束 owner 作用域。L04 负责更复杂的并发关闭；不能从这里没有线程就推导任意 callback 场景安全。

## Part 2：验证契约和错误

分别运行四个已构建的后端。`checks.py` 使用 Python `bytes.translate` 的独立查表预期，覆盖空输入、NUL、高位字节、全256字节及重复记录，并检查奇数位/非法字符拒绝。需要扩展输入时保留这些边界，不只挑纯英文文本。

**解析：** 总长度一致只证明没有明显截断，仍要逐字节比较。全256字节能暴露 locale 转换或 signed char 误用。非法输入在调用后端之前拒绝，避免把解析故障算成 ABI 故障。

## Part 3：在自己的副本中扩展

本项目提供完整观察起点，不把预提供运行程序当成实现作业完成。实现型作业在各语言/插件题的 Student。集成扩展请在构建目录副本加入第二个处理操作，同时更新版本协商、输入契约、各后端及独立 oracle；提交解释说明哪些改动保持旧语义、哪些要求新版本。

**解析：** 仅新增一个 backend 字符串分支不等于完成互操作。新路径必须证明它真的执行相应运行时、在错误和退出时回收对象，且没有重新引入跨模块 free 或悬空借用。若缺对应依赖，只能记录未验证，不能改测试跳到 native 获得绿灯。
