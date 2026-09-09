#pragma once

#include <v2_engine.hpp>

#include <string>
#include <string_view>

namespace l15 {

class CompatClient {
public:
    std::string fetch(std::string_view) const { return "student placeholder"; }
    std::string fetch(std::string_view, int) const { return "student placeholder"; }
    std::string fetch_compact(std::string_view) const { return "student placeholder"; }

private:
    v2::Engine engine_{};
};

} // namespace l15
