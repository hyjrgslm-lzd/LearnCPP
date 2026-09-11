# A2 value channel and laziness

本题看 `just -> then -> sync_wait`。sender 构造阶段只保存描述；`sync_wait` 连接最终 receiver 并启动 operation state 后，value 才沿图移动。

## Part 1: int 值链

构造 `just(5) | then(+1) | then(*2)`。日志必须显示：构造 sender 后，两个 `then` body 还没有执行；到 `sync_wait` 后才出现 `plus_one` 和 `times_two`。

## Part 2: 结构体值

把 `TaskInput{10,"hello"}` 送入 value channel。第一阶段产出新结构体并加后缀，第二阶段只改变数值。不要用全局共享对象传中间结果。

## Part 3: `void` value 形状

一个 `then` 返回 `void` 后，下游收到的是空 value，不是“带一个假值”。因此下一阶段 lambda 必须无参，并显式生成新值。

## Part 4: move-only 值

`std::unique_ptr<int>` 只能移动。实现若复制参数，会在编译或运行中暴露。value channel 保留值类别，这是后续 adaptor 实现的基本功。
