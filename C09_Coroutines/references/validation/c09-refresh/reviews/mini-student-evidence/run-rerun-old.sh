set -euo pipefail
cd /mnt/f/CPPTrain/LearnCPP
BUILD=C09_Coroutines/exercises/build/c09-review-mini-rerun
REC=C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/records
GOOD=C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/good/include
BAD_WA=C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/bad_when_any_no_drain/include
BAD_SH=C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/bad_shared_single_waiter/include
BAD_EX=C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/bad_executor_inline/include
STU=C09_Coroutines/exercises/Capstone5_mini_corolib/include
TST=C09_Coroutines/exercises/Capstone5_mini_corolib/tests
COMMON=C09_Coroutines/exercises/include
mkdir -p "$BUILD" "$REC"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$GOOD" -I "$STU" -I "$TST" -I "$COMMON" "$TST/when_any_test.cpp" -o "$BUILD/good_when_any_test"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$GOOD" -I "$STU" -I "$TST" -I "$COMMON" "$TST/shared_task_test.cpp" -o "$BUILD/good_shared_task_test"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$GOOD" -I "$STU" -I "$TST" -I "$COMMON" "$TST/run_loop_test.cpp" -o "$BUILD/good_run_loop_test"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$BAD_WA" -I "$STU" -I "$TST" -I "$COMMON" "$TST/when_any_test.cpp" -o "$BUILD/bad_when_any_no_drain_test"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$BAD_SH" -I "$STU" -I "$TST" -I "$COMMON" "$TST/shared_task_test.cpp" -o "$BUILD/bad_shared_single_waiter_test"
g++ -std=c++20 -Wall -Wextra -pedantic -I "$BAD_EX" -I "$STU" -I "$TST" -I "$COMMON" "$TST/run_loop_test.cpp" -o "$BUILD/bad_executor_inline_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_good_when_any --records "$REC" --timeout 30 --contains "when_any:" -- "$BUILD/good_when_any_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_good_shared_task --records "$REC" --timeout 30 --contains "shared_task:" -- "$BUILD/good_shared_task_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_good_run_loop --records "$REC" --timeout 30 --contains "run_loop:" -- "$BUILD/good_run_loop_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_bad_when_any_no_drain_rejected --records "$REC" --timeout 30 --expect-exit 1 --contains "winner must cancel and wait for loser" -- "$BUILD/bad_when_any_no_drain_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_bad_shared_single_waiter_rejected --records "$REC" --timeout 30 --expect-exit 1 --contains "all shared waiters must resume" -- "$BUILD/bad_shared_single_waiter_test"
python3 C07_OS_Memory_System_IO/exercises/tools/run_test.py --name rerun_bad_executor_inline_rejected --records "$REC" --timeout 30 --expect-exit 1 --contains "schedule must queue, not resume inline" -- "$BUILD/bad_executor_inline_test"