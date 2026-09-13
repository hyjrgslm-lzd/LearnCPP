#pragma once
#include <c12/sqlite.hpp>
namespace solution {
inline c12::result<void> move_units(c12::sql::database& db,int from,int to,int amount){
    try{
        auto subtract=db.prepare("UPDATE accounts SET units=units-?1 WHERE id=?2");
        subtract.bind(1,amount);subtract.bind(2,from);subtract.step();
        auto add=db.prepare("UPDATE accounts SET units=units+?1 WHERE id=?2");
        add.bind(1,amount);add.bind(2,to);add.step();
        if(sqlite3_changes(db.handle())!=1)return std::unexpected(c12::error{c12::errc::invalid});
        return {};
    }catch(const c12::sql::db_error& e){return std::unexpected(c12::error{c12::errc::io,e.code});}
}
}
