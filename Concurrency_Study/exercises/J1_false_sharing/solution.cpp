#include "reference.hpp"
#include <iostream>
int main() {
    using namespace cs::layout;
    for (auto v : {variant::packed,variant::padded,variant::shared,variant::batched})
        for (auto n : {std::size_t{0},std::size_t{1},std::size_t{257},std::size_t{4097}}) {
            counters c;
            c.run(v,n,256);
            c.verify(v,n,256);
        }
    bool rejected=false;
    try { counters c; c.run(variant::batched,1,0); } catch (const std::exception&) { rejected=true; }
    cs::check(rejected,"zero batch rejected");
    std::cout << "J1 OK: private counts, true sharing, batched tail and publication count\n";
}
