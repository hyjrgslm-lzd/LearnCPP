# 12. 编译成本：先定位，再谈优化

泛型代码的成本不只在运行时。模板要经历查找、替换、约束检查、实例化、常量求值、代码生成和链接合并。某个写法看起来“更现代”或“更短”，不自动代表编译更快、二进制更小。本章只讨论两个可复现问题：类型集合查询的实例化形状，以及多翻译单元里同一个模板实体是否被反复生成。

读本章前需要能用 C01 的方式区分编译、链接和运行，知道 C02 的对象文件不是最终程序。B01 默认构建只跑有限正确性观察，正式 benchmark 必须显式运行，并把过程记录写到本地未跟踪目录。

## 成本模型

一次普通 C++ 编译至少有三层成本。

第一层是前端成本：预处理、解析、名字查找、模板替换、约束检查、实例化和常量求值。`clang-cl /clang:-ftime-trace=...` 给出的 `Total Frontend`、`Total InstantiateClass`、`Total InstantiateFunction`、`Total EvaluateAsConstantExpr` 等事件属于这一层的观察入口。trace 里的事件有嵌套关系，不能把所有事件时长相加后说成 CPU 占比；`Total InstantiateClass count` 是 trace 的 `Total ...` scope 计数，不是所有嵌套实例化总数。本章只把这些 `Total ...` 事件当同一工具口径下的定位线索。

第二层是后端成本：优化、代码生成、汇编和对象文件输出。一个 traits 查询只产生很少机器码，但仍可能让前端做很多模板工作；一个函数模板则可能在多个翻译单元各自产生 COMDAT 代码。

第三层是链接成本与最终体积。Windows/COFF 下模板函数常进入 COMDAT，链接器可以合并重复定义。于是“4 个 `.obj` 里都有代码”和“最终 `.exe` 里有几份代码”是两个问题。用 `.obj` 总大小冒充最终代码体积会把编译工作和链接结果混在一起。

## 实验一：递归查询与折叠查询

契约固定为：给定一个目标类型和一个类型集合，回答目标类型是否存在。输入规模固定为 32、128、256 项，目标放在最后一项，让递归版本必须走完整个列表。

递归版本：

```cpp
template<class T, class... Ts>
struct contains : std::false_type {};

template<class T, class Head, class... Tail>
struct contains<T, Head, Tail...>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         contains<T, Tail...>> {};
```

折叠版本：

```cpp
template<class T, class... Ts>
constexpr bool contains = (std::is_same_v<T, Ts> || ...);
```

预测很小：折叠版本应减少类模板递归实例化数量；是否减少墙钟时间，要看编译器前端、头文件成本和进程启动成本在这个小程序里占多大比例。正式结论只能来自同源、同编译器、同选项、同输入下的 trace 与独立进程样本。

正式测量要先用trace定位，再用不带trace插桩的独立进程样本计时。记录固定随机种子、一次预热、五次样本、时钟分辨率和环境；原始样本留在本地未跟踪目录。

结果不应写成“规模越大必然加速”的简单叙事。小规模样本常被时钟tick、进程启动和头文件解析淹没；只有差异超过噪声边界并能从trace看到前端工作变化时，才可写成该输入下的可见收益。`Total InstantiateClass`等scope计数不能当作所有嵌套实例化总数，也不能单独解释全部时间。可以说“某组输入下折叠表达式减少了前端工作并带来可见编译时间收益”，不能说“所有类型 traits 改成折叠都会有可见加速”。

## 实验二：隐式实例化与显式实例化

契约固定为：4 个翻译单元都调用同一个函数模板 `transform<64>(std::uint32_t)`，程序逐项核对 4 个调用结果。隐式版本把模板定义放进头文件，每个 TU 都能实例化；显式版本在头文件声明：

```cpp
extern template std::uint32_t transform<64>(std::uint32_t);
```

并在一个 `.cpp` 中放唯一的显式实例化定义：

```cpp
template std::uint32_t transform<64>(std::uint32_t);
```

预测同样有限：显式实例化应减少调用方 TU 的模板实例化和对象代码生成，但会新增一个专门的实例化 TU。最终 `.exe` 的 `.text` 未必变小，因为隐式版本的重复 COMDAT 可能已被链接器合并。

本机 trace 按 TU 分别记录。观察重点是编译工作迁移：显式版本的 4 个调用方对象应更薄，实例化工作集中到 `instantiation.cpp`。正式计时中，隐式版本比较 4 个调用方编译进程；显式版本比较 4 个调用方加 1 个 provider/instantiation 编译进程。`main` 编译和链接另记，用于 correctness 与 section 绑定，不纳入“调用方/实例化编译工作”样本。

section证据要分别看调用方对象、全部对象和最终可执行文件。显式实例化可能减少调用方`.obj`里的`.text`，但链接后的`.exe`可能已由COMDAT合并而没有变化。多个短编译进程耗时相加时，clock tick和进程启动开销都会放大解释风险；结论最多写到“显式实例化改变了编译产物分布”，是否减少总体时间要由当前样本证明。

## 命令

有限正确性观察：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost -B C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local --config Release
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/B01_compile_cost/build/local -C Release --output-on-failure
```

正式成本实验应生成源码、trace、对象/符号/section摘要、预热记录、独立样本和环境记录，输出到本地未跟踪目录。运行前后按进程名、PID和CPU增量检查构建活动；完全发生在两次快照之间的短任务仍可能漏检，所以结果不能宣称排除了所有系统噪声。

## 代价

折叠查询的代价是表达式形式更集中，诊断有时不如分层 traits 容易插入自定义说明。显式实例化的代价是要维护一份实例化清单；支持域变化时，忘记更新单定义会造成链接错误或重新落回隐式实例化设计。小项目通常不值得为这些成本引入规则；当 trace 或构建时间已经指向重复实例化，再做局部显式实例化更稳。

## 扩展实验：手写 type map 与 Mp11 `mp_map_find`

正式结果要同时看中位数、范围和散布；若成对比较的范围重叠，不能证明普遍或稳定加速。单组“中位数较低”只能作为下一步定位线索，不是库实现一定更快的结论。

trace中的`Total EvaluateAsConstantExpr`等计数只能提供当前查找形状的定位线索，不能把粗粒度总计数当全部实例化数量，更不能用一次trace代替正式采样。高分辨率计时器不等于排除了OS噪声。

前两组实验只覆盖“是否包含某个类型”和“函数模板实体在哪里生成”。元编程库还常见另一类成本：把很多 `key -> value` 类型对组成 type map，然后按 key 查找 value。这个操作会出现在 schema、消息签名、字段标签、variant 分发和 concept 适配层里。

B01 的扩展 driver 用同一份契约比较两种实现：

```cpp
using source_map = boost::mp11::mp_list<
    boost::mp11::mp_list<key<0>, value<0>>,
    boost::mp11::mp_list<key<1>, value<1>>,
    /* ... */>;
```

```cpp
// 两份 TU 只替换这一行：manual_find 或 Boost.Mp11 查找。
using entry = typename manual_find<source_map, query_key>::type;
using entry = boost::mp11::mp_map_find<source_map, query_key>;
using answer = typename entry_value<entry>::type;
```

规模固定为 32、128、256 个键；查询目标固定为最后一项和缺失项，所以一共 12 组。manual 与 Mp11 源文件使用同一个 `source_map`、同一个 `entry_value` 输出适配、同一 Boost.Mp11 头依赖、同一 include path、同一编译选项；两组只选择不同查找算法。正例先编译、链接、运行，确认手写版本与 Mp11 版本答案相同，缺键结果是 `void`。然后单独生成 trace 定位前端工作；正式 timing 仍是一预热、五个独立进程样本，固定随机种子，且 trace 插桩不混入正式计时。

正式测量必须在独占窗口执行，输出到本地未跟踪目录，并把源码/产物哈希记录在计时外。

这组实验不能被简化成“库一定更快”或“手写一定更快”。Mp11 的 `mp_map_find` 使用继承、重载解析和 unevaluated context 做一次性查找形状；本实验让 manual/Mp11 使用同类型输入、同输出适配、同 Boost.Mp11 头解析成本，避免把数据形状或额外头文件解析误算成查找算法成本。手写递归版本短、直观，规模小时可能足够；当 map 查找成为公共基础设施，trace 和样本才决定是否值得迁到成熟库实现。
