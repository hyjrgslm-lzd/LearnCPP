#pragma once
#include <c11/task_policy.hpp>
namespace exercise {
struct policy:c11::tasks::task_policy {
    // Deliberate bug: a late successful worker result overwrites accepted cancellation.
    static c11::tasks::state finish(std::optional<c11::tasks::state>,bool failed) {
        return failed?c11::tasks::state::failed:c11::tasks::state::succeeded;
    }
};
}
