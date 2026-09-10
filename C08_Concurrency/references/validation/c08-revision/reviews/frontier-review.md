# C08 frontier 非作者审查报告

- 审查日期：2026-09-10
- 审查范围：`topics/frontier/**`、`exercises/F01_thread_attributes/**`、`exercises/F02_hazard_pointer_batches/**`、`exercises/F03_native_facilities/**`、`exercises/cmake/NativeFeatures.cmake`、`exercises/feature_probes/native_probe/**`
- 证据基线：`references/validation/c08-revision/frontier-r3-evidence.json`、`references/validation/c08-revision/frontier-r3-ctest.xml`
- 结论：**REVISE**

## 版本绑定

| 文件 | SHA256 |
| --- | --- |
| `topics/frontier/README.md` | `76D7C1C18AB5FF10A7893EE65F0F8FFE7A859F3524E4FF8C814CCE7857F37701` |
| `topics/frontier/01-thread-attributes.md` | `2600E4F0AE7843D80AF347910D25F0DA289DDCB7E52D9A79DC0A0DC4AC70BC81` |
| `topics/frontier/02-hazard-pointer-batches.md` | `E4E5094BB2070458DCB33571564AB32ED210D596D4472EA5B4DCB92F5E029055` |
| `topics/frontier/03-native-facilities.md` | `8C9D7884F34461237DD2592A89277D0317CAE8DD5D08C817E5C0EB45E302C57C` |
| `exercises/F01_thread_attributes/main.cpp` | `8918E250493E3F6D1D0FE321F5444F10B3ECBED3DF2ACE15C24C5C38860D8F58` |
| `exercises/F01_thread_attributes/solution.cpp` | `A666C16C7F8025EE40B440C429BE64912E232883C0D28CD5FE306AB794A15BA2` |
| `exercises/F01_thread_attributes/README.md` | `A4BA775EC2780405BEB1EDD250820986428AE022F87CF9079F58D9632B70E910` |
| `exercises/F02_hazard_pointer_batches/main.cpp` | `4A7D218F31AE13AD6C4D10A2D0F4D6B709C2077595E6D5D78FA9F67791FDF73A` |
| `exercises/F02_hazard_pointer_batches/solution.cpp` | `6BCE6958264FEEBC035A4E34FFE701893704CE266CFBACFB0539C472D6CD3146` |
| `exercises/F02_hazard_pointer_batches/README.md` | `BAD0DF894DC0748FA7F841A219686A450FD8DBF3251437A846934089876CCD2A` |
| `exercises/F03_native_facilities/main.cpp` | `D85AE5FAFBC6A837C48D3A0EF34D51067559F68D168D96AC6110783D973D44E9` |
| `exercises/F03_native_facilities/solution.cpp` | `6C1190BFCD30A59BC13517621CFB457900FC4DE9DAEF2EFA2F6D1BDF860BBE80` |
| `exercises/F03_native_facilities/std_senders_reference.cpp` | `0EF402B99128871E648A9005FF3ACDBA4442E4E2C572A44A936E94B8F57719B0` |
| `exercises/F03_native_facilities/std_hazard_pointer_reference.cpp` | `0EC8C1461796B70CAE4039B116443AB792671BD9F2ECE443B78591AB931E7B5E` |
| `exercises/F03_native_facilities/std_rcu_reference.cpp` | `AB836C1DD19D43EF98A48EBDDA31BEEEBC05ACAAE35C3B77DC75998192C4D99D` |
| `exercises/F03_native_facilities/CMakeLists.txt` | `A2F7032A163295D55E0AA4CB2157C2A21CDC2D25CFE56957426E59C64B3D20CD` |
| `exercises/cmake/NativeFeatures.cmake` | `614854EA0345A285357FC3C255A85342ABF0A61A26CFDD48A579551B3F58CF01` |
| `exercises/feature_probes/native_probe/feature_probes.cpp` | `395DA1626B16B2920F901100BE96DC5926EAD4CFB360229CBCD10CD67CA9A2EE` |
| `exercises/feature_probes/native_probe/CMakeLists.txt` | `885277EC9665A05C3E460B7E8A400B34B34C7A9A17D20BC196BC5DAA5D849D67` |

## 标准依据核对

- N5055 工作论文投票记录显示 P3428R4 与 P2019R9 已应用到工作论文。
- N5054/P2019R9：`std::thread::name_hint<char>` / `std::thread::stack_size_hint` 是 thread/jthread 构造前缀属性；`name_hint` 只约束 `char` 与编码，不要求 thread/jthread 对象保存可查询名称；`stack_size_hint(0)` 可被忽略。
- N5050：RCU 设施包含 `std::rcu_domain`、`std::rcu_default_domain()`、`std::rcu_synchronize()`、`std::rcu_barrier()`；`rcu_domain` 提供 `lock/try_lock/unlock`，示例用 `std::scoped_lock<rcu_domain>`，没有 `std::rcu_reader`。
- P3428R4 对 hazard pointer batch 的关键语义是：`make_hazard_pointer_batch(span<hazard_pointer>)` 对空元素构造 hazard pointer 并使其拥有；`clear_hazard_pointer_batch(span<hazard_pointer>)` 对非空元素销毁其拥有的 hazard pointer，并使元素变空。它清理句柄所有权，不释放已经受保护的业务对象本身。

## 阻断项

### P1：F02 的 `clear_hazard_pointer_batch` 语义反了

证据：

- `topics/frontier/02-hazard-pointer-batches.md` 把 `clear_hazard_pointer_batch` 解释为“清空保护，不释放句柄对象本身”。
- `exercises/F02_hazard_pointer_batches/main.cpp` 的模型 `clear_batch` 只把 `protects=false`，并在重复 clear 后断言 `handles[i].owns == true`。
- `exercises/F02_hazard_pointer_batches/solution.cpp` 在 native 分支调用 `clear_hazard_pointer_batch` 后检查 `!hps[0].empty() && !hps[1].empty()`，即期待 clear 后仍持有 hazard pointer。
- `exercises/F02_hazard_pointer_batches/README.md` 的题解没有明确“clear 后 span 内元素变 empty”，容易让学生按当前错误模型理解。

影响：当前 F02 model 可以稳定 PASS，但它验证的是反标准语义。未来编译器提供真实 C++29 batch 接口时，native reference 要么失败，要么被迫继续教授错误结论。这也属于“能力缺失时 SKIP 掩盖主体错误”的风险：当前平台没有 `<hazard_pointer>`，r3 证据只显示 native SKIP，无法证明 native 主体正确。

建议修复：

- 文档改为：`make_*` 获得/填充空句柄；`clear_*` 销毁 span 中非空 hazard pointer 并让元素 empty；被保护业务对象的释放仍由 retire/reclamation 决定。
- F02 模型改成 clear 后 `owns=false`、`protects=false`，重复 clear 保持 empty。
- native reference 改成 clear 后检查 `hps[i].empty()`，如需继续保护必须重新 `make_hazard_pointer_batch`。

### P1：F03 的 3 个 native subject reference target 忽略 `CONCURRENCY_STUDY_BUILD_REFERENCE=OFF`

证据：

- `exercises/F03_native_facilities/CMakeLists.txt` 中 `f03_add_native_subject` 直接 `add_executable(F03_${subject_name}_reference ...)` 和 `add_test(...)`，没有受 `CONCURRENCY_STUDY_BUILD_REFERENCE` 保护。
- 我用 ref-off 配置验证：`CONCURRENCY_STUDY_BUILD_REFERENCE=OFF` 时，`F03_native_facilities_reference` 不存在，但 `F03_std_senders_reference` 仍可构建。这说明 `cs_add_exercise` 的 reference 边界生效，手写 subject target 没有生效。

影响：Student-only / reference-off 构建边界被破坏。课程可在关闭参考实现时仍生成和运行 F03 标准设施参考目标。

建议修复：把 `f03_add_native_subject(...)` 调用或函数体整体包在 `if(CONCURRENCY_STUDY_BUILD_REFERENCE)` 内，保持与 `cs_add_exercise` reference 目标一致。

### P1/P2：`F03_native_facilities_reference` 是空总结 target，却在 CTest 中显示 PASS

证据：

- `exercises/F03_native_facilities/solution.cpp` 只打印“run the separate subject targets”并 `return 0`。
- `frontier-r3-ctest.xml` 和本地复跑都显示 `F03_native_facilities_reference` PASS；同一轮中真正的 `F03_std_senders_reference`、`F03_std_hazard_pointer_reference`、`F03_std_rcu_reference` 因当前 MSVC 缺能力 SKIP。

影响：审查或发布汇总如果只看 reference 标签/通过数，会把这个空总结 target 当成 F03 reference 通过。当前已有 separate subject target，空总结 target 不应承担参考实现验证含义。

建议修复：移除/禁用 `solution.cpp` 生成的 reference 测试，或把它降为 observation/说明目标，不计入 reference PASS。F03 的真实结论应只来自三个 native subject target 的 PASS/SKIP/FAIL。

## 次要问题

### P2：F01 文档声称检查重复属性，但 model 未覆盖

证据：

- `topics/frontier/01-thread-attributes.md` 写明 `main.cpp` 会检查“重复属性被拒绝”。
- `exercises/F01_thread_attributes/main.cpp` 的模型只记录 name/stack 是否出现在 callable 前，以及 callable 是否存在；没有表达“两个 name_hint”或“两个 stack_size_hint”的输入，也没有失败用例。
- `exercises/F01_thread_attributes/README.md` 答案说“同一属性类型不能重复”。

影响：F01 当前对接口形态、前缀位置、borrowed name、jthread stop 的验证基本足够，但重复属性这一教学主张没有练习证据支撑。

建议修复：补一个 duplicate-attribute model 用例，或收窄文档/答案，不声称 starter 已验证该点。

### Low：`NativeFeatures.cmake` 的 CXX26/CXX29 probe 循环重复

证据：`exercises/cmake/NativeFeatures.cmake` 中 CXX26 与 CXX29 probe 循环结构几乎一致。

影响：非阻断。当前 CXX26-only、CXX29-only、OFF 配置行为可验证；后续可在不改变证据格式的前提下合并 helper，降低维护成本。

## 已通过项

- F01 native solution 对 `std::thread::name_hint<char>`、`std::thread::stack_size_hint`、thread/jthread 属性前缀构造的接口形态与 N5054/P2019R9 对齐。
- F03 RCU probe/reference 使用 `std::rcu_default_domain()`、`lock/unlock`、`std::rcu_synchronize()`、`std::rcu_barrier()`，未再使用不存在的 `std::rcu_reader`。
- F03 senders / hazard pointer / RCU 已拆成独立 subject target；真实 subject 的 SKIP 没有被合并成一个总 PASS。但空总结 target 仍需处理，见上。
- CXX26 与 CXX29 probe option 已可独立开启/关闭；OFF 配置会写 DISABLED，不会误跑 native probe。

## 本地复验

```text
ctest --test-dir C08_Concurrency/exercises/build/c08-frontier-author-r3 -C Release -R "F0[123]_" --output-on-failure
=> 9 tests: 4 passed, 5 skipped, 0 failed, 0.24 sec
```

```text
cmake -S C08_Concurrency/exercises -B build/c08-frontier-review-cxx26-only -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX26=ON -DCONCURRENCY_STUDY_ENABLE_CXX29=OFF -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
=> Configure / Generate passed; CXX26 probes attempted, CXX29 probes DISABLED
```

```text
cmake -S C08_Concurrency/exercises -B build/c08-frontier-review-cxx29-only -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX26=OFF -DCONCURRENCY_STUDY_ENABLE_CXX29=ON -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/c08-frontier-review-cxx29-only --config Release --target F01_thread_attributes F01_thread_attributes_reference F02_hazard_pointer_batches F02_hazard_pointer_batches_reference F03_native_facilities F03_native_facilities_reference F03_std_senders_reference F03_std_hazard_pointer_reference F03_std_rcu_reference
ctest --test-dir build/c08-frontier-review-cxx29-only -C Release -R "F0[123]_" --output-on-failure
=> Configure / build passed; 9 tests: 4 passed, 5 skipped, 0 failed, 4.01 sec
```

```text
cmake -S C08_Concurrency/exercises -B build/c08-frontier-review-off -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX26=OFF -DCONCURRENCY_STUDY_ENABLE_CXX29=OFF -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
=> Configure / Generate passed; CXX26 and CXX29 probes DISABLED
```

```text
cmake -S C08_Concurrency/exercises -B build/c08-frontier-review-ref-off -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_BUILD_REFERENCE=OFF -DCONCURRENCY_STUDY_TEST_STARTERS=ON -DCONCURRENCY_STUDY_ENABLE_CXX26=OFF -DCONCURRENCY_STUDY_ENABLE_CXX29=OFF
cmake --build build/c08-frontier-review-ref-off --config Release --target F03_std_senders_reference
=> Build passed, proving this hand-written reference target exists under BUILD_REFERENCE=OFF

cmake --build build/c08-frontier-review-ref-off --config Release --target F03_native_facilities_reference
=> MSB3202 project file not found, proving cs_add_exercise-generated reference target is correctly absent under BUILD_REFERENCE=OFF
```

## 结论

REVISE。F01/F03 的方向基本正确，CXX26/CXX29 能力门也能独立工作；但 F02 batch clear 的标准语义错误是内容阻断，F03 reference-off 泄漏是构建边界阻断，F03 空总结 reference PASS 会污染验证口径。修完这些后，建议重新绑定文件 hash，并在同一套 F0[123] CTest、CXX26-only、CXX29-only、OFF、ref-off 配置下复验。
