#include <stdexec/execution.hpp>
#include <iostream>
#include <string>

namespace ex = stdexec;

// ============ Custom query tag ============
struct get_trace_id_t {
    // TODO [必做]: implement as a CPO using tag_invoke
    // When called on an environment, returns a std::string trace_id
    template <typename Env>
    auto operator()(const Env& env) const {
        // TODO [必做]: dispatch through tag_invoke
    }
};
inline constexpr get_trace_id_t get_trace_id{};

// ============ Custom environment ============
struct trace_env {
    std::string trace_id;
    // TODO [必做]: friend tag_invoke(get_trace_id_t, const trace_env&) -> std::string
};

// ============ override_env combinator ============
template <typename Base, typename Override>
struct override_env {
    Base base_;
    Override override_;

    // TODO [必做]: For each query, check Override first, then fallback to Base
    // Hint: use if constexpr + requires to check if Override responds to a query
};

// TODO [必做]: Factory function
template <typename Base, typename Override>
auto make_override_env(Base base, Override override_part) {
    return override_env<Base, Override>{std::move(base), std::move(override_part)};
}

// ============ Adaptor receiver that overrides get_scheduler ============
// TODO [进阶]: write a receiver wrapper that uses override_env in get_env()

int main() {
    trace_env env{"trace-abc-123"};

    // TODO [必做]: query the custom trace_id from env
    std::cout << "Trace ID: " << get_trace_id(env) << "\n";

    // TODO [必做]: create an override_env that overrides trace_id
    // auto combined = make_override_env(original_env, trace_env{"new-trace-456"});
    // Verify: get_trace_id(combined) returns "new-trace-456"
    // Verify: other queries fall through to original_env

    return 0;
}
