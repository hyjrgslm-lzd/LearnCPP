# C08 Frontier r5 非作者复验报告

结论：APPROVE。

本轮只审批 r5 的前沿样章结构、能力门控、模型/标准证据分离和当前源码逻辑；不审批未来标准库可用后的性能或实现质量结论。当前工具链探测 `CS_HAS_STD_THREAD_ATTRIBUTES=0`、`CS_HAS_STD_HAZARD_POINTER_BATCH=0`、`CS_HAS_STD_SENDERS=0`、`CS_HAS_STD_HAZARD_POINTER=0`、`CS_HAS_STD_RCU=0`，所以标准主体缺能力时 SKIP 是正确结果，不是 PASS。

## 绑定范围与文件指纹

| 文件 | SHA256 |
| --- | --- |
| `C08_Concurrency/exercises/CMakeLists.txt` | `89C35D287D5B9B26C7C9FFE94B62797B30A5EB5B819DD616A15152758CB2E0FF` |
| `C08_Concurrency/exercises/cmake/NativeFeatures.cmake` | `3A5B32D1C219D7B494A7F6B5C2CFA235227B387BFE3FC5D056718B3B91DDA33F` |
| `C08_Concurrency/exercises/cmake/feature_probes.cpp` | `35D3D10176CDD5F49F62D9AEB50AE726A09D95B504BDA9F12102B8E98204A830` |
| `C08_Concurrency/topics/frontier/README.md` | `76D7C1C18AB5FF10A7893EE65F0F8FFE7A859F3524E4FF8C814CCE7857F37701` |
| `C08_Concurrency/topics/frontier/01-thread-attributes.md` | `2600E4F0AE7843D80AF347910D25F0DA289DDCB7E52D9A79DC0A0DC4AC70BC81` |
| `C08_Concurrency/topics/frontier/02-hazard-pointer-batches.md` | `B9D8350CB5A79274B528B34B8D5CC502954D3C7A638F6331D54778FAAC6E5387` |
| `C08_Concurrency/topics/frontier/03-native-facilities.md` | `749A429CFA7590FF10E4B2BAF4F56C48189E78849E9E5ED167189BD41C05A5C5` |
| `C08_Concurrency/exercises/F01_thread_attributes/README.md` | `A4BA775EC2780405BEB1EDD250820986428AE022F87CF9079F58D9632B70E910` |
| `C08_Concurrency/exercises/F01_thread_attributes/main.cpp` | `15FB53C55C2191308A8A363A4603697A285BC81A37CFFED0F436EB02250BEE37` |
| `C08_Concurrency/exercises/F01_thread_attributes/solution.cpp` | `A666C16C7F8025EE40B440C429BE64912E232883C0D28CD5FE306AB794A15BA2` |
| `C08_Concurrency/exercises/F02_hazard_pointer_batches/README.md` | `AE89BCC933CA43CCA3737FD4DED004E905AF34A912F3592D2B997C7C889E4E6A` |
| `C08_Concurrency/exercises/F02_hazard_pointer_batches/main.cpp` | `B7A6606AC5192F4F8E3B894B095F65827432151930C44FDA7071D9DD92A02068` |
| `C08_Concurrency/exercises/F02_hazard_pointer_batches/solution.cpp` | `14F32CED6B06E5494798FF94592A13573A4E2B2AA2FD75627211B9163F9A7D70` |
| `C08_Concurrency/exercises/F03_native_facilities/README.md` | `1DC84B2A1875CF843E434337399CD7EAF9770E930CD5E5F1976184C949826CB1` |
| `C08_Concurrency/exercises/F03_native_facilities/CMakeLists.txt` | `2C030EB381A73D4CC4542E4608693D2EE32F3D060E24FD30BBDED5A4C57C0F7D` |
| `C08_Concurrency/exercises/F03_native_facilities/std_senders.cpp` | `0EF402B99128871E648A9005FF3ACDBA4442E4E2C572A44A936E94B8F57719B0` |
| `C08_Concurrency/exercises/F03_native_facilities/std_hazard_pointer.cpp` | `0EC8C1461796B70CAE4039B116443AB792671BD9F2ECE443B78591AB931E7B5E` |
| `C08_Concurrency/exercises/F03_native_facilities/std_rcu.cpp` | `AB836C1DD19D43EF98A48EBDDA31BEEEBC05ACAAE35C3B77DC75998192C4D99D` |
| `C08_Concurrency/references/standards-and-implementations.md` | `6B59313CDEABE4DF29EC34EE618A9839112C14FDC4F7399F84697F743D39F467` |
| `C08_Concurrency/references/validation/c08-revision/frontier-r5-evidence.json` | `0DF1D9A0D56DF04FF0CBFDED7B48C609F17309649B29A4B4BE66511ABEA68A47` |
| `C08_Concurrency/references/validation/c08-revision/frontier-r5-ctest.xml` | `D7F815785288200C1CA23338AF997CE736D03513CA73E601550CF65605A88F8C` |
| `C08_Concurrency/references/validation/c08-revision/frontier-r5-f03leaf-ctest.xml` | `5B9262E7B4C5C68F3EB18485725A70217C4E1F890A57A16EBC88734AAECA273A` |

## 审查结论

1. 标准来源边界已修正。`topics/frontier/README.md:3` 明确本专题只讲进入 N5054 工作草案的 C08 增量，并通过 N5055 说明 P2019R9、P3428R4 的纳入状态；`references/standards-and-implementations.md:69` 也把“已入稿”与“最终发布标准/本机实现声明”分开。事实边界清楚。

2. F01 duplicate 模型已测，且没有伪装成标准主体。`F01_thread_attributes/main.cpp:16-18`、`:38-44` 覆盖重复属性拒绝；`topics/frontier/01-thread-attributes.md:57-59` 和 `F01_thread_attributes/README.md:14` 明确 `main.cpp` 是 observation，`solution.cpp` 才是标准主体。当前 `solution.cpp:1-10`、`:15-36` 由 `CS_HAS_STD_THREAD_ATTRIBUTES` 门控，缺能力返回 77。

3. F02 batch 语义已按最终口径表达。`topics/frontier/02-hazard-pointer-batches.md:16-18` 和 `F02_hazard_pointer_batches/README.md:26` 明确 `make_hazard_pointer_batch` 只补 empty、保留已有非空 HP 与 protection，`clear_hazard_pointer_batch` 销毁 owned HP 并使元素 empty，且不同于 `reset_protection()`。模型代码 `F02_hazard_pointer_batches/main.cpp:52-58` 覆盖 make 保留非空和重复 clear 后 empty。

4. F02 标准主体的 raw node 退休时机没有发现当前阻断。`F02_hazard_pointer_batches/solution.cpp:27-35` 先 protect、断开 atomic root、确认两个 HP 仍非空，再 clear 使句柄 empty，最后 retire 已取得的 raw node。该主体没有并发读者，且 retire 前没有进入回收队列；因此静态审查下没有 use-after-retire 证据。边界：当前本机 `CS_HAS_STD_HAZARD_POINTER_BATCH=0`，该标准主体未实际运行；审批的是源码路径与门控逻辑，不是标准库行为实测通过。

5. F03 已改为纯原生主体集合。目录检查只发现 `CMakeLists.txt`、`README.md`、`std_senders.cpp`、`std_hazard_pointer.cpp`、`std_rcu.cpp`；没有 `main.cpp`、`solution.cpp` 或摘要占位可执行文件。`F03_native_facilities/README.md:5`、`:17` 明确 PASS 只来自标准头、特性宏、实例化、链接和运行。`F03_native_facilities/CMakeLists.txt:5-23` 只在 `CONCURRENCY_STUDY_BUILD_REFERENCE=ON` 时注册三个独立 reference 目标，并用 77 表示能力缺失。

6. Root CMake 已显式纳入 F03。`C08_Concurrency/exercises/CMakeLists.txt:17` 有 `add_subdirectory(F03_native_facilities)`，不会依赖普通练习目录 glob 是否含 `main.cpp`。

7. C++26/C++29 能力探测粒度可接受。`NativeFeatures.cmake:2-4` 把 C++26 原生设施和 C++29 线程属性/HP batch 分开；`feature_probes.cpp:71-110` 分别编译链接线程属性和 HP batch 最小主体。OFF、缺能力 SKIP、能力具备后主体 FAIL 的边界与 `references/standards-and-implementations.md:78` 一致。

8. r5 证据与本机复跑一致。作者证据 `frontier-r5-ctest.xml` 为 7 项：2 个 observation PASS，5 个 reference/native SKIP，0 FAIL；本机复跑同样为 7 项、2 PASS、5 SKIP、0 FAIL。`frontier-r5-f03leaf-ctest.xml` 为 3 个 F03 leaf 全 SKIP，符合当前能力缺失。

9. 旧 r3/r4 证据保留。目录中仍存在 `frontier-r3-ctest.xml`、`frontier-r3-evidence.json`、`frontier-r4-ctest.xml`、`frontier-r4-evidence.json`；r5 没有覆盖旧证据。

## 本机复验

- `cmake --build C08_Concurrency\exercises\build\c08-frontier-author-r5 --config Release --target F01_thread_attributes F01_thread_attributes_reference F02_hazard_pointer_batches F02_hazard_pointer_batches_reference F03_std_senders_reference F03_std_hazard_pointer_reference F03_std_rcu_reference` → PASS。CMake 重新探测后输出 8 个前沿/原生宏均为 `0`，目标全部构建成功。
- `ctest --test-dir C08_Concurrency\exercises\build\c08-frontier-author-r5 -C Release -R "F0[123]_" --output-on-failure` → PASS：7/7 tests passed，0 failed；其中 F01/F02 observation 通过，F01/F02 reference 与 F03 三个 native reference 返回 SKIP。
- `ctest --test-dir C08_Concurrency\exercises\build\c08-frontier-author-r5-refoff -C Release -N -R "F03_"` → `Total Tests: 0`。
- `Get-ChildItem C08_Concurrency\exercises\build\c08-frontier-author-r5-refoff -Recurse -Filter 'F03*.vcxproj'` → 无输出，未生成 F03 工程。
- `cmake --build C08_Concurrency\exercises\build\c08-frontier-author-r5-refoff --config Release --target F03_std_senders_reference` → 预期失败，`MSBUILD : error MSB1009: 项目文件不存在。开关:F03_std_senders_reference.vcxproj`，证明 Reference OFF 下不是隐藏测试，而是无目标。

## 审批边界

- APPROVE r5 的教学结构、标准证据边界、能力门控、当前源码生命周期路径和验证记录。
- 不把当前 SKIP 解释为标准库实现通过。
- 不审批未来当 `CS_HAS_* = 1` 后的运行结果；届时主体编译或行为失败应按 FAIL 处理。

