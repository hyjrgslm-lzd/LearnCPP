# 14：输入校验、断言、契约与库 hardening

接口边界有多种检查机制，不能互相替代。输入校验处理不可信外部数据，失败结果是业务语义的一部分；断言表达程序员假设，通常用于开发期发现内部错误；contracts 把函数前置条件、后置条件和断言写进接口语法；library hardening 是标准库实现对自身前置条件的运行时检查策略。它们都能“发现错误”，但责任、可配置性、诊断含义和部署边界不同。

输入校验必须在信任边界存在。读取配置、解析网络包、处理用户输入时，调用者本来就可能传错；接口应返回 `std::expected`、错误码、异常或拒绝结果。把这类检查写成 `assert(x)` 会在 Release 消失，也会把外部错误伪装成程序员 bug。L14 的 `parse_port()` 使用 `std::expected<int, std::string>`，因为端口文本无效是可预期输入，不是前置条件违反。

断言适合内部不变量。比如一个解析器已经确认下标范围，后续内部函数可以断言“这里必须非空”。断言失败说明程序自身逻辑错了。C++ 的传统 `assert` 受 `NDEBUG` 控制；课程检查不能依赖 Release 中的 `assert` 执行。因此练习里用显式 `check()` 验证模型行为，只把 assert 当概念解释。

C++26 contracts 来自 P2900R14，进入 C++26 工作草案。语言层提供 `pre`、`post` 和 `contract_assert`；checking mode 与 violation handling 决定违约时观察、强制还是快速强制等行为。contracts 不是异常规范，也不是输入校验替代品。一个函数的 `pre(x > 0)` 表示调用者必须满足条件；对不可信输入，正确设计仍是在进入有前置条件函数前先校验并拒绝。contracts 的实现支持还在推进，本机 MSVC STL/编译器当前未提供 `__cpp_contracts`，所以 C03 主线只能讲模型和边界，合法语法调用路径由 F01 在实现支持时单独验证。

C++29 virtual contracts 来自 P3097R3，并由 N5055 记录纳入 C++29 工作草案。先分清版本：C++26 contracts MVP 不含 virtual function 的 `pre/post`，P3097R3 是 C++29 增量。它解决虚函数、override 与契约组合的问题：基类和派生类都可能声明条件，动态派发时必须说明哪些条件参与检查。教学上要强调这不是简单“派生类更严格就安全”。如果基类契约允许更宽输入，而派生 override 要求更窄输入，基类引用调用时就可能破坏替换原则。C++29 的规则属于后续工作草案增量，不能写成 C++26 主线能力。

P3097R3 的核心模型是 caller-facing assertions 与 callee-facing assertions。caller-facing 指调用表达式静态选中的函数；callee-facing 指动态派发最终实际调用的 final overrider。普通非虚直接调用时，两者通常是同一个函数；函数指针或成员函数指针调用时，P3097R3 还要区分调用表达式实际命名的目标；虚调用时二者最容易不同。固定顺序是：先检查 caller-facing precondition，再检查 callee-facing precondition，再执行函数体，再检查 callee-facing postcondition，最后检查 caller-facing postcondition。这个顺序解释了诊断归属：调用者是否满足它静态看到的接口，和最终派生实现是否能服务这个调用，是两件事。

看一个最小层次：`Base::f(int x) pre(x > 0) post(r: r > 0)`，`Derived::f(int x) override pre(x < 100) post(r: r < 1000)`。若有 `Derived d; Base& b = d; b.f(150);`，调用表达式静态选中 `Base::f`，final overrider 是 `Derived::f`。caller-facing pre `x > 0` 通过，因为调用者按 `Base` 接口看是合法输入；callee-facing pre `x < 100` 失败，说明派生实现不能服务基类接口允许的调用，这暴露替换风险。若直接 `Derived& dref = d; dref.f(150);`，静态选中和最终调用都指向 `Derived::f`，失败归属就是调用者没有满足 `Derived` 接口。这个例子不用触发 violation 也能推导出为什么“派生类收窄 precondition”危险。

同一函数的重复检查也要谨慎表述。P3097R3 的意图是只评估与当前调用直接相关的断言：静态选中函数与 final overrider。若二者相同，不应把同一组断言讲成无意义重复两遍；若二者不同，则 caller-facing 与 callee-facing 都有意义。显式继承或复用基类断言还涉及提案中的 `pre(Base::f)` 等形式，是否重复、如何查找 `Base::f`、`this` 的类型语境，都必须按固定提案核对，不能靠直觉写成“自动继承所有基类契约”。

library hardening 和 language contracts 分开。P3471R4 讨论把库前置条件检查纳入 hardening 语境；MSVC STL 还提供 `_MSVC_STL_HARDENING` 这类实现控制。本支线只记录本机宏状态，不通过越界访问、空函数调用或违反容器前置条件来制造观察结果。安全课程不能把未定义行为或强前置条件违反当默认测试。

pattern matching 也只作为提案跟踪项进入本章。它会影响未来 variant/state-space 的写法，但当前没有标准 `std` 支持宏，C03 不能造一个同名兼容层冒充标准能力。若后续纳入工作草案，再把语法、匹配穷尽性和错误处理模型桥接回 C03/C06。

练习 L14 是观察型。Part 1 对不可信端口文本做输入校验，返回错误通道。Part 2 把已校验的端口交给有前置条件的内部函数，展示“先校验，再进入契约模型”。Part 3 打印 `_MSVC_STL_HARDENING`、`__cpp_contracts`、`__cpp_lib_contracts`，说明本机只观察宏，不触发 violation。Part 4 记录 pattern matching 的提案状态，不声明本机支持。

完整解析：外部输入错误属于接口正常失败路径；内部断言失败属于程序 bug；contract violation 属于调用者破坏声明条件；library hardening 属于库实现对标准库前置条件的检查策略。把它们混用会导致两个坏后果：一是 Release 环境失去必须存在的校验；二是把未实现的未来语法当成当前可运行保证。C03 的核心安全模型可用 C++23 证明，但它不能替代 contracts 实现支持。
