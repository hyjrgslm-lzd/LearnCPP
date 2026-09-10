# C08 性能归因文字独立审查

审查时间：2026-09-10  
审查人：C08 async logging lane，未参与旧 performance / SIMD / scheduling / NUMA 正文写作。本报告只落盘刚才的只读审查结论，不批准新增正式采样，不修改仍在 review 的 U01 async logging 文件。

## 结论

APPROVE，本轮范围内未发现阻断性性能归因问题。

排除队列专题后，`C08_Concurrency/topics/performance`、`topics/simd`、`topics/scheduling`、`topics/numa` 与 `chapters/12-16` 当前文字没有把未定位原因写成已确证瓶颈，也没有把采纳或排除优化写成无证结论。风险句基本保持为“可能 / 候选 / 当前输入 / 当前机器 / 未测 / 不能推出”的边界，并与 `CONTENT_REFACTORING_GUIDE.md` 的推进门槛一致：总耗时变化不能单独证明锁竞争、分配器、cache miss、带宽、NUMA、worker 数或调度根因。

## 实际读过文件与指纹

| 文件 | SHA256 |
| --- | --- |
| `C08_Concurrency/chapters/12-measurement.md` | `adea9e234faa394b26f2d06dbcf4fd503fa742d266dbf14f7d90cfcf9101a8dc` |
| `C08_Concurrency/chapters/13-cache-and-layout.md` | `d28bb240d4bfd92147906723d21d7544a237217375f810ff5e76a97922db6cac` |
| `C08_Concurrency/chapters/14-parallel-algorithms.md` | `eb926171497a7b234d8c4b1d8c97a79710d8dea7959c2d1d1badc2f69b9a8e41` |
| `C08_Concurrency/chapters/15-simd.md` | `b67675dba8c30a6b3abc21120e8072eea615167233601b54a3c3e81a731b5753` |
| `C08_Concurrency/chapters/16-scheduling.md` | `2195735320af3cc8623c34633d70d81418a2cf4df44b28ad8c08b88d29e9ef8e` |
| `C08_Concurrency/topics/numa/01-topology.md` | `f01c8647b4eaa2c6fa168ffdaed8280c5208e14b79563a4edfd818eed2ad4699` |
| `C08_Concurrency/topics/numa/02-affinity.md` | `73545b7b1af35ba50aa3cf619657cb03401cbddf4d3f98e1106d5440250304fa` |
| `C08_Concurrency/topics/numa/03-placement.md` | `f6952a4e5cdba1945839e26ed6be2429e802feec806c10fb4ddf4298ccf4efb0` |
| `C08_Concurrency/topics/numa/04-sharding.md` | `6dd695de0153698eb4abd1d3580ebd901a48bcbeff4387e1dcbb3dddfa907665` |
| `C08_Concurrency/topics/performance/01-cache-layout.md` | `b816263352fefa62d89024f5bb83c8e913d8a488758cb20c26602823146ba620` |
| `C08_Concurrency/topics/performance/02-execution-policies.md` | `7657c54005eb495dd9fcd1d991489a0a137ad20ce8c67632719c36b139a1441c` |
| `C08_Concurrency/topics/performance/03-reduce-scan.md` | `4d9a5403c044978dd3e940386c4da5883ffa083fee829bc973035e76f9574d58` |
| `C08_Concurrency/topics/performance/04-parallel-compute.md` | `27fc91de3219af24839f053f5f3a9b77f7e8fd377ab937131c5022dbd23e2ef2` |
| `C08_Concurrency/topics/performance/05-light-heavy-scaling.md` | `5ffcb50ef09e29134eeeeb09c65f0bf065a613f5d120af2c281fdf8fe2c22cfb` |
| `C08_Concurrency/topics/performance/VALIDATION.md` | `b89b3e2a6eaa906f7ed69955c7567372d5705a9a1142cb0480e623ecaf8c0a8b` |
| `C08_Concurrency/topics/scheduling/01-static-dynamic.md` | `cf20c618890f4bd2c12e6ee7313b6ae566acbb30ad619c16a86c4cff2be36532` |
| `C08_Concurrency/topics/scheduling/02-work-stealing.md` | `38d67ce2e447f9fc33d6d23d050336a12c24a5eb184bab8643d5cb478052731f` |
| `C08_Concurrency/topics/scheduling/03-execution-bridge.md` | `0cf2ee494955079dd3cf8cc832d6d16e97f47901a8d8f6026276e208248c00b5` |
| `C08_Concurrency/topics/scheduling/verification.md` | `2e9f2e91f13cb5661818fff4500b77be897fb474eb1a88a27372ecafd498199f` |
| `C08_Concurrency/topics/simd/01-scalar-and-layout.md` | `180f9a593342e88378fc49ac75ff84d55fd1934691105c3e0096cf42450eb7ab` |
| `C08_Concurrency/topics/simd/02-explicit-vectors.md` | `f7037b9c1d48765c438ea565a308298fdee1b454b435790ea99289c98613eb90` |
| `C08_Concurrency/topics/simd/03-reductions-and-precision.md` | `12b1274c54b95822dbfa03d35fde18e7d9ae22fca91192be66b618e4430679c2` |
| `C08_Concurrency/topics/simd/04-diagnostics-and-bandwidth.md` | `a80ed002088db408b0e049da47bd91c95cd6d50fd6d248d5b753e61d802f1753` |
| `C08_Concurrency/topics/simd/fast-math-build.md` | `177b0ac909d97ee9a19d7ee9a898caa2ec72bf473042c760b9b34a2b00655f72` |

## 原始数据抽查

抽查对象来自 `C08_Concurrency/references/measurements/final-20260908`。本报告只核对样本存在、字段口径和文档使用边界；没有批准或发起新的正式采样。

| 样本文件 | SHA256 | 首条有效样本 |
| --- | --- | --- |
| `C08_Concurrency/references/measurements/final-20260908/layout/samples.csv` | `ea872bea3ac0e2b21ee4833ada217c637e055be1c5b5aea0fb44ca41098fbbbe` | `layout,batched,100000,2,0.4016,200000,includes zeroing/spawn/join; final-total contract; batch=256; atomic_updates=782; hint=64; packed_same_hint_region=1; atomic_lock_free=1,0` |
| `C08_Concurrency/references/measurements/final-20260908/simd/samples.csv` | `bfe6c208003871d6aa6144d0722de3f5aadf6e0ee06b5e62acc1ec4a7a054366` | `simd_dot,dot_scalar,262147,1,0.1493,262147,double accumulation; logical read bytes=8*n; allocation/check excluded,0` |
| `C08_Concurrency/references/measurements/final-20260908/scheduling/samples.csv` | `a969cb63cccd614ee64a3fd4e04821320f4fcea56bde0a8ca4a863cd317fb6a4` | `scheduling,stealing,1024,4,1.3137,1024,includes allocation; thread/pool creation; submit; compute; join; first quarter 100x rounds,0` |
| `C08_Concurrency/references/measurements/final-20260908/numa/samples.csv` | `b8b3ae4977a0aee9db3c0eae234da889c0215fd279ac7af7fe42b86df98c58b5` | `numa,parallel-init,1048576,2,0.6051,131072,size=bytes; completed=uint64 words; includes reader creation binding page-pointer scan join; excludes allocation initial touch pre-read queries; independent-base-page-reservations; reservations=256; allocation_granularity=65536; page_bytes=4096,0` |
| `C08_Concurrency/references/measurements/final-20260908/cap3/samples.csv` | `0662f8aaa44d9342980e779ad00cd7f28156134e005df5aaa279571056a38dc6` | `cap3_gemm,sse2,48,1,0.0142,2304,"C overwrite/zeroing and worker lifecycle included; block=16; FLOP=2*n^3; caller=1; workers=0; threads counts workers when nonzero, otherwise caller (backend unknown=0)",0` |

抽查确认：

- layout 样本保留 `shared`、`packed`、`padded`、`batched` 的五进程样本与 `atomic_updates` / `hint` 口径；正文只把 padded 低耗时作为当前环境观察，没有写成 false sharing 主因已证。
- simd 样本保留 `dot_scalar`、`dot_sse2`、`xsimd`、`aos/soa` 等变体；README 明确 xsimd 更慢也保留，且无 profiler 时不归因到某条指令。
- scheduling 样本保留 `static`、`dynamic`、`stealing` 的端到端计时；正文写成当前偏斜输入下的运行时分工观察，并列出 allocation、future、管理锁、扫描、粒度等候选成本，未把 stealing 慢直接归因为 context switch。
- numa 样本只有 node 0；正文和 measurements README 均明确 remote/interleaved 没有耗时证据，页面前后相同不证明无迁移或零 page fault。
- cap3 样本保留 `threads=0` 的库后端未知口径和 GEMM 小规模线程/分块未击败 naive 的结果；正文未把该结果推广成通用排除结论。

## APPROVE 范围

本次批准的是文字归因边界：

- 可以保留当前 performance / SIMD / scheduling / NUMA 正文中对机制的解释，只要继续以当前输入、当前构建、当前机器和已有样本为边界。
- 可以保留 final-20260908 原始样本作为已归档证据；它证明采样协议执行过，不证明整机空闲、硬件事件已定位或所有后续源码状态都已重新正式采样。
- 可以保留 `threads=0` 表示标准库后端实际线程数未测；不能用 `hardware_concurrency()` 或耗时倒推 worker 数。
- 可以讨论固定开销摊销、数据规模影响、逻辑带宽模型、NUMA 放置模型、调度粒度模型；这些仍是解释框架或候选原因，除非后续补充定位证据。

## 未测限制

以下内容本报告不批准为已证结论：

- 未批准新的正式性能采样，也未确认源码稳定后的 full run 已完成。
- 未测 CPU affinity / 频率控制 / SMT 关系 / 硬件 counter / ETW 或 perf trace；不能声称 cache miss、cache coherency、context switch、AVX downclock、DRAM bandwidth、page fault 或迁移是主因。
- NUMA 只有 node 0 样本；不能写 local/remote/interleaved 的速度排名。
- `std::execution` 后端 worker 数未观测；不能写实际并行度。
- xsimd、GEMM threaded、parallel policies 在某些样本无收益或负收益，只能绑定对应规模、构建和机器；不能推广为该方向普遍无效。

## 若后续需要升级归因

最小受控方案应按问题分别补证：

1. false sharing / cache：固定 CPU/SMT，记录对象地址与 cache-line hint，保留 packed/padded/shared/batched 五进程样本，加硬件事件或受控替换证明共享写频率是主成本。
2. scheduling：分阶段计时 submit / compute / wait / join，计数任务数、future 数、steal 次数、空扫描次数和管理锁等待，再与 batched dynamic 或去 future 对照。
3. SIMD / bandwidth：跨 L1/L2/LLC/内存规模采样，保存生成代码证据，加 bandwidth/cache counter 或阶段对照，区分逻辑字节数与实测 DRAM 流量。
4. NUMA：在至少两个 memory node 和对应 CPU node 上，固定 reader CPU，只改变页面节点；保存 page node、CPU node、fault/migration 证据与 local/remote/interleaved 同口径样本。

## 审查停止条件

已完成只读全文检索、强归因词上下文复核、专题文件枚举、样本头部抽查和 SHA256 指纹记录。未发现需要返修的性能归因文字；后续若正文或样本更新，应重新生成本报告或追加新的独立审查记录。
