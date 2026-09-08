#include "debug_story.hpp"

namespace a1_reference {
int add_offset(int seed, int offset)
{
    int adjusted = seed + offset;
    return adjusted;
}

Trace compute_trace(int seed)
{
    int offset = 2;
    int adjusted = add_offset(seed, offset);
    int answer = adjusted * 2;
    return Trace{seed, offset, adjusted, answer};
}

int compute_answer(int seed)
{
    Trace trace = compute_trace(seed);
    return trace.answer;
}
}
