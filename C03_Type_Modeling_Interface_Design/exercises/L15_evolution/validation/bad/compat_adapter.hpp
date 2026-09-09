#pragma once

#include <v2_engine.hpp>

#include <string>
#include <string_view>

namespace l15 {

class CompatClient {
public:
    std::string fetch(std::string_view name) const
    {
        return engine_.fetch(name);
    }

    std::string fetch(std::string_view name, int timeout_ms) const
    {
        return engine_.fetch(name, timeout_ms, v2::Format::legacy);
    }

    std::string fetch_compact(std::string_view name) const
    {
        return engine_.fetch(name);
    }

private:
    v2::Engine engine_{};
};

} // namespace l15
