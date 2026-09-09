> 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-1

## 目标

深度阅读 `FRDGBuilder` 的构造、`AddPass`、`Execute`/`Compile` 四段源码，画出 RDG 三阶段时序，并完成本课程核心教学杠杆——**RDG ↔ stdexec 六维类比表**。

## 前置理解

- 已读 `01-心智模型.md` §5：RDG = lazy graph
- 已完成模块 G（`FRHICommandList` 命令录制模式）
- 已完成 `C10_Execution` 模块 A-D，能说出"sender 是惰性蓝图，`start(op)` 才真正启动"

## 必做任务

1. 阅读 `RenderGraphBuilder.h` 第 42–65 行（构造函数），确认构造期无 GPU 命令产生
2. 阅读 `RenderGraphBuilder.h` 第 200–230 行（`AddPass` 声明），确认 lambda 仅被存储不被调用
3. 阅读 `RenderGraphBuilder.cpp` 第 1316 行（`Compile()`）和第 1755 行（`Execute()`），理解三阶段
4. 在笔记中画出 RDG 三阶段时序图（描述期 / 编译期 / 执行期）
5. 填写六维 RDG ↔ stdexec 类比表（见 .cpp 中的注释骨架）
6. 手动推导三 pass pipeline 的 barrier + transient lifetime（Heightfield → Normal → Gamma）

## 进阶任务

- 在 `RenderGraphBuilder.cpp` 中找到 `bCullPasses` 判断分支，理解 pass culling 触发条件
- 阅读 `RenderGraphDefinitions.h` 中 `ERDGPassFlags` 的全部定义（Compute / Raster / AsyncCompute / NeverCull）
- 统计同样三 pass pipeline 用传统 `FRHICommandList` 写所需的 barrier 代码行数，与 RDG 的 0 行对比

## 验收点

- [ ] 能不看资料说出"RDG 描述期 = sender 构图，Execute() = start(op)"这句话
- [ ] 填写的六维类比表每行有实质内容（不得照抄注释）
- [ ] 三 pass pipeline 的 barrier 位置与 lifetime 计算结果正确
- [ ] 能指出 `FRDGTextureRef` 在描述期代表什么（元数据句柄）、执行期代表什么（关联了真实 FRHITexture）
- [ ] 画出的 RDG pass 图包含 pass 节点、资源节点、读写边、transient lifetime 标注四个要素

## 观察点

- `FRDGBuilder` 在栈上分配（注释第 45 行明确说明），整个图的生命周期绑定到这个栈对象
- pass culling 是"未被消费不执行"的渲染版本，等价于 stdexec 惰性 sender 未被 `start` 时不运行
- `FRDGTextureRef` 不是 `TSharedPtr<FRHITexture>`，它是 `FRDGTexture*`，指向纯 CPU 元数据

## 常见坑

- **在 AddPass lambda 内部调用 `GetRHI()` 并传给外部变量**：lambda 在 Execute() 才运行，GetRHI() 结果在 Execute() 后可能已由 transient pool 回收
- **在 AddPass lambda 内捕获 `UObject*`**：lambda 在 RenderThread 执行，此时 GC 可能已在 GameThread 回收该对象；必须按值捕获数据副本
- **把 FRDGBuilder 移出栈帧**：RDG allocator 生命周期绑定到 FRDGBuilder，不得拷贝或 move

## 提示

- 用 IDE 全局搜索 `RDG_EVENT_NAME` 可找到大量真实 AddPass 调用示例
- `DeferredShadingRenderer.cpp` 第 595 行有完整的真实 pass 用例可对照阅读

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** `FRDGBuilder::Execute()` 被调用，Compile() 完成后的 pass 遍历阶段每个 lambda 才被执行
2. **生命周期拥有者？** `FRDGBuilder`（栈对象）持有 allocator + transient pool 引用；Execute() 返回后所有 transient 资源归还
3. **涉及哪些 Named Thread？** AddPass / Execute 均在 **RenderThread**；GPU 命令最终由 **RHIThread** 提交驱动
4. **涉及 UObject 时怎么对 GC 可见？** RDG 层不操作 UObject；需要时通过 `ENQUEUE_RENDER_COMMAND` 按值传入 RHI 级别句柄，再 `RegisterExternalTexture` 注入 RDG
5. **[本题专属]** `FRDGBuilder::Execute()` 内部依次做了哪三件事？答：Compile()（依赖分析 + barrier 推导 + 资源 lifetime 计算）→ 资源分配（transient pool）→ pass 遍历（调用 lambda，录制 RHICommandList）

## 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`（第 42–65 行类注释；第 200–230 行 AddPass）
- `Engine/Source/Runtime/RenderCore/Private/RenderGraphBuilder.cpp`（第 1316 行 `Compile()`；第 1755 行 `Execute()`）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphDefinitions.h`（`ERDGPassFlags` 全定义）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h`（`FRDGTextureDesc`、`FRDGSubresourceState`）
