#include <c12/sqlite.hpp>
#include <check.hpp>
#include <chrono>
#include <iostream>
int main(){
    try{
        using namespace c12::sql;
        check(sqlite3_libversion_number()==3053004,"pinned SQLite runtime");
        database db(":memory:");
        db.exec("CREATE TABLE items(id INTEGER PRIMARY KEY,name TEXT NOT NULL,note TEXT,payload BLOB NOT NULL) STRICT");
        const std::string hostile="x'); DROP TABLE items; --";
        auto insert=db.prepare("INSERT INTO items VALUES(?1,?2,?3,?4)");
        insert.bind(1,std::int64_t{1});insert.bind(2,hostile);insert.bind_null(3);insert.bind_blob(4,{});
        check(!insert.step(),"prepared insertion");insert.reset();
        insert.bind(1,std::int64_t{2});insert.bind(2,std::string_view(""));insert.bind(3,std::string_view(""));
        const c12::bytes payload{std::byte{0},std::byte{255}};insert.bind_blob(4,payload);
        check(!insert.step(),"second insertion");
        auto query=db.prepare("SELECT name,note,payload FROM items ORDER BY id");
        check(query.step()&&query.text(0)==hostile&&query.is_null(1)&&query.blob(2).empty(),"binding separates SQL and data");
        auto saved=query.text(0);
        check(query.step()&&query.text(0).empty()&&!query.is_null(1)&&query.text(1).empty()&&query.blob(2)==payload,"NULL versus empty and binary bytes");
        check(!query.step()&&saved==hostile,"owning column values survive step");
        bool no_row=false;try{(void)query.text(0);}catch(const db_error& e){no_row=e.code==SQLITE_MISUSE;}
        check(no_row,"column access needs a current row");
        std::optional<statement> surviving;
        {database owner(":memory:");surviving.emplace(owner.prepare("SELECT 42"));}
        check(surviving->step()&&surviving->integer(0)==42,"statement keeps connection alive");
        surviving.reset();
        try{transaction change(db);db.exec("INSERT INTO items VALUES(3,'rolled back',NULL,X'')");throw std::runtime_error("application failure");}
        catch(const std::runtime_error&){}
        check(db.integer("SELECT count(*) FROM items")==2&&sqlite3_get_autocommit(db.handle()),"RAII rollback");
        {transaction change(db);db.exec("INSERT INTO items VALUES(3,'committed',NULL,X'')");change.commit();}
        check(db.integer("SELECT count(*) FROM items")==3,"explicit commit");
        bool rejected=false;try{auto bad=db.prepare("SELECT 1; SELECT 2");(void)bad;}catch(const db_error& e){rejected=e.code==SQLITE_MISUSE;}
        check(rejected,"single prepared statement boundary");
        // Trusted controlled counterexample: tail rejection is not an SQL sandbox.
        rejected=false;
        try{auto pragma=db.prepare("PRAGMA foreign_keys=OFF; SELECT 1");(void)pragma;}
        catch(const db_error& e){rejected=e.code==SQLITE_MISUSE;}
        check(rejected&&db.integer("PRAGMA foreign_keys")==0,"prepare-time PRAGMA precedes tail rejection");
        db.exec("PRAGMA foreign_keys=ON");
        const auto dir=std::filesystem::temp_directory_path()/("c12-sqlite-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(dir),"own backup directory");
        backup(db,dir/"copy.db");
        {database restored(dir/"copy.db");check(restored.integer("SELECT count(*) FROM items")==3,"online backup restore");
         auto integrity=restored.prepare("PRAGMA integrity_check");check(integrity.step()&&integrity.text(0)=="ok","backup integrity");}
        rejected=false;try{backup(db,dir/"copy.db");}catch(const db_error& e){rejected=e.code==SQLITE_CANTOPEN;}
        check(rejected,"backup does not overwrite user destination");
        const auto failed_destination=dir/"unfinished.db";
        {
            transaction writer(db);db.exec("INSERT INTO items VALUES(4,'uncommitted',NULL,X'')");
            rejected=false;try{backup(db,failed_destination);}catch(const db_error& e){rejected=(e.code&255)==SQLITE_BUSY;}
            check(rejected&&std::filesystem::exists(failed_destination),"failed backup retains caller-owned incomplete destination");
        }
        check(db.integer("SELECT count(*) FROM items")==3,"backup failure does not commit writer");
        rejected=false;try{backup(db,failed_destination);}catch(const db_error& e){rejected=e.code==SQLITE_CANTOPEN;}
        check(rejected,"failed backup path is not silently overwritten");
        auto before_move=db.prepare("SELECT 1");auto after_move=std::move(before_move);
        rejected=false;try{(void)before_move.status(SQLITE_STMTSTATUS_VM_STEP);}catch(const db_error& e){rejected=e.code==SQLITE_MISUSE;}
        check(rejected,"moved statement cannot produce fake zero counters");
        rejected=false;try{(void)after_move.status(-99);}catch(const db_error& e){rejected=e.code==SQLITE_MISUSE;}
        check(rejected,"counter operation validated");
        std::cout<<"SQLite bindings, lifetimes, transactions and backup checks passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
