#pragma once
#include <c10/test.hpp>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexec/execution.hpp>
#include <utility>

namespace c10_h1 {
namespace ex = stdexec;

class run_loop_closed : public std::runtime_error {
public:
  run_loop_closed() : std::runtime_error("run_loop closed") {}
};

struct detached_receiver {
  using receiver_concept = stdexec::receiver_tag;
  void set_value() noexcept {}
  void set_error(std::exception_ptr error) noexcept { std::rethrow_exception(error); }
  void set_stopped() noexcept {}
};

struct queue_node {
  queue_node *next = nullptr;
  virtual void run_node() noexcept = 0;
  virtual ~queue_node() = default;
};

class run_loop {
  queue_node *head_ = nullptr;
  queue_node *tail_ = nullptr;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool closed_ = false;

public:
  bool push(queue_node *node) noexcept {
    {
      std::lock_guard lock(mutex_);
      if (closed_) {
        return false;
      }
      node->next = nullptr;
      if (tail_ != nullptr) {
        tail_->next = node;
        tail_ = node;
      } else {
        head_ = tail_ = node;
      }
    }
    cv_.notify_one();
    return true;
  }

  void close() noexcept {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }

  void run() noexcept {
    for (;;) {
      queue_node *node = nullptr;
      {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return closed_ || head_ != nullptr; });
        node = head_;
        if (node != nullptr) {
          head_ = node->next;
          if (head_ == nullptr) {
            tail_ = nullptr;
          }
          node->next = nullptr;
        } else if (closed_) {
          return;
        }
      }
      node->run_node();
    }
  }

  struct sender {
    using sender_concept = ex::sender_tag;
    run_loop *loop = nullptr;

    template <class Self, class Env> static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
    }

    template <class Receiver> struct op : queue_node {
      using operation_state_concept = ex::operation_state_tag;
      Receiver receiver;
      run_loop *loop;

      op(Receiver r, run_loop *target) : receiver(std::move(r)), loop(target) {}
      op(const op &) = delete;
      op(op &&) = delete;

      void start() & noexcept {
        if (!loop->push(this)) {
          ex::set_error(std::move(receiver), std::make_exception_ptr(run_loop_closed{}));
        }
      }

      void run_node() noexcept override { ex::set_value(std::move(receiver)); }
    };

    template <class Receiver> auto connect(Receiver receiver) const {
      return op<std::remove_cvref_t<Receiver>>(std::move(receiver), loop);
    }
  };

  sender schedule() noexcept { return sender{this}; }
};

} // namespace c10_h1
