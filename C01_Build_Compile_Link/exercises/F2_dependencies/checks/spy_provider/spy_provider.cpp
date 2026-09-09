#include "spy_provider.hpp"
#include <f2_provider/provider.hpp>

namespace {
int calls = 0;
int last = 0;
}

namespace f2_provider {
int api_version() {
    return 901;
}

int compute_answer(int input) {
    ++calls;
    last = input;
    return 1000 + input;
}
}

namespace f2_provider_spy {
void reset() {
    calls = 0;
    last = 0;
}

int call_count() {
    return calls;
}

int last_input() {
    return last;
}
}
