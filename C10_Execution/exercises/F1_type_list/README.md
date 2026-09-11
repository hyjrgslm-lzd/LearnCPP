# F1 type_list 类型集合

实现 C10 后续签名变换需要的最小类型列表工具。

Part:

1. `type_list<Ts...>`。
2. `concat_t<A, B>` 连接类型序列。
3. `unique_t<List>` 保留首次出现顺序去重。
4. `transform_t<List, Meta>` 对每个类型应用元函数。
5. `filter_t<List, Pred>` 保留满足谓词的类型。
6. 用 `concat + unique` 模拟 sender value pack 合并。

Student 初态提供合法别名占位，但 checker 会在运行时以 `concat joins lists` 拒绝第一个错误类型结果；这是可追踪的真实类型检查失败，预期 exit 1，不是任意非零或完成标记。
