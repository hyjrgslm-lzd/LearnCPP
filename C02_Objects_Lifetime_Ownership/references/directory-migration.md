# 课程目录与项目命名迁移记录

2026-09-09，用户确认将8门已有课程统一为`C编号_具体主题`，随后要求VS项目名也与目录一致。完整映射和学习路线见[根导航](../../README.md#目录迁移与旧记录)。

## 改动范围

- 8个课程目录及当前文档、工具和跨课构建引用迁移；根README按公共基础、专题进阶、领域应用组织，保留课程空号与实际先修边界。
- 7个CMake课程的顶层`project()`与目录同名。练习target仍按题号命名，课程功能与作业接口保持。
- UE宿主项目改为`C15_Unreal_Engine.uproject`，主模块目录/文件、Game/Editor Target类及文件、模块注册、配置与文档成套同步。39个模块的注册与Build.cs路径核对通过；`UELearn.Cap1.Run`等题目控制台命令保留。
- 旧CMake缓存及构建产物保留在本地；新目录重新配置验证，Git交付排除编译产物、依赖缓存和`.omx`。

## 本机验证

| 范围 | 结果 | 原始证据 |
|---|---|---|
| C01 Release | 34/34通过 | [CTest](validation/directory-migration/c01-release-ctest.json) |
| C02 Release / Debug | 各40/40通过 | [Release](validation/directory-migration/c02-release-ctest.json)、[Debug](validation/directory-migration/c02-debug-ctest.json) |
| C08 Release | 58通过、2跳过、0失败；stdexec未启用、无多节点NUMA条件 | [CTest](validation/directory-migration/c08-release-ctest.json) |
| C09 Release | 43/43通过 | [CTest](validation/directory-migration/c09-release-ctest.json) |
| C06 / C10 | 顶层配置及代表性入口build/run通过；只证明构建入口可用，不代表未完成Student作业通过 | [C06](validation/directory-migration/c06-smoke-run.json)、[C10](validation/directory-migration/c10-smoke-run.json) |
| 新项目名 | C01/C02/C06/C08/C09/C10在独立新目录实际生成与课程同名的.slnx，缓存project名一致 | [生成结果](validation/directory-migration/project-name-verification.json) |
| L14 ASan | 新目录Debug构建、运行通过；程序旁DLL与实际MSVC工具链SHA相同 | [运行](validation/directory-migration/l14-asan-run.json)、[完整性核对](validation/directory-migration/final-integrity-and-scope.json) |
| 验证记录器 | 7个契约自检通过，包括预期失败、超时及Windows错误模式继承 | [自检](validation/directory-migration/recorder-selfcheck.json) |

上表完整CTest合计217项：215通过、2明确跳过。算法/练习矩阵在迁移后的课程路径运行；顶层project名后续调整另做fresh生成核对。C14项目名与UE注册本轮仅做静态验证，没有执行GPU设备或UE引擎构建，不外推为平台验证通过。

初次并行MSBuild提前退出，改为单节点并关闭node reuse后通过；C10的参数拆分和依赖元数据缺失分别修正，使用本地已缓存stdexec/RAPIDS源及元数据完成配置，没有升级课程依赖。失败记录使用原文件名保留，后续成功另存。

## 完整性与历史记录边界

迁移前记录6147个Git可见文件指纹，迁移后逐文件映射核对没有缺失。活动文档扫描498份，实际断链和活动旧目录路径均为0；7处C++内联语法被旧链接检查器误识别，按代码跨度核对后排除。详见[路径检查](validation/directory-migration/path-audit-final-r2.json)。

并行代理曾超出只读/验证分工执行额外替换，又把部分历史文件按Git文本写回。主线程停止该代理后，按迁移前SHA恢复并复核：4488份历史文件中4461份字节SHA一致；其余27份C01记录恢复为迁移前HEAD的Git blob，文字与已提交历史一致，但原工作树的混合换行布局未能完全恢复，不能声称原物理SHA仍匹配。旧路径与原SHA不改写，27份文件的原SHA、恢复后SHA和Git来源逐项记在[完整性记录](validation/directory-migration/final-integrity-and-scope.json)。原始失败记录、恢复前观察和换行恢复过程在本地保留，未伪造旧验证结果。

历史清单和日志中的目录按根导航映射查找；历史SHA仍绑定旧版本。当前路径变更与项目名验证以本记录为准。完整`git diff --cached --check`会报告原有课程起点尾部空行、原始日志空白；[空白审计](validation/directory-migration/whitespace-audit.json)保留结果，不为消除提示而改写历史证据。

## 独立审查

[非作者迁移审查](validation/directory-migration/independent-review.md)：APPROVE。覆盖目录与导航、CMake项目名、UE主模块/Target注册及共享工具路径；历史SHA与GPU/UE运行边界如上。
