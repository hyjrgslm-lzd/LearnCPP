#include "../J1_false_sharing/reference.hpp"
#include <iostream>
int main() {
    using namespace cs::layout;
    padded_counter p[2];
    packed_pair q;
    const auto a=reinterpret_cast<std::uintptr_t>(&p[0]);
    const auto b=reinterpret_cast<std::uintptr_t>(&p[1]);
    cs::check(a%destructive==0 && b%destructive==0,"array elements preserve alignment");
    cs::check(b-a==sizeof(padded_counter) && b-a>=destructive,"array stride includes padding");
    cs::check(alignof(packed_pair)>=destructive,"packed container starts aligned");
    const auto q0=reinterpret_cast<std::uintptr_t>(&q.value[0]);
    const auto q1=reinterpret_cast<std::uintptr_t>(&q.value[1]);
    std::cout << "J2 OK: hints=" << destructive << '/' << constructive
              << " packed_offsets=0," << q1-q0 << " padded_stride=" << b-a
              << " same_hint_region=" << (q0/destructive==q1/destructive)
              << " hot_record=" << sizeof(hot_record)
              << " lock_free=" << q.value[0].is_lock_free() << '\n';
    // Addresses and hints do not discover the actual cache coherence granule.
}
