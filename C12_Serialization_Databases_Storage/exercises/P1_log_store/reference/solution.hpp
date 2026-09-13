#pragma once
#include <c12/bytes.hpp>
namespace solution {
inline c12::result<void> apply(c12::dictionary& values,std::span<const c12::mutation> operations){
    auto staged=c12::prepare_batch(values,operations);
    if(!staged)return std::unexpected(staged.error());
    values.swap(*staged);return {};
}
}
