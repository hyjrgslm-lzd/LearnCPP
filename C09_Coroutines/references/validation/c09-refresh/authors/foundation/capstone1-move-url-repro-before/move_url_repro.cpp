#include <iostream>
#include <stdexcept>
#include <string>

struct UrlRecord {
    std::string url;
    std::string body;
};

struct FetchResult {
    std::string url;
    bool error = false;
};

FetchResult original_shape(UrlRecord rec) {
    try {
        [[maybe_unused]] std::string consumed = std::move(rec.url);
        throw std::runtime_error("injected submit failure after moving record");
    } catch (...) {
        return FetchResult{rec.url, true};
    }
}

int main() {
    auto result = original_shape(UrlRecord{"data/fail.csv", "body"});
    std::cout << "observed_url='" << result.url << "'\n";
    if (result.url == "data/fail.csv") return 0;
    return 7;
}
