# A3 fan-out and merge

本题把一个 `user_id` 扇出到三个 sender 分支：profile、quota、flags。每个分支只生产自己的值；`when_all` 是合流点；最终 `then` 才组装 `UserDashboard`。

## Part 1: 构图不启动

`observe_before_start` 只构造图，不消费图。事件列表必须为空。若构图时就 fetch，说明把 sender 写成了立即执行函数。

## Part 2: 三条独立分支

每条分支从 `just(user_id)` 起步并产出一种值。分支之间不共享可变 dashboard，也不互相读中间结果。

## Part 3: `when_all` 合流

`when_all` 收集三种 value completion。它表达“这三条都完成后，把三个值交给下一阶段”，不是“自动创建三条线程”。真正开始仍发生在最终消费时。

## Part 4: 解析

新增第四个分支时，改的是图形状和合流 lambda 的参数，不该把字段偷偷塞进某个共享对象。后续错误、取消、scope 都依赖这个清楚边界。
## IDE 工程入口

VS solution 中本题主入口是 `A3_fan_out_merge_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`A3_fan_out_merge` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
