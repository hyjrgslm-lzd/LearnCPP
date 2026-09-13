#pragma once
#include <c12/sqlite.hpp>
namespace solution {
inline c12::result<void> move_units(c12::sql::database&,int,int,int){
    return std::unexpected(c12::error{c12::errc::unfinished});
}
}
