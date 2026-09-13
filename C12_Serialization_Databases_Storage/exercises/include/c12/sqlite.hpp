#pragma once
#include <c12/bytes.hpp>
#include <sqlite3.h>
#include <filesystem>
#include <memory>
#include <cctype>
#include <array>

namespace c12::sql {
class db_error:public std::runtime_error{
public:
    int code;
    db_error(int value,const std::string& message):std::runtime_error(message),code(value){}
};
inline void require(sqlite3* db,int code){
    if(code!=SQLITE_OK)throw db_error(code,db?sqlite3_errmsg(db):sqlite3_errstr(code));
}
using native_connection=std::unique_ptr<sqlite3,decltype(&sqlite3_close_v2)>;
struct connection_state{
    native_connection native{nullptr,sqlite3_close_v2};
    int cleanup_error=SQLITE_OK;
    explicit connection_state(native_connection db):native(std::move(db)){}
    sqlite3* checked()const{
        if(cleanup_error!=SQLITE_OK)throw db_error(cleanup_error,"connection quarantined after rollback failure; reopen");
        if(!native)throw db_error(SQLITE_MISUSE,"no connection");
        return native.get();
    }
};
inline sqlite3* checked(const std::shared_ptr<connection_state>& owner){
    if(!owner)throw db_error(SQLITE_MISUSE,"moved connection or statement");
    return owner->checked();
}
inline void execute(const std::shared_ptr<connection_state>& owner,std::string_view query){
    auto* db=checked(owner);
    if(query.size()>1024*1024||query.find('\0')!=std::string_view::npos)
        throw db_error(SQLITE_MISUSE,"invalid SQL command boundary");
    const std::string owned(query);
    require(db,sqlite3_exec(db,owned.c_str(),nullptr,nullptr,nullptr));
}
class database;
class statement{
    friend class database;
    std::shared_ptr<connection_state> owner_;
    std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> statement_{nullptr,sqlite3_finalize};
    bool on_row_=false;
    sqlite3_stmt* current()const{
        (void)checked(owner_);
        if(!statement_)throw db_error(SQLITE_MISUSE,"no statement");
        return statement_.get();
    }
    statement(std::shared_ptr<connection_state> owner,std::string_view query):owner_(std::move(owner)){
        auto* db=checked(owner_);
        if(query.empty()||query.size()>1024*1024||query.find('\0')!=std::string_view::npos)
            throw db_error(SQLITE_MISUSE,"invalid prepared SQL boundary");
        sqlite3_stmt* raw=nullptr;const char* tail=nullptr;
        const auto rc=sqlite3_prepare_v3(db,query.data(),static_cast<int>(query.size()),SQLITE_PREPARE_PERSISTENT,&raw,&tail);
        statement_.reset(raw);require(db,rc);
        if(!raw)throw db_error(SQLITE_MISUSE,"SQL produced no statement");
        const auto end=query.data()+query.size();
        while(tail<end&&std::isspace(static_cast<unsigned char>(*tail)))++tail;
        if(tail!=end)throw db_error(SQLITE_MISUSE,"prepare accepts exactly one trusted statement");
    }
    void column(int index)const{
        auto* value=current();
        if(!on_row_)throw db_error(SQLITE_MISUSE,"no current row");
        if(index<0||index>=sqlite3_column_count(value))throw db_error(SQLITE_RANGE,"column index");
    }
public:
    statement(const statement&)=delete;statement& operator=(const statement&)=delete;
    statement(statement&&) noexcept=default;statement& operator=(statement&&) noexcept=default;
    void bind(int index,std::int64_t value){require(checked(owner_),sqlite3_bind_int64(current(),index,value));}
    void bind(int index,std::string_view value){
        auto* query=current();
        if(value.size()>1024*1024)throw db_error(SQLITE_TOOBIG,"text limit");
        require(checked(owner_),sqlite3_bind_text64(query,index,value.empty()?"":value.data(),value.size(),SQLITE_TRANSIENT,SQLITE_UTF8));
    }
    void bind_null(int index){require(checked(owner_),sqlite3_bind_null(current(),index));}
    void bind_blob(int index,std::span<const std::byte> value){
        auto* query=current();
        if(value.size()>1024*1024)throw db_error(SQLITE_TOOBIG,"blob limit");
        static constexpr std::byte empty{};
        require(checked(owner_),sqlite3_bind_blob64(query,index,value.empty()?&empty:value.data(),value.size(),SQLITE_TRANSIENT));
    }
    bool step(){
        auto* query=current();on_row_=false;const auto rc=sqlite3_step(query);
        if(rc==SQLITE_ROW){on_row_=true;return true;}
        if(rc==SQLITE_DONE)return false;
        throw db_error(rc,sqlite3_errmsg(checked(owner_)));
    }
    void reset(){
        auto* query=current();on_row_=false;const auto rc=sqlite3_reset(query);
        const auto cleared=sqlite3_clear_bindings(query);
        require(checked(owner_),rc);require(checked(owner_),cleared);
    }
    bool is_null(int index)const{column(index);return sqlite3_column_type(statement_.get(),index)==SQLITE_NULL;}
    std::int64_t integer(int index)const{
        column(index);
        if(sqlite3_column_type(statement_.get(),index)!=SQLITE_INTEGER)throw db_error(SQLITE_MISMATCH,"expected INTEGER");
        return sqlite3_column_int64(statement_.get(),index);
    }
    std::string text(int index)const{
        column(index);
        if(sqlite3_column_type(statement_.get(),index)!=SQLITE_TEXT)throw db_error(SQLITE_MISMATCH,"expected TEXT");
        auto* p=sqlite3_column_text(statement_.get(),index);
        if(!p)throw db_error(SQLITE_NOMEM,"column text allocation");
        const int n=sqlite3_column_bytes(statement_.get(),index);
        return n?std::string(reinterpret_cast<const char*>(p),static_cast<std::size_t>(n)):std::string{};
    }
    bytes blob(int index)const{
        column(index);
        if(sqlite3_column_type(statement_.get(),index)!=SQLITE_BLOB)throw db_error(SQLITE_MISMATCH,"expected BLOB");
        auto* p=static_cast<const std::byte*>(sqlite3_column_blob(statement_.get(),index));
        if(!p&&sqlite3_errcode(checked(owner_))==SQLITE_NOMEM)throw db_error(SQLITE_NOMEM,"column blob allocation");
        const auto n=sqlite3_column_bytes(statement_.get(),index);
        if(n&&!p)throw db_error(SQLITE_NOMEM,"column blob allocation");
        return n?bytes(p,p+n):bytes{};
    }
    int status(int operation,bool reset=false){
        auto* query=current();
        constexpr std::array allowed{SQLITE_STMTSTATUS_FULLSCAN_STEP,SQLITE_STMTSTATUS_SORT,SQLITE_STMTSTATUS_AUTOINDEX,
            SQLITE_STMTSTATUS_VM_STEP,SQLITE_STMTSTATUS_REPREPARE,SQLITE_STMTSTATUS_RUN,SQLITE_STMTSTATUS_FILTER_MISS,
            SQLITE_STMTSTATUS_FILTER_HIT,SQLITE_STMTSTATUS_MEMUSED};
        if(std::find(allowed.begin(),allowed.end(),operation)==allowed.end())throw db_error(SQLITE_MISUSE,"unknown statement counter");
        return sqlite3_stmt_status(query,operation,reset?1:0);
    }
};
class database{
    friend class transaction;
    std::shared_ptr<connection_state> connection_;
public:
    explicit database(const std::filesystem::path& path,int busy_ms=1000){
        if(busy_ms<0||busy_ms>5000)throw db_error(SQLITE_MISUSE,"busy timeout range");
        const auto utf8=path.u8string();
        const std::string name(reinterpret_cast<const char*>(utf8.data()),utf8.size());
        if(name.find('\0')!=std::string::npos)throw db_error(SQLITE_MISUSE,"database path NUL");
        sqlite3* raw=nullptr;
        const auto rc=sqlite3_open_v2(name.c_str(),&raw,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,nullptr);
        native_connection opened(raw,sqlite3_close_v2);require(raw,rc);
        connection_=std::make_shared<connection_state>(std::move(opened));
        require(raw,sqlite3_extended_result_codes(raw,1));require(raw,sqlite3_busy_timeout(raw,busy_ms));
        exec("PRAGMA foreign_keys=ON");
    }
    database(const database&)=delete;database& operator=(const database&)=delete;
    database(database&&) noexcept=default;database& operator=(database&&) noexcept=default;
    sqlite3* handle()const{return checked(connection_);}
    int cleanup_error()const noexcept{return connection_?connection_->cleanup_error:SQLITE_MISUSE;}
    statement prepare(std::string_view query){return statement(connection_,query);}
    void exec(std::string_view query){execute(connection_,query);}
    std::int64_t integer(std::string_view query){
        auto q=prepare(query);if(!q.step())throw db_error(SQLITE_MISMATCH,"expected one integer row");
        const auto value=q.integer(0);if(q.step())throw db_error(SQLITE_MISMATCH,"expected exactly one row");return value;
    }
};
enum class transaction_kind{deferred,immediate};
class transaction{
    std::shared_ptr<connection_state> owner_;
    bool active_=false;
    int rollback_nothrow()noexcept{
        auto* db=owner_->native.get();
        int rc=SQLITE_OK;
        if(!sqlite3_get_autocommit(db)){
            rc=sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);
            if(rc==SQLITE_OK&&!sqlite3_get_autocommit(db))rc=SQLITE_ABORT;
        }
        active_=false;
        if(rc!=SQLITE_OK&&owner_->cleanup_error==SQLITE_OK)owner_->cleanup_error=rc;
        return rc;
    }
public:
    explicit transaction(database& db,transaction_kind kind=transaction_kind::immediate):owner_(db.connection_){
        if(kind!=transaction_kind::immediate&&kind!=transaction_kind::deferred)throw db_error(SQLITE_MISUSE,"transaction kind");
        execute(owner_,kind==transaction_kind::immediate?"BEGIN IMMEDIATE":"BEGIN DEFERRED");active_=true;
    }
    transaction(const transaction&)=delete;transaction& operator=(const transaction&)=delete;
    ~transaction(){if(active_)(void)rollback_nothrow();}
    void rollback(){
        if(!active_)throw db_error(SQLITE_MISUSE,"transaction already ended");
        const auto rc=rollback_nothrow();
        if(rc!=SQLITE_OK)throw db_error(rc,"rollback failed; connection quarantined");
    }
    void commit(){
        if(!active_)throw db_error(SQLITE_MISUSE,"transaction already ended");
        execute(owner_,"COMMIT");active_=false;
    }
};
// Caller exclusively owns the new destination path. On failure keep the incomplete file for diagnosis.
inline void backup(database& source,const std::filesystem::path& destination){
    auto* input=source.handle();
    if(std::filesystem::exists(destination))throw db_error(SQLITE_CANTOPEN,"backup requires a new destination");
    database copy(destination);
    std::unique_ptr<sqlite3_backup,decltype(&sqlite3_backup_finish)> operation{
        sqlite3_backup_init(copy.handle(),"main",input,"main"),sqlite3_backup_finish};
    if(!operation)throw db_error(sqlite3_extended_errcode(copy.handle()),sqlite3_errmsg(copy.handle()));
    int rc=SQLITE_OK;
    for(int round=0;round!=4096&&rc==SQLITE_OK;++round)rc=sqlite3_backup_step(operation.get(),64);
    if(rc==SQLITE_OK)throw db_error(SQLITE_ABORT,"backup page budget exhausted");
    if(rc!=SQLITE_DONE)throw db_error(rc,sqlite3_errmsg(copy.handle()));
    require(copy.handle(),sqlite3_backup_finish(operation.release()));
}
}
