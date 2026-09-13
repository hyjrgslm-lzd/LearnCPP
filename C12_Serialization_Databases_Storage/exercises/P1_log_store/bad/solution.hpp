#pragma once
#include <c12/bytes.hpp>
namespace solution {
inline c12::result<void> apply(c12::dictionary& values,std::span<const c12::mutation> operations){
    for(const auto& operation:operations){
        if(operation.key.empty())return std::unexpected(c12::error{c12::errc::invalid});
        if(operation.value)values[operation.key]=*operation.value;else values.erase(operation.key);
    }
    return {};
}
}
