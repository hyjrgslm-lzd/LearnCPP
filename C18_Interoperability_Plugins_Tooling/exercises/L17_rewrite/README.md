# L17 source rewrite

本题只做一个受控迁移：`c18_process_old` 到 `c18_process`。学生编辑 `student/solution.cpp`，实现 `c18_tooling::register_rewrite_matchers()`。

正确做法不是字符串替换。必须让 Clang 解析源码，用 `MatchFinder` 找声明和已解析引用，再用 `SourceManager` 判断编辑位置：

- 宏展开拒绝。
- 模板声明或模板实例化拒绝。
- 系统头拒绝。
- replacement conflict 拒绝。
- 默认只输出 patch；指定 `out-dir` 时写新副本，不改输入文件，也不覆盖已有输出。

bad 用 Clang matcher 找到符号后直接加 replacement，不做宏、模板、系统头拒绝，checker 必须拒绝。ON 构建时，`l17_driver.cpp` 调用所选 variant 注册 `MatchFinder`，再由 CTest driver 验证输入哈希不变、普通源迁移、宏/模板拒绝、已有输出拒绝和幂等。默认 OFF 不注册算法 PASS。
