#include <check.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct Counts {
    int copies = 0;
    int moves = 0;
};

struct Tracked {
    Counts* counts = nullptr;

    explicit Tracked(Counts& c) : counts(&c) {}
    Tracked(Tracked const& other) : counts(other.counts) {
        ++counts->copies;
    }
    Tracked(Tracked&& other) noexcept : counts(other.counts) {
        ++counts->moves;
        other.counts = nullptr;
    }
};

Tracked make_prvalue(Counts& counts) {
    return Tracked{counts};
}

Tracked make_named(Counts& counts) {
    Tracked local{counts};
    return local;
}

Tracked force_move(Counts& counts) {
    Tracked local{counts};
    return std::move(local);
}

struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove const&) = default;
    ThrowingMove(ThrowingMove&&) noexcept(false) {}
};

} // namespace

int main() {
    Counts prvalue_counts;
    auto prvalue = make_prvalue(prvalue_counts);
    (void)prvalue;
    check(prvalue_counts.copies == 0 && prvalue_counts.moves == 0, "same-type prvalue return is guaranteed elision");

    Counts named_counts;
    auto named = make_named(named_counts);
    (void)named;
    check(named_counts.copies == 0, "named return should not copy on this toolchain path");
    check(named_counts.moves <= 1, "named return may use NRVO or one move");

    Counts moved_counts;
    auto moved = force_move(moved_counts);
    (void)moved;
    check(moved_counts.moves == 1, "std::move in return forces a move path here");

    std::vector<Tracked> values;
    values.reserve(1);
    values.emplace_back(prvalue_counts);
    values.reserve(2);
    check(prvalue_counts.moves >= 1, "vector can move nothrow elements during growth");

    std::vector<ThrowingMove> throwing_values;
    throwing_values.reserve(1);
    throwing_values.emplace_back();
    throwing_values.reserve(2);
    static_assert(!std::is_nothrow_move_constructible_v<ThrowingMove>);
    static_assert(std::is_copy_constructible_v<ThrowingMove>);
}
