#pragma once
#include <c11/task_types.hpp>
#include <boost/json.hpp>
namespace c11::tasks {
inline boost::json::object json(const snapshot& s) {
    boost::json::object out{{"id",s.task},{"state",state_name(s.phase)},{"progress",s.progress},{"revision",s.revision},{"stop_requested",s.stop_requested}};
    if(s.value)out["result"]=*s.value;
    return out;
}
inline result<spec> parse_spec(std::string_view input) {
    boost::system::error_code error;
    auto parsed=boost::json::parse(input,error);
    if(error || !parsed.is_object())return std::unexpected(tasks::error::invalid);
    const auto& object=parsed.as_object();
    spec request;
    for(const auto& item:object)if(item.key()!="left" && item.key()!="right" && item.key()!="steps" && item.key()!="budget_ms")return std::unexpected(tasks::error::invalid);
    const auto number=[&](const char* name,std::int64_t fallback)->std::optional<std::int64_t>{
        const auto* v=object.if_contains(name);if(!v)return fallback;
        if(!v->is_int64())return {};return v->as_int64();
    };
    if(!object.contains("left") || !object.contains("right"))return std::unexpected(tasks::error::invalid);
    auto left=number("left",0),right=number("right",0),steps=number("steps",0),budget=number("budget_ms",5000);
    if(!left || !right || !steps || !budget || *left< -1000000 || *left>1000000 || *right< -1000000 || *right>1000000 ||
       *steps<0 || *steps>1000 || *budget<1 || *budget>60000)return std::unexpected(tasks::error::invalid);
    request.left=static_cast<std::int32_t>(*left);request.right=static_cast<std::int32_t>(*right);
    request.steps=static_cast<std::uint32_t>(*steps);request.budget_ms=static_cast<std::uint32_t>(*budget);return request;
}
}
