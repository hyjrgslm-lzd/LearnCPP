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

struct stop_env {
  std::stop_token token;
  std::stop_source *source{};
};

struct int_receiver {
  using receiver_concept = ex::receiver_tag;
  int *value{};
  bool *has_value{};
  bool *stopped{};
  std::string *error{};
  stop_env env;

  void set_value(int v) && noexcept {
    *value = v;
    *has_value = true;
  }
  void set_error(std::exception_ptr ep) && noexcept {
    try {
      if (ep)
        std::rethrow_exception(ep);
    } catch (const std::exception &err) {
      *error = err.what();
    }
  }
  void set_stopped() && noexcept { *stopped = true; }
  auto get_env() const noexcept -> stop_env { return env; }
};

enum class scenario { before_start, during_run, after_completion, error };

struct cooperative_sender {
  using sender_concept = ex::sender_tag;

  scenario which{};
  std::vector<std::string> *events{};

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> struct op {
    using operation_state_concept = ex::operation_state_tag;
    scenario which{};
    std::vector<std::string> *events{};
    Receiver receiver;

    op(scenario s, std::vector<std::string> *log, Receiver r)
        : which(s), events(log), receiver(std::move(r)) {}
    op(const op &) = delete;
    op(op &&) = delete;

    void start() & noexcept {
      auto env = ex::get_env(receiver);
      if (which == scenario::error) {
        ex::set_error(std::move(receiver), std::make_exception_ptr(std::runtime_error("boom")));
        return;
      }
      if (which == scenario::before_start) {
        events->push_back("request-before-start");
        env.source->request_stop();
      }
      if (env.token.stop_requested()) {
        ex::set_stopped(std::move(receiver));
        return;
      }
      for (int step = 0; step < 3; ++step) {
        if (which == scenario::during_run && step == 2) {
          events->push_back("request-during-step-2");
          env.source->request_stop();
        }
        if (env.token.stop_requested()) {
          ex::set_stopped(std::move(receiver));
          return;
        }
      }
      ex::set_value(std::move(receiver), 3);
    }
  };

  template <class Receiver> auto connect(Receiver receiver) const {
    return op<std::remove_cvref_t<Receiver>>{which, events, std::move(receiver)};
  }
};

inline auto stopped_by(scenario which, std::vector<std::string> &events) -> bool {
  std::stop_source source;
  int value = 0;
  bool has_value = false;
  bool stopped = false;
  std::string error;
  auto op = ex::connect(
      cooperative_sender{which, &events},
      int_receiver{&value, &has_value, &stopped, &error, {source.get_token(), &source}});
  ex::start(op);
  return stopped && !has_value;
}

inline auto run_cancellation_story() -> cancellation_report {
  cancellation_report report;
  report.before_start_stopped = stopped_by(scenario::before_start, report.events);
  report.during_run_stopped = stopped_by(scenario::during_run, report.events);

  std::stop_source after_source;
  bool has_after = false;
  bool stopped_after = false;
  std::string after_error;
  auto after_op = ex::connect(cooperative_sender{scenario::after_completion, &report.events},
                              int_receiver{&report.after_completion_value,
                                           &has_after,
                                           &stopped_after,
                                           &after_error,
                                           {after_source.get_token(), &after_source}});
  ex::start(after_op);
  report.events.push_back("request-after-completion");
  after_source.request_stop();

  std::stop_source error_source;
  int value = 0;
  bool has_value = false;
  bool stopped = false;
  auto error_op = ex::connect(cooperative_sender{scenario::error, &report.events},
                              int_receiver{&value,
                                           &has_value,
                                           &stopped,
                                           &report.error_message,
                                           {error_source.get_token(), &error_source}});
  ex::start(error_op);
  return report;
}

} // namespace c10_c1_8