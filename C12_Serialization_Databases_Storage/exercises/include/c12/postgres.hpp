#pragma once
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#include <cerrno>
#endif
#include <libpq-fe.h>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace c12::pg {
using clock=std::chrono::steady_clock;
using parameter=std::optional<std::string>;
class sql_error:public std::runtime_error{
public:
    std::string state;
    sql_error(std::string code,std::string message):std::runtime_error(std::move(message)),state(std::move(code)){}
};
struct readiness{bool readable;bool writable;};
inline readiness wait_socket(int fd,bool read,bool write,clock::time_point deadline){
    if(fd<0)throw std::runtime_error("invalid database socket");
#ifndef _WIN32
    if(fd>=FD_SETSIZE)throw std::runtime_error("socket exceeds select capacity");
#endif
    for(;;){
        auto remaining=std::chrono::duration_cast<std::chrono::microseconds>(deadline-clock::now());
        if(remaining.count()<=0)throw std::runtime_error("database transport deadline; outcome may be unknown");
        fd_set readers,writers;FD_ZERO(&readers);FD_ZERO(&writers);
#ifdef _WIN32
        const auto socket=static_cast<SOCKET>(fd);
#else
        const auto socket=fd;
#endif
        if(read)FD_SET(socket,&readers);if(write)FD_SET(socket,&writers);
        timeval timeout{static_cast<long>(remaining.count()/1000000),static_cast<long>(remaining.count()%1000000)};
        #ifdef _WIN32
        const int descriptor_bound=0;
#else
        const int descriptor_bound=fd+1;
#endif
        const auto status=select(descriptor_bound,read?&readers:nullptr,write?&writers:nullptr,nullptr,&timeout);
        if(status<0){
#ifdef _WIN32
            if(WSAGetLastError()==WSAEINTR)continue;
#else
            if(errno==EINTR)continue;
#endif
            throw std::runtime_error("database select failed");
        }
        if(!status)throw std::runtime_error("database transport deadline; outcome may be unknown");
        return {read&&FD_ISSET(socket,&readers)!=0,write&&FD_ISSET(socket,&writers)!=0};
    }
}
class result{
    std::unique_ptr<PGresult,decltype(&PQclear)> value_{nullptr,PQclear};
    PGresult* current()const{if(!value_)throw std::logic_error("moved database result");return value_.get();}
public:
    explicit result(PGresult* value):value_(value,PQclear){}
    result(result&&) noexcept=default;result& operator=(result&&) noexcept=default;
    int rows()const{return PQntuples(current());}
    int columns()const{return PQnfields(current());}
    std::optional<std::string> value(int row,int column)const{
        if(row<0||row>=rows()||column<0||column>=columns())throw std::out_of_range("database result cell");
        if(PQgetisnull(value_.get(),row,column))return {};
        const int size=PQgetlength(value_.get(),row,column);
        if(size<0||size>1024*1024)throw std::runtime_error("database result field budget");
        return std::string(PQgetvalue(value_.get(),row,column),static_cast<std::size_t>(size));
    }
    std::int64_t integer(int row=0,int column=0)const{
        const auto text=value(row,column);if(!text)throw std::runtime_error("SQL NULL is not an integer");
        std::int64_t number=0;
        const auto [end,error]=std::from_chars(text->data(),text->data()+text->size(),number);
        if(error!=std::errc{}||end!=text->data()+text->size())throw std::runtime_error("invalid database integer representation");
        return number;
    }
};
class connection{
    std::unique_ptr<PGconn,decltype(&PQfinish)> connection_{nullptr,PQfinish};
    bool active_=false,cancel_sent_=false;
    clock::time_point deadline_{};
    [[noreturn]] void transport(const char* operation){
        const std::string message=connection_?PQerrorMessage(connection_.get()):"closed connection";
        throw std::runtime_error(std::string(operation)+": "+message);
    }
    void require_connected()const{
        if(!connection_||PQstatus(connection_.get())!=CONNECTION_OK)throw std::runtime_error("database connection is closed");
    }
    void flush(){
        for(;;){
            const auto pending=PQflush(connection_.get());
            if(pending==0)return;
            if(pending<0)transport("flush");
            // Reading while output is pending avoids both peers waiting on full buffers.
            const auto ready=wait_socket(PQsocket(connection_.get()),true,true,deadline_);
            if(ready.readable&&!PQconsumeInput(connection_.get()))transport("consume during flush");
        }
    }
public:
    connection(){
        const char* keys[]={"host","port","dbname","user","password","sslmode","connect_timeout","client_encoding",nullptr};
        const char* values[]={std::getenv("PGHOST"),std::getenv("PGPORT"),std::getenv("PGDATABASE"),std::getenv("PGUSER"),
                              std::getenv("PGPASSWORD"),std::getenv("PGSSLMODE"),"3","UTF8",nullptr};
        for(int n=0;n!=6;++n)if(!values[n])throw std::invalid_argument("explicit fixture connection parameters required");
        connection_.reset(PQconnectdbParams(keys,values,0));
        if(!connected())transport("connect");
        if(PQsetnonblocking(connection_.get(),1)!=0)transport("nonblocking mode");
    }
    connection(const connection&)=delete;connection& operator=(const connection&)=delete;
    void close()noexcept{active_=false;cancel_sent_=false;connection_.reset();}
    bool connected()const noexcept{return connection_&&PQstatus(connection_.get())==CONNECTION_OK;}
    PGTransactionStatusType transaction_status()const noexcept{return connection_?PQtransactionStatus(connection_.get()):PQTRANS_UNKNOWN;}
    int pid()const{require_connected();return PQbackendPID(connection_.get());}
    void start(std::string_view sql,std::span<const parameter> parameters={},std::chrono::milliseconds budget=std::chrono::seconds(5)){
        require_connected();
        if(active_)throw std::logic_error("previous database command must drain");
        if(sql.empty()||sql.size()>1024*1024||sql.find('\0')!=std::string_view::npos||parameters.size()>32||
           budget<=std::chrono::milliseconds::zero()||budget>std::chrono::seconds(30))
            throw std::invalid_argument("database command boundary");
        std::size_t total=sql.size();std::vector<const char*> pointers;pointers.reserve(parameters.size());
        for(const auto& value:parameters){
            if(value&&(value->find('\0')!=std::string::npos||value->size()>1024*1024-total))
                throw std::invalid_argument("text parameter boundary");
            if(value)total+=value->size();
            pointers.push_back(value?value->c_str():nullptr);
        }
        const std::string query(sql);deadline_=clock::now()+budget;cancel_sent_=false;
        try{
            if(!PQsendQueryParams(connection_.get(),query.c_str(),static_cast<int>(pointers.size()),nullptr,
                                  pointers.empty()?nullptr:pointers.data(),nullptr,nullptr,0))transport("send");
            active_=true;flush();
        }catch(...){close();throw;}
    }
    // One PGresult is available; callers still have to drain the entire command.
    void wait_ready(){
        require_connected();if(!active_)throw std::logic_error("no active query");
        try{
            if(clock::now()>=deadline_)throw std::runtime_error("database transport deadline; outcome may be unknown");
            while(PQisBusy(connection_.get())){
                (void)wait_socket(PQsocket(connection_.get()),true,false,deadline_);
                if(!PQconsumeInput(connection_.get()))transport("consume");
            }
        }catch(...){close();throw;}
    }
    result finish(){
        require_connected();if(!active_)throw std::logic_error("no active query");
        std::unique_ptr<PGresult,decltype(&PQclear)> last{nullptr,PQclear};
        std::string error_state,error_message;
        try{
            for(;;){
                wait_ready();
                std::unique_ptr<PGresult,decltype(&PQclear)> next{PQgetResult(connection_.get()),PQclear};
                if(!next)break;
                const auto status=PQresultStatus(next.get());
                if(status!=PGRES_TUPLES_OK&&status!=PGRES_COMMAND_OK&&status!=PGRES_FATAL_ERROR&&status!=PGRES_EMPTY_QUERY)
                    throw std::runtime_error("unsupported database result protocol; connection closed");
                if(status!=PGRES_TUPLES_OK&&status!=PGRES_COMMAND_OK){
                    const char* state=PQresultErrorField(next.get(),PG_DIAG_SQLSTATE);
                    if(error_message.empty()){error_state=state?state:"";error_message=PQresultErrorMessage(next.get());
                        if(error_message.empty())error_message="empty or unexpected database command result";}
                }
                last=std::move(next);
            }
            if(PQstatus(connection_.get())!=CONNECTION_OK)transport("result completion");
            active_=false;
            if(!last)transport("missing result");
        }catch(...){close();throw;}
        // A late cancel with no cancellation result is not returned to a reusable pool.
        if(cancel_sent_&&error_state!="57014")close();
        if(!error_message.empty())throw sql_error(error_state,error_message);
        if(PQntuples(last.get())>4096)throw std::runtime_error("result row budget; use a cursor/chunked mode for large results");
        return result(last.release());
    }
    result query(std::string_view sql,std::span<const parameter> parameters={},std::chrono::milliseconds budget=std::chrono::seconds(5)){
        start(sql,parameters,budget);return finish();
    }
    bool cancel(){
        require_connected();if(!active_)return false;
        std::unique_ptr<PGcancelConn,decltype(&PQcancelFinish)> control{PQcancelCreate(connection_.get()),PQcancelFinish};
        try{
            if(!control||PQcancelStatus(control.get())!=CONNECTION_ALLOCATED)throw std::runtime_error("cancel allocation failed");
            if(PQcancelStart(control.get())!=1)throw std::runtime_error(PQcancelErrorMessage(control.get()));
            auto status=PGRES_POLLING_WRITING;
            const auto limit=std::min(deadline_,clock::now()+std::chrono::seconds(3));
            for(;;){
                const auto fd=PQcancelSocket(control.get());
                (void)wait_socket(fd,status==PGRES_POLLING_READING,status==PGRES_POLLING_WRITING,limit);
                status=PQcancelPoll(control.get());
                if(status==PGRES_POLLING_OK){cancel_sent_=true;return true;} // dispatch is not query completion
                if(status==PGRES_POLLING_FAILED)throw std::runtime_error(PQcancelErrorMessage(control.get()));
                if(status!=PGRES_POLLING_READING&&status!=PGRES_POLLING_WRITING)
                    throw std::runtime_error("unexpected cancel polling state");
            }
        }catch(...){close();throw;}
    }
};
}
