# A04：类型管线

先读 `../../chapters/21-type-pipelines.md`。只编辑 `src/student/type_pipelines.hpp`。

实现 `zip_t`、`flatten_t`、`cartesian_product_t`、`variant_product_t`、`lazy_type_t` 和 `transform_completion_signatures_t`。无效输入通过 concept 查询为 false；只有直接请求错误别名时才诊断失败。

完成签名规则：`value_sig<Ts...>` 调用 F，返回 `void` 时为 `value_sig<>`；可能抛出时额外加入 `error_sig<std::exception_ptr>`；原有 `error_sig<E>` 和 `stopped_sig` 保留；重复签名用 `unique` 合并。

本题是 C10 sender 的类型层预习，不实现 scheduler、environment 或 operation state。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `A04_type_pipelines_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/type_pipelines.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
