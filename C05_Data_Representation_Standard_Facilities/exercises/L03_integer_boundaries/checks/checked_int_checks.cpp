#include <checked_int.hpp>

#include <check.hpp>

#include <cstdint>
#include <limits>

int main()
{
    auto zero = c05_ex::checked_cast_u32(0);
    check(zero && *zero == 0u, "keeps zero");
    auto max = c05_ex::checked_cast_u32(std::numeric_limits<std::uint32_t>::max());
    check(max && *max == std::numeric_limits<std::uint32_t>::max(), "keeps u32 max");
    auto too_big = c05_ex::checked_cast_u32(static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1);
    check(!too_big, "rejects one-past u32 max");
    check(too_big.error().code == c05::Errc::out_of_range, "uses out_of_range");
}
