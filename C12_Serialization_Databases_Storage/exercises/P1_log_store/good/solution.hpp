#pragma once
#include <c12/bytes.hpp>
namespace solution {
inline c12::result<void> apply(c12::dictionary& values,std::span<const c12::mutation> operations){
    if(operations.empty()||operations.size()>c12::max_batch)return std::unexpected(c12::error{c12::errc::invalid});
    try {
        c12::dictionary staged(values.begin(),values.end());
        for(const auto& operation:operations){
            if(operation.key.empty()||operation.key.size()>c12::max_key||
               (operation.value&&operation.value->size()>c12::max_value))
                return std::unexpected(c12::error{c12::errc::invalid});
            staged.erase(operation.key);
            if(operation.value)staged.emplace(operation.key,*operation.value);
            if(staged.size()>c12::max_keys)return std::unexpected(c12::error{c12::errc::capacity});
        }
        values=std::move(staged);return {};
    }catch(const std::bad_alloc&){return std::unexpected(c12::error{c12::errc::allocation});}
}
}
