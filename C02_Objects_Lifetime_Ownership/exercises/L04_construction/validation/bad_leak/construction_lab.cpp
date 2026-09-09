#include "construction_lab.hpp"

namespace l04 {

std::vector<std::string> observe_order() {
    return {
        "Base()",
        "Member first()",
        "Member second()",
        "Derived body",
        "~Derived body",
        "~Member second()",
        "~Member first()",
        "~Base()",
    };
}

ResourceOwner::ResourceOwner(l04_checks::Recorder& recorder, l04_checks::ResourceName name)
    : recorder_(&recorder), name_(name), handle_(recorder.acquire(name)) {}

ResourceOwner::~ResourceOwner() {
    recorder_->release(name_, handle_);
}

TwoResourceOwner::TwoResourceOwner(l04_checks::Recorder& input)
    : recorder(&input),
      first(input.acquire(l04_checks::ResourceName::first)),
      second(input.acquire(l04_checks::ResourceName::second)) {
}

TwoResourceOwner::~TwoResourceOwner() {
    recorder->release(l04_checks::ResourceName::second, second);
    recorder->release(l04_checks::ResourceName::first, first);
}

} // namespace l04
