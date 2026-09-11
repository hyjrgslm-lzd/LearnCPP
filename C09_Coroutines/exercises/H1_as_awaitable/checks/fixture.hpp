#pragma once

#include <stdexec/execution.hpp>

#include <type_traits>
#include <utility>

namespace ex = stdexec;

class h1_fixture {
public:
    template <class Sender>
    auto sender(Sender sender)
    {
        return traced_sender<std::remove_cvref_t<Sender>>{std::move(sender), state_};
    }

    bool saw_exactly(int started, int completed) const noexcept
    {
        return state_.started == started && state_.completed == completed;
    }

    int started() const noexcept { return state_.started; }
    int completed() const noexcept { return state_.completed; }

private:
    struct state {
        int started = 0;
        int completed = 0;
    };

    template <class Sender>
    class traced_sender {
    public:
        using sender_concept = ex::sender_tag;
        using completion_signatures = ex::completion_signatures<ex::set_value_t(int)>;

        template <class Receiver>
        struct op_state {
            using operation_state_concept = ex::operation_state_t;

        private:
            std::remove_cvref_t<Sender> sender;
            std::remove_cvref_t<Receiver> receiver;
            state* counters;

        public:
            op_state(Sender s, Receiver r, state* c)
                : sender(std::move(s)), receiver(std::move(r)), counters(c) {}

            void start() noexcept
            {
                ++counters->started;
                auto op = ex::connect(
                    std::move(sender) | ex::then([this](int v) noexcept {
                        ++counters->completed;
                        return v;
                    }),
                    std::move(receiver));
                ex::start(op);
            }
        };

        template <class Receiver>
        op_state<Receiver> connect(Receiver receiver) &&
        {
            return {std::move(sender_), std::move(receiver), counters_};
        }

    private:
        friend class h1_fixture;
        traced_sender(Sender sender, state& counters) : sender_(std::move(sender)), counters_(&counters) {}

        Sender sender_;
        state* counters_;
    };

    state state_;
};
