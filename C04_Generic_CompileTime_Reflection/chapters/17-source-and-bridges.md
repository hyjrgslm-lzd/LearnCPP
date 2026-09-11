# 17. 从教学接口回到真实源码与下游应用

本章的目标是让读者带着问题读实现：哪个表达式被选择，哪一步检查了它，返回值与异常规格如何对应，失败时在哪一层退出。仅打开一个上游仓库首页不算源码导读。

本轮固定的本机STL输入为MSVC工具集14.51.36231、STL更新202604。这些是安装头文件的真实版本，不冒称某个上游Git提交；后续机器行号变化时，先匹配版本/哈希和符号，再跟随调用。

## `std::invoke`：分派不是凭空发生

从`type_traits`中的`invoke`公开模板进入。零参数callable的路径直接形成`f()`；有第一个参数时，先由`_Invoker1`根据callable是否是成员函数指针、成员对象指针，以及第一个参数是否为对象/派生对象、reference_wrapper或可解引用对象，选择相应策略。

本机真实入口在`type_traits`约1776行的`_Invoker1`、1799行起的`invoke`。读代码时先圈出模板形参，再给一个具体表达式，例如`invoke(&Person::id, person)`。callable是成员对象指针，首参是所属类对象，所以进入成员对象访问分支。若改为`reference_wrapper<Person>`，会先get；若是可解引用对象，则先解引用。这是不同表达式，而非统一函数指针调用。

随后对照三处：候选的返回类型、noexcept中的探测表达式、函数体真正执行的表达式。该安装实现以`_Call`等辅助类型形成前两者，并在函数体选择策略。模板替换失败不应被误读成已经进入函数体后执行了一个失败操作。

教学`read_value`只约定member/ADL两条路。生产invoke处理的是标准INVOKE表达式族，不能把它替换为`f(args...)`后宣称等价。本课[转发练习](../exercises/L03_forwarding/README.md)与[字段项目](../exercises/P1_static_record/README.md)分别检查callable转发和成员指针表达式，后者不靠字节偏移模拟invoke。

**阅读自测。** 为什么`invoke(&Person::id, const_person)`应产生const访问？因为最后仍是对具体const对象的成员表达式。为什么成员函数抛出时invoke可能抛？因为异常规格来自实际表达式，invoke没有替调用者捕获一切。为什么不能从源码内部类名推出跨实现ABI保证？这些是厂商内部组织，不是用户可依赖的标准接口。

## `ranges::begin`：借用限制与值类别来自具体协议

进入`xutility`约2721行的`_Begin` namespace。它先放置一个普通查找屏障，再分别检测member和ADL路径。`_Choose`有Array、Member、Non_member、None四种策略；最后公开`ranges::begin`对象调用被选分支。None通过约束排除，不提供无条件的“返回空迭代器”fallback。

这一实现还有教学read_value没有承诺的语义。入口先检查`_Should_range_access`：左值range或允许borrowed_range的右值才进入。内部对具名range按左值进行访问，是range访问协议的选择，不能看到没有forward就断言它是bug。返回类型与`input_or_output_iterator`以及结果形成条件相连，不等于CPO只要找到一个叫begin的成员就成功。

读者可先对数组、带合法begin的类、仅ADL begin的类和无路径类型各画一条候选路径，再说明拥有型临时range为何会在入口被拒绝。[L11样章](11-customization-points.md)教普通机制，[C06](../../C06_Ranges/README.md)负责iterator、sentinel、borrowed_range与具体view协议；两个课程的接口契约不能混写。

## 下游反向核对

| 实际入口 | 先手算或解释什么 | C04主讲与可验证入口 |
|---|---|---|
| [C06 G1 my_take_view](../../C06_Ranges/exercises/G1_my_take_view/README.md) | V怎样推导、引用和const如何影响begin、约束何时存在 | 02—06；L02/L03/L04/L06 |
| [C06 H2 proxy iterator](../../C06_Ranges/exercises/H2_common_iter_proxy/README.md) | `*it`的表达式类别与关联类型为何不能只看value_type；语法概念和多遍历性质有何差别 | 02、06、09；L02/L06/L09 |
| [C10 G1 my_then](../../C10_Execution/exercises/G1_my_then/README.md) | value签名怎样变换，void怎样表示，错误/停止如何保留，调用的异常规格意味着什么 | 03、06、08、11；L03/L08/L11 |
| [C09 awaitable变换](../../C09_Coroutines/07-模块E-awaitable三层与co_await变换.md) | await_transform之后的成员/ADL operator co_await候选，以及表达式和临时对象的生命期 | 03—05；L03/L04/L05，生命周期回C02/C09 |
| [C14 H6 cuTe布局](../../C14_GPU/exercises/H6_cute_layout_and_tensor/README.md) | 哪些布局信息属于类型/NTTP，哪些是运行时值；修改实例化参数为何可能改变编译工作 | 07、08、10、12；L07/L08/L10/B01 |
| [C15 UObject与反射](../../C15_Unreal_Engine/05-模块D-UObject-反射-GC.md) | 语言元信息、UHT生成物、运行时注册和GC追踪分别由谁负责 | 13—16；F01/P1，UE协议仍由C15主讲 |

C10的具体库源码入口和版本由其课程负责。本次不会通过滚动FetchContent main下载一个新版本，然后把旧课描述自动算作已匹配。这里反查的是当前repo中的任务与表达式，不给未运行的sender程序加通过记录。

## 完成签名不是只换一个类型名

设函数对象将int变成string，类型层面可把`value_sig<int>`映射为`value_sig<string>`。若返回void，表示“成功但无值参数”的是`value_sig<>`，不是`value_sig<void>`。原来的error/stopped签名不能因为没有参与函数调用而消失。

如果真实adaptor捕获函数调用异常并向下游发送exception_ptr，还需要把这个可能性体现在完成签名中。推导依据是同一实际调用表达式的`is_nothrow_invocable`，概念示意为：

```cpp
using added_errors = std::conditional_t<
    std::is_nothrow_invocable_v<F, Ts...>,
    type_list<>, type_list<error_sig<std::exception_ptr>>>;
```

先证明F对Ts...可调用，才请求invoke_result；再组合值变换、原通道及新增异常通道并去重。真正调用时的F是左值、右值还是const对象，也必须与类型推导一致。L08的教学签名模型只负责其明示的静态契约；运行时捕获、环境查询和完成发送仍须按C10具体协议检查，不能仅凭type_list相等宣称adaptor正确。

## 遮住答案的迁移练习

1. 先读C06 G1要求，不打开其Reference。写出一个view实例中V、iterator、sentinel与`*it`表达式的类型；无法解释的位置回到主讲。解析重点是按表达式逐项推导，不能以“返回iterator”替代具体类型与约束。
2. 给L08的类型模型增加一个返回void的callable，手算输出，再给真实sender场景列出需要补充的异常与环境责任。解析重点是区分无值成功、错误和停止，以及静态集合与实际完成行为。
3. 对P1的Person列出手工metadata与真实反射分别提供的事实。解析：手工schema完整性来自注册契约，反射字段集合来自语言查询；外部名称、数据兼容和运行期对象仍需其他规则。
4. 读本机ranges::_Begin，指出一处与教学read_value不同但正确的分支。解析：array策略、borrowed_range入口以及对具名range的左值访问均源于不同协议，不能机械套“所有对象都forward”的结论。

审查者应记录自己实际完成了哪些推导、哪些运行和哪些部分未测。链接/锚点检查只能证明入口存在，不能替代这组迁移任务及独立教学审查。
