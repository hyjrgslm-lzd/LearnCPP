R3 checker hardening audit.

Old R3 failures addressed:
- H2 fake could write task_trace->{1,1,1} and return constant observations. Fixed by deleting task_trace* from the student API. Checker counters now live in lambda captures and checker-created stdexec::task coroutine frames: first_started, worker_ran, stopped_started, token_queries.
- H1 fake could read sender_.trace and write counters. Fixed by moving traced_sender state into h1_fixture private nested types. Student receives only a generic Sender and can use legal connect/start.
- H3 fake could write bridge sender trace. Fixed by moving bridge sender state into h3_bridge_fixture private nested types. Task body counters are held in checker-local state and are not function parameters to my_task bodies.

Runtime evidence:
- 28-r3-h-checkers-build.json: all H1/H2/H3 starter, good, bad_constant, probe targets built in ReferenceON variant build.
- 30/32/35 r3 good executables return 0.
- 31/33/36 r3 bad_constant executables return 1 while still attempting value-level cheating. H2 returns correct observations but counters stay 0/0/0/0. H3 returns correct values 31/22/41 but child/bridge sender counters are missing.
- 40-r3-referenceon-variant-ctest.json: good tests and bad_constant_rejected tests pass through the common supervision harness.
- 37/38 r3 ReferenceOFF starter build/ctest: starter tests finite-fail, H2 probe skips with 77.
- 39-r3-referenceoff-showonly.json: H tests registered under ReferenceOFF are H1_as_awaitable, H2_std_execution_task, H2_std_task_probe, H3_bidirectional_bridge; no reference-like entries.

Compile-negative boundary:
- 24-H1-fake-trace-compile-negative.json: old fake trace member access fails to compile because h1_fixture::traced_sender has no public trace member.
- 25-H2-fake-trace-compile-negative.json: old task_trace* API no longer matches the checker call.
- 26-H3-fake-trace-compile-negative.json: old bridge sender trace member access fails to compile because h3_bridge_fixture::traced_sender has no public trace member.

No H CMake changes were made in this R3 pass.
