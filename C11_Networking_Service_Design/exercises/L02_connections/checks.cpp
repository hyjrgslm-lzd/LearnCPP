#include <solution.hpp>
#include <check.hpp>
#include <iostream>
#include <string_view>
#include <string>
#include <array>
#include <utility>
int main() {
    try {
        exercise::queue q;
        check(q.enqueue("abcdef"), "enqueue accepted");
        q.consume(2);
        check(std::string_view(q.front().data(), q.front().size()) == "cdef", "partial send retains unsent suffix");
        check(q.bytes() == 6, "inflight allocation remains charged");
        q.consume(4);
        check(q.empty() && q.bytes() == 0, "completed message released");
        for (const auto message : {"first", "second-long", "z"}) check(q.enqueue(message), "FIFO input accepted");
        std::string ordered;
        while (!q.empty()) {
            const auto count = std::min<std::size_t>(2, q.front().size());
            ordered.append(q.front().data(), count);
            q.consume(count);
        }
        check(ordered == "firstsecond-longz", "multiple messages preserve FIFO through partial writes");
        std::string reserved;
        reserved.reserve(1024 * 1024);
        reserved = "x";
        const char* original = reserved.data();
        check(q.enqueue(std::move(reserved)), "small payload from large-capacity input accepted");
        check(q.front().data() != original, "queue owns compact payload independently of input capacity");
        reserved[0] = 'y';
        check(q.front()[0] == 'x' && q.bytes() == 1, "queue copies borrowed input and charges owned byte");
        q.consume(1);
        for (int i = 0; i < 12; ++i) check(q.enqueue(std::string(4096, static_cast<char>('a'+i))), "bounded backlog accepted");
        check(q.paused(), "high water pauses reads");
        q.consume(4096);
        check(q.paused(), "hysteresis retains pause above low water");
        for (int i = 0; i < 3; ++i) q.consume(4096);
        check(!q.paused(), "low water resumes reads");
        const auto before = q.bytes();
        check(!q.enqueue(std::string(65537, 'x')) && q.bytes() == before, "rejection has no effect");
        while (!q.empty()) q.consume(q.front().size());
        bool invalid = false;
        try { q.consume(1); } catch (const std::invalid_argument&) { invalid = true; }
        check(invalid, "invalid completion rejected");
        check(q.enqueue(std::string(65536, 'z')) && !q.enqueue("x"), "exact capacity and one byte overflow");
        std::cout << "queue checks passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
