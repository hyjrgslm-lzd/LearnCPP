#include <utility>

struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(MoveOnly const&) = delete;
    MoveOnly(MoveOnly&&) = default;
};

int main() {
    const MoveOnly value;
    MoveOnly other(std::move(value));
    (void)other;
}
