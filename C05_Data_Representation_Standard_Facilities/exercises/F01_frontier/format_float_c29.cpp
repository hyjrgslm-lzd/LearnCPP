#include <format>
#include <iostream>
#include <string>

int main() {
    const std::string hundred_thousand = std::format("{}", 100000.0);
    const std::string huge             = std::format("{}", 1234567890123456700000.0);

    if (hundred_thousand == "100000" && huge == "1.2345678901234568e+21") {
        std::cout << "PASS P3505-style default floating-point format\n";
        return 0;
    }

    if (hundred_thousand == "1e+05" && huge == "1234567890123456774144") {
        std::cout << "SKIP P3505-style default floating-point format not implemented; observed " << hundred_thousand
                  << " and " << huge << '\n';
        return 77;
    }

    std::cerr << "FAIL unexpected default floating-point format: " << hundred_thousand << " and " << huge << '\n';
    return 1;
}
