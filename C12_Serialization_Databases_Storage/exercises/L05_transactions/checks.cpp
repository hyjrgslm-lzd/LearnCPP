#include <solution.hpp>
#include <check.hpp>
#include <array>
int main(){
    using namespace c12;
    sql::database db(":memory:");
    db.exec("CREATE TABLE accounts(id INTEGER PRIMARY KEY, units INTEGER NOT NULL CHECK(units BETWEEN 0 AND 1000)) STRICT;"
            "INSERT INTO accounts VALUES(1,10),(2,20)");
    auto state=[&]{return std::array{db.integer("SELECT units FROM accounts WHERE id=1"),db.integer("SELECT units FROM accounts WHERE id=2")};};
    auto previous=state();
    auto invalid=solution::move_units(db,1,99,3);
    if(!invalid&&invalid.error().code==errc::unfinished){std::cerr<<"UNFINISHED\n";return 1;}
    check(!invalid&&state()==previous,"failed transfer preserves both rows");
    check(sqlite3_get_autocommit(db.handle())!=0,"failed transaction closes");
    check(solution::move_units(db,1,2,3).has_value()&&state()==std::array<std::int64_t,2>{7,23},"successful conserved update");
    previous=state();
    for(const auto& args:std::array{std::array{1,1,2},std::array{1,2,0},std::array{1,2,-1},std::array{1,2,1001},std::array{1,2,8},std::array{-1,2,1}})
        check(!solution::move_units(db,args[0],args[1],args[2])&&state()==previous,"invalid request leaves state");
    db.exec("UPDATE accounts SET units=999 WHERE id=2");previous=state();
    check(!solution::move_units(db,1,2,2)&&state()==previous,"destination capacity rollback");
    db.exec("UPDATE accounts SET units=10 WHERE id=2;"
            "CREATE TRIGGER injected BEFORE UPDATE ON accounts WHEN NEW.id=2 AND NEW.units=13 "
            "BEGIN SELECT RAISE(ABORT,'injected destination failure'); END");
    previous=state();auto interrupted=solution::move_units(db,1,2,3);
    check(!interrupted&&interrupted.error().native==SQLITE_CONSTRAINT_TRIGGER&&state()==previous,"database failure rolls back earlier statement");
    check(sqlite3_get_autocommit(db.handle())!=0,"error leaves reusable connection");
    std::cout<<"transaction implementation checks passed\n";
}
