#pragma once
#include <c12/bytes.hpp>
namespace solution {
inline c12::result<void> apply(c12::dictionary&,std::span<const c12::mutation>) {
    // Part 1: validate/stage the whole batch; publish only on success.
    return std::unexpected(c12::error{c12::errc::unfinished});
}
}
