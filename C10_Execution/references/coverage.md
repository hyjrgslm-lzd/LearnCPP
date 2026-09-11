# C10 知识覆盖与反向检查

实现、能力探针和运行结果分开登记；本表只记录知识责任，不以旧文件还存在或标题相同视为覆盖。本机运行结果留在验证目录，不提交为课程文档。

## 原课程责任迁移

| 原主题/题号 | 必须解释与实现的内容 | 当前主讲与实际入口 |
|---|---|---|
| 心智模型、A1 | 五对象，惰性，connect/start，完成位置；并行算法与 sender 图的差异 | 01/02；A1_two_executions |
| A2/A3 | owning/borrowed 值，多输入合流，then 与 let_value，错误/停止，输入尾段 | 02；A2_value_channel、A3_fan_out_merge |
| B4/B5 | scheduler 能力，schedule 的 sender，starts_on/continues_on/on 的进入/离开/返回 | 03；B4_scheduler_capability、B5_context_switch |
| B6 | 分资源 parse/enrich，批次与收束，实际工作线程而非展示标签 | 03；B6_pipeline 的外部 EnrichHook |
| C1_7 | error 与异常、upon_error 值恢复、let_error sender 恢复、完整消费输入 | 04；真实 stdexec 图及两种独立解析 |
| C1_8 | request_stop、token/callback、合作式停止、三通道与最终完成 | 04；C1_8_cancellation；平台竞态延伸 I1 |
| C2_9/C2_10 | receiver get_env、属性来源、scope 的接受/活跃/收束与对象寿命 | 05；C2_9_environment、C2_10_scope_lifetime |
| D11/D12/D13 | 最小 receiver/sender/operation，完成签名、连接状态、value 适配及异常 | 01/08；D11–D13 |
| D14 | sender 图与 coroutine task 等价边界、as_awaitable、stopped 状态及实际 scheduler hop | 10；D14_coroutine_bridge |
| E1/E2/E3 | Niebloid、CPO、ADL、tag_invoke 历史与成员定制、约束/转发/noexcept | 06；E1–E3；当前 H2 使用成员 query |
| F1/F2/F3 | 类型列表、concat/transform/filter/unique，签名提取与变换，concept 语法和语义界限 | 07；F1–F3；P2 实际连接消费者 |
| G1 | 环境依赖的签名推导、void/单值/多值、异常转 error、稳定不可移动 op 与转发 | 08；G1_my_then；样章独立审查 |
| G2/G3 | pipe closure、绑定与移动；retry 总尝试数、每 connect 重置、stopped 不重试、同步栈有界 | 08；G2_pipe_syntax、G3_retry |
| H1 | scheduler/队列/驱动线程，FIFO、CV 发布、稳定地址、关闭拒收并 drain | 09；H1_run_loop |
| H2 | 自定义 query、通用覆盖优先与回退、缺失查询 SFINAE、forwarding query、真实 receiver 环境 | 09；H2_custom_query |
| H3 | promise/frame、sender await、void、通道、continuation/final transfer、嵌套 task、task-as-sender | 10；H3_coroutine_task，支持范围见题面 |
| 项目 1 | 真实文件→校验/解析→CPU 阶段→统计→借用资源收束 | 13；P1_pipeline，依赖 I1 原生完成 |
| 项目 2 | 使用级阅读 task/scope/scheduler，区分 stdexec/exec 扩展，做真实观察 | 14；S1_usage_reading |
| 项目 3 | 实现级阅读 when_all/run_loop，跟踪状态成员、分支和资源责任 | 14；S2_implementation_reading |
| 项目 4 必做 | CPO、signatures/traits/concept、just 多值/error/stopped、then、sync_wait、get_scheduler/schedule(void)、不可移动操作 | 15；P2_mini_execution |
| 项目 4 进阶 | 两子 when_all、pipe、custom query、stdexec 边界 interop | 15；P2 checker 实际调用，非纸面预留 |

## 全局计划新增责任

| 要求 | 主讲与实现 | 验证边界 |
|---|---|---|
| 标准 task、simple/counting scope、associate/close/join/spawn/spawn_future | 10；R1_task_scope；S1 对照 | 观察单元有完整程序；程序通过不替代读者分析 |
| bulk、domain 与 lowering | 12；B02_bulk_domain | CPU 模型验证真实 transform_sender，不能代表 GPU |
| IOCP/io_uring 完成桥接 | 11；I1_native_io | 真正提交、完成、错误与取消；仅明确 capability 缺失才 SKIP |
| nvexec 单/多 GPU | 12；V1_nvexec 的两份完整主体 | nvc++ 缺失，两项分别 SKIP；无物理多卡参与证明 |
| 原生 std::execution | F01 的 core/scheduler/task/scope/bulk 独立 probe+body | 当前设施缺失与主体失败严格分开 |
| 优化先有证据 | 16；B01_costs | 计数/阶段定位后串行、逐项和 bulk 对照，保存全部独立进程样本 |
| 最新可用 C++ | BUILD_GUIDE、standards-and-implementations | 语言模式、宿主库、后端能力逐轴记录；不假造宏或静默降低主体标准 |

## 下游反查

从 P1 倒查 I1 的系统完成、C1 的错误/停止、C2 的 scope 收束和 B6 的调度边界；从 H3 倒查 C09 的语言协议、D11/D12 的完成契约、G1 的签名/连接状态；从 P2 倒查 E/F 的 CPO/类型计算、H1 的发布与关闭、A3 的合流规则。

从 V1 倒查 C14 的设备内存/stream/event、B02 的执行资源定制，再问每项证据是否真的经过 GPU；从 B01 倒查被测算法是否等价、测量区间是否一致、结果是否校验、编译产物是否绑定本次源文件。源码导读同时要求固定提交、对象成员、调用/退出路径和读者自己的解释，不能由一个源码链接代替教学。

每条路线最后落到共同 checker、独立 good、可编译的行为 bad。观察/能力单元例外有明确类型，不制造空 Student/good/bad 目录凑数量。历史失败可在本机记录中保留，但不写入知识覆盖表。
