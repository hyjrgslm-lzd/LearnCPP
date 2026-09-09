#include <bit>
#include <cstdint>
#include <type_traits>

struct BaselineAggregate {
    int x;
    int y;
};

constexpr BaselineAggregate make_baseline() {
    return BaselineAggregate{.x = 1, .y = 2};
}

static_assert(make_baseline().x == 1);
static_assert(make_baseline().y == 2);
static_assert(std::is_trivially_copyable_v<BaselineAggregate>);
static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3f800000u);
