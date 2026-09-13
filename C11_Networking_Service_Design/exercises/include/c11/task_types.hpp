#pragma once
#include <chrono>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string_view>
namespace c11::tasks {
using clock=std::chrono::steady_clock;
using time=clock::time_point;
using milliseconds=std::chrono::milliseconds;
using id=std::uint64_t;
enum class state { queued,running,succeeded,failed,cancelled,expired };
inline bool terminal(state s) {return s==state::succeeded || s==state::failed || s==state::cancelled || s==state::expired;}
inline std::string_view state_name(state s) {
    switch(s){case state::queued:return "queued";case state::running:return "running";case state::succeeded:return "succeeded";
    case state::failed:return "failed";case state::cancelled:return "cancelled";case state::expired:return "expired";}
    return "invalid";
}
enum class error { invalid,overloaded,conflict,not_found,shutting_down,id_exhausted,slow_consumer,request_cancelled,request_timeout };
template<class T> using result=std::expected<T,error>;
struct spec {
    std::int32_t left=0,right=0;
    std::uint32_t steps=0,budget_ms=5000;
    bool operator==(const spec&) const=default;
};
struct snapshot {
    id task=0;state phase=state::queued;std::optional<std::int64_t> value;
    std::uint32_t progress=0,revision=0;bool stop_requested=false;
};
struct limits {
    std::size_t live=128,records=1024,watches=32,watch_queue=8;
    milliseconds retention{300000};
    id id_ceiling=std::numeric_limits<id>::max();
};
struct previous_submission {spec input;snapshot current;};
struct submission_context {
    const spec& input;std::string_view key;const previous_submission* previous;
    bool draining;std::size_t live,records;const limits& bound;
};
enum class admission { create,reuse };
enum class worker_error { computation_failed };
using completion=std::expected<std::int64_t,worker_error>;
}
