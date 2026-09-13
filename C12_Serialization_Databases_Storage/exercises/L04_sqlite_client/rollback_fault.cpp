#include <c12/sqlite.hpp>
#include <check.hpp>
#include <iostream>
static sqlite3_mem_methods original{};
static bool refuse_sqlite_allocations=false;
static void* fail_malloc(int size){return refuse_sqlite_allocations?nullptr:original.xMalloc(size);}
static void* fail_realloc(void* old,int size){return refuse_sqlite_allocations?nullptr:original.xRealloc(old,size);}
void scenario(bool explicit_rollback){
    using namespace c12::sql;
    database db(":memory:");
    auto* borrowed_for_diagnosis=db.handle();
    require(db.handle(),sqlite3_db_config(db.handle(),SQLITE_DBCONFIG_LOOKASIDE,nullptr,0,0));
    db.exec("CREATE TABLE probe(id INTEGER)");
    auto held=db.prepare("SELECT ?1");held.bind(1,std::int64_t{42});check(held.step(),"pre-existing statement");
    int explicit_error=0;
    {
        transaction change(db);db.exec("INSERT INTO probe VALUES(1)");
        refuse_sqlite_allocations=true;
        if(explicit_rollback){
            try{change.rollback();}catch(const db_error& e){explicit_error=e.code;}
        }
    }
    refuse_sqlite_allocations=false;
    // Deliberate diagnostic bypass only: proves SQLite still has an open transaction.
    check(sqlite3_get_autocommit(borrowed_for_diagnosis)==0,"actual rollback failure was exercised");
    check(db.cleanup_error()==SQLITE_NOMEM,"original rollback error retained");
    if(explicit_rollback)check(explicit_error==SQLITE_NOMEM,"explicit rollback reports failure");
    auto frozen=[](auto operation){
        bool rejected=false;
        try{operation();}catch(const db_error& e){rejected=e.code==SQLITE_NOMEM;}
        check(rejected,"all owners observe the same quarantined connection");
    };
    frozen([&]{(void)db.handle();});
    frozen([&]{db.exec("INSERT INTO probe VALUES(2)");});
    frozen([&]{auto another=db.prepare("SELECT 1");(void)another;});
    frozen([&]{(void)held.step();});
    frozen([&]{held.reset();});
    frozen([&]{held.bind(1,std::int64_t{7});});
    frozen([&]{(void)held.integer(0);});
    frozen([&]{(void)held.status(SQLITE_STMTSTATUS_VM_STEP);});
    frozen([&]{transaction another(db);});
    std::cout<<"rollback="<<(explicit_rollback?"explicit":"destructor")<<" native_error="<<db.cleanup_error()<<" frozen=true\n";
}
int main(){
    check(sqlite3_config(SQLITE_CONFIG_GETMALLOC,&original)==SQLITE_OK,"obtain this process allocator");
    auto injected=original;injected.xMalloc=fail_malloc;injected.xRealloc=fail_realloc;
    check(sqlite3_config(SQLITE_CONFIG_MALLOC,&injected)==SQLITE_OK,"install process-local SQLite fault injector");
    try{scenario(false);scenario(true);}
    catch(const std::exception& e){refuse_sqlite_allocations=false;std::cerr<<e.what()<<"\n";return 1;}
    check(sqlite3_shutdown()==SQLITE_OK,"all test connections closed");
    check(sqlite3_config(SQLITE_CONFIG_MALLOC,&original)==SQLITE_OK,"restore process-local allocator");
    std::cout<<"rollback allocation failure checks passed\n";
}
