#include <check.hpp>
#include <my_begin.hpp>

#include <concepts>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

struct MemberOnly {
    int values[3]{1, 2, 3};
    int* begin() noexcept { return values; }
};

namespace adl_only {
struct Range {
    int values[2]{10, 20};
};

int* begin(Range& range) noexcept { return range.values; }
} // namespace adl_only

namespace both_paths {
struct Range {
    int member_values[2]{30, 31};
    int adl_values[2]{40, 41};
    int* begin() noexcept { return member_values; }
};

int* begin(Range& range) noexcept { return range.adl_values; }
} // namespace both_paths

struct ThrowingMember {
    int values[1]{7};
    int* begin() { return values; }
};

struct NoBegin {};

template<class R>
concept can_my_begin = requires(R&& r) {
    c06_e1::my_begin(static_cast<R&&>(r));
};

} // namespace

static_assert(std::is_object_v<decltype(c06_e1::my_begin)>);
static_assert(can_my_begin<MemberOnly&>);
static_assert(can_my_begin<adl_only::Range&>);
static_assert(can_my_begin<int (&)[2]>);
static_assert(can_my_begin<std::string_view>);
static_assert(!can_my_begin<std::vector<int>>);
static_assert(!can_my_begin<NoBegin&>);

int main() {
    MemberOnly member;
    check(*c06_e1::my_begin(member) == 1, "member begin returns the first member element");

    adl_only::Range adl;
    check(*c06_e1::my_begin(adl) == 10, "ADL begin fallback returns the first ADL element");

    both_paths::Range both;
    check(*c06_e1::my_begin(both) == 30, "member path wins over ADL begin");

    int array[2]{5, 6};
    check(*c06_e1::my_begin(array) == 5, "array begin returns the first array element");

    check(*c06_e1::my_begin(std::string_view{"hi"}) == 'h',
          "borrowed rvalue range may return an iterator");

    check(noexcept(c06_e1::my_begin(member)), "nothrow member begin propagates noexcept");
    check(!noexcept(c06_e1::my_begin(std::declval<ThrowingMember&>())),
          "throwing member begin is not reported noexcept");
}
