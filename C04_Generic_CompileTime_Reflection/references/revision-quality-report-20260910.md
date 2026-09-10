# C04 2026-09-10 修订增量质量报告

本报告只记录本轮C04修订增量：L07—L10独立good和L10十进制解析修复、18—24章、A01—A05、U01 Mp11、U02 Hana、F01前沿补充、依赖准备和B01 meta map成本检查。旧[quality-report.md](quality-report.md)仍是历史基线；旧59项、13项frontier SKIP和原成本实验不能直接当成本轮新增范围的最终结果。

## 范围与教学边界

新增内容按问题域分开：

| 范围 | 教学点 | 证据入口 |
|---|---|---|
| L07—L10修复 | good不再include Reference；L10只接受非空、完整ASCII十进制`int`，允许前导零，拒绝NUL、符号、空白、非数字和溢出 | [revision-core-author.md](reviews/revision-core-author.md)、[verifier-l10-v4-final-summary.json](validation/revision-20260910/verifier-l10-v4-final-summary.json) |
| A01/18章 | 值级编译期表、排序、首个重复保留、静态形状、二分查找、consteval参数/NTTP边界、scratch不逃逸 | [revision-values-author.md](reviews/revision-values-author.md)、[values-a01-boundary-final-status.json](validation/revision-20260910/values-a01-boundary-final-status.json) |
| A02/19章 | 字段DSL、`get<"id">`、`project<"id,name">`、投影顺序、唯一字段、cvref、拒绝临时投影 | [revision-sample-review.md](reviews/revision-sample-review.md)、[sample-projection-evidence.md](validation/revision-20260910/sample-projection-evidence.md) |
| A03/20章 | deducing this、`forward_like`、四类cvref、move-only、`const&&`限制、递归lambda、借用不延寿 | [revision-values-author.md](reviews/revision-values-author.md) |
| A04/21章 | zip、flatten、product、惰性provider、variant返回集合、教学版completion signatures | [revision-pipelines-review.md](reviews/revision-pipelines-review.md) |
| A05/22章 | 固定向量表达式模板、左值借用/右值拥有、`eval`拥有结果、alias-safe assign、直接写回反例 | [revision-pipelines-review.md](reviews/revision-pipelines-review.md) |
| U01/23章 | Boost.Mp11 type算法、meta map、缺键、insert/replace/update、惰性、`mp_with_index`、`mp_map_find`源码 | [revision-meta-libs-author.md](reviews/revision-meta-libs-author.md)、[meta-libs-u01-student-contract summary](validation/revision-20260910/meta-libs-u01-student-contract-20260910-135125/summary.json) |
| U02/24章 | Boost.Hana `type_c`/`integral_c`、string key、异构map、`find`/`at_key`、copy map与`std::ref`借用 | [revision-meta-libs-author.md](reviews/revision-meta-libs-author.md)、[meta-libs-u02-migration summary](validation/revision-20260910/meta-libs-u02-migration-20260910-134300/summary.json) |
| F01前沿 | C++26 constexpr placement new、`template for`控制流、C++29 conditional `noexcept`复合要求；能力与主体分离 | [revision-frontier-author.md](reviews/revision-frontier-author.md)、[frontier-p2747-p3822-summary.md](validation/revision-20260910/frontier-p2747-p3822-summary.md) |
| B01正式成本 | 手写递归meta map find与Mp11 `mp_map_find`同契约定位与12组正式采样 | [正式摘要](benchmarks/results/meta-map-run-20260910-143607/summary.md)、[独立复算](reviews/revision-cost-review-final.md) |

## 审查状态

作者、盲解、后置修复、非作者复验分开记录。

- L07—L10：作者记录说明初始good不再include Reference，但fork继承旧上下文，不能声明完全盲写。后置复验曾因L10溢出路径触发MSVC ICE而FAIL；最终`verifier-l10-v4-final-summary.json`记录PASS，溢出诊断为普通语义失败，且`final_overflow_has_c1001=false`。
- A02：盲审初版ITERATE，原因是正文缺少从字符串NTTP到投影表达式的连续推导。R2 addendum APPROVE，正文已补解析、descriptor查找、展开、引用tuple和lvalue-only边界。
- A01/A03：非作者审查发现A01“最长15个ASCII字符”与实现不一致。已按文档修复，补15字符/空键正例及16字符/非ASCII/内部NUL负例；[r2独立复验](reviews/revision-values-review-r2.md)已批准。原编译可通过的越界探针现在按预期拒绝。
- A04/A05：非作者审查APPROVE。A05早期`noexcept`口径被审查者纠正：本题不承诺公开包装`noexcept`，只验证不吞异常和先`eval`再写目标。
- U01/U02：初审发现导航和Hana迁移任务不足；root另发现U01完成宏绕过真实Student操作。现已接入导航，补Hana复制/显式借用迁移，移除完成宏并用真实schema形状拒绝初态。[r2独立复验](reviews/revision-meta-libs-review-r2.md)已批准；完整实现无需另切完成开关。
- 依赖：依赖准备和CMake污染检查已有r2窄范围APPROVE。Mp11/Hana固定为Boost 1.91.0 standalone repo，对应commit分别为`b94b089d4ec83cd397f20958f34edf25bc3e06f4`和`bc49ee25638e59d977edff5737b4e6bf12c1e5ea`。
- 前沿：MSVC 19.51未通过P2747/P3822能力探测，本机结论是14项F01 SKIP和force控制路径验证；不能写成主体通过。

## 证据边界

`meta-libs-u02-migration-deleted-evidence-note-20260910-135125/deletion-note.json`记录了一个必须保留的历史缺口：U02早期`134229` raw/build记录被作者误删，不能原样恢复。该组曾有debug configure PASS、debug build进程exit 0，但记录器因中文MSBuild输出未匹配英文`Build succeeded.`而判FAIL。当前用`meta-libs-u02-migration-20260910-134300`作为替代完整PASS证据，不能声称旧失败原文仍完整保留。

`meta-map-run-20260910-124819`保留为check-only前置证据。[正式run](benchmarks/results/meta-map-run-20260910-143607/raw.json)完成12组各1次quiet warmup与5次有效采样，含全部72条过程记录。[非作者独立复算](validation/revision-20260910/cost-independent-final.json)通过。Mp11在6个成对比较中有5个样本中位数较低，但所有范围重叠；本报告不宣称普遍、稳定或跨平台加速。旧B01实验和latest-summary未被覆盖。

## 最终构建与检查矩阵

主线程回读并集成以下实际结果。计数包含Reference、观察、检查器正反控制与编译诊断，不代表同样数量的学生作业已完成。

| 配置/检查 | 结果 | 实际记录 |
|---|---|---|
| 核心 Debug | 102 PASS，0 FAIL | [构建](validation/revision-20260910/root-debug-build-01.json)、[CTest](validation/revision-20260910/root-debug-ctest-01.json) |
| 核心 Release | 102 PASS，0 FAIL | [构建](validation/revision-20260910/root-core-build-02.json)、[CTest](validation/revision-20260910/root-core-ctest-02.json) |
| Mp11/Hana组合 Release | 108 PASS，0 FAIL | [构建](validation/revision-20260910/root-meta-build-01.json)、[CTest](validation/revision-20260910/root-meta-ctest-01.json)；库单元Debug另有非作者叶级复验 |
| ASan RelWithDebInfo | 35 PASS，0 FAIL | [构建](validation/revision-20260910/root-asan-build-01.json)、[CTest](validation/revision-20260910/root-asan-ctest-01.json) |
| frontier | 102 PASS、15 SKIP、0 FAIL | [CTest](validation/revision-20260910/root-frontier-ctest-01.json)；15项主体未运行，不把CTest的100%汇总当作前沿通过 |
| Student-only，含Mp11 | 18个Student按题意失败，其余53项PASS | [JUnit](validation/revision-20260910/final-audits-student/13-ctest-meta-student-r2.junit.xml)、[独立分类及审计](reviews/revision-final-audits-r2.md) |
| Student实际include | 18目标、2723条include、0违规 | [审计](validation/revision-20260910/final-audits-student/11-audit-student-r2.json) |
| good实际include | 18目标、2742条include、0违规 | [审计](validation/revision-20260910/final-audits-good/14-audit-good-r2.json)；受控注入Reference路径必须被拒绝 |

源码前后检查、实际include和合成负控的边界见[最终接线审查](reviews/revision-final-audits-r2.md)。[整课矩阵编译诊断归档](validation/revision-20260910/closeout-diagnostics/index.json)保存control/subject输出与输入指纹；U01空schema修复后的Debug/Release记录见[补充归档](validation/revision-20260910/closeout-diagnostics-r2/index.json)。旧失败/中间态仍按前述历史缺口保留其含义；空schema分派已用编译期分支隔离`mp_with_index<0>`，Debug/Release 4/4及Student/新include审计的[最后复验](validation/revision-20260910/final-audits-r2-summary.json)通过。完整交付文件与指纹见[联合清单](revision-delivery-manifest.md)。

[最终集成审查](reviews/revision-integration-closeout.md)批准两课范围、计数与证据边界的一致性；其审查时清单为准备态，最终状态以联合清单JSON及`delivery-readback.json`为准。
