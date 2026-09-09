#include <utility>

void only_rvalue(int&&) {}

template <class T>
void bad_forward(T&& value) {
    only_rvalue(value);
}

int main() {
    int value = 1;
    bad_forward(value);
}
