# C04 01-06/L01-L06 非作者静态审查

日期：2026-09-09

结论：ITERATE。

本轮只做教学与技术静态审查。按任务要求未运行 configure、build、ctest、编译诊断或 LSP。审查依据为 `CONTENT_REFACTORING_GUIDE.md` 第 4-8 行和第 4 节、`LEARNCPP_GLOBAL_PLAN.md` 4.2 下限、`C04_Generic_CompileTime_Reflection/references/implementation-spec.md`，以及 C04 01-06 正文、L01-L06 练习说明、Student、checker、observation、diagnostic case 和 CMake 接线。Reference 未作为判断正文可推导性的依据。

## 阻断项

### 1. [BLOCKER][foundations] C++23 `decltype(auto)` 返回局部对象的说明不正确

位置：

- `chapters/02-deduction.md:71`
- `chapters/03-forwarding-ctad.md:77`

触发：正文把 `return (value);` / `return (local);` 配合 `decltype(auto)` 简化为“括号表达式是左值，会推成左值引用”。本课核心预设是 C++23；P2266R3 已改变 return operand 中 move-eligible id-expression 的值类别规则，`decltype(auto) { return (x); }` 在 C++23 可能推成 `T&&`，并且某些显式 `T&` 返回会变成 ill-formed。结论仍是危险，但原理不能写成 C++20 的旧规则。

原理：C++23 将 move-eligible id-expression 在 `return` 语境中视为 xvalue；`decltype(auto)` 的推导仍按 `decltype(E)`，但 E 的值类别已可能改变。局部引用变量、非 move-eligible 表达式和非局部表达式还要分开讲。

最小补齐：把两处改成版本化说明：C++20 前的 `return (x)` 可推成 `T&`；C++23 对 move-eligible 局部对象/右值引用参数会按 xvalue 推导，常见结果是 `T&&` 或绑定失败。保留“不要返回局部对象引用”的结论，并给一个 `int x` 与 `int& r = x` 的对比例。

### 2. [BLOCKER][foundations] `std::forward` 被说成只在 forwarding wrapper 中才正确

位置：

- `chapters/03-forwarding-ctad.md:25`

触发：正文写“只有在 forwarding reference wrapper 中才是正确工具”。这会让读者把工具和某一种函数外形绑定，而不是绑定到“模板实参/保存的 cvref 是否代表原始值类别”这个规则。

原理：`std::forward<T>(x)` 的契约是按 `T` 恢复值类别；它常用于 forwarding reference wrapper，但也可用于其他已保存/显式携带 cvref 的转发场景。真正错误的是把它用于普通已确定的右值引用成员、无推导的 `T&&`，或用错误的 `T` 伪造值类别。

最小补齐：改成“当 `T` 来自本次推导或明确保存了调用者 cvref 时，`std::forward<T>` 是恢复原始值类别的工具；普通移动用 `std::move`，普通 `T&&` 成员不因此自动变成 forwarding reference。”再保留现有 wrapper 作为主例。

### 3. [BLOCKER][foundations] L01 对显式实例化、`extern template` 和 ODR 没有真实多 TU 观察入口

位置：

- `LEARNCPP_GLOBAL_PLAN.md:88`
- `C04_Generic_CompileTime_Reflection/references/implementation-spec.md:18`
- `chapters/01-template-model.md:75`
- `chapters/01-template-model.md:91`
- `exercises/L01_templates/README.md:17`
- `exercises/L01_templates/CMakeLists.txt:12`
- `exercises/L01_templates/observations/instantiation.cpp:7`

触发：全局 4.2 明确要求“显式实例化及与 ODR 的关系”。正文有 header/one `.cpp` 片段和概念说明，但 L01 练习只注册了单 TU 的按需实例化 observation；没有 header + explicit-instantiation `.cpp` + 多个使用 TU 的最小案例，也没有漏实例化/重复不一致定义的受控诊断。

原理：`extern template` 的意义只在多翻译单元的实例化抑制和链接提供者边界上成立。单文件 observation 只能证明成员按需实例化，不能证明“声明在头、定义在 cpp、显式实例化提供实体、漏掉变链接错误、ODR 不一致”的主线。

最小补齐：给 L01 增加一个小型多 TU observation：`twice.hpp` 声明模板和 `extern template int twice<int>(int)`，`twice_instantiation.cpp` 定义模板并 `template int twice<int>(int)`，两个 use TU 只调用 `twice<int>`；可选再加一个隔离 compile/link negative，证明漏掉提供者不是语法错而是链接边界。无需引入新框架。

### 4. [BLOCKER][foundations] L01 声称检查 `centimeters` 特化路径，但 checker 没有覆盖且当前倍率无法区分默认路径

位置：

- `exercises/L01_templates/README.md:5`
- `exercises/L01_templates/checks/quantity_checks.cpp:27`
- `exercises/L01_templates/checks/quantity_checks.cpp:30`

触发：练习说明写 `centimeters` “只检查特化路径”，但 checker 只检查 `meters` 默认值和 `kilometers` 特化。即使加入 `centimeters`，当前 `meters=1, kilometers=1000, centimeters=1` 也无法区分“显式特化为 1”和“落回主模板默认 1”。

原理：checker 需要证明题意，而不是证明结果数值碰巧相同。默认值和待证明特化值相同，就无法验证特化路径。

最小补齐：二选一即可。若 `centimeters` 不是教学必要项，删掉 README 中“检查特化路径”的承诺；若要保留，改成可区分的整数基准，例如以厘米为 base：`centimeters=1, meters=100, kilometers=100000`，并在 checker 直接读取三个 `unit_scale_v`。

### 5. [BLOCKER][lookup] L05 没有可运行地证明函数模板偏序与 SFINAE hard-error 边界

位置：

- `LEARNCPP_GLOBAL_PLAN.md:91`
- `chapters/05-overload-sfinae.md:45`
- `chapters/05-overload-sfinae.md:87`
- `chapters/05-overload-sfinae.md:98`
- `exercises/L05_overload/README.md:35`
- `exercises/L05_overload/observations/overload_observation.cpp:41`
- `exercises/L05_overload/observations/overload_observation.cpp:48`
- `exercises/L05_overload/validation/diagnostics/function_partial_specialization.cpp:6`

触发：L05 正文讲函数模板偏序和“函数体里的错误不是 SFINAE”。现有 observation 只证明“模板 exact match 胜过需要转换的非模板候选”、类模板偏特化、`requires` 中缺失表达式为 false；diagnostic case 只证明函数模板不能偏特化。读者可以完成 L05 而没有一个最小入口观察两个函数模板如何偏序，也没有一个 subject 证明把检查挪进函数体会变 hard error。

原理：偏序、偏特化非法、SFINAE 立即上下文、函数体 hard error 是四个不同规则。现在代码覆盖了后两个的一部分和“函数不能偏特化”，但没有覆盖函数模板偏序本身，也没有证明 hard-error 反面。

最小补齐：增加一个很小的 observation 或 diagnostic pair：`select(T*)` vs `select(const T*)` / `select(T&)` vs `select(const T&)` 用类型标签证明函数模板偏序；再加一个 negative subject，把 `x.size()` 放进函数体并实际实例化，control 用 trailing return/requires 把失败留在立即上下文。

### 6. [BLOCKER][lookup] L06 的约束 subsumption observation 被参数偏序混淆，不能证明约束偏序

位置：

- `LEARNCPP_GLOBAL_PLAN.md:92`
- `chapters/06-constraints.md:105`
- `chapters/06-constraints.md:120`
- `exercises/L06_constraints/observations/constraints_observation.cpp:41`
- `exercises/L06_constraints/observations/constraints_observation.cpp:47`
- `exercises/L06_constraints/observations/constraints_observation.cpp:67`

触发：observation 想证明“复用同一 concept 原子的 overload 更受约束”，但两个候选的形参分别是 `T` 和 `T*`。对 `choose(&value)`，`T*` 版本可因普通函数模板偏序/参数形态更特化而胜出，不能归因到约束 subsumption。

原理：约束偏序要排除参数匹配差异。否则结果可由函数模板偏序解释，checker 文案就把错误证据当成约束证据。

最小补齐：改成相同形参列表、只靠约束区分的两个 overload，例如 `rank(T)` 一个 `requires atom_a<T>`，另一个 `requires (atom_a<T> && larger<T>)`，并用满足二者的同一类型验证更受约束候选胜出。保留一个“逻辑等价但不同 concept 不形成包含”的注释或独立例子即可。

### 7. [BLOCKER][shared integration] L01-L06 未接入顶层 C04 exercises，根预设不会覆盖本轮被审内容

位置：

- `C04_Generic_CompileTime_Reflection/references/implementation-spec.md:52`
- `C04_Generic_CompileTime_Reflection/exercises/CMakeLists.txt:4`
- `C04_Generic_CompileTime_Reflection/exercises/CMakeLists.txt:5`
- `C04_Generic_CompileTime_Reflection/references/validation/lookup-author/summary.md:32`

触发：顶层 `exercises/CMakeLists.txt` 仍只注册 `L11_customization`。L01-L06 的各自 CMake 文件存在，但根 `verify-core`、`student`、`asan` 等预设当前不会自然覆盖这些单元。lookup 作者交付记录也注明 L04-L06 未加入顶层。

原理：实施规格要求核心/叶级/Student/ASan/frontier 验证分别报告。若单元只存在于子目录，最终集成验证可能漏掉本轮内容，Student-only 目标也不能代表 01-06。

最小补齐：构建窗口开放后，由共享 CMake owner 把已审可构建的 `L01_templates` 到 `L06_constraints` 加入顶层 `lessons` 列表，或在质量报告中明确这些单元只用叶级命令验证且根预设未覆盖。最终完整交付前应以前者为准。

## 已确认的非阻断点

- L02 正文和 checker 能从正文推出按值退化、左值引用保留 cv、数组引用保留 extent、`decltype(auto)` identity 和 `type_identity_t` 非推导上下文；Student 占位返回 `void`/`0` token，静态看会被 checker 拒绝。
- L03 checker 对透明调用的值类别、返回引用、`noexcept`、异常传播、CTAD 和 initializer_list 独立入口有实际消费；问题集中在正文两处标准语义/绝对化说法。
- L04 checker 覆盖成员优先、ADL、hidden friend、不可调用成员后继续 ADL、no-route 拒绝、cvref 返回和 `noexcept`；Student 起点是可编译的无约束返回值占位，静态看会被 no-route/type/value 检查拒绝。
- L05 `describe` 的主要分类 checker 比较实：成员调用计数、字符串字面量 extent、bool/no-path 拒绝、range fallback 都有直接消费；问题是偏序和 hard-error 边界缺少独立最小证明。
- L06 字段形状、forward_range、input_range 拒绝和 Student 宽约束占位都有 checker 入口；问题是 subsumption observation 的证据归因不干净。

## 验证边界

未运行任何编译、测试、诊断 case、LSP 或外部构建命令；因此本报告不声明源码可编译、不声明 bad 控制实际被拒绝、不声明 Student-only 构建状态。完整交付仍需在构建窗口开放后运行叶级和根级验证，并把 raw evidence 绑定到实际文件指纹。
