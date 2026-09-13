#include <c12/postgres.hpp>
#include <check.hpp>
#include <array>
#include <iostream>
using c12::pg::connection;
using c12::pg::parameter;
using c12::pg::sql_error;
using query_clock=c12::pg::clock;
using namespace std::chrono_literals;

void client(){
    check(PQlibVersion()==180006,"pinned libpq runtime");
    connection db;
    check(db.query("SHOW server_version_num").integer()==180006,"pinned server runtime");
    db.query("CREATE TABLE payload(id integer PRIMARY KEY,name text NOT NULL,note text,body bytea NOT NULL)");
    const std::string hostile="x'); DROP TABLE payload; --";
    const std::array parameters{parameter{"1"},parameter{hostile},parameter{},parameter{"\\x00ff"}};
    db.query("INSERT INTO payload VALUES($1::integer,$2::text,$3::text,$4::bytea)",parameters);
    auto row=db.query("SELECT id,name,note,encode(body,'hex') FROM payload");
    check(row.rows()==1&&row.integer()==1&&row.value(0,1)==parameter{hostile}&&!row.value(0,2)&&row.value(0,3)==parameter{"00ff"},"parameters, NULL and bytea");
    const std::array empty{parameter{""}};
    check(db.query("SELECT $1::text",empty).value(0,0)==parameter{""},"empty text is present");
    bool rejected=false;
    const std::array nul{parameter{std::string("a\0b",3)}};
    try{db.query("SELECT $1::text",nul);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&db.connected(),"NUL rejected before text protocol");
    rejected=false;try{db.query("INSERT INTO payload VALUES(1,'duplicate',NULL,''::bytea)");}
    catch(const sql_error& e){rejected=e.state=="23505";}
    check(rejected&&db.transaction_status()==PQTRANS_IDLE&&db.query("SELECT 1").integer()==1,"SQLSTATE and error drain");
    std::optional<c12::pg::result> held;
    {connection owner;held.emplace(owner.query("SELECT 'owned'::text"));}
    check(held->value(0,0)==parameter{"owned"},"PGresult outlives connection");
    rejected=false;try{(void)db.query("SELECT NULL::bigint").integer();}catch(const std::runtime_error&){rejected=true;}
    check(rejected,"NULL is not numeric zero");
    connection unsupported;
    rejected=false;try{unsupported.query("COPY (SELECT 1) TO STDOUT");}catch(const std::runtime_error&){rejected=true;}
    check(rejected&&!unsupported.connected(),"unsupported COPY closes instead of leaving protocol unread");
    rejected=false;try{db.query("-- no statement");}catch(const sql_error&){rejected=true;}
    check(rejected&&db.query("SELECT 2").integer()==2,"empty command cannot masquerade as success");
    rejected=false;try{db.query("SELECT generate_series(1,4097)");}catch(const std::runtime_error&){rejected=true;}
    check(rejected&&db.query("SELECT 3").integer()==3,"result budget checked after complete drain");
    std::cout<<"libpq parameters, results, SQLSTATE and protocol checks passed\n";
}
void isolation(){
    connection a,b;
    a.query("CREATE TABLE counter(id integer PRIMARY KEY,value integer NOT NULL)");
    a.query("INSERT INTO counter VALUES(1,0)");
    for(const auto* level:{"READ COMMITTED","REPEATABLE READ"}){
        a.query("UPDATE counter SET value=0");
        a.query(std::string("BEGIN ISOLATION LEVEL ")+level);
        check(a.query("SELECT value FROM counter").integer()==0,"first statement establishes snapshot");
        b.query("UPDATE counter SET value=1");
        const auto observed=a.query("SELECT value FROM counter").integer();
        const auto expected=std::string_view(level)=="READ COMMITTED"?1:0;
        check(observed==expected,"statement versus transaction snapshot");
        a.query("COMMIT");
        check(a.query("SELECT value FROM counter").integer()==1,"new transaction sees committed value");
        std::cout<<"isolation="<<level<<" second_read="<<observed<<"\n";
    }
    a.query("CREATE TABLE doctors(id integer PRIMARY KEY,on_call boolean NOT NULL)");
    a.query("INSERT INTO doctors VALUES(1,true),(2,true)");
    for(const auto* level:{"REPEATABLE READ","SERIALIZABLE"}){
        a.query("UPDATE doctors SET on_call=true");
        a.query(std::string("BEGIN ISOLATION LEVEL ")+level);
        b.query(std::string("BEGIN ISOLATION LEVEL ")+level);
        check(a.query("SELECT count(*) FROM doctors WHERE on_call").integer()==2,"A decision snapshot");
        check(b.query("SELECT count(*) FROM doctors WHERE on_call").integer()==2,"B decision snapshot");
        bool a_failed=false,b_failed=false;std::string observed_state;
        auto change=[&](connection& db,const char* sql,bool& failed){
            try{db.query(sql);}catch(const sql_error& e){
                check(e.state=="40001","expected serialization failure");
                failed=true;observed_state=e.state;db.query("ROLLBACK");
            }
        };
        change(a,"UPDATE doctors SET on_call=false WHERE id=1",a_failed);
        change(b,"UPDATE doctors SET on_call=false WHERE id=2",b_failed);
        if(!a_failed)change(a,"COMMIT",a_failed);
        if(!b_failed)change(b,"COMMIT",b_failed);
        const auto remaining=a.query("SELECT count(*) FROM doctors WHERE on_call").integer();
        if(std::string_view(level)=="REPEATABLE READ"){
            check(!a_failed&&!b_failed&&remaining==0,"snapshot isolation permits disjoint write skew");
        }else{
            check((a_failed||b_failed)&&observed_state=="40001"&&remaining>=1,"serializable rejects the anomalous history");
            connection& retry=a_failed?a:b;
            const auto id=a_failed?1:2;
            retry.query("BEGIN ISOLATION LEVEL SERIALIZABLE");
            const auto fresh=retry.query("SELECT count(*) FROM doctors WHERE on_call").integer();
            if(fresh>1){
                const std::array key{parameter{std::to_string(id)}};
                retry.query("UPDATE doctors SET on_call=false WHERE id=$1::integer",key);
            }
            retry.query("COMMIT");
            check(a.query("SELECT count(*) FROM doctors WHERE on_call").integer()>=1,"retry repeats read and decision");
        }
        std::cout<<"isolation="<<level<<" remaining_on_call="<<remaining<<" sqlstate="<<observed_state<<"\n";
    }
}
void await_lock(connection& holder,connection& worker){
    const std::array ids{parameter{std::to_string(worker.pid())},parameter{std::to_string(holder.pid())}};
    const auto end=query_clock::now()+5s;
    do{
        const auto waiting=holder.query(
            "SELECT count(*) FROM pg_locks WHERE pid=$1::integer AND locktype='advisory' "
            "AND classid=12 AND objid=1 AND objsubid=2 AND NOT granted "
            "AND $2::integer=ANY(pg_blocking_pids($1::integer))",ids,1s).integer();
        if(waiting>0)return;
    }while(query_clock::now()<end);
    throw std::runtime_error("lock wait was not observed");
}
void cancellation(){
    connection holder,worker;
    holder.query("SET statement_timeout=0");holder.query("SET lock_timeout=0");
    worker.query("SET statement_timeout=0");worker.query("SET lock_timeout=0");
    check(worker.query("SHOW statement_timeout").value(0,0)==parameter{"0"}&&
          worker.query("SHOW lock_timeout").value(0,0)==parameter{"0"},"only explicit cancellation is active");
    holder.query("SELECT pg_advisory_lock(12,1)");
    worker.query("BEGIN");
    worker.start("SELECT pg_advisory_xact_lock(12,1)");
    await_lock(holder,worker);
    check(worker.cancel(),"cancel connection dispatched request");
    std::string state;
    try{worker.finish();}catch(const sql_error& e){state=e.state;}
    check(state=="57014"&&worker.transaction_status()==PQTRANS_INERROR,"original query confirms cancellation");
    worker.query("ROLLBACK");
    check(worker.transaction_status()==PQTRANS_IDLE&&worker.query("SELECT 1").integer()==1,"drain and rollback restore reuse");
    check(holder.query("SELECT pg_advisory_unlock(12,1)").value(0,0)==parameter{"t"},"holder remained until cancellation");
    std::cout<<"cancel_sqlstate="<<state<<" connection_reused=true\n";

    connection completed;
    completed.start("SELECT 42");completed.wait_ready();
    const std::array pid{parameter{std::to_string(completed.pid())}};
    const auto idle_deadline=query_clock::now()+5s;
    bool idle=false;
    do{
        idle=holder.query("SELECT (state='idle')::integer FROM pg_stat_activity WHERE pid=$1::integer",pid,1s).integer()==1;
    }while(!idle&&query_clock::now()<idle_deadline);
    check(idle&&completed.cancel(),"completion precedes cancel dispatch");
    check(completed.finish().integer()==42&&!completed.connected(),"completed result retained; late-cancel connection discarded");
    std::cout<<"completion_first_result=42 connection_reused=false\n";

    connection timed;
    holder.query("SELECT pg_advisory_lock(12,1)");
    timed.start("SELECT pg_advisory_xact_lock(12,1)",{},250ms);
    await_lock(holder,timed);
    bool timeout=false;try{timed.finish();}catch(const std::runtime_error&){timeout=true;}
    check(timeout&&!timed.connected(),"deadline discards unread connection");
    holder.query("SELECT pg_advisory_unlock(12,1)");
    std::cout<<"deadline_connection_closed=true\n";
}
int main(int argc,char** argv){
    try{
        const std::string_view selected=argc>1?argv[1]:"all";
        if(selected=="all"||selected=="client")client();
        if(selected=="all"||selected=="isolation")isolation();
        if(selected=="all"||selected=="cancel")cancellation();
        if(selected!="all"&&selected!="client"&&selected!="isolation"&&selected!="cancel")return 2;
    }catch(const sql_error& e){std::cerr<<"SQLSTATE "<<e.state<<": "<<e.what()<<"\n";return 1;}
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
