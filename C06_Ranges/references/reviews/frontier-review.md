# F01/new-container/frontier 非作者审查

Verdict: APPROVE

审查范围：`chapters/08-new-containers.md`、`chapters/09-frontier-ranges.md`、`exercises/F01_frontier/**`（排除build）、`references/standards.md`、`references/source-reading.md`、`references/validation/standard-snapshot*.json`。

## 结论

本 slice 满足当前 Guide/spec 的前沿交付边界：正文保留中文讲解、机制实验和完整解析；标准、提案、草案、实现能力分轴记录；F01 使用真实标准 API 名称和真实主体，缺能力时返回 77 并由 CTest 标为 SKIP，没有把未启用主体冒充为已运行通过。

无 BLOCK。可进入后续集成审查。

## 直接证据

- `references/implementation-spec.md`：要求 C++26 新容器、Ranges 新能力和 C++29 已入稿项提供完整解释、真实实验主体；缺能力必须 SKIP，不能只有宏探测或伪运行。
- `references/standards.md`：N5047 记录 `to_input` 更名为 `as_input`、P3725 const input filter DR、P3981 `inplace_vector` try 接口返回 `optional<T&>`；N5055 记录 C++29 `view_interface::at` 和关联容器 `lookup` 入稿，且明确旧 `get` 名称不作为当前接口。
- 官方复核：N5047 LWG Poll 13/14/19 分别对应 P3725R3、P3828R1、P3981R2；N5055 声明 N5054 是 C++29 工作草案。
- `chapters/08-new-containers.md`：flat 容器、`inplace_vector`、`hive` 都说明契约、生命周期、异常/容量分界和本机 SKIP 边界。
- `chapters/09-frontier-ranges.md`：`concat`、`cache_latest`、`as_input`、const filter、`reserve_hint`、optional range、C++29 `at/lookup` 均按能力和语义分开讲，不把 CTest 的 SKIP 写成主体通过。
- `exercises/F01_frontier/*.cpp`：宏守卫内使用真实 API：`std::views::as_input`、`std::views::cache_latest`、`std::views::concat`、`std::ranges::reserve_hint`、`view.at(...)`、`values.lookup(...)`、`std::inplace_vector::try_push_back` 的 `std::optional<T&>` 断言；未满足宏/头条件时走 `unavailable(...)` 返回 77。
- `references/source-reading.md`：不是纯链接清单；绑定 MSVC STL 145 / 202604、本机 include 路径、5 个文件 SHA、入口符号、状态图问题和标准/实现分界。我独立复算的本机 SHA 与表格一致。

## 独立验证

- `references/validation/frontier-review-configure.json`：`cmake -S C06_Ranges/exercises/F01_frontier -B C06_Ranges/exercises/F01_frontier/build-review -G "Visual Studio 18 2026" -DRANGES_ENABLE_FRONTIER=ON -DRANGES_ENABLE_WARNINGS=ON`，exit 0，MSVC 19.51.36256.0。
- `references/validation/frontier-review-build-r2.json`：Release build exit 0，生成 11 个 `F01_*` 可执行文件。
- `references/validation/frontier-review-ctest.json`：CTest exit 0，11 tests，1 passed / 10 skipped / 0 failed。
- `references/validation/frontier-review-direct-runs.json`：直接运行 11 个 exe，`F01_flat_containers.exe` exit 0；其余 10 个 exit 77，并输出具体缺失原因，例如 `as_input` 要求当前名宏、`inplace_vector` 要求 202603L 且旧指针 API 是不同修订、`map_lookup` 不使用旧 `get` 名。
- `references/validation/frontier-review-sourcehash.json`：绑定本次审查的正文、标准/source-reading 和 F01 源文件 SHA。

## 边界

本机只证明 flat 观察程序真实运行通过，以及 10 个前沿目标能编译出可执行并在当前库能力不足时按预期 SKIP。宏守卫内的未来主体在本机未实例化、未链接、未运行；本审查没有把这些主体声明为行为通过。

第一次 `frontier-review-build.json` 记录到一次无诊断 exit 1，只停在 MSBuild `Checking Build System`；随后同一 `build-review` 目录的 verbose build 成功，记录器复跑 `frontier-review-build-r2.json` 也成功。该失败保留为环境/工具瞬时记录，不构成代码 BLOCK。

本审查是非作者 verifier 审查，不冒称 architect 审查。
