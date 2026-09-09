#include "construction_lab.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace l04 {
namespace {

struct OrderLog {
    std::array<char const*, 8> events{};
    std::size_t count = 0;

    void push(char const* event) noexcept {
        if (count < events.size()) {
            events[count] = event;
        }
        ++count;
    }

    std::vector<std::string> to_vector() const {
        std::vector<std::string> result;
        result.reserve(count);
        for (std::size_t index = 0; index < count && index < events.size(); ++index) {
            result.emplace_back(events[index]);
        }
        return result;
    }
};

struct TraceBase {
    explicit TraceBase(OrderLog& events) : events_(&events) {
        events_->push("Base()");
    }

    ~TraceBase() {
        events_->push("~Base()");
    }

    OrderLog* events_;
};

struct TraceMember {
    TraceMember(OrderLog& events, char const* name)
        : events_(&events), is_first_(name == std::string_view("first")) {
        events_->push(is_first_ ? "Member first()" : "Member second()");
    }

    ~TraceMember() {
        events_->push(is_first_ ? "~Member first()" : "~Member second()");
    }

    OrderLog* events_;
    bool is_first_;
};

struct TraceDerived : TraceBase {
    TraceMember first;
    TraceMember second;
    OrderLog* events;

    explicit TraceDerived(OrderLog& output)
        : TraceBase(output), second(output, "second"), first(output, "first"), events(&output) {
        events->push("Derived body");
    }

    ~TraceDerived() {
        events->push("~Derived body");
    }
};

} // namespace

std::vector<std::string> observe_order() {
    OrderLog events;
    {
        TraceDerived derived(events);
    }
    return events.to_vector();
}

ResourceOwner::ResourceOwner(l04_checks::Recorder& recorder, l04_checks::ResourceName name)
    : recorder_(&recorder), name_(name), handle_(recorder.acquire(name)) {}

ResourceOwner::~ResourceOwner() {
    recorder_->release(name_, handle_);
}

TwoResourceOwner::TwoResourceOwner(l04_checks::Recorder& recorder)
    : first(recorder, l04_checks::ResourceName::first),
      second(recorder, l04_checks::ResourceName::second) {
}

} // namespace l04
