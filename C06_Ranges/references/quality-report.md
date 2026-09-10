# C06质量报告

2026-09-10：完整C06教材、练习与Windows可用路径已交付；已知代码/教学阻断经过修复和非作者复验。前沿10项因本机能力不足保持SKIP，不宣称这些主体已经实例化或运行。最终材料核验见[交付审查](reviews/final-delivery-review.md)。

## 交付范围

- 保留原12篇正文、29个学习入口；新增10篇正文、8个基础练习、F01和B01，共22篇正文、39个单元。
- 补齐复杂度、容器、失效、算法、动态数组、堆、链式哈希和AVL；Ranges使用、CPO/iterator/view/cache/generator及六层mini-ranges继续保留并修订。
- 14个实现单元提供Student/Reference/独立good/bad；其余为观察、前沿或成本单元。AVL额外保留假结构、陈旧高度两个控制，checker独立检查真实节点。
- 默认仍为C++26，保留单题构建，补齐Debug/Release、ASan、Student-only与前沿预设。复用C01检查器、C02记录/审计工具及C05失败验证工具，没有新增第三方依赖或机器安装。
- [覆盖与迁移](coverage.md)、[逐文件清单](changed-files.md)、[源码阅读](source-reading.md)、[标准/实现索引](standards.md)、[实施规格](implementation-spec.md)相互衔接。

## 当前最终验证

环境：Windows x64，MSVC19.51.36256.0，工具目录14.51.36231，STL145/202604，CMake4.2.3。受检源码、构建文件与复用工具共182个输入，见[最终代码指纹](validation/final-code-inputs-r4.json)。最后两项G3修补后重新构建并运行下表，不汇总过期作者日志当最终矩阵。

| 配置/对象 | 结果 | 原始证据 |
|---|---|---|
| 核心Release | 69 PASS，0 FAIL | [CTest](validation/post-value-release-ctest.json)、[build](validation/post-value-release-build.json) |
| 核心Debug | 69 PASS，0 FAIL | [CTest](validation/post-value-debug-ctest.json)、[build](validation/post-value-debug-build.json) |
| ASan RelWithDebInfo安全集 | 53 PASS，0 FAIL；排除negative标签 | [CTest](validation/post-value-asan-ctest.json)、[build](validation/post-value-asan-build.json) |
| 前沿配置Release | 69 PASS、10 SKIP、0 FAIL，共79项 | [CTest](validation/post-value-frontier-ctest.json)、[build](validation/post-value-frontier-build.json) |
| Student-only源接线 | 14个Student、2186条真实include记录；无Reference依赖 | [审计](validation/post-value-student-isolation.json)、[显式clean-first重编](validation/post-value-student-include-build.json) |
| Student初态原始运行 | 14个程序均exit 1且有实际check诊断，无超时/崩溃充数 | [直跑记录](validation/post-value-student-direct-rejections.json) |
| 正式成本实验 | 336独立进程全部有效；56组各1预热+5样本；hash无漂移 | [执行记录](validation/formal-benchmark-run.json)、[完整数据](benchmarks/results/b01-formal-20260910-143713/report.json) |

Student初态失败是正确的学习状态，不计入核心通过数。bad控制体必须exit 1且命中指定检查诊断，不能把任意错误当通过。CTest的“100% tests passed”可能包含SKIP，本报告分别计数。ASan无报告不证明未执行路径或所有生命周期组合都正确。

前沿跳过项：inplace_vector当前optional引用接口、hive、concat、cache_latest、as_input、P3725 const-input filter、reserve_hint、optional range、C++29 view_interface::at、关联容器lookup。F01保留完整主体和宏/头/约束门槛；选项关闭与真实缺能力另行说明。

## 非作者闭环

| 主题 | 已关闭的问题与最终复验 |
|---|---|
| 规格/样章 | [规格](reviews/spec-review.md)；[样章r2](reviews/sample-review-r2.md)：同形管道计数、实际ranges::to、负限额、字段类型修正 |
| 序列容器/动态数组 | [r2](reviews/sequence-review-r2.md)：超大reserve只接受length_error，不再让bad_alloc掩盖契约错误 |
| 关联/AVL | [r2](reviews/associative-review-r2.md)：真实结构inspector、独立AVL good、假结构/高度坏例、完整旋转推导；之后追加重复插入后的结构复查并纳最终矩阵 |
| Ranges使用 | [r3](reviews/usage-review-r3.md)：stored join的common规则、generator DFS顺序/首值、子范围字符串物化、题面与实际检查一致 |
| Ranges协议 | [r4](reviews/protocol-review-r4.md)：G1短unsized输入、G2/G3 non-common/move-only、惰性traits、完整sentinel/closure正文和可编译doc probe |
| 高级实现 | [r3](reviews/advanced-review-r3.md)：句柄唯一所有权、真实const引用语义、独立mini-ranges good及当前checker入口 |
| 新容器与前沿 | [审查](reviews/frontier-review.md)：一手版本边界、固定源码、真实主体与10项未验证范围；BUILD_TESTING=OFF接线另经[配置验证](validation/frontier-no-tests-configure.json) |
| 性能工具 | [r2](reviews/benchmark-review-r2.md)：timed/counted分离、独立全字段oracle、运行前后hash与漂移拒绝 |
| 最后集成修补 | [r2](reviews/closing-review-r2.md)：F1/G3真正独立good、G2合法regular_invocable观察、G3 move-only prvalue转发与索引 |

旧BLOCK、工具错误、原始失败和复验均保留，不把旧失败文件覆盖为PASS。G2旧函数只在普通调用中复现其非相等保持行为，见[语义基线](validation/baseline/g2-stateful-semantic.cpp)；G3用有值相等语义的MoveOnlyValue作合法纯函数输入，旧good编译拒绝与修后同源通过见closing-value-before/after记录。

专用architect在规划阶段因固定模型不可用未完成；本次实际使用非作者code-reviewer/verifier完成教学、技术、实验审查，没有冒称正式architect/ralplan流程批准。

## 成本结论与限制

[正式结果](benchmarks/results/latest-summary.md)保留所有样本、范围及负结果。四组日志负载中显式循环均胜出，实体化并非总比复算快；索引查找收益同时列出构建成本。计数与固定MSVC源码路径说明额外求值，不能从总耗时宣称cache miss、分配次数或平台无关加速。

采样开始时未观测到编译进程，本任务在正式运行期间没有构建；并未全程控制整机所有后台负载。其他平台未测，10项前沿主体未在本机实例化/运行，不能把这些限制写成完全支持。

## 范围与文件保护

本任务只修改C06及根README/全局Plan中的C06导航状态，未提交或推送。保留其他线程的C04/C05变化；[保护审计](validation/protected-files-final.json)对开工快照覆盖的7844个范围外文件逐项比较，记录其中41项并行变化而未回滚，hash本身不用于推定作者。

课程级.gitignore排除了review目录中的IDE项目、tlog/recipe及其他生成物；原始JSON/TXT证据和重放probe源码仍可交付。G2 doc probe从被build-*忽略的旧目录迁到可交付位置，见[位置说明](validation/ranges-protocols-r5/probe-location.md)。构建目录、exe/PDB及本机私有上下文不作为仓库交付物。
