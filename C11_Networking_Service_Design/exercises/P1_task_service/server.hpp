#pragma once
#include <c11/task_policy.hpp>
#include <c11/task_runtime.hpp>
#include <c11/task_json.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <charconv>

namespace c11::service {
namespace asio=boost::asio;namespace beast=boost::beast;namespace http=beast::http;namespace websocket=beast::websocket;
using tcp=asio::ip::tcp;using error_code=boost::system::error_code;
using response=http::response<http::string_body>;
using request=http::request<http::string_body>;
inline std::optional<tasks::id> parse_id(std::string_view text) {
    if(text.empty())return {};
    tasks::id value=0;const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
    if(error!=std::errc{} || end!=text.data()+text.size() || !value)return {};return value;
}
inline http::status status(tasks::error error) {
    switch(error){case tasks::error::invalid:return http::status::bad_request;case tasks::error::conflict:return http::status::conflict;
    case tasks::error::not_found:return http::status::not_found;case tasks::error::overloaded:return http::status::too_many_requests;
    default:return http::status::service_unavailable;}
}
class server;
class session:public std::enable_shared_from_this<session> {
    std::shared_ptr<server> owner_;
    std::optional<beast::tcp_stream> stream_;
    std::optional<websocket::stream<beast::tcp_stream>> websocket_;
    beast::flat_buffer buffer_{64*1024},inbound_{4096};
    std::optional<http::request_parser<http::string_body>> parser_;
    std::optional<response> response_;
    asio::steady_timer timer_,write_timeout_;
    std::optional<tasks::id> subscription_;
    std::string event_;
    bool writing_=false,draining_=false,closed_=false,terminal_event_=false,closing_=false;
    void read();void route(request);void send(http::status,boost::json::value,bool keep_alive);
    void poll_events();void read_control();void begin_close();void arm_write_timeout();
public:
    session(tcp::socket socket,std::shared_ptr<server> owner);
    ~session();
    void start(){read();}
    void drain();void force_close();
};
class server:public std::enable_shared_from_this<server> {
    tcp::acceptor acceptor_;asio::steady_timer shutdown_timer_;
    std::vector<std::weak_ptr<session>> sessions_;std::uint16_t port_;
    bool stopping_=false,jobs_drained_=false;
    void accept() {
        auto self=shared_from_this();
        acceptor_.async_accept([self](error_code error,tcp::socket socket){
            if(!error) {
                if(self->connections>=16){++self->refused;error_code ignored;socket.close(ignored);}
                else {
                    auto connection=std::make_shared<session>(std::move(socket),self);
                    std::erase_if(self->sessions_,[](const auto& item){return item.expired();});
                    self->sessions_.push_back(connection);++self->connections;++self->accepted;connection->start();
                }
            }
            if(!self->stopping_)self->accept();
        });
    }
    friend class session;
public:
    asio::io_context& io;
    std::shared_ptr<tasks::runtime<tasks::task_policy>> engine;
    std::size_t connections=0,accepted=0,refused=0,requests=0,lost_responses=0;
    bool deadline_exceeded=false,cleanup_failed=false;
    void cancel_timer(asio::steady_timer& timer) noexcept {
        try{(void)timer.cancel();}catch(...){cleanup_failed=true;}
    }
    explicit server(asio::io_context& context,tasks::limits limits={},std::shared_future<void> gate={})
        :acceptor_(context,{asio::ip::address_v4::loopback(),0}),shutdown_timer_(context),port_(acceptor_.local_endpoint().port()),io(context),
         engine(std::make_shared<tasks::runtime<tasks::task_policy>>(context,limits,std::move(gate))){sessions_.reserve(16);}
    std::uint16_t port()const noexcept{return port_;}
    void start(){engine->start();accept();}
    void maybe_done(){if(jobs_drained_ && connections==0){error_code ignored;cancel_timer(shutdown_timer_);}}
    void stop() {
        if(stopping_)return;stopping_=true;error_code ignored;acceptor_.close(ignored);
        auto self=shared_from_this();shutdown_timer_.expires_after(std::chrono::seconds(5));
        shutdown_timer_.async_wait([self](error_code error){if(!error){
            self->deadline_exceeded=true;
            self->engine->registry.cancel_all(tasks::clock::now());
            for(auto& weak:self->sessions_)if(auto s=weak.lock())s->force_close();
        }});
        engine->stop([weak=weak_from_this()]{if(auto self=weak.lock()){
            self->jobs_drained_=true;for(auto& item:self->sessions_)if(auto s=item.lock())s->drain();self->maybe_done();
        }});
    }
    bool clean()const{return jobs_drained_ && connections==0 && !cleanup_failed && !deadline_exceeded && !engine->deadline_exceeded && !engine->failure && lost_responses==0;}
};
inline session::session(tcp::socket socket,std::shared_ptr<server> owner)
    :owner_(std::move(owner)),stream_(std::in_place,std::move(socket)),timer_(owner_->io),write_timeout_(owner_->io){}
inline session::~session(){if(subscription_)owner_->engine->registry.unsubscribe(*subscription_);--owner_->connections;owner_->maybe_done();}
inline void session::force_close() {
    if(closed_)return;closed_=true;
    if(writing_)++owner_->lost_responses;
    error_code ignored;owner_->cancel_timer(timer_);owner_->cancel_timer(write_timeout_);
    if(subscription_){owner_->engine->registry.unsubscribe(*subscription_);subscription_.reset();}
    if(websocket_)beast::get_lowest_layer(*websocket_).socket().close(ignored);
    else if(stream_)stream_->socket().close(ignored);
}
inline void session::drain() {
    draining_=true;
    if(!websocket_){if(!writing_)force_close();}
    else if(!writing_)poll_events();
}
inline void session::arm_write_timeout() {
    auto self=shared_from_this();write_timeout_.expires_after(std::chrono::seconds(5));
    write_timeout_.async_wait([self](error_code error){if(!error && self->writing_)self->force_close();});
}
inline void session::read() {
    if(closed_)return;if(draining_){force_close();return;}
    parser_.emplace();parser_->header_limit(8192);parser_->body_limit(4096);
    stream_->expires_after(std::chrono::seconds(5));auto self=shared_from_this();
    http::async_read(*stream_,buffer_,*parser_,[self](error_code error,std::size_t){
        if(error){self->force_close();return;}
        try{self->route(self->parser_->release());}
        catch(const std::exception&){self->send(http::status::bad_request,{{"error","invalid request"}},false);}
    });
}
inline void session::send(http::status code,boost::json::value body,bool keep_alive) {
    if(closed_)return;
    response_.emplace(code,11);response_->set(http::field::content_type,"application/json");
    response_->keep_alive(keep_alive && !draining_);response_->body()=boost::json::serialize(body);response_->prepare_payload();
    writing_=true;arm_write_timeout();stream_->expires_after(std::chrono::seconds(5));auto self=shared_from_this();
    http::async_write(*stream_,*response_,[self](error_code error,std::size_t){
        if(error){self->force_close();return;}
        self->writing_=false;error_code ignored;self->owner_->cancel_timer(self->write_timeout_);
        const bool again=self->response_->keep_alive() && !self->draining_;self->response_.reset();
        if(again)self->read();else self->force_close();
    });
}
inline void session::route(request req) {
    ++owner_->requests;const std::string_view path(req.target().data(),req.target().size());
    const auto reply=[&](tasks::result<tasks::snapshot> result){
        if(result)send(http::status::ok,tasks::json(*result),req.keep_alive());
        else send(status(result.error()),{{"error",static_cast<int>(result.error())}},req.keep_alive());
    };
    if(req.method()==http::verb::get && path=="/health") {send(http::status::ok,{{"status","ready"}},req.keep_alive());return;}
    if(req.method()==http::verb::get && path=="/metrics") {
        auto& r=owner_->engine->registry;
        send(http::status::ok,{{"accepted",r.accepted},{"duplicates",r.duplicates},{"rejected",r.rejected},{"terminal",r.completed},{"live",r.live()},{"running",r.running()},{"connections",owner_->connections}},req.keep_alive());return;
    }
    if(req.method()==http::verb::post && path=="/tasks") {
        if(req.base().count("Idempotency-Key")!=1 || req[http::field::content_type]!="application/json") {
            send(http::status::bad_request,{{"error","one Idempotency-Key and application/json required"}},req.keep_alive());return;
        }
        auto input=tasks::parse_spec(req.body());
        if(!input){send(http::status::bad_request,{{"error","invalid task specification"}},req.keep_alive());return;}
        const auto key=req["Idempotency-Key"];
        auto created=owner_->engine->submit(*input,std::string_view(key.data(),key.size()));
        if(created)send(http::status::accepted,tasks::json(*created),req.keep_alive());else reply(std::unexpected(created.error()));
        return;
    }
    if(path.starts_with("/tasks/")) {
        const auto task=parse_id(path.substr(7));if(!task){send(http::status::bad_request,{{"error","invalid id"}},req.keep_alive());return;}
        if(req.method()==http::verb::get)reply(owner_->engine->registry.get(*task,tasks::clock::now()));
        else if(req.method()==http::verb::delete_)reply(owner_->engine->registry.cancel(*task,tasks::clock::now()));
        else send(http::status::method_not_allowed,{{"error","method not supported"}},req.keep_alive());
        return;
    }
    if(req.method()==http::verb::get && path.starts_with("/events?task_id=") && websocket::is_upgrade(req)) {
        const auto task=parse_id(path.substr(16));
        if(!task || buffer_.size()!=0){send(http::status::bad_request,{{"error","invalid subscription upgrade"}},false);return;}
        auto subscription=owner_->engine->registry.subscribe(*task,tasks::clock::now());
        if(!subscription){send(status(subscription.error()),{{"error","subscription refused"}},false);return;}
        subscription_=*subscription;
        websocket_.emplace(std::move(*stream_));stream_.reset();
        websocket_->set_option(websocket::stream_base::timeout{std::chrono::seconds(5),std::chrono::seconds(5),true});
        websocket_->read_message_max(4096);websocket_->text(true);beast::get_lowest_layer(*websocket_).expires_never();
        auto self=shared_from_this();
        websocket_->async_accept(req,[self](error_code error){if(error){self->force_close();return;}self->read_control();self->poll_events();});
        return;
    }
    send(http::status::not_found,{{"error","route not found"}},req.keep_alive());
}
inline void session::read_control() {
    if(closed_ || closing_)return;auto self=shared_from_this();
    websocket_->async_read(inbound_,[self](error_code error,std::size_t){
        if(error){self->force_close();return;}
        self->inbound_.consume(self->inbound_.size());self->draining_=true;
        if(!self->writing_)self->begin_close(); // This endpoint only streams server events.
    });
}
inline void session::begin_close() {
    if(closed_ || closing_ || writing_)return;closing_=true;auto self=shared_from_this();
    websocket_->async_close(websocket::close_code::normal,[self](error_code){self->force_close();});
}
inline void session::poll_events() {
    if(closed_ || closing_)return;
    if(!writing_ && subscription_) {
        auto event=owner_->engine->registry.next_event(*subscription_);
        if(!event){subscription_.reset();begin_close();return;}
        if(*event) {
            terminal_event_=tasks::terminal((**event).phase);event_=boost::json::serialize(tasks::json(**event));
            writing_=true;arm_write_timeout();auto self=shared_from_this();
            websocket_->async_write(asio::buffer(event_),[self](error_code error,std::size_t){
                if(error){self->force_close();return;}
                self->writing_=false;error_code ignored;self->owner_->cancel_timer(self->write_timeout_);
                if(self->terminal_event_){self->subscription_.reset();self->begin_close();}
                else self->poll_events();
            });return;
        }
    }
    auto self=shared_from_this();timer_.expires_after(std::chrono::milliseconds(5));
    timer_.async_wait([self](error_code error){if(!error)self->poll_events();});
}
}
