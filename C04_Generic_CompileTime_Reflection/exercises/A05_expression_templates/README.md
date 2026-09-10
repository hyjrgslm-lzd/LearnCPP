# A05：表达式模板

先读 `../../chapters/22-expression-templates.md`。只编辑 `src/student/expression_templates.hpp`。

实现固定大小 `vec<N>`、向量加法、标量乘、`reverse`、`eval` 和 `assign`。左值子表达式借用，右值子表达式按值保存；`eval` 返回拥有型 `vec<N>`；`assign` 先物化临时，保证 `assign(v, reverse(v))` 正确。

本题只处理 double 固定向量，不引入 Eigen。观察程序只比较 eager 数值参考与直接写回反例，不给速度结论。

异常规格保持普通函数默认值：本单元不承诺公开包装为 `noexcept`，也不捕获或吞掉异常；不能从“当前内置 double 算术不会抛”推断接口已有不抛保证。条件 `noexcept` 的接口推导由11/20章主讲。本题检查的赋值提交点是先完成 `eval` 再写目标。索引访问的前置条件为 `i < N`；`N == 0` 时允许构造、反转和求值，求值不访问任何元素。
