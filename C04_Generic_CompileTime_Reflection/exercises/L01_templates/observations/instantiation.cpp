#include <check.hpp>

#include <iostream>

namespace l01_observation {

template<class T>
struct lazy_probe {
    T value{};

    constexpr int ordinary() const {
        return 1;
    }

    constexpr auto only_valid_for_pointer() const {
        return *value;
    }
};

void run() {
    lazy_probe<int> probe{42};
    check(probe.ordinary() == 1, "unused dependent member body is not instantiated");

    int value = 7;
    lazy_probe<int*> pointer_probe{&value};
    check(pointer_probe.only_valid_for_pointer() == 7, "dependent member is checked when used with a valid type");
}

} // namespace l01_observation

int main() {
    l01_observation::run();
    std::cout << "L01_templates instantiation observation OK\n";
}
