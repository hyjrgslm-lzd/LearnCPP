# P1 pipeline

目标：综合使用真实文件 completion sender、两个 scheduler、三条并行统计分支和 `when_all` 合流，处理小型 `name,category,value` 记录文件。

Part 1：用 `c10_native::io_context` / `async_read_at` 后端读取真实文件；`run_pipeline` 自己负责把 completion 接入流水线。文件和 context 必须在 scope 收束后再销毁。

Part 2：解析记录。非法行局部恢复，计入 `invalid`，不得让整条流水线静默成功或崩溃。

Part 3：把 value 求和、派生值求和、category 计数放进独立分支；至少两个 scheduler，最后 `when_all` 合流生成 report。

Part 4：保留阶段计数。计数只用于解释传输/计算/排空边界，不声明性能收益。

独立位置：只改 `src/student/solution.hpp`。

## 参考实现的因果链

第一张图从自定义file_text_sender开始，系统完成后切换到parse scheduler，解析记录并进行局部错误恢复；它实际消费原生完成，不是先同步读文件再伪装sender。取得拥有的records后，第二张图在compute scheduler上创建三条统计分支，以when_all保持结果位置，scope收束全部已接受工作。拆成两张图使阶段寿命明确，也避开当前GCC13+ASan在更深嵌套图中的模板编译问题。

file、buffer、context及两个pool都由run_pipeline的局部拥有者持有；scope.on_empty完成后才退出资源作用域。基础设施、容量和pipeline_stopped错误继续传播，不能被普通解析恢复吞掉。停止重载从receiver environment的get_stop_token读取真实token，env_queries计数来自查询路径，不允许普通lambda直接增加展示数字。

输入限制为1 MiB、4096条记录、value绝对值不超过1000000。checker使用不同文件、非法行、空输入、预请求停止及容量边界，按原数据独立求和和计数。Good采用不同解析/组合组织；Bad违反输入驱动结果，不能靠输出固定report通过。

## 怎样提交与复盘

按[构建指南](../BUILD_GUIDE.md)独立配置P1目录；Linux需要明确启用固定io_uring前缀。Student初态exit2，Reference/Good为0，Bad按题目诊断退出1。正文[记录流水线](../../chapters/13-pipeline.md)提供每个Part的状态/所有权分析。画出两个图之间的拥有值交接，解释为何磁盘完成不等于CPU统计完成，再说明每个计数实际观察了什么。性能判断另用B01，不能从这里的阶段计数宣称加速。
