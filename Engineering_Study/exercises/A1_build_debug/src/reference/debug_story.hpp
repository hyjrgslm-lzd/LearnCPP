#pragma once

namespace a1_reference {
struct Trace {
    int seed;
    int offset;
    int adjusted;
    int answer;
};

int add_offset(int seed, int offset);
Trace compute_trace(int seed);
int compute_answer(int seed);
}
