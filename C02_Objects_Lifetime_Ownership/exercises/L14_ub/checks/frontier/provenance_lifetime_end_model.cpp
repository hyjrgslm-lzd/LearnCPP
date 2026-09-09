#include <cstdint>
#include <memory>

struct OwnerTag {
    int value;
};

void provenance_review_model() {
    auto first = std::make_unique<OwnerTag>(OwnerTag{1});
    auto raw = first.get();
    auto printed_address = reinterpret_cast<std::uintptr_t>(raw);
    (void)printed_address;
    first.reset();

    auto second = std::make_unique<OwnerTag>(OwnerTag{2});
    (void)second;
    // The saved integer is only diagnostic data. This compile-only source makes
    // no runtime claim that a pointer reconstructed from printed_address could
    // access second or revive raw after first's lifetime ended.
}
