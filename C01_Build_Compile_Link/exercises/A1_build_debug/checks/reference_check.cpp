#include "check.hpp"
#include "debug_story.hpp"

int main()
{
    const auto first = a1_reference::compute_trace(19);
    check(first.seed == 19, "seed is preserved in the trace");
    check(first.offset == 2, "offset is initialized before use");
    check(first.adjusted == 21, "add_offset participates in the call path");
    check(first.answer == 42, "first answer is computed from the call path");
    check(a1_reference::compute_answer(20) == 44, "second input uses the same computation, not a constant answer");
}
