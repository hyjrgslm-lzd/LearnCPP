#pragma once
#include <stdexec/execution.hpp>

#include <exception>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace c10_c1_8 {

namespace ex = stdexec;

struct cancellation_report {
  bool before_start_stopped{};
  bool during_run_stopped{};
  int after_completion_value{};
  std::string error_message;
  std::vector<std::string> events;
};

struct cancel_env {
  std::stop_token token;
  std::stop_source *controller{};
};

enum class phase { pre, mid, post, fail };

struct receiver_box {
  using receiver_concept = ex::receiver_tag;
  int *value{};
  bool *valued{};
  bool *stopped{};
  std::string *error{};
  cancel_env env;
  void set_value(int v) && noexcept {
    *value = v;
    *valued = true;
  }
  void set_error(std::exception_ptr ep) && noexcept {
    try {
      if (ep)
        std::rethrow_exception(ep);
    } catch (const std::exception &e) {
      *error = e.what();
    }
  }
  void set_stopped() && noexcept { *stopped = true; }
  auto get_env() const noexcept -> cancel_env { return env; }
};

struct step_sender {
  using sender_concept = ex::sender_tag;
  phase where{};
  std::vector<std::string> *log{};

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct state {
    using operation_state_concept = ex::operation_state_tag;
    phase where{};
    std::vector<std::string> *log{};
    Receiver out;
    state(phase p, std::vector<std::string> *l, Receiver r) : where(p), log(l), out(std::move(r)) {}
    state(const state &) = delete;
    state(state &&) = delete;
    void start() & noexcept {
      auto env = ex::get_env(out);
      if (where == phase::fail) {
        ex::set_error(std::move(out), std::make_exception_ptr(std::runtime_error("boom")));
        return;
      }
      if (where == phase::pre) {
        log->push_back("request-before-start");
        env.controller->request_stop();
      }
      for (int i = 0; i != 3; ++i) {
        if (env.token.stop_requested()) {
          ex::set_stopped(std::move(out));
          return;
        }
        if (where == phase::mid && i == 2) {
          log->push_back("request-during-step-2");
          env.controller->request_stop();
        }
      }
      if (env.token.stop_requested())
        ex::set_stopped(std::move(out));
      else
        ex::set_value(std::move(out), 3);
    }
  };

  template <class Receiver> auto connect(Receiver r) const {
    return state<std::remove_cvref_t<Receiver>>{where, log, std::move(r)};
  }
};

inline bool probe_stop(phase p, std::vector<std::string> &events) {
  std::stop_source src;
  int value = 0;
  bool got = false;
  bool stopped = false;
  std::string error;
  auto op = ex::connect(step_sender{p, &events},
                        receiver_box{&value, &got, &stopped, &error, {src.get_token(), &src}});
  ex::start(op);
  return stopped && !got;
}

inline auto run_cancellation_story() -> cancellation_report {
  cancellation_report r;
  r.before_start_stopped = probe_stop(phase::pre, r.events);
  r.during_run_stopped = probe_stop(phase::mid, r.events);
  std::stop_source after;
  bool got_after = false;
  bool stopped_after = false;
  std::string after_error;
  auto op =
      ex::connect(step_sender{phase::post, &r.events}, receiver_box{&r.after_completion_value,
                                                                    &got_after,
                                                                    &stopped_after,
                                                                    &after_error,
                                                                    {after.get_token(), &after}});
  ex::start(op);
  r.events.push_back("request-after-completion");
  after.request_stop();
  std::stop_source fail;
  int ignored = 0;
  bool got = false;
  bool stopped = false;
  auto fail_op = ex::connect(
      step_sender{phase::fail, &r.events},
      receiver_box{&ignored, &got, &stopped, &r.error_message, {fail.get_token(), &fail}});
  ex::start(fail_op);
  return r;
}

} // namespace c10_c1_8