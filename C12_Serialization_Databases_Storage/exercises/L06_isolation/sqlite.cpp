#include <c12/sqlite.hpp>
#include <check.hpp>
#include <chrono>
#include <iostream>
int main(){
    using namespace c12::sql;
    try{
        const auto directory=std::filesystem::temp_directory_path()/("c12-isolation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(directory),"own isolation directory");
        database a(directory/"main.db",0),b(directory/"main.db",0);
        {auto mode=a.prepare("PRAGMA journal_mode=WAL");check(mode.step()&&mode.text(0)=="wal","actual WAL mode");}
        a.exec("PRAGMA synchronous=FULL; PRAGMA wal_autocheckpoint=0");
        b.exec("PRAGMA synchronous=FULL; PRAGMA wal_autocheckpoint=0");
        check(a.integer("PRAGMA synchronous")==2&&b.integer("PRAGMA synchronous")==2,"actual FULL synchronization");
        a.exec("CREATE TABLE counter(id INTEGER PRIMARY KEY, value INTEGER NOT NULL); INSERT INTO counter VALUES(1,10)");
        bool stale=false;
        try{
            transaction read(a,transaction_kind::deferred);
            check(a.integer("SELECT value FROM counter")==10,"reader starts at 10");
            b.exec("UPDATE counter SET value=11");
            check(a.integer("SELECT value FROM counter")==10,"existing snapshot remains 10");
            a.exec("UPDATE counter SET value=12");
        }catch(const db_error& e){stale=e.code==SQLITE_BUSY_SNAPSHOT;}
        check(stale&&sqlite3_get_autocommit(a.handle()),"stale reader must restart entire transaction");
        {transaction retry(a);check(a.integer("SELECT value FROM counter")==11,"retry reads fresh state");
         a.exec("UPDATE counter SET value=value+1");retry.commit();}
        check(b.integer("SELECT value FROM counter")==12,"retry committed");
        {
            transaction writer(b);bool busy=false;
            try{transaction second(a);}catch(const db_error& e){busy=(e.code&255)==SQLITE_BUSY;}
            check(busy,"only one writer admitted");
        }
        int frames=-1,copied=-1;
        {
            transaction reader(a,transaction_kind::deferred);
            check(a.integer("SELECT value FROM counter")==12,"long reader snapshot");
            b.exec("UPDATE counter SET value=13");
            require(b.handle(),sqlite3_wal_checkpoint_v2(b.handle(),nullptr,SQLITE_CHECKPOINT_PASSIVE,&frames,&copied));
            check(frames>copied&&copied>=0,"long reader limits checkpoint progress");
            std::cout<<"{\"stage\":\"reader_active\",\"wal_frames\":"<<frames<<",\"checkpointed\":"<<copied<<"}\n";
        }
        require(b.handle(),sqlite3_wal_checkpoint_v2(b.handle(),nullptr,SQLITE_CHECKPOINT_TRUNCATE,&frames,&copied));
        check(frames==0&&copied==0&&b.integer("SELECT value FROM counter")==13,"reader release permits truncation");
        std::cout<<"{\"stage\":\"reader_released\",\"wal_frames\":"<<frames<<",\"checkpointed\":"<<copied<<"}\n";
        std::cout<<"SQLite snapshot, writer contention and checkpoint checks passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
