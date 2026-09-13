# L02：借用边界与稳定身份

正文：[02](../../chapters/02-views-and-object-graphs.md)。本单元是完整观察程序，没有伪装成实现题的完成标记。

| Part | 操作 | 解析与有效检查 |
|---|---|---|
| 1 表偏移 | 先预测空字段、二进制值、递减偏移和截断的结果，再运行 main.cpp | 首尾与单调性共同约束每个切片；程序逐个调用真实 decoder |
| 2 拥有与借用 | 说明哪个对象拥有 payload，核对地址，再复制第一字段 | 地址相同证明合法期内的借用；复制后的 string 独立；不执行悬空访问 |
| 3 图身份 | 推导 10→20→10 和共享节点的遍历结果，增加一个悬空 ID | 两遍校验先建立身份，再发布引用；visited 让每个节点最多入队一次 |
| 4 预算 | 将字段数或图边数推进到上限，解释为何应在分配/展开前拒绝 | 按 wire.hpp 的具体限额设计输入，结果必须来自实际调用 |

Reference 是 main.cpp 中的完整实验与 wire.hpp 实现。程序成功仅覆盖已有自动检查；预测、预算扩展与解释需单独完成。逐 Part 的因果解析见正文，不要求读取外部文章才能开始。

```powershell
cmake -S C12_Serialization_Databases_Storage/exercises/L02_views_graphs -B build/c12-l02
cmake --build build/c12-l02 --config Release
ctest --test-dir build/c12-l02 -C Release --output-on-failure
```
