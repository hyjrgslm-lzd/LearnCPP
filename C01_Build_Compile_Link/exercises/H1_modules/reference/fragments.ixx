module;
#include <cstdint>

export module h1_fragments;

export int fragment_answer();

module :private;

std::int32_t fragment_helper() {
    return 42;
}

int fragment_answer() {
    return static_cast<int>(fragment_helper());
}
