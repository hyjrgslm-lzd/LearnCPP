# C04 L04-L06 作者交付记录

时间：2026-09-09。范围：`chapters/04-lookup-adl.md`、`chapters/05-overload-sfinae.md`、`chapters/06-constraints.md`、`exercises/L04_lookup/**`、`exercises/L05_overload/**`、`exercises/L06_constraints/**`。未修改共享 CMake、根导航、C05 或其他课程。

## 已交付

- L04：主讲普通/限定/非限定查找、非依赖与依赖名、两阶段查找、`this->` 依赖基类、`typename`/`template` 消歧、ADL 关联 namespace、抑制风险、hidden friend、访问与查找区别；练习实现 `c04_lookup::inspect`，覆盖成员优先、ADL、hidden friend、不可调用成员 fallback、无路径拒绝、引用和值类别、`noexcept`。
- L05：主讲候选集、可行性、转换等级、非模板优先前提、函数模板偏序、类模板偏特化、函数模板不能偏特化、overload 与 specialization 顺序、SFINAE 立即上下文与函数体 hard error、`void_t` 到 requires 的桥接；练习实现 `c04_overload::describe`，覆盖成员调用、字符串字面量 extent、整数/range 路径、bool 与 no-path 拒绝；r2 补入函数模板偏序 observation 和函数体 hard error 编译诊断 pair。
- L06：主讲 simple/type/compound/nested requirement、dependent failure 与 non-dependent hard error、concept-id、原子约束身份、归一化、映射、subsumption、语法满足和 equal-preserving/多遍历语义边界、C++23 与 C++26 包折叠约束差异；练习实现 `field_like`、`stable_field_range`、`field_count`，覆盖字段形状、forward range、单遍 input range 拒绝；r2 将 subsumption observation 改成同形参列表，排除参数偏序混淆。

## 未执行验证

初版按 leader 指令只写作和代码，未启动 configure、build 或 ctest。构建窗口开放后已用 C02 的 `record_process.py` 包装执行 r2 自检，日志追加保存到本目录，未覆盖初次失败。

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/L04_lookup -B C04_Generic_CompileTime_Reflection/exercises/L04_lookup/build/lookup-author -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/L04_lookup/build/lookup-author --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/L04_lookup/build/lookup-author -C Debug --output-on-failure

cmake -S C04_Generic_CompileTime_Reflection/exercises/L05_overload -B C04_Generic_CompileTime_Reflection/exercises/L05_overload/build/lookup-author -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/L05_overload/build/lookup-author --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/L05_overload/build/lookup-author -C Debug --output-on-failure

cmake -S C04_Generic_CompileTime_Reflection/exercises/L06_constraints -B C04_Generic_CompileTime_Reflection/exercises/L06_constraints/build/lookup-author -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/L06_constraints/build/lookup-author --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/L06_constraints/build/lookup-author -C Debug --output-on-failure
```

## r2 验证结果

- L04 叶级：`r2-01-l04-configure.json` PASS；`r2-11-l04-build-debug-after-template-guards.json` PASS；`r2-12-l04-ctest-debug-after-template-guards.json` PASS，5/5。
- L04 Student-only：`r2-06-l04-student-configure.json` PASS；`r2-13-l04-student-build-debug-after-template-guards.json` PASS；`r2-14-l04-student-ctest-debug-expected-fail.json` PASS，CTest exit 8，命中 `check failed: no-route object must not satisfy c04_inspectable`。
- L05 叶级：`r2-15-l05-configure.json` PASS；`r2-24-l05-build-debug-after-member-guard.json` PASS；`r2-25-l05-ctest-debug-after-member-guard.json` PASS，6/6。
- L05 Student-only：`r2-22-l05-student-configure.json` PASS；`r2-26-l05-student-build-debug-after-member-guard.json` PASS；`r2-27-l05-student-ctest-debug-expected-fail.json` PASS，CTest exit 8，命中 `check failed: member describe returns member_result`。
- L06 叶级：`r2-28-l06-configure.json` PASS；`r2-32-l06-build-debug-after-short-name.json` PASS；`r2-33-l06-ctest-debug-after-short-name.json` PASS，5/5。
- L06 Student-only：`r2-34-l06-student-configure.json` PASS；`r2-35-l06-student-build-debug.json` PASS；`r2-36-l06-student-ctest-debug-expected-fail.json` PASS，CTest exit 8，命中 `check failed: missing name is not field_like`。

## 已保留的初次失败与修复

- `r2-02-l04-build-debug.json`：L04 observation 未先声明 CPO 名字，MSVC 在模板定义点报 `C3861`；已通过前置声明 `inspect_fn` 和 `extern inspect` 修复。
- `r2-04-l04-ctest-debug.json`：L04 `missing_typename` 原 subject 被 MSVC 接受；已改为函数体内依赖嵌套类型声明，`evidence-2.json` 命中 `error C7510`。
- `r2-07-l04-student-build-debug.json`、`r2-10-l04-student-build-debug-after-checker-guard.json`：checker 直接绑定 `int&`，Student 不能有限失败；已抽成模板 guard helper。
- `r2-16-l05-build-debug.json`、`r2-23-l05-student-build-debug.json`：checker 直接访问 bad/student 不存在成员；已抽成模板 guard helper。
- `r2-18-l05-ctest-debug.json`、`r2-30-l06-ctest-debug.json`：compile_case 嵌套路径过长触发 MSBuild `FTK1011`，并暴露共享 `compile_case.py` 失败打印路径 GBK 编码问题；未改共享工具，已缩短 L05/L06 diagnostic test name 后复验通过。

## 负编译正则实测

- L04 `L04_lookup_missing_typename`：`diagnostic-build/L04_lookup_missing_typename/evidence-2.json` PASS，subject exit 1，命中 `error C7510`。
- L05 `L05_func_partial`：`diagnostic-build/L05_func_partial/evidence-1.json` PASS，subject exit 1，命中 `error C2768`。
- L05 `L05_body_error`：`diagnostic-build/L05_body_error/evidence-1.json` PASS，subject exit 1，命中 `error C2039`。
- L06 `L06_non_dep`：`diagnostic-build/L06_non_dep/evidence-1.json` PASS，subject exit 1，命中 `error C3861`。

## 剩余边界

- 本作者只做 L04-L06 叶级和 Student-only 自检；根级整课验证、ASan/frontier、最终集成和非作者复验不在本切片内。
- 顶层 `exercises/CMakeLists.txt` 已由 root 注册全部单元；本作者未修改共享注册。
- `compile_case.py` 在失败打印路径有 `control` 字段覆盖和 GBK 输出问题；本轮只保留证据并通过缩短 case name 避开路径失败，没有改共享工具。
