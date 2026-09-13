#pragma once
#include <c12/sqlite.hpp>
namespace solution {
inline c12::result<void> move_units(c12::sql::database& db,int from,int to,int amount){
    using namespace c12;
    if(from==to||from<=0||to<=0||amount<1||amount>1000)return std::unexpected(error{errc::invalid});
    try{
        sql::transaction tx(db,sql::transaction_kind::immediate);
        auto lookup=[&](int id)->std::optional<std::int64_t>{
            auto query=db.prepare("SELECT units FROM accounts WHERE id=?1");query.bind(1,id);
            return query.step()?std::optional<std::int64_t>{query.integer(0)}:std::optional<std::int64_t>{};
        };
        const auto source=lookup(from),destination=lookup(to);
        if(!source||!destination||*source<amount||*destination>1000-amount)return std::unexpected(error{errc::invalid});
        auto update=db.prepare("UPDATE accounts SET units=?1 WHERE id=?2");
        update.bind(1,*source-amount);update.bind(2,from);update.step();update.reset();
        update.bind(1,*destination+amount);update.bind(2,to);update.step();
        tx.commit();return {};
    }catch(const sql::db_error& e){return std::unexpected(error{errc::io,e.code});}
    catch(const std::bad_alloc&){return std::unexpected(error{errc::allocation});}
}
}
