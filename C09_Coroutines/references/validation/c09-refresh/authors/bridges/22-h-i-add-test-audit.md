H/I manual add_test audit after H checker hardening.

Command:
rg -n "add_test\(|REFERENCE|reference" C09_Coroutines/exercises/H1_as_awaitable/CMakeLists.txt C09_Coroutines/exercises/H2_std_execution_task/CMakeLists.txt C09_Coroutines/exercises/H3_bidirectional_bridge/CMakeLists.txt C09_Coroutines/exercises/I1_asio_echo/CMakeLists.txt C09_Coroutines/exercises/I2_io_uring_iocp/CMakeLists.txt C09_Coroutines/exercises/I3_folly_safe_task/CMakeLists.txt C09_Coroutines/exercises/I4_cobalt_channel/CMakeLists.txt C09_Coroutines/exercises/I5_cppcoro_patterns/CMakeLists.txt

Result:
- H1/H3 starter add_test only under BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS.
- H1/H3 reference target/test only under COROUTINE_STUDY_BUILD_REFERENCE.
- H2 starter add_test only under BUILD_TESTING AND COROUTINE_STUDY_TEST_STARTERS.
- H2 probe add_test labeled probe, SKIP_RETURN_CODE 77.
- H2 reference target/test only under COROUTINE_STUDY_BUILD_REFERENCE AND H2_STDEXEC_TASK_REFERENCE_SUPPORTED.
- I1/I2/I3/I4/I5 use coroutine_study_add_exercise(... REFERENCE_SOURCES ... STUDENT_TEST); no hand-written add_test leaks in this slice.
- ReferenceOFF show-only JSON: 12-h-checkers-referenceoff-showonly.json. Parsed H tests: H1_as_awaitable, H2_std_execution_task, H2_std_task_probe, H3_bidirectional_bridge. Parsed reference-like entries: [].
