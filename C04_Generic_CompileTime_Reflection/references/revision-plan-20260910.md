# C04 修正与编译期进阶扩展实施记录

用户已批准会话中的完整计划（包括 C05 跨课交付）。状态：实现与独立验证完成，最终交付状态以冻结清单及回读记录为准。初始范围见 `validation/revision-20260910/initial-scope.json`；保留 C06 等其它工作。

## 交付目标

1. 修正 L07—L10 good 的 Reference 依赖；独立遮答案实现后冻结。L10 只接受非空、完整 ASCII 非负十进制 int，允许前导零；拒绝 NUL、符号、空白、非法字符与溢出。
2. 先交付 A02 字段投影 DSL / chapter19 代表样章，独立审查通过后展开其它专题。`get<"id">` 保留 cvref；`project<"id,name">` 只接收左值，空串为空投影，拒绝空项/未知/重复键。
3. C04 追加 chapters18—24：值级编译期算法、DSL、deducing this/forward_like、类型算法组合、表达式模板、Mp11、Hana。对应 A01—A05、U01_mp11（实现题）和 U02_hana（观察/迁移）。表达式支持固定向量加法/标量乘/reverse；左值借用右值拥有，eval 拥有结果，赋值先物化保证别名正确。
4. 补 constexpr placement new、template for 控制流；chapter15/F01 加 C++29 P3822R2。能力与主体分离，非反射能力不依赖 meta 或反射 flags。
5. C05 追加 chapters19/20、U02_fmt/U03_spdlog（观察/迁移）。区分格式检查、代码生成、运行时格式化、宏裁剪、运行期日志过滤和 pattern 编译；异步日志系统归 C08/C11 后续。
6. 依赖固定：mp11 boost-1.91.0 b94b089d4ec83cd397f20958f34edf25bc3e06f4；hana boost-1.91.0 bc49ee25638e59d977edff5737b4e6bf12c1e5ea；fmt12.1.0 407c905e45ad75fc29bf0f9bb7c5c2fd3475976f；spdlogv1.17.0 79524ddd08a4ec981b7fea76afd08ee05f83755d。显式准备至各课 build/_deps，记录 License/SHA；配置不联网，ON 缺失/错版 FAIL。默认 OFF，但本次交付必须实际验证。
7. B01 增元查找同契约对照：手写/Mp11，32/128/256键，末项与缺失；trace 与计时分开，1预热5独立样本，记录高分辨率单调时钟。其它新主题不预设速度结论。

## 检查和停止条件

- C04 Debug/Release、Student-only、good 隔离、相关 ASan、新单元；C05 核心回归与 fmt/std 两 backend 的扩展 Debug/Release。
- 新实现题配正文、Student/Reference/独立good/行为bad、诊断正反例、完整解析；观察题附独立陌生输入迁移证据。
- 更新覆盖、先修、构建指南、当前质量报告、导航及全局增量状态；历史原始证据不覆盖，追加新冻结清单。
- 样章、各批及集成均由非作者审查与复验。非前沿要求全部通过；前沿仅能力缺失可 SKIP；已知阻断清零后完成。
- 不提交、不推送、不安装机器级组件、不接触凭据和生产系统。

## 进度

- [x] 已确认任务授权和范围；保存初始工作区快照。
- [x] 基线修正及复验：L07—L10及诊断13项通过；见revision-core-final-review.md的v4最终复验。
- [x] 代表样章及独立审查：revision-sample-review.md r2 APPROVE，已放行后续作者。
- [x] 全部 C04 进阶和库教学：各批非作者复验已批准，102项核心与108项meta组合通过。
- [x] C05 fmt/spdlog 扩展：两backend的Debug/Release根矩阵各38项通过，非作者r2批准。
- [x] 元查找成本实验：meta-map-run-20260910-143607，12组各1预热5有效样本，非作者复算批准；不宣称稳定加速。
- [x] 全矩阵与独立教学审查完成；[交付清单](revision-delivery-manifest.md)收录最终导航和冻结证据，冻结结果以JSON状态及回读为准。

## 当前检查点

- 基础修正已复验；新上下文盲写的原稿和后置修复保存在 `validation/revision-20260910/blind-core/`。L10 good最终采用值循环+constexpr状态断言，避开MSVC递归错误路径；根线程补了L08两层SFINAE说明和L10参数常量表达式诊断。
- A02样章初审ITERATE（正文推导不足），补齐后r2已APPROVE。A01/A03、A04/A05、Mp11/Hana、C05 fmt/spdlog均完成并通过非作者复验；A01键边界、U01真实Student接口、Hana迁移、fmt借用与异常分类的发现均已关闭。
- Mp11/Hana/fmt/spdlog固定依赖已准备，CMake启用/关闭/缺失/污染等控制已验证；核心预设默认关闭扩展。
- F01新增两项，本机14项能力SKIP，含P1反射的根frontier共15项SKIP；强制主体失败控制有记录。短诊断目录、空platform、缺头/ICE分类已修，helper r5独立复验通过；最终18个Student/18个good实际include审计通过，Student为18个预期失败与53个其它通过。
- B01正式12组采样及非作者复算已通过；两份TU共用同一source_map、头和适配，只切换查找算法。6组成对范围均重叠，不宣称稳定加速。
- 根线程已完成共享CMake/presets、全部新单元与阅读导航接线、真实接线审计和根矩阵。U01空schema修复后的Debug/Release复验与Student/good r2审计通过；旧原始记录的一个Hana误删缺口已如实注明，当前验证不依赖该缺失记录。
