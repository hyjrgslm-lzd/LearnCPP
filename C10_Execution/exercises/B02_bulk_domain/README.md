# B02_bulk_domain: bulk granularity and domain lowering

This is an observation unit. It shows two real paths that have the same output:

- the default path uses `just(7) | then(square)`;
- the custom path returns a sender whose completion domain lowers `set_value`
  through `transform_sender` into `just(49)`.

The point is the protocol boundary: domain lowering happens during connection,
before the operation state starts. The `bulk` check is separate and records
logical item count. With `seq` the logical work runs inline; a scheduler or
implementation may later map the same logical shape to different execution
locations.
## IDE 工程入口

VS solution 中本题主入口是 `B02_bulk_domain`。本单元是观察/阅读入口，没有伪造 Student 占位；源码和 README 显示在同一项目中，额外依赖或构建辅助目标收在 Support。程序通过只证明对应运行检查，不代替 README 要求的解释任务。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
