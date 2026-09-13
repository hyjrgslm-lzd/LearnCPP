#pragma once
#include <boost/asio.hpp>
#include <stdexec/execution.hpp>
#include <atomic>
#include <array>
#include <optional>
#include <memory>
namespace c11 {
namespace ex=stdexec;
namespace asio=boost::asio;
// One receive_some, not one application message. Exclusive socket use until completion.
// Executor must serialize handlers: one io.run owner or a strand.
// ponytail: 5ms stop polling; use a backend cancellation bridge if latency requires it.
struct receive_sender {
    using sender_concept=ex::sender_tag;
    std::shared_ptr<asio::ip::tcp::socket> socket;
    template<class Self,class... Env>static consteval auto get_completion_signatures(){
        return ex::completion_signatures<ex::set_value_t(std::string),ex::set_error_t(std::exception_ptr),ex::set_stopped_t()>{};
    }
    template<class R>struct operation {
        using operation_state_concept=ex::operation_state_tag;
        struct state;
        struct stop_fn {std::weak_ptr<state> target;void operator()()const noexcept{if(auto s=target.lock())s->stop.store(true);}};
        using token_t=ex::stop_token_of_t<ex::env_of_t<R>>;
        using callback_t=ex::stop_callback_for_t<token_t,stop_fn>;
        struct state:std::enable_shared_from_this<state>{
            std::shared_ptr<asio::ip::tcp::socket> socket;R receiver;asio::steady_timer timer;
            std::array<char,32> buffer{};std::size_t bytes=0;std::atomic_bool stop=false;
            bool read_done=false,timer_pending=false,cancelled_by_stop=false,stopped_completion=false;std::exception_ptr error;
            std::optional<callback_t> callback;
            std::chrono::steady_clock::time_point end=std::chrono::steady_clock::now()+std::chrono::seconds(2);
            state(std::shared_ptr<asio::ip::tcp::socket> s,R r):socket(std::move(s)),receiver(std::move(r)),timer(socket->get_executor()){}
            void finish()noexcept{
                callback.reset();auto out=std::move(receiver);
                // A successful read wins a late stop; cancellation alone does not erase delivered data.
                if(!error){try{auto value=std::string(buffer.data(),bytes);ex::set_value(std::move(out),std::move(value));}catch(...){ex::set_error(std::move(out),std::current_exception());}}
                else if(stopped_completion)ex::set_stopped(std::move(out));
                else ex::set_error(std::move(out),error);
            }
            void tick(){
                timer.expires_after(std::chrono::milliseconds(5));timer_pending=true;
                try{timer.async_wait([self=this->shared_from_this()](boost::system::error_code){
                    self->timer_pending=false;
                    if(self->read_done){self->finish();return;}
                    if(self->stop.load() || std::chrono::steady_clock::now()>=self->end){
                        self->cancelled_by_stop=self->stop.load();
                        boost::system::error_code ignored;self->socket->cancel(ignored);return;
                    }
                    try{self->tick();}catch(...){self->error=std::current_exception();boost::system::error_code ignored;self->socket->cancel(ignored);}
                });}catch(...){timer_pending=false;throw;}
            }
            void begin()noexcept{
                try{
                    tick();
                    socket->async_read_some(asio::buffer(buffer),[self=this->shared_from_this()](boost::system::error_code ec,std::size_t n){
                        self->read_done=true;self->bytes=n;
                        self->stopped_completion=ec==asio::error::operation_aborted && self->cancelled_by_stop && !self->error;
                        if(ec && !self->error)self->error=std::make_exception_ptr(boost::system::system_error(ec));
                        if(self->timer_pending){try{self->timer.cancel();}catch(...){self->error=std::current_exception();}}
                        else self->finish();
                    });
                }catch(...){error=std::current_exception();read_done=true;if(!timer_pending)finish();}
            }
        };
        std::shared_ptr<state> shared;
        operation(std::shared_ptr<asio::ip::tcp::socket> s,R r):shared(std::make_shared<state>(std::move(s),std::move(r))){}
        operation(operation&&)=delete;
        void start()&noexcept{
            auto s=shared;auto token=ex::get_stop_token(ex::get_env(s->receiver));
            if(token.stop_requested()){ex::set_stopped(std::move(s->receiver));return;}
            try{
                if constexpr(!ex::unstoppable_token<token_t>)s->callback.emplace(token,stop_fn{s});
                asio::post(s->socket->get_executor(),[s]{s->begin();});
            }catch(...){s->error=std::current_exception();s->finish();}
        }
    };
    template<class R>auto connect(R r)const{return operation<R>{socket,std::move(r)};}
};
}
