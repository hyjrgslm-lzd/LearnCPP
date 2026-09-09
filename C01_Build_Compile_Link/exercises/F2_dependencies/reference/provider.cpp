#include "f2_provider/provider.hpp"

namespace f2_provider {
int api_version() {
    return F2_PROVIDER_VERSION;
}

int compute_answer(int input) {
    return input * 2 + 2;
}
}
