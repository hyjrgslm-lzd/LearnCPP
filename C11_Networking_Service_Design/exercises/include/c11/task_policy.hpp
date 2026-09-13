#pragma once
#include <c11/task_types.hpp>
namespace c11::tasks {
struct task_policy {
    static result<admission> admit(const submission_context& c) {
        if(c.key.empty() || c.key.size()>64 || c.input.left< -1000000 || c.input.left>1000000 ||
            c.input.right< -1000000 || c.input.right>1000000 || c.input.steps>1000 || !c.input.budget_ms || c.input.budget_ms>60000)
            return std::unexpected(error::invalid);
        for(unsigned char x:c.key) if(!((x>='a'&&x<='z')||(x>='A'&&x<='Z')||(x>='0'&&x<='9')||x=='-'||x=='_'||x=='.'))
            return std::unexpected(error::invalid);
        if(c.previous) {
            if(c.previous->input!=c.input) return std::unexpected(error::conflict);
            return admission::reuse;
        }
        if(c.draining) return std::unexpected(error::shutting_down);
        if(c.live>=c.bound.live || c.records>=c.bound.records) return std::unexpected(error::overloaded);
        return admission::create;
    }
    static state finish(std::optional<state> reason,bool failed) {
        return reason.value_or(failed?state::failed:state::succeeded);
    }
};
}
