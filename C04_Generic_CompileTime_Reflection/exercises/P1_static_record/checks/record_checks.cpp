#include <record_ops.hpp>
#include "record_checks.hpp"
#include <iostream>
int main() {
    c04_record::checks::run<c04_record::implementation>();
    std::cout << "P1 static record contract passed\n";
}
