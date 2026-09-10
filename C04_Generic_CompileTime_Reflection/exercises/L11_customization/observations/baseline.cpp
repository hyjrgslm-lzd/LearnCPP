#include <check.hpp>

#include <iostream>

namespace c04_l11_baseline {

struct Sensor {
    int value{};

    int& read_value() & noexcept { return value; }
    const int& read_value() const& noexcept { return value; }
};

int& read_known_sensor(Sensor& sensor) noexcept {
    return sensor.read_value();
}

const int& read_known_sensor(const Sensor& sensor) noexcept {
    return sensor.read_value();
}

} // namespace c04_l11_baseline

int main() {
    c04_l11_baseline::Sensor sensor{7};
    int& result = c04_l11_baseline::read_known_sensor(sensor);
    check(&result == &sensor.value, "known-type baseline keeps reference identity");
    result = 8;
    check(sensor.value == 8, "known-type baseline writes through original object");

    const c04_l11_baseline::Sensor const_sensor{9};
    const int& const_result = c04_l11_baseline::read_known_sensor(const_sensor);
    check(&const_result == &const_sensor.value, "known-type baseline keeps const reference identity");

    std::cout << "L11_customization baseline observation OK\n";
}
