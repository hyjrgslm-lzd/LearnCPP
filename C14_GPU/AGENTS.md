# C14_GPU AI 工作流约定

## 前置阅读

每次开工前必须读完以下四项，再动手：

1. 本仓库 `README.md`——确认技术基线、判定标准、术语定义
2. 当前模块对应的编号 `.md` 文件——确认模块目标、前置知识、CC 要求
3. 上一模块的 `.md` 文件——确认依赖概念已覆盖
4. `.omc/state/` 下的会话 state——避免重复生成或与上次上下文冲突

## 代理路由

| 任务类型 | 主代理 | 备选 / 协作 |
|---|---|---|
| 查询 NV 官方文档（CUDA、cuDNN、CUTLASS、OptiX 最新 API） | `document-specialist`（必用 `chub`/context7，降级 web） | — |
| 撰写或修改 `.md` 教学文档 | `writer`（opus，中文，保持 Ranges/Execution 语气） | `document-specialist` 提供 API 事实核查 |
| 撰写 `.cu` 练习骨架（A–F 模块基础题） | `executor`（sonnet） | — |
| 撰写 `.cu` 练习骨架（G–K 算子题，wgmma/TMA/CUTLASS） | `executor`（opus） | `architect` 先出 kernel 层级设计 |
| 设计 H/I 模块 kernel 优化阶梯骨架 | `architect` | `executor` 落地代码 |
| Review 教学文档（技术事实 + 可验证性） | `code-reviewer` | `critic`（第二意见） |
| Review kernel 代码（正确性 + 性能分析） | `code-reviewer`（opus） | — |
| 提取本次会话的可复用技能 | `oh-my-claudecode:learner` | — |
| 最终全量验证（编译 + Nsight 冒烟） | `verifier` | `qa-tester` |

## 并行策略

- 同时运行的代理不超过 3 个（对齐 Plan 阶段限制）。
- P1/P2/P3/P4 阶段使用 `/oh-my-claudecode:team` 派出 3 工人并行推进。
- 同一模块内，`writer` 写文档与 `executor` 写骨架可同时进行；`code-reviewer` 必须在两者完成后才启动。

## 生成顺序

| 阶段 | 产物 | 负责代理 | 并行度 |
|---|---|---|---|
| P0 基础设施 | `README.md` / `AGENTS.md` / `exercises/CMakeLists.txt` / `CMakePresets.json` / `common/*.cuh` / `.gitignore` | `executor`（opus，主会话驱动） | 1 |
| P1 Stage 1 模块文档 | `01-心智模型.md` 至 `08-结课项目1.md`，共 8 份 | `/team` 3 工人：`writer` + `document-specialist` 配对 | 最多 3 并行 |
| P2 Stage 1 习题代码 | `exercises/A*/B*/C*/D*/E*/F*/` + 结课 1 目录 | `/team` 3 工人，按模块拆分 | 最多 3 并行 |
| P3 Stage 2 模块文档 | `09-模块G.md` 至 `14-结课项目2.md`，共 6 份；H/I/J 必须用 `document-specialist` 拉最新 CUTLASS/cuDNN/TE | `/team` 3 工人 | 最多 3 并行 |
| P4 Stage 2 习题代码 | `exercises/G*/H*/I*/J*/K*/` + 结课 2 目录 | `architect` 出 kernel 骨架 → `executor` 落地；`/team` 3 工人 | 最多 3 并行 |
| P5 全量验证 | 编译通过 + 必做任务骨架编译失败（TODO 未填）+ 参考答案全通过 + Nsight 冒烟 | `verifier` + `qa-tester` | 串行 |
| P6 Review 门禁 | 全仓一致性、技术事实、链接可达 | `code-reviewer`（独立 pass）+ `critic`（第二意见） | 串行 |
| P7 清理 | 删除 `build-*/`、`exercises/third_party/`、`.omc/state/sessions/` 临时产物；保留 `.omc/notepad.md` | 主会话 | — |

## 严格 Review 规则

1. **Writer / Reviewer 分离**：任何 `.md` 文档写完后，必须在新的独立 agent 上下文中调 `code-reviewer` 或 `critic` 进行评审；原作者不得自评。
2. **Reviewer 检查清单**（每份模块文档必须通过）：
   - 技术事实：API 签名、compute capability 门槛、CUDA Toolkit 版本要求是否正确
   - 知识依赖：是否明确标注依赖的上一模块章节
   - 验收点：是否可由 Nsight Compute 指标或程序输出机械验证
   - 常见坑：不少于 8 条，且必须是真实陷阱（非编造）
   - 官方参考链接：由 `document-specialist` 抽查可达性
3. **Executor / Verifier 分离**：`main.cu` 骨架写完后由 `verifier` 隔离编译——未填 TODO 的骨架应编译失败（设计如此），`reference/` 参考答案必须全部通过。
4. **模块完结门禁**：每个模块所有习题通过 P5 + P6 双重验证前，不得开始下一模块。
5. **Commit 粒度**：per-exercise 提交格式 `feat(<题号>): <短描述>`，body 摘录"验收点"；per-module 提交格式 `feat(模块X): <主题>完整落地`；不使用 `--no-verify` 或 `--amend`。

## 硬件与工具链约束

- **宿主编译器**：VS2026 + MSVC 19.5x，`/std:c++latest /Zc:__cplusplus /utf-8 /permissive-`
- **设备编译器**：nvcc 13.x，`CMAKE_CUDA_STANDARD 20`，`--expt-relaxed-constexpr --extended-lambda`
- **目标架构**：`CMAKE_CUDA_ARCHITECTURES "80;86;89;90a;100a;120a"`
- **Hopper 专用特性**（wgmma、TMA、cluster、mbarrier）：必须在 sm_90a+ 设备上运行，CMake 守卫 `if(CMAKE_CUDA_ARCHITECTURES MATCHES "90a")`；在不具备 Hopper 硬件的环境中，此类题目跳过运行但必须编译通过（host 存根）
- **Blackwell 专用特性**（MXFP8/FP4、新 MMA 指令）：标注 sm_100a+，代码中用 `#if __CUDA_ARCH__ >= 1000` 守卫；文档中明确写出"需要 Blackwell 硬件，sm_90a 以下跳过"
- **FP8 E4M3/E5M2**：需要 sm_89+（Ada Lovelace 或 Hopper），在 sm_80/86 设备上跳过 FP8 路径
- **OptiX 8 SDK**：手动安装，路径通过 `OptiX_INSTALL_DIR` CMake 变量传入；不使用 FetchContent；模块 K 所有习题在 CMake 配置阶段检查 `OptiX_INSTALL_DIR` 是否有效，缺失时打印提示并跳过
- **CUTLASS 3.x**：通过 FetchContent 拉取，`CUTLASS_ENABLE_TESTS OFF`，`CUTLASS_ENABLE_PROFILER OFF`；仅链接 header-only 目标 `nvidia::cutlass`
- **Nsight 工具**：`ncu`（Nsight Compute CLI）和 `nsys`（Nsight Systems CLI）需在 `PATH` 中可用；`compute-sanitizer` 随 CUDA Toolkit 安装；P5 验证阶段由 `verifier` 调用

## 外部文档优先级

1. NV 官方文档（docs.nvidia.com）——最高优先级，API 签名以此为准
2. NV 官方 GitHub（cuda-samples、cutlass、cccl、TransformerEngine、optix-toolkit）——示例代码参考
3. 学术论文（FlashAttention、CUTLASS paper、Triton paper）——算法设计参考
4. 其他教程、博客——仅作对照，不作为 API 事实依据

## 禁止项

1. 同一 agent 既撰写又审核同一份文档或代码。
2. 跳过 Nsight 验证直接 commit 算子题（Stage 2 题目，每题必须有 Nsight Compute 指标记录）。
3. 在非 sm_90a+ 硬件上运行以 Hopper/Blackwell 为目标的结课项目，并以此结果作为验收依据。
4. 在 `.cu` 文件中使用未在本文档"外部文档优先级"列出的第三方 CUDA 封装库。
5. 将 `build-*/`、`exercises/third_party/`、`.omc/state/sessions/` 下的文件提交到 git。
6. 混用 CUTLASS 2.x 和 3.x API（本仓库全部使用 CUTLASS 3.x）。

## 提交协议

- **per-exercise 格式**：`feat(<题号>): <短描述>`，例如 `feat(A1): hello kernel scaffold`；commit body 摘录该题"验收点"中的 1-2 条
- **per-module 格式**：`feat(模块X): <主题>完整落地`，例如 `feat(模块B): 内存层级全部习题 + 模块文档`
- **不使用** `--no-verify`、`--amend`、`--force-push`
- 结课项目单独一次 commit：`feat(结课1): 数据并行管道 + Nsight 报告`

## 会话状态

| 文件 | 用途 | 生命周期 |
|---|---|---|
| `.omc/notepad.md` | 跨会话的学习笔记、待查 API、模块间依赖提醒 | 永久保留，不得在 P7 清理 |
| `.omc/state/sessions/{id}/` | 单次 agent 会话的临时执行状态（当前阶段、已完成题目列表） | P7 清理 |
| `.omc/project-memory.json` | 仓库级记忆：技术决策记录、模块完成状态、已确认的库版本 | 永久保留 |

每次开工前读 `.omc/state/` 的目的是避免重复生成已存在的文件，以及继承上次会话确认的 API 版本决策。每次会话结束后，`verifier` 负责把本次验证结论写入 `.omc/notepad.md`，格式为 `[模块X 验证 YYYY-MM-DD] 通过 / 失败原因`。
