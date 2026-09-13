#pragma once
#include <c12/sqlite.hpp>
namespace solution {
inline c12::result<void> move_units(c12::sql::database& db,int from,int to,int amount){
    using namespace c12;
    if(from<=0||to<=0||from==to||amount<=0||amount>1000)return std::unexpected(error{errc::invalid});
    try{
        sql::transaction tx(db);
        auto subtract=db.prepare("UPDATE accounts SET units=units-?1 WHERE id=?2 AND units>=?1");
        subtract.bind(1,amount);subtract.bind(2,from);subtract.step();
        if(sqlite3_changes(db.handle())!=1)return std::unexpected(error{errc::invalid});
        auto add=db.prepare("UPDATE accounts SET units=units+?1 WHERE id=?2 AND units<=1000-?1");
        add.bind(1,amount);add.bind(2,to);add.step();
        if(sqlite3_changes(db.handle())!=1)return std::unexpected(error{errc::invalid});
        tx.commit();return {};
    }catch(const sql::db_error& e){return std::unexpected(error{errc::io,e.code});}
    catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
}
}
