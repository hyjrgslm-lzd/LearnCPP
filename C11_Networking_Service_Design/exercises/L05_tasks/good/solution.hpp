#pragma once
#include <c11/task_types.hpp>
#include <algorithm>
namespace exercise {
struct policy {
    static c11::tasks::result<c11::tasks::admission> admit(const c11::tasks::submission_context& c) {
        using namespace c11::tasks;
        const auto valid_char=[](unsigned char x){return (x>='0'&&x<='9')||(x>='A'&&x<='Z')||(x>='a'&&x<='z')||x=='-'||x=='_'||x=='.';};
        if(c.key.size()<1 || c.key.size()>64 || !std::all_of(c.key.begin(),c.key.end(),valid_char))return std::unexpected(error::invalid);
        if(c.input.steps>1000 || c.input.budget_ms<1 || c.input.budget_ms>60000)return std::unexpected(error::invalid);
        for(auto x:{c.input.left,c.input.right})if(x< -1000000 || x>1000000)return std::unexpected(error::invalid);
        if(c.previous) return c.previous->input==c.input?result<admission>(admission::reuse):result<admission>(std::unexpected(error::conflict));
        if(c.draining)return std::unexpected(error::shutting_down);
        if(c.records==c.bound.records || c.live==c.bound.live)return std::unexpected(error::overloaded);
        return admission::create;
    }
    static c11::tasks::state finish(std::optional<c11::tasks::state> stop,bool failed) {
        if(stop)return *stop;
        if(failed)return c11::tasks::state::failed;
        return c11::tasks::state::succeeded;
    }
};
}
