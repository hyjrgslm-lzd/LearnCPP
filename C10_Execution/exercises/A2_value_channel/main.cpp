#include <c10/test.hpp>
#include <solution.hpp>
#include <memory>
#include <string>

int main() {
  return c10::test_main([] {
    c10_a2::Trace trace;
    auto int_result = c10_a2::run_int_pipeline(trace);
    c10::require(int_result == 12, "int value channel transforms 5 to 12");
    c10::require(trace.events == std::vector<std::string>{"constructed", "before sync_wait",
                                                          "plus_one:5", "times_two:6",
                                                          "after sync_wait"},
                 "sender body stays lazy until sync_wait");

    auto task = c10_a2::run_struct_pipeline(c10_a2::TaskInput{10, "hello"});
    c10::require(task.base == 45, "struct value base transformed");
    c10::require(task.label == "hello_processed", "struct value label transformed");

    bool void_stage_called = false;
    auto after_void = c10_a2::run_void_pipeline([&] { void_stage_called = true; });
    c10::require(void_stage_called, "void stage executes");
    c10::require(after_void == 42,
                 "void stage sends empty value shape before new value is generated");

    auto moved = c10_a2::run_move_only(std::make_unique<int>(20));
    c10::require(moved == 21, "move-only value travels through value channel");
  });
}
