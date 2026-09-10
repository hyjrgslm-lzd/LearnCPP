# F02：C++29 Hazard Pointer Batch

本题对应 [HP batch 正文](../../topics/frontier/02-hazard-pointer-batches.md)。

## Part

| Part | 任务 | 检查 |
|---|---|---|
| A | 空 span 批量申请和清空 | 无操作且不改变现有状态 |
| B | 整批申请失败 | 已有句柄和 protection 保持原状态，符合强异常保证 |
| C | mixed span 重复 clear | owned HP 被销毁，元素变 empty，重复调用安全 |
| D | protect/retire 集成 | clear 不回收业务对象，退休和最终清场仍独立 |

`main.cpp` 运行课程模型，验证异常和状态边界。`solution.cpp` 只在 `CS_HAS_STD_HAZARD_POINTER_BATCH=1` 时运行标准主体；缺能力返回 77。原生主体覆盖空 span、非空 batch、一个保护/一个未关联的 mixed span、重复 clear 后 empty 和 protect/retire；不注入标准库 OOM，不强制证明回收队列排空。

## 运行

```powershell
cmake -S C08_Concurrency/exercises/F02_hazard_pointer_batches -B C08_Concurrency/exercises/build/c08-frontier-author/f02 -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX29=ON
cmake --build C08_Concurrency/exercises/build/c08-frontier-author/f02 --config Release
ctest --test-dir C08_Concurrency/exercises/build/c08-frontier-author/f02 -C Release --output-on-failure
```

## 答案要点

`make_hazard_pointer_batch` 不是“循环申请”的教学替身；它只填 empty 元素，已有非空 HP 连同 protection 保持不变，关键是整批失败时保持调用前状态。`clear_hazard_pointer_batch` 销毁 span 中非空元素拥有的 HP，并让元素 empty；它不是 `hp.reset_protection()`。`reset_protection()` 只解除当前保护，HP 对象仍拥有槽。clear 也不释放被保护业务对象，不等于回收域已经排空。算法仍要先 protect、验证结构关系，再 retire；batch 不生成一致快照。

一手来源：[N5055](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)、[P3428R4](https://wg21.link/P3428R4)。
