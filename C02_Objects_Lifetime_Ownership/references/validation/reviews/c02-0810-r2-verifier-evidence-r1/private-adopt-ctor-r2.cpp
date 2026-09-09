#include <rc.hpp>

int main() {
    l10::rc_ptr<int> p(nullptr, false);
    return static_cast<bool>(p) ? 0 : 1;
}
