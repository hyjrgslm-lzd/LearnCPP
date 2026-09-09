#include "concurrency_study/bounded_channel.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <stop_token>

// Single consumer mailbox. put rejects an occupied slot (no silent overwrite).
// Data wins if both data and stop are observable under the lock.
class mailbox {
public:
    bool put(int value) {
        { std::lock_guard lock(mutex_); if (slot_) return false; slot_ = value; }
        cv_.notify_one();
        return true;
    }
    std::optional<int> take(std::stop_token token) {
        std::unique_lock lock(mutex_);
        if (!cv_.wait(lock, token, [&] { return slot_.has_value(); })) return std::nullopt;
        return std::exchange(slot_, std::nullopt);
    }
private:
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::optional<int> slot_;
};

int main() {
    mailbox box;
    std::stop_source source;
    auto value = std::async(std::launch::async, [&] { return box.take(source.get_token()); });
    cs::check(box.put(42), "normal delivery");
    cs::check(value.get() == 42, "normal wait yields payload");
    auto stopped = std::async(std::launch::async, [&] { return box.take(source.get_token()); });
    source.request_stop(); // safe whether before or during the library wait
    cs::check(!stopped.get(), "stop releases empty wait without manual notification");
    cs::check(!box.take(source.get_token()), "pre-requested empty wait");
    cs::check(box.put(7) && !box.put(8), "occupied slot rejects overwrite");
    cs::check(box.take(source.get_token()) == 7, "ready predicate wins over requested stop");

    std::mutex mutex;
    std::condition_variable_any cv;
    std::unique_lock lock(mutex);
    std::stop_source live;
    cs::check(!cv.wait_until(lock, live.get_token(), std::chrono::steady_clock::now(), [] { return false; }),
              "timed interruptible wait may be false without cancellation");
    cs::check(!live.stop_requested(), "timeout is distinct from stop");
    lock.unlock();
    cs::bounded_channel<int> stream(2);
    cs::check(stream.push(1) && stream.push(2), "stream accepts");
    stream.close();
    cs::check(stream.pop() == 1 && stream.pop() == 2 && !stream.pop(), "close drains stream");
    std::cout << "C3 OK: data, cancellation, pre-stop, data/stop priority, timeout, close\n";
}
