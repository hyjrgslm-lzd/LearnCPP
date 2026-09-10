# C08 本轮交付文件清单

[完整逐文件清单与SHA256](revision-delivery-manifest.json)包含本轮变更、每文件说明和字节指纹。二进制、build目录、第三方checkout、WSL镜像及原先八个无关dirty文件不在交付清单。

清单自身和最终集成审查/核验文件是收尾证明文件，单独回读验证以避免自引用哈希；不以文件存在代替测试通过。原始验证记录中的本机路径保留原义，可随仓库读取的证据由报告中的相对链接和public-index定位。

本轮内容清单：2205个文件；下面列出81个主要内容/源码/说明文件，详细过程证据在JSON中逐项列出。

| 文件 | 变更 | 内容 |
|---|---|---|
| [C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md](../../C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/IMPLEMENTATION.md](../IMPLEMENTATION.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/README.md](../README.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/exercises/B3_call_once/solution.cpp](../exercises/B3_call_once/solution.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/BUILD_GUIDE.md](../exercises/BUILD_GUIDE.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/exercises/CMakeLists.txt](../exercises/CMakeLists.txt) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/CMakePresets.json](../exercises/CMakePresets.json) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/Capstone3_parallel_compute/checks.hpp](../exercises/Capstone3_parallel_compute/checks.hpp) | added | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/Capstone3_parallel_compute/main.cpp](../exercises/Capstone3_parallel_compute/main.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/Capstone3_parallel_compute/reference.hpp](../exercises/Capstone3_parallel_compute/reference.hpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/F01_thread_attributes/CMakeLists.txt](../exercises/F01_thread_attributes/CMakeLists.txt) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F01_thread_attributes/README.md](../exercises/F01_thread_attributes/README.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F01_thread_attributes/main.cpp](../exercises/F01_thread_attributes/main.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F01_thread_attributes/solution.cpp](../exercises/F01_thread_attributes/solution.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F02_hazard_pointer_batches/CMakeLists.txt](../exercises/F02_hazard_pointer_batches/CMakeLists.txt) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F02_hazard_pointer_batches/README.md](../exercises/F02_hazard_pointer_batches/README.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F02_hazard_pointer_batches/main.cpp](../exercises/F02_hazard_pointer_batches/main.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F02_hazard_pointer_batches/solution.cpp](../exercises/F02_hazard_pointer_batches/solution.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F03_native_facilities/CMakeLists.txt](../exercises/F03_native_facilities/CMakeLists.txt) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F03_native_facilities/README.md](../exercises/F03_native_facilities/README.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F03_native_facilities/std_hazard_pointer.cpp](../exercises/F03_native_facilities/std_hazard_pointer.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F03_native_facilities/std_rcu.cpp](../exercises/F03_native_facilities/std_rcu.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F03_native_facilities/std_senders.cpp](../exercises/F03_native_facilities/std_senders.cpp) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp](../exercises/F3_seqcst_fence/solution.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/L3_par_vs_seq_bench/checks.hpp](../exercises/L3_par_vs_seq_bench/checks.hpp) | added | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/L3_par_vs_seq_bench/main.cpp](../exercises/L3_par_vs_seq_bench/main.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/L3_par_vs_seq_bench/reference.hpp](../exercises/L3_par_vs_seq_bench/reference.hpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/M1_work_stealing_pool/README.md](../exercises/M1_work_stealing_pool/README.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/exercises/M1_work_stealing_pool/solution.cpp](../exercises/M1_work_stealing_pool/solution.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/U01_async_logging/CMakeLists.txt](../exercises/U01_async_logging/CMakeLists.txt) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/README.md](../exercises/U01_async_logging/README.md) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/check_main.cpp](../exercises/U01_async_logging/check_main.cpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/checks.hpp](../exercises/U01_async_logging/checks.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/expect_failure.cmake](../exercises/U01_async_logging/expect_failure.cmake) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/main.cpp](../exercises/U01_async_logging/main.cpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/solution.cpp](../exercises/U01_async_logging/solution.cpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/src/reference/async_logging_submission.hpp](../exercises/U01_async_logging/src/reference/async_logging_submission.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/src/student/async_logging_submission.hpp](../exercises/U01_async_logging/src/student/async_logging_submission.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/validation/bad_flush/async_logging_submission.hpp](../exercises/U01_async_logging/validation/bad_flush/async_logging_submission.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/validation/bad_overflow/async_logging_submission.hpp](../exercises/U01_async_logging/validation/bad_overflow/async_logging_submission.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/validation/evidence-20260910-async-logging.json](../exercises/U01_async_logging/validation/evidence-20260910-async-logging.json) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/validation/evidence-20260910-timeout-r2.json](../exercises/U01_async_logging/validation/evidence-20260910-timeout-r2.json) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/U01_async_logging/validation/good/async_logging_submission.hpp](../exercises/U01_async_logging/validation/good/async_logging_submission.hpp) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/exercises/benchmarks/CMakeLists.txt](../exercises/benchmarks/CMakeLists.txt) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/benchmarks/queue_diagnostics.cpp](../exercises/benchmarks/queue_diagnostics.cpp) | added | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/cmake/NativeFeatures.cmake](../exercises/cmake/NativeFeatures.cmake) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/cmake/Sanitizers.cmake](../exercises/cmake/Sanitizers.cmake) | added | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/cmake/SpdlogSetup.cmake](../exercises/cmake/SpdlogSetup.cmake) | added | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/cmake/StudySetup.cmake](../exercises/cmake/StudySetup.cmake) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/cmake/feature_probes.cpp](../exercises/cmake/feature_probes.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/include/concurrency_study/queue_baseline.hpp](../exercises/include/concurrency_study/queue_baseline.hpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/include/concurrency_study/queue_linked.hpp](../exercises/include/concurrency_study/queue_linked.hpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/include/concurrency_study/queue_versions.hpp](../exercises/include/concurrency_study/queue_versions.hpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/runtime_tests/CMakeLists.txt](../exercises/runtime_tests/CMakeLists.txt) | modified | 构建、依赖、Sanitizer或测试登记 |
| [C08_Concurrency/exercises/runtime_tests/scheduling_allocation_test.cpp](../exercises/runtime_tests/scheduling_allocation_test.cpp) | added | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp](../exercises/runtime_tests/scheduling_test.cpp) | modified | 实现、兼容修复、检查器或检测边界 |
| [C08_Concurrency/exercises/tools/run_benchmarks.py](../exercises/tools/run_benchmarks.py) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/exercises/tools/verify_students.py](../exercises/tools/verify_students.py) | added | 可重放学生验证与独立性检查 |
| [C08_Concurrency/exercises/validation/README.md](../exercises/validation/README.md) | added | 可重放学生验证与独立性检查 |
| [C08_Concurrency/references/coverage.md](coverage.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/references/measurements/c08-revision-final/README.md](measurements/c08-revision-final/README.md) | added | 测量数据、诊断或结果解释 |
| [C08_Concurrency/references/measurements/c08-revision-queue-evidence/README.md](measurements/c08-revision-queue-evidence/README.md) | added | 测量数据、诊断或结果解释 |
| [C08_Concurrency/references/revision-audit-20260910.md](revision-audit-20260910.md) | added | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/references/revision-plan-20260910.md](revision-plan-20260910.md) | added | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/references/revision-quality-report-20260910.md](revision-quality-report-20260910.md) | added | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/references/standards-and-implementations.md](standards-and-implementations.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/references/wsl-validation.md](wsl-validation.md) | added | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/frontier/01-thread-attributes.md](../topics/frontier/01-thread-attributes.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/topics/frontier/02-hazard-pointer-batches.md](../topics/frontier/02-hazard-pointer-batches.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/topics/frontier/03-native-facilities.md](../topics/frontier/03-native-facilities.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/topics/frontier/README.md](../topics/frontier/README.md) | added | 前沿正文、模型、原生主体或能力门控 |
| [C08_Concurrency/topics/logging/01-async-spdlog.md](../topics/logging/01-async-spdlog.md) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/topics/logging/02-spdlog-source-reading.md](../topics/logging/02-spdlog-source-reading.md) | added | 异步日志教学、实现与有效检查 |
| [C08_Concurrency/topics/performance/c08-revision-queue-evidence.md](../topics/performance/c08-revision-queue-evidence.md) | added | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/queues/01-mutex-baseline.md](../topics/queues/01-mutex-baseline.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/queues/02-bounded-and-batch.md](../topics/queues/02-bounded-and-batch.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/queues/03-spsc.md](../topics/queues/03-spsc.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/queues/08-validation-and-benchmark.md](../topics/queues/08-validation-and-benchmark.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [C08_Concurrency/topics/queues/VALIDATION.md](../topics/queues/VALIDATION.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [LEARNCPP_GLOBAL_PLAN.md](../../LEARNCPP_GLOBAL_PLAN.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
| [README.md](../../README.md) | modified | 课程导航、覆盖、操作说明或交付状态 |
