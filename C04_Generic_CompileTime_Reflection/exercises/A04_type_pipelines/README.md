# A04：类型管线

先读 `../../chapters/21-type-pipelines.md`。只编辑 `src/student/type_pipelines.hpp`。

实现 `zip_t`、`flatten_t`、`cartesian_product_t`、`variant_product_t`、`lazy_type_t` 和 `transform_completion_signatures_t`。无效输入通过 concept 查询为 false；只有直接请求错误别名时才诊断失败。

完成签名规则：`value_sig<Ts...>` 调用 F，返回 `void` 时为 `value_sig<>`；可能抛出时额外加入 `error_sig<std::exception_ptr>`；原有 `error_sig<E>` 和 `stopped_sig` 保留；重复签名用 `unique` 合并。

本题是 C10 sender 的类型层预习，不实现 scheduler、environment 或 operation state。
