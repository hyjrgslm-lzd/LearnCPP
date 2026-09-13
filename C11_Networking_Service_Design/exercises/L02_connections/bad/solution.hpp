#pragma once
#include <c11/write_queue.hpp>
namespace exercise {
class queue : public c11::write_queue {
public:
    // Deliberate real bug: drops the entire front message after any successful send.
    void consume(std::size_t) { c11::write_queue::consume(front().size()); }
};
}
