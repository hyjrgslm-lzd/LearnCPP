# 09 新Ranges能力：拼接、缓存、降级与容量提示

先修是原有view、sentinel、borrowed、const与单遍/多遍迭代。这里不按年份堆API名，而是对照前面已经遇到的问题，说明新能力改变了哪条契约、留下什么责任。

规范取[标准索引](../references/standards.md)中的采纳记录；特性宏来自[固定草案快照](../references/validation/standard-snapshot.json)。默认`/std:c++latest`只选择语言模式，不能让缺失的库实现出现。F01每个源文件包含真实主体，能力不足返回77；主体通过则必须确实实例化、链接并运行检查。

## concat：把几个范围接成一个范围

`join`消费“元素本身又是range”的外层范围；`concat(a,b,c)`将若干直接给出的范围顺次拼接。类型可以不同，但必须满足产生一致value/reference/rvalue-reference所需的约束。不能因为两种元素都能打印，就认为它们能形成一个合格的concat iterator。

实现上，iterator需要知道当前处在哪个底层范围、什么时候跳过空范围、什么时候跨到下一段。其关联类型由所有参与范围共同决定，能力不能只看第一段。解引用是否仍为引用也必须看公共引用类型；若能够保持int&，修改它才会作用到原owner。

[concat.cpp](../exercises/F01_frontier/concat.cpp)连接空vector、非空vector和array，检查跨段结果与lvalue别名；另用临时vector验证适配器通过拥有型view保留数据。不要据此认为所有concat借用关系都相同：元素owner、各层view和复合iterator仍应分别画图。临时view销毁后一个iterator是否可用，不能只凭底层vector还活着判断。

复盘：把第一段换成单遍输入能否继续假定整体random access？不能。拼接iterator的操作必须对所有可能当前段合法，且跨段操作还需要对应边界能力。完整约束见[concat条款](https://eel.is/c++draft/range.concat)。

## cache_latest：缓存一个位置，不是缓存整条管道

样章中安全的transform/filter/transform链对有效行可能重复解析。`cache_latest`可缓存最新一次解引用的结果，让同一位置的后续解引用复用它；推进iterator后，缓存与位置的关系改变。它没有建立“任意历史位置都能再访问”的存储，所以不能因为底层vector能多遍访问，就假定缓存视图仍是forward_range。

[cache_latest.cpp](../exercises/F01_frontier/cache_latest.cpp)在三个整数的纯倍增计算旁插入观察计数：同一iterator解引用两次只计算一次；每次递增后再次解引用才新增一次计算。计数是诊断插桩，不作为正式计时方案，也不是用带任意可变业务副作用的函数绕过range语义。

这个缓存可能保存对象，也可能依据引用性质采用其他表示；对缓存内对象的引用不能越过缓存替换继续使用。它解决重复求值问题，同时带来存储、生命周期和能力变化。手写缓存必须先说明这些语义，不能只增加一个optional成员就宣称等价。详见[cache_latest条款](https://eel.is/c++draft/range.cache.latest)。

复盘：为何不能用缓存版与原版总耗时的差直接证明减少了内存分配？因为减少求值可能减少分配，但也可能增加缓存构造与移动；要单独计数或归因。缺少实际标准实现时，只保留上述假设和主体，不伪造缓存版时间。

## as_input：主动放弃多遍能力

早期名字`to_input`已于2026-03更名为`as_input`。它调整消费者可依赖的遍历能力，并不等于像`ranges::to`那样立即将元素转换到新容器。F01不自动用旧名替代新名，因为这会掩盖库版本不匹配。

[as_input.cpp](../exercises/F01_frontier/as_input.cpp)把span适配为input而非forward范围，验证元素仍按原序出现，并检查借用关系。底层数据仍在原数组内；适配器不负责延长数组存活。主动减弱能力意味着reverse、排序或依赖多遍的算法不能继续不加判断地使用。

为什么主动失去能力有用？某些适配器为多遍语义需要保存位置、维护缓存，或者限制修改后元素的行为；消费者只需单遍时，准确声明较弱能力可以避免作出不需要的承诺。这不是自动提速保证，必须分清接口适用性与运行成本。

## filter的const支线不再能用一句绝对判断概括

普通vector底层的filter是forward或更强范围，begin缓存与const限制仍要理解。新采纳的P3725补充另一条路：当const底层满足input_range但不满足forward_range，而且const谓词可用时，filter可以提供受约束的const begin/end。这里不需要维护forward begin的缓存。

因此“const对象可迭代”“元素是否可写”“iterator是否多遍”是三个维度。`as_const`调整元素访问；`as_input`调整能力；在某些新规范支线上它们会影响可用重载，但不是互换的万能修复。[const_input_filter.cpp](../exercises/F01_frontier/const_input_filter.cpp)逐项检测as_input前提与实际const filter能力，缺后者会明确报告具体SKIP。

复盘：为何给vector filter变量加const仍可能失败？因为它仍是forward底层，不符合新const输入支线；不能只看到P3725新增const成员就忽略requires。也不能把本机旧实现失败推广为规范永远禁止。

## reserve_hint：上界提示不等于元素个数

`sized_range`提供与实际范围匹配的size及相应语义；某些惰性范围只能便宜地给出容量估计或上界。`approximately_sized_range`与`ranges::reserve_hint`允许消费者获得提示，但它不能把提示值直接当作已经存在的元素数。

[reserve_hint.cpp](../exercises/F01_frontier/reserve_hint.cpp)构造一个实际三个元素、提示值八的范围，sentinel不提供距离，且没有size成员。它检查范围确实不是sized_range，再用真实ranges::to收束，结果必须只有三个元素。实际vector容量另外打印，不能要求每个库都生成某个固定capacity作为正确性门槛。

核心区别是reserve与resize：前者准备存储，后者影响已构造元素数量。提示也不是无限可信的资源授权；泛型消费者仍受容器max_size、分配异常和自身资源约束限制。小型教学实验有固定工作量，不代表服务端可以按外部声明无上限分配。

## optional作为0/1范围的桥接

C03主讲optional的状态与对象模型；此处只承接其range协议。[optional_range.cpp](../exercises/F01_frontier/optional_range.cpp)检查空optional不产出元素，有值时通过transform与to产生一个结果。按值optional自己持有T，右值optional传给返回iterator的算法仍需考虑dangling，不能因为它是view就自动认定borrowed。

这条桥接让“可能不存在一项”能接入范围组合，但不会消除错误通道设计：空值究竟是合法缺席还是解析失败，仍由业务和C03/C05的契约决定。

## C++29：边界检查与不插入的查找

N5055已将两项直接影响本课的能力纳入N5054工作草案。它们是已入稿内容，但不是本机已实现内容；[C++29宏快照](../references/validation/standard-snapshot-cpp29.json)与真实主体分别记录这两条状态轴。

`view_interface::at`要求random-access并且sized：前者提供按偏移访问，后者提供可检查的边界。负数和达到size的下标会抛`out_of_range`；有random access却没有size的范围不能自动获得这个成员。[view_interface_at.cpp](../exercises/F01_frontier/view_interface_at.cpp)检查合法下标、两个边界和const view仍可借用可变元素。filter降级后不满足相同访问条件，也在类型检查中明确拒绝。

关联容器新增的当前名称是`lookup`，不是早期提案里的`get`。它查到时返回指向mapped对象的optional引用，未查到时为空，而不会像`operator[]`那样插入默认值。const容器返回const mapped对象的引用；比较器或hash等操作仍可能抛异常，不应擅自声明整个lookup为noexcept。[map_lookup.cpp](../exercises/F01_frontier/map_lookup.cpp)对map、unordered_map、flat_map分别验证命中、缺失不插入、引用修改和const返回类型。

这两项能力延续了前面的契约分类：at选择异常报告非法位置；lookup把合法的查找缺失表示为可检查状态。它们都不自动解决引用失效；flat_map插入导致的重分配仍然会影响之前取得的借用。C++29 constexpr hive等尚未在所核对报告中采纳的提案只登记跟踪，不凭目标年份补造运行通过记录。

## 实验步骤与完整解析

1. 先运行F01普通flat容器观察，再显式打开RANGES_ENABLE_FRONTIER。选项关闭与能力不足是两种状态；前者没有尝试主体，后者有具体缺失记录。
2. 支持concat时检验空段和引用；支持cache_latest时检验重复解引用及推进计数；支持as_input时用concept解释为何不能reverse。这些检查各证明一条行为，不能合并成一句“Ranges全支持”。
3. 对const filter和reserve_hint先写出预期类型约束，再运行主体。解析分别是input而非forward的const重载前提，以及提示值与实际元素数分离。
4. 查看SKIP记录。解析应写清缺的是头、宏/版本、const重载，还是仅关闭了选项。构建已进入真实主体后出现编译、链接或运行错误属于FAIL，不能把异常包起来当SKIP。

本机初次作者验证为1个普通观察通过、8个前沿能力跳过，随后追加两项C++29主体；扩展后的结果另存r2及全课最终记录。跳过主体在该环境没有被实例化和运行；教材与静态审查不能替代这些实测。完整上游环境路线和命令见[F01说明](../exercises/F01_frontier/README.md)，不要求安装新编译器来掩盖限制。
