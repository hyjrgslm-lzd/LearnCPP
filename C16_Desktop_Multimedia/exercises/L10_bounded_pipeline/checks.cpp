#include <solution.hpp>
#include <c16/check.hpp>

#include <chrono>
#include <thread>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif

namespace {

void check_capacity_and_partial_write()
{
    c16_l10::BoundedPcmPipe pipe(3);
    c16::require(pipe.write({1, 2, 3, 4}) == 3, "write accepts only available capacity");
    c16::require(pipe.backpressure_count() == 1, "partial write records backpressure");
    c16::require(pipe.size() == 3, "capacity is not exceeded");
    c16::require(pipe.read(2) == std::vector<int>({1, 2}), "read preserves FIFO order");
    c16::require(pipe.write({5, 6}) == 2, "space after read accepts more");
    c16::require(pipe.read(8) == std::vector<int>({3, 5, 6}), "drain returns remaining samples");
}

void check_eof_and_drain()
{
    c16_l10::BoundedPcmPipe pipe(2);
    c16::require(!pipe.eof(), "open empty pipe is underrun, not EOF");
    c16::require(pipe.read(1).empty(), "empty read is bounded");
    c16::require(pipe.underrun_count() == 1, "empty read records underrun");
    c16::require(pipe.write({7, 8}) == 2, "write before close");
    pipe.close_input();
    c16::require(!pipe.eof(), "closed pipe with buffered data must drain first");
    c16::require(pipe.read(1) == std::vector<int>({7}), "closed pipe still drains");
    c16::require(pipe.read(1) == std::vector<int>({8}), "last frame drains before EOF");
    c16::require(pipe.eof(), "EOF after close and empty");
    c16::require(pipe.write({9}) == 0, "write after EOF is refused");
}

void check_cancel_flush()
{
    c16_l10::BoundedPcmPipe pipe(4);
    c16::require(pipe.write({1, 2, 3}) == 3, "preload before cancel");
    pipe.cancel_flush();
    c16::require(pipe.cancelled(), "flush cancel marks stream cancelled");
    c16::require(pipe.size() == 0, "flush cancel discards queued data");
    c16::require(pipe.eof(), "cancelled stream is terminal");
    c16::require(pipe.write({4}) == 0, "cancelled stream refuses writes");
}

void check_threaded_waiting_observation()
{
    using namespace std::chrono_literals;
    c16_l10::BoundedPcmPipe pipe(2);
    c16::require(pipe.read_wait(0).empty(), "zero-count waiting read returns immediately");
    std::size_t produced = 0;
    std::thread producer([&] {
        produced = pipe.write_wait({10, 11, 12, 13});
        pipe.close_input();
    });

    for (int i = 0; i != 50 && pipe.size() < 2; ++i) {
        std::this_thread::sleep_for(2ms);
    }
    c16::require(pipe.size() == 2, "producer blocks at bounded capacity");
    c16::require(pipe.read_wait(2) == std::vector<int>({10, 11}), "consumer releases capacity in FIFO order");
    producer.join();
    c16::require(produced == 4, "producer completes after consumer drains space");
    c16::require(pipe.read_wait(4) == std::vector<int>({12, 13}), "remaining data drains after close");
    c16::require(pipe.eof(), "threaded close reaches EOF after drain");
}

} // namespace

int main()
{
    return c16::run([] {
        check_capacity_and_partial_write();
        check_eof_and_drain();
        check_cancel_flush();
        check_threaded_waiting_observation();
    });
}
