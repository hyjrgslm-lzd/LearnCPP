# 笔记 03：CPO 使用清单

## 八个 ranges CPO

| CPO | 定制意图 | 源码阅读入口 |
|---|---|---|
| `std::ranges::begin` | 统一数组、成员 `begin` 和 ADL begin；外部算法不直接拼查找规则 | `<ranges>` / `<xutility>` 中 begin CPO |
| `std::ranges::end` | 支持 iterator/sentinel 异型，算法只要求 sentinel 可比较 | end CPO 与 `sentinel_for` 检查 |
| `std::ranges::iter_move` | proxy iterator 能定义移动底层元素，而不是移动代理对象 | `transform_view`、zip 类 view 的 hidden friend |
| `std::ranges::iter_swap` | proxy iterator 能逐字段交换底层元素 | zip 类 view 的 hidden friend |
| `std::ranges::advance` | 根据 iterator 能力选择 `++`、`+=` 或 sentinel 限界推进 | iterator CPO 区域 |
| `std::ranges::distance` | common/sized sentinel 可走 O(1)，否则线性扫描 | distance CPO 与 sized sentinel 分支 |
| `std::ranges::size` | 统一成员 size、ADL size、数组大小和 sized sentinel 差值 | size CPO 与 `sized_range` |
| `std::ranges::data` | 只在 contiguous 语义成立时暴露原始地址 | data CPO 与 contiguous range 检查 |

## ranges CPO vs stdexec tag_invoke

| 维度 | ranges CPO | stdexec tag_invoke |
|---|---|---|
| 操作模型 | 拉模型：算法主动问 range 的 begin/end/size | 推模型：sender 组合后由 receiver 接收完成信号 |
| 定制入口 | 每个 CPO 固定自己的成员/ADL/回退顺序 | tag 对象携带语义，`tag_invoke(tag, args...)` 集中分派 |
| 状态位置 | view 持有 callable/cache，iterator 指回 parent | operation state 持有连接后的 receiver、scheduler 和中间状态 |
| 惰性点 | 构造 view 通常不遍历，begin/++ 才拉取元素 | connect/start 前不执行，start 后推送 value/error/stopped |
| 类型安全 | concept 检查 range、iterator、sentinel、reference | completion signatures 检查 value/error/stopped 通道 |

`filter_view` 的 `begin` dispatch 先走成员函数。forward 底层需要写 begin cache，因此普通 vector/filter 路径没有 const
`begin`。N5047/P3725R3 新增的是 input-only 且 const 谓词可用的支线，它不使用同一个 forward cache 语义。
