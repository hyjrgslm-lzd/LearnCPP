#include "construction_lab.hpp"

namespace l04 {

std::vector<std::string> observe_order() {
    return {};
}

ResourceOwner::ResourceOwner(l04_checks::Recorder& recorder, l04_checks::ResourceName name) {
    (void)recorder;
    (void)name;
}

ResourceOwner::~ResourceOwner() = default;

TwoResourceOwner::TwoResourceOwner(l04_checks::Recorder& recorder)
    : first(recorder, l04_checks::ResourceName::first),
      second(recorder, l04_checks::ResourceName::second) {
}

} // namespace l04
