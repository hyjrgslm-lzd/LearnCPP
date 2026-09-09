#pragma once

#include "l04_checks.hpp"

namespace l04 {

std::vector<std::string> observe_order();

class ResourceOwner {
public:
    ResourceOwner(l04_checks::Recorder& recorder, l04_checks::ResourceName name);
    ~ResourceOwner();

    ResourceOwner(ResourceOwner const&) = delete;
    ResourceOwner& operator=(ResourceOwner const&) = delete;
};

struct TwoResourceOwner {
    ResourceOwner first;
    ResourceOwner second;

    explicit TwoResourceOwner(l04_checks::Recorder& recorder);
};

} // namespace l04
