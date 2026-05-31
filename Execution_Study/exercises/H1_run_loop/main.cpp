#include <stdexec/execution.hpp>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

namespace ex = stdexec;

// ============================================================
// operation_base - intrusive list node + virtual execute
// ============================================================

struct operation_base {
    operation_base* next_ = nullptr;
    virtual void execute() = 0;
    virtual ~operation_base() = default;
};

// ============================================================
// intrusive_queue - FIFO queue using intrusive linked list
// ============================================================

struct intrusive_queue {
    operation_base* head_ = nullptr;
    operation_base* tail_ = nullptr;

    // TODO [必做]: push_back(operation_base*)
    // Append the node to the tail of the queue.
    // If the queue is empty, both head_ and tail_ point to the new node.
    // Otherwise, link current tail_->next_ to the new node and update tail_.
    //
    // void push_back(operation_base* op) {
    //     op->next_ = nullptr;
    //     if (tail_) {
    //         tail_->next_ = op;
    //         tail_ = op;
    //     } else {
    //         head_ = tail_ = op;
    //     }
    // }

    // TODO [必做]: pop_front() -> operation_base*
    // Remove and return the head node.
    // If the queue becomes empty, reset tail_ to nullptr.
    // Returns nullptr if the queue is empty.
    //
    // operation_base* pop_front() {
    //     if (!head_) return nullptr;
    //     auto* op = head_;
    //     head_ = head_->next_;
    //     if (!head_) tail_ = nullptr;
    //     op->next_ = nullptr;
    //     return op;
    // }

    // TODO [必做]: empty() -> bool
    //
    // bool empty() const { return head_ == nullptr; }
};

// ============================================================
// my_run_loop - single-threaded event loop
// ============================================================

class my_run_loop {
    intrusive_queue queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool finished_ = false;

public:
    void push(operation_base* op) {
        // TODO [必做]: lock, enqueue, notify
        //
        // {
        //     std::lock_guard lock(mtx_);
        //     queue_.push_back(op);
        // }
        // cv_.notify_one();
    }

    void run() {
        // TODO [必做]: loop: wait for work or finished, dequeue, execute
        //
        // Main event loop pattern:
        //   while (true) {
        //       operation_base* op = nullptr;
        //       {
        //           std::unique_lock lock(mtx_);
        //           cv_.wait(lock, [&]{ return !queue_.empty() || finished_; });
        //           if (queue_.empty() && finished_) break;
        //           op = queue_.pop_front();
        //       }
        //       // Execute outside the lock!
        //       if (op) op->execute();
        //   }
    }

    void finish() {
        // TODO [必做]: set finished, notify
        //
        // {
        //     std::lock_guard lock(mtx_);
        //     finished_ = true;
        // }
        // cv_.notify_one();
    }

    // TODO [必做]: schedule() returns a sender
    //
    // The sender's connect(receiver) returns a run_loop_operation_state<Receiver>.
    // run_loop_operation_state:
    //   - inherits from operation_base
    //   - holds receiver and pointer to my_run_loop
    //   - start() enqueues `this` into run_loop via push(this)
    //   - execute() override calls ex::set_value(std::move(receiver_))
    //
    // The sender should declare completion_signatures with at least:
    //   ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>
    //
    // Skeleton:
    //
    // template <typename Receiver>
    // struct run_loop_operation_state : operation_base {
    //     Receiver receiver_;
    //     my_run_loop* loop_;
    //
    //     using operation_state_concept = ex::operation_state_t;
    //
    //     run_loop_operation_state(Receiver recv, my_run_loop* loop)
    //         : receiver_(std::move(recv)), loop_(loop) {}
    //
    //     // Non-movable (address must be stable for intrusive queue)
    //     run_loop_operation_state(const run_loop_operation_state&) = delete;
    //     run_loop_operation_state& operator=(const run_loop_operation_state&) = delete;
    //
    //     void execute() override {
    //         ex::set_value(std::move(receiver_));
    //     }
    //
    //     friend void tag_invoke(ex::start_t, run_loop_operation_state& self) noexcept {
    //         self.loop_->push(&self);
    //     }
    // };
    //
    // struct schedule_sender {
    //     using sender_concept = ex::sender_t;
    //     using completion_signatures = ex::completion_signatures<
    //         ex::set_value_t(),
    //         ex::set_error_t(std::exception_ptr)
    //     >;
    //
    //     my_run_loop* loop_;
    //
    //     template <typename Receiver>
    //     friend auto tag_invoke(ex::connect_t, schedule_sender self, Receiver&& recv) {
    //         return run_loop_operation_state<std::remove_cvref_t<Receiver>>{
    //             std::forward<Receiver>(recv), self.loop_
    //         };
    //     }
    // };
    //
    // auto schedule() { return schedule_sender{this}; }
};

// ============================================================
// Tests
// ============================================================

int main() {
    my_run_loop loop;

    // TODO [必做]: create senders via loop.schedule()
    // TODO [必做]: attach then() to print task IDs
    // TODO [必做]: run loop in a thread, or run main work in a thread
    //
    // Example:
    //
    // auto task1 = loop.schedule()
    //            | ex::then([]{ std::cout << "Task 1 on thread "
    //                           << std::this_thread::get_id() << "\n"; });
    // auto task2 = loop.schedule()
    //            | ex::then([]{ std::cout << "Task 2 on thread "
    //                           << std::this_thread::get_id() << "\n"; });
    // auto task3 = loop.schedule()
    //            | ex::then([]{ std::cout << "Task 3 on thread "
    //                           << std::this_thread::get_id() << "\n"; });
    //
    // auto all = ex::when_all(std::move(task1), std::move(task2), std::move(task3))
    //          | ex::then([](auto&&...){ std::cout << "All done!\n"; });
    //
    // // Run sync_wait in a separate thread because it blocks,
    // // and loop.run() also blocks on the main thread.
    // std::jthread worker([&]{
    //     ex::sync_wait(std::move(all));
    //     loop.finish();
    // });
    //
    // loop.run();

    std::cout << "All run_loop tasks completed.\n";
    return 0;
}
