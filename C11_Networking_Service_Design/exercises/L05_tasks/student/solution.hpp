#pragma once
#include <c11/task_types.hpp>
#include <stdexcept>
namespace exercise {
struct policy {
    static c11::tasks::result<c11::tasks::admission> admit(const c11::tasks::submission_context&) {
        throw std::logic_error("UNFINISHED: admission/idempotency policy");
    }
    static c11::tasks::state finish(std::optional<c11::tasks::state>,bool) {
        throw std::logic_error("UNFINISHED: terminal decision");
    }
};
}
