#include <c12/sqlite.hpp>
#include <check.hpp>
#include <functional>
#include <iostream>
using namespace c12::sql;
void create_v1(database& db){
    db.exec("CREATE TABLE jobs(id INTEGER PRIMARY KEY,name TEXT NOT NULL) STRICT;"
            "INSERT INTO jobs VALUES(1,'before'); PRAGMA user_version=1");
}
void migrate(database& db,const std::function<void(std::string_view)>& hook={}){
    transaction change(db);
    const auto version=db.integer("PRAGMA user_version");
    if(version==2){change.commit();return;}
    if(version!=1)throw db_error(SQLITE_SCHEMA,"unsupported application schema version");
    db.exec("ALTER TABLE jobs ADD COLUMN note TEXT; UPDATE jobs SET note='legacy'");
    if(hook)hook("after_ddl");
    db.exec("CREATE INDEX jobs_name ON jobs(name); PRAGMA user_version=2");
    change.commit();
    if(hook)hook("after_commit");
}
void inspect(database& db){
    const auto version=db.integer("PRAGMA user_version");
    const auto columns=db.integer("SELECT count(*) FROM pragma_table_info('jobs')");
    const auto rows=db.integer("SELECT count(*) FROM jobs");
    const auto legacy=columns==3?db.integer("SELECT note='legacy' FROM jobs WHERE id=1"):0;
    std::cout<<"version="<<version<<" columns="<<columns<<" rows="<<rows<<" legacy="<<legacy<<"\n";
}
void self_test(){
    database db(":memory:");create_v1(db);
    bool interrupted=false;
    try{migrate(db,[](std::string_view point){if(point=="after_ddl")throw std::runtime_error("controlled application interruption");});}
    catch(const std::runtime_error&){interrupted=true;}
    check(interrupted&&db.integer("PRAGMA user_version")==1&&db.integer("SELECT count(*) FROM pragma_table_info('jobs')")==2,"DDL and version roll back together");
    migrate(db);migrate(db);
    check(db.integer("PRAGMA user_version")==2&&db.integer("SELECT count(*) FROM pragma_table_info('jobs')")==3,"idempotent schema upgrade");
    db.exec("INSERT INTO jobs(id,name) VALUES(2,'old writer')");
    check(db.integer("SELECT note IS NULL FROM jobs WHERE id=2")==1,"old explicit-column writer remains compatible");
    {
        transaction outer(db);
        db.exec("INSERT INTO jobs(id,name) VALUES(3,'outer'); SAVEPOINT optional_step;"
                "UPDATE jobs SET name='temporary' WHERE id=1; ROLLBACK TO optional_step; RELEASE optional_step");
        bool nested=false;try{transaction invalid(db);}catch(const db_error& e){nested=(e.code&255)==SQLITE_ERROR;}
        check(nested&&!sqlite3_get_autocommit(db.handle()),"nested BEGIN rejection keeps outer transaction");
        outer.commit();
    }
    check(db.integer("SELECT count(*) FROM jobs")==3&&db.integer("SELECT name='before' FROM jobs WHERE id=1")==1,"savepoint rolls back only inner work");
    db.exec("PRAGMA user_version=3");
    bool future=false;try{migrate(db);}catch(const db_error& e){future=e.code==SQLITE_SCHEMA;}
    check(future&&db.integer("PRAGMA user_version")==3,"unknown future schema rejected");
    std::cout<<"migration and savepoint checks passed\n";
}
int main(int argc,char** argv){
    try{
        if(argc==1){self_test();return 0;}
        if(argc<3)return 2;
        const std::string mode=argv[1],argument=argc>3?argv[3]:"";
        const std::filesystem::path path=argv[2];
        if(mode=="init"&&std::filesystem::exists(path))return 2;
        if(mode!="init"&&!std::filesystem::exists(path))return 2;
        database db(path);db.exec("PRAGMA synchronous=FULL; PRAGMA wal_autocheckpoint=0");
        check(db.integer("PRAGMA synchronous")==2,"explicit FULL synchronization");
        if(mode=="init"){
            if(argument!="wal"&&argument!="delete")return 2;
            {auto journal=db.prepare(argument=="wal"?"PRAGMA journal_mode=WAL":"PRAGMA journal_mode=DELETE");
             check(journal.step()&&journal.text(0)==argument,"actual journal mode");}
            create_v1(db);std::cout<<"INIT\n";
        }else if(mode=="migrate"){
            migrate(db,[&](std::string_view point){
                if(point==argument){std::cout<<"READY "<<point<<std::endl;std::string release;std::getline(std::cin,release);}
            });
            std::cout<<"ACK 2\n";
        }else if(mode=="inspect")inspect(db);
        else return 2;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
