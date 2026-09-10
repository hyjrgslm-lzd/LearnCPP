> 对应章节：模块 H — 高级实现模式 / H-3 generator promise_type + `basic_const_iterator`

## 目标

实现一个教学版 `my_generator<T>` 和一个最小 `my_basic_const_iterator<I>`，重点不是完整复刻标准库，而是把两个高风险实现契约讲清楚：

- `std::coroutine_handle<>` 本身只是可复制的句柄值，不代表所有权；generator 对象必须把它当作唯一所有资源管理。
- `basic_const_iterator` 不能把所有解引用结果都强转成 `const value_type&`；本题采用标准 `iter_const_reference_t` 核心公式，按 value/reference 共同只读类型决定返回形态。教学 wrapper 不实现 `std::basic_const_iterator` 的全部成员，但它承诺的值/引用/能力边界必须完整一致。

本题验证四条路径：

- `src/student/generator_const_iter.hpp`：学生可编辑入口。
- `src/reference/generator_const_iter.hpp`：标准答案。
- `validation/good/generator_const_iter.hpp`：独立正确实现。
- `validation/bad/generator_const_iter.hpp`：安全但真实错误的实现，移动赋值泄漏旧协程帧，必须被 checker 拒绝。

## 必做任务

1. 实现 move-only generator。
   - 拷贝构造和拷贝赋值删除。
   - 移动构造用 `std::exchange` 取走源对象句柄。
   - 移动赋值先 `destroy()` 目标已有帧，再接管源对象句柄。
   - 自移动赋值不破坏当前对象。
   - 析构时销毁仍被持有的协程帧。

2. 实现惰性 `begin()`。
   - `initial_suspend()` 返回 `std::suspend_always`，构造 generator 时协程体不运行。
   - 第一次 `begin()` resume 到首个 `co_yield` 或结束。
   - 空 generator 的 `begin()` 直接等于 sentinel。
   - 首次 resume 或后续 `operator++` 中记录的异常必须重新抛给调用方。

3. 实现安全 `yield_value`。
   - 教学实现把当前 yield 值保存在 promise 内部的 `Val current_value_` 中。
   - 这样 `co_yield prvalue` 不会因为存临时地址而悬垂。
   - 本实现牺牲一部分泛化能力，换取清晰且可验证的生命周期契约。

4. 实现 `my_basic_const_iterator<I>`。
   - 对底层返回 lvalue reference 的 iterator，解引用结果应变成对 const 元素的引用。
   - 解引用结果使用 `std::common_reference_t<const std::iter_value_t<I>&&, std::iter_reference_t<I>>`。
   - 典型结果：`vector<int>` 为 `const int&`，`vector<bool>` 为不可写 `bool`，`move_iterator<vector<int>::iterator>` 为 `const int&&`。
   - 只有当底层 iterator 真有对应能力时才声明 `--`、`+`、`-`、`[]`、`<=>` 等操作。
   - `iterator_concept` 继承底层能力；不能只写 type alias 伪装成 random access。

## 验收点

- generator 析构、早毁、移动构造、移动赋值、自移动都有受控 `destroy` 计数。
- move assignment 会释放目标旧帧。
- 空 generator 正确结束。
- `co_yield` 后抛出的异常会在迭代推进时传播。
- `my_basic_const_iterator<vector<int>::iterator>` 满足 `std::random_access_iterator`，且解引用为 `const int&`。
- `my_basic_const_iterator<vector<bool>::iterator>` 解引用为 `bool`，不保留可写 proxy。
- `my_basic_const_iterator<move_iterator<vector<int>::iterator>>` 解引用为 `const int&&`，保留 const rvalue 类别。
- 包装 transform prvalue iterator 时，解引用结果是 `int`，不会返回悬垂 `const int&`。
- 包装 proxy iterator 时，不能保留可写 proxy；以 `vector<bool>` 为例，结果应收敛为不可写 `bool`。
- `validation/bad` 的移动赋值错误被行为 checker 拒绝。

## 常见坑

- 默认移动 `std::coroutine_handle<>`，导致两个 generator 同时持有同一帧，最终双销毁。
- 移动赋值直接覆盖句柄，忘记销毁目标旧帧。
- `yield_value` 存 `std::addressof(val)`，遇到 prvalue `co_yield` 时留下悬垂指针。
- `begin()` 不做首次 resume，导致第一次解引用读取未初始化值。
- `basic_const_iterator::operator*` 固定返回 `const value_type&`，会错误保留或制造引用类别：`vector<bool>` 的可写 proxy 不能泄出，`move_iterator<vector<int>::iterator>` 不能被降成 `const int&`。
- 声明 random access 类型别名，但没有完整随机访问运算，concept 不满足。

## 与 C09 的边界

本题只讲 generator 接入 range 协议所需的协程骨架：promise、first resume、yield 存储、异常传播、句柄所有权。协程调度、对称转移、跨线程恢复、任务组合等更完整主题放到 C09。本题不能删除协程语言深度，但不展开 C09 的并发模型。

## 对应参考

- `references/implementation-spec.md`
- `references/source-reading.md`
- P2502R2：`std::generator`
- P2278R4：`basic_const_iterator`
