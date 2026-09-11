#pragma once

#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <stdexec/execution.hpp>

namespace c10_h1 {
namespace ex = stdexec;

class run_loop_closed : public std::runtime_error {
public:
  run_loop_closed() : std::runtime_error("run_loop closed") {}
};

struct operation_base {
  operation_base *next = nullptr;
  virtual void execute() noexcept = 0;
  virtual ~operation_base() = default;
};

class run_loop {
  operation_base *head_ = nullptr;
  operation_base *tail_ = nullptr;
  std::mutex mutex_;
  std::condition_variable ready_;
  bool closed_ = false;

  operation_base *pop_locked() noexcept {
    operation_base *op = head_;
    if (op == nullptr) {
      return nullptr;
    }
    head_ = op->next;
    if (head_ == nullptr) {
      tail_ = nullptr;
    }
    op->next = nullptr;
    return op;
  }

public:
  run_loop() = default;
  run_loop(const run_loop &) = delete;
  run_loop &operator=(const run_loop &) = delete;

  bool enqueue(operation_base *op) noexcept {
    {
      std::lock_guard lock(mutex_);
      if (closed_) {
        return false;
      }
      op->next = nullptr;
      if (tail_ != nullptr) {
        tail_->next = op;
        tail_ = op;
      } else {
        head_ = tail_ = op;
      }
    }
    ready_.notify_one();
    return true;
  }

  void close() noexcept {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    ready_.notify_all();
  }

  void run() noexcept {
    for (;;) {
      operation_base *op = nullptr;
      {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [&] { return closed_ || head_ != nullptr; });
        op = pop_locked();
        if (op == nullptr && closed_) {
          return;
        }
      }
      if (op != nullptr) {
        op->execute();
      }
    }
  }

  struct scheduler {
    run_loop *loop = nullptr;

    bool operator==(const scheduler &) const = default;

    struct sender {
      using sender_concept = ex::sender_tag;
      run_loop *loop_ = nullptr;

      template <class Self, class Env> static consteval auto get_completion_signatures() {
        return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
      }

      template <class Receiver> struct op : operation_base {
        using operation_state_concept = ex::operation_state_tag;
        Receiver receiver_;
        run_loop *loop_;

        op(Receiver receiver, run_loop *loop) : receiver_(std::move(receiver)), loop_(loop) {}
        op(const op &) = delete;
        op(op &&) = delete;

        void start() & noexcept {
          if (!loop_->enqueue(this)) {
            ex::set_error(std::move(receiver_), std::make_exception_ptr(run_loop_closed{}));
          }
        }

        void execute() noexcept override { ex::set_value(std::move(receiver_)); }
      };

      template <class Receiver> auto connect(Receiver receiver) const {
        return op<std::remove_cvref_t<Receiver>>(std::move(receiver), loop_);
      }
    };

    sender schedule() const noexcept { return sender{loop}; }
  };

  scheduler get_scheduler() noexcept { return scheduler{this}; }
  auto schedule() noexcept { return get_scheduler().schedule(); }
};
} // namespace c10_h1

namespace c10_h1 {
struct detached_receiver {
  using receiver_concept = stdexec::receiver_tag;
  void set_value() noexcept {}
  void set_error(std::exception_ptr error) noexcept { std::rethrow_exception(error); }
  void set_stopped() noexcept {}
};
} // namespace c10_h1
