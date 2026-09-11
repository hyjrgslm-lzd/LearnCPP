#pragma once

#include <stdexec/execution.hpp>

#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c10_g3 {

namespace ex = stdexec;

template <class Operation> struct operation_slot {
  alignas(Operation) unsigned char storage_[sizeof(Operation)];
  bool engaged_ = false;

  operation_slot() = default;
  operation_slot(const operation_slot &) = delete;
  operation_slot(operation_slot &&) = delete;
  ~operation_slot() { reset(); }

  template <class Make> auto emplace_from(Make make) -> Operation & {
    reset();
    auto *ptr = ::new (static_cast<void *>(storage_)) Operation(make());
    engaged_ = true;
    return *ptr;
  }

  void reset() noexcept {
    if (engaged_) {
      ptr()->~Operation();
      engaged_ = false;
    }
  }

  auto ptr() noexcept -> Operation * { return reinterpret_cast<Operation *>(storage_); }
};

template <class Factory, class Receiver> struct retry_state;

template <class Factory, class Receiver> struct attempt_receiver {
  using receiver_concept = ex::receiver_tag;
  using state_t = retry_state<Factory, Receiver>;
  std::weak_ptr<state_t> state_;

  void set_value(int value) && noexcept {
    if (auto state = state_.lock()) {
      state->set_value(value);
    }
  }

  void set_error(std::exception_ptr error) && noexcept {
    if (auto state = state_.lock()) {
      state->set_error(std::move(error), state);
    }
  }

  void set_stopped() && noexcept {
    if (auto state = state_.lock()) {
      state->set_stopped();
    }
  }

  auto get_env() const noexcept -> decltype(ex::get_env(std::declval<Receiver const &>())) {
    auto state = state_.lock();
    return ex::get_env(*state->receiver_);
  }
};

template <class Factory, class Receiver> struct retry_state {
  using sender_t = decltype(std::declval<Factory &>()());
  using inner_receiver_t = attempt_receiver<Factory, Receiver>;
  using inner_op_t = ex::connect_result_t<sender_t, inner_receiver_t>;

  std::mutex mutex_;
  Factory factory_;
  std::optional<Receiver> receiver_;
  int max_attempts_;
  int attempts_ = 0;
  bool driving_ = false;
  bool drive_again_ = false;
  bool retry_requested_ = false;
  bool finished_ = false;
  std::exception_ptr last_error_;
  operation_slot<inner_op_t> current_;

  retry_state(Factory factory, Receiver receiver, int max_attempts)
      : factory_(std::move(factory)), receiver_(std::move(receiver)), max_attempts_(max_attempts) {}

  retry_state(const retry_state &) = delete;
  retry_state(retry_state &&) = delete;

  auto stop_requested() noexcept -> bool {
    return receiver_ && ex::get_stop_token(ex::get_env(*receiver_)).stop_requested();
  }

  void move_receiver_to(std::optional<Receiver> &receiver) noexcept {
    if (receiver_) {
      receiver.emplace(std::move(*receiver_));
      receiver_.reset();
    }
  }

  void set_value(int value) noexcept {
    std::optional<Receiver> receiver;
    {
      std::lock_guard lock(mutex_);
      if (finished_) {
        return;
      }
      finished_ = true;
      move_receiver_to(receiver);
    }
    if (receiver) {
      ex::set_value(std::move(*receiver), value);
    }
  }

  void set_stopped() noexcept {
    std::optional<Receiver> receiver;
    {
      std::lock_guard lock(mutex_);
      if (finished_) {
        return;
      }
      finished_ = true;
      move_receiver_to(receiver);
    }
    if (receiver) {
      ex::set_stopped(std::move(*receiver));
    }
  }

  void set_error(std::exception_ptr error, std::shared_ptr<retry_state> self) noexcept {
    {
      std::lock_guard lock(mutex_);
      if (finished_) {
        return;
      }
      last_error_ = std::move(error);
      retry_requested_ = true;
    }
    drive(std::move(self));
  }

  void complete_error(std::exception_ptr error) noexcept {
    std::optional<Receiver> receiver;
    {
      std::lock_guard lock(mutex_);
      if (finished_) {
        return;
      }
      finished_ = true;
      move_receiver_to(receiver);
      driving_ = false;
    }
    if (receiver) {
      ex::set_error(std::move(*receiver), std::move(error));
    }
  }

  void complete_stopped() noexcept {
    std::optional<Receiver> receiver;
    {
      std::lock_guard lock(mutex_);
      if (finished_) {
        return;
      }
      finished_ = true;
      move_receiver_to(receiver);
      driving_ = false;
    }
    if (receiver) {
      ex::set_stopped(std::move(*receiver));
    }
  }

  void drive(std::shared_ptr<retry_state> self) noexcept {
    {
      std::lock_guard lock(mutex_);
      if (driving_) {
        drive_again_ = true;
        return;
      }
      driving_ = true;
    }

    for (;;) {
      bool should_stop = false;
      bool exhausted = false;
      std::exception_ptr exhausted_error;
      {
        std::lock_guard lock(mutex_);
        drive_again_ = false;
        if (finished_) {
          driving_ = false;
          return;
        }
        if (retry_requested_) {
          current_.reset();
          retry_requested_ = false;
        }
        should_stop = stop_requested();
        exhausted = attempts_ >= max_attempts_;
        if (exhausted) {
          exhausted_error = last_error_
                                ? last_error_
                                : std::make_exception_ptr(std::runtime_error("retry exhausted"));
        } else if (!should_stop) {
          ++attempts_;
        }
      }

      if (should_stop) {
        complete_stopped();
        return;
      }
      if (exhausted) {
        complete_error(std::move(exhausted_error));
        return;
      }

      try {
        sender_t sender = factory_();
        current_.emplace_from([&] {
          return ex::connect(std::move(sender), inner_receiver_t{std::weak_ptr<retry_state>{self}});
        });
      } catch (...) {
        std::lock_guard lock(mutex_);
        last_error_ = std::current_exception();
        retry_requested_ = true;
        continue;
      }

      ex::start(*current_.ptr());

      std::lock_guard lock(mutex_);
      if (finished_) {
        driving_ = false;
        return;
      }
      if (!drive_again_ && !retry_requested_) {
        driving_ = false;
        return;
      }
    }
  }
};

template <class Factory, class Receiver> struct retry_op {
  using operation_state_concept = ex::operation_state_tag;
  std::shared_ptr<retry_state<Factory, Receiver>> state_;

  retry_op(Factory factory, Receiver receiver, int max_attempts)
      : state_(std::make_shared<retry_state<Factory, Receiver>>(
            std::move(factory), std::move(receiver), max_attempts)) {}
  retry_op(const retry_op &) = delete;
  retry_op(retry_op &&) = delete;

  void start() & noexcept {
    auto state = state_;
    state->drive(state);
  }
};

template <class Factory> struct retry_sender {
  using sender_concept = ex::sender_tag;
  Factory factory_;
  int max_attempts_;

  template <class Self, class... Env> static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                     ex::set_stopped_t()>{};
  }

  template <class Receiver> auto connect(Receiver receiver) && {
    return retry_op<Factory, std::remove_cvref_t<Receiver>>{std::move(factory_),
                                                            std::move(receiver), max_attempts_};
  }
};

template <class Factory> auto retry(Factory factory, int max_attempts) {
  if (max_attempts <= 0) {
    throw std::invalid_argument("max_attempts must be positive");
  }
  return retry_sender<std::remove_cvref_t<Factory>>{std::move(factory), max_attempts};
}

} // namespace c10_g3
