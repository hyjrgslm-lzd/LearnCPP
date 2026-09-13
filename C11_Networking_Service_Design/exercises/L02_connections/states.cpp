#include <c11/connection.hpp>
#include <check.hpp>
#include <iostream>
int main() {
    const auto wire = *c11::encode_frame("hello");
    c11::framed_connection c;
    check(c.receive(wire.substr(0, 3)) && c.output().empty(), "partial header produces no response");
    check(c.receive(wire.substr(3)), "remainder completes frame");
    check(c.eof() && !c.can_read(), "half-close stops reads but retains response");
    std::string output;
    while (!c.output().empty()) { output += c.output()[0]; c.sent(1); }
    check(output == wire && c.state() == c11::connection_state::closed, "half-close drains exact response");
    c.drain(); c.drain();
    check(c.state() == c11::connection_state::closed, "repeat drain remains closed");
    c11::framed_connection short_frame;
    check(short_frame.receive("00000003ab") && !short_frame.eof(), "truncated EOF is protocol error");
    c11::framed_connection slow;
    const auto full = *c11::encode_frame(std::string(4088, 'x'));
    for (int i = 0; i < 12; ++i) check(slow.receive(full), "slow peer accepts bounded input");
    check(!slow.can_read() && slow.queued_bytes() == 49152, "slow peer stops input at high water");
    c11::framed_connection other;
    check(other.receive(wire) && !other.output().empty(), "other connection can progress independently");
    for (int i = 0; i < 4; ++i) slow.sent(slow.output().size());
    check(slow.can_read(), "draining to low water resumes input");
    slow.drain();
    check(!slow.can_read(), "shutdown does not accept new work");
    while (!slow.output().empty()) slow.sent(slow.output().size());
    check(slow.state() == c11::connection_state::closed, "all accepted writes drained");
    // Safe counterexample: a read fragment is not a complete protocol frame.
    check(wire.substr(0, 3) != wire, "single-recv-as-message counterexample");
    std::cout << "controlled connection state checks passed (model, not kernel scheduling evidence)\n";
}
