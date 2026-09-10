# A04/A05 类型管线与表达式模板作者自查交接

范围：`chapters/21-type-pipelines.md`、`chapters/22-expression-templates.md`、`exercises/A04_type_pipelines/`、`exercises/A05_expression_templates/`、`references/validation/revision-20260910/pipelines-*`。未修改 A02、根 CMake、根导航或共享 helper。

## A04 契约核对

- 正文在 L08 基础上继续展开 `zip`、递归 `flatten`、`cartesian_product`、lazy provider、variant 组合和 toy completion signatures，不只列术语。
- `zip` 明确等长约束：合法输入保持 pair 顺序；长度不等通过 `zipable` 查询为 false，强行取 `zip_t` 才产生诊断。
- `flatten` 只接受根 `type_list`；内部 `type_list` 递归展开，普通类型保留；`flattenable<int>` 为 false。
- `cartesian_product_t<>` 使用单位元 `type_list<type_list<>>`；任一输入列表为空时结果为空；顺序为 left-major。
- `lazy_type_t` 只实例化选中分支，配套 eager 反例验证未选分支不应访问缺失 `::type`。
- `variant_product_t` 先把 `std::variant` 展开成 `type_list`，再复用笛卡尔积，保留组合顺序。
- `transform_completion_signatures` 保留原有 `error_sig<E>` 和 `stopped_sig`；对可抛值通道加入 `error_sig<std::exception_ptr>`，对 `noexcept void` 生成 `value_sig<>` 且不加异常通道。
- `transformable` 使用单步合法性折叠，避免 alias 在 concept 查询中触发 MSVC 硬错误；不可调用值通道为 false boundary。

## A05 契约核对

- 正文从 eager 数值基线进入表达式节点，展开 `store_t` 左值借用/右值拥有、`operator[]` 递归求值、大小约束、`eval` 物化和 `assign` 先物化再写回。
- 练习只支持固定大小 `double` 向量、加法、标量乘和 `reverse`；没有引入 Eigen 依赖，不承诺性能收益。
- `validation/good` 与 `src/reference` 都按左值借用、右值按值保存；嵌套临时在表达式对象内被拥有。
- `eval` 返回拥有型 `vec<N>`；checker 修改原输入后确认已物化结果不变。
- `assign(out, reverse(out))` 先 `eval` 再整体赋值，覆盖 reverse/重叠别名；`validation/bad` 故意直接写回并被 checker 拒绝。
- 诊断用例覆盖 `vec<2> + vec<3>` 大小不匹配，pattern 采用 MSVC 稳定错误码 `C2676/C2672`，避免本机中文文本差异。

## 独立性

- A04 `validation/good/type_pipelines.hpp` 先于 Reference 落盘，初始 SHA256 为 `779fdea77a6acf9952fc3e3e1d6a6cb66161b8500c2b5758a91b5546379a379`；后续为 MSVC false-boundary 和完成签名 concept 修正，最终 SHA256 为 `54e89ccf68cedadeda53d5882635b8a92737a200d6f70b61d142b0a0dfe3b712`。
- A05 `validation/good/expression_templates.hpp` 先于 Reference 落盘，初始与最终 SHA256 均为 `82dc1d4eba80a50c1cb7f3fc292edb63e8bde2fcf8ed31db9b2f34f8c15744af`。
- `22-good-no-reference-include.json` 记录 `rg` 反向检查：两个 good 目录没有引用 `src/reference`、`validation/good` 以外答案路径或 `reference/good` include。

## 已验证

见 `references/validation/revision-20260910/pipelines-author-evidence.md` 与 `references/validation/revision-20260910/pipelines-author-records/`。

## 交接状态

作者自查：PASS。A04/A05 已具备正文推导、Student 可构建初态失败、Reference/good/bad、diagnostic 正反 control、Debug/Release 与 A05 ASan 原始 JSON 证据。保留两条失败原始记录：`01-good-hashes.json` 是 recorder 子进程内 `Get-FileHash` 不可用，已由 `01b-good-hashes.json` 替代；`08-a04-student-build.json` 是修正前 Student 不能构建，已由 `08b-a04-student-build.json` 与 `09b-a04-student-ctest-expected-fail.json` 替代。
