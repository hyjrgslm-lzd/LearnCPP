#pragma once
#include <c11/socket.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace c11 {
// A pool for one loopback teaching endpoint; leases own sockets throughout IO.
// Waiting threads have deadlines, but std::condition_variable does not promise FIFO.
class socket_pool {
    struct idle_socket {unique_socket socket;deadline expiry;};
    struct state {
        socket_runtime network;
        std::mutex mutex;std::condition_variable changed;std::vector<idle_socket> idle;
        std::size_t live=0,waiting=0,created=0,capacity,max_waiters;std::uint16_t port;
        bool closed=false;std::chrono::milliseconds ttl;
        state(std::uint16_t p,std::size_t cap,std::size_t waiters,std::chrono::milliseconds age)
            :capacity(cap),max_waiters(waiters),port(p),ttl(age){idle.reserve(cap);}
    };
    std::shared_ptr<state> state_;
    static void reap(state& s,deadline now) {
        std::erase_if(s.idle,[&](const idle_socket& item){if(item.expiry<=now){--s.live;return true;}return false;});
    }
public:
    class lease {
        std::shared_ptr<state> state_;unique_socket socket_;bool reusable_=false;
        friend class socket_pool;
        lease(std::shared_ptr<state> state,unique_socket socket):state_(std::move(state)),socket_(std::move(socket)){}
    public:
        lease(lease&& other)noexcept:state_(std::move(other.state_)),socket_(std::move(other.socket_)),reusable_(other.reusable_){}
        lease& operator=(lease&& other)noexcept{if(this!=&other){reset();state_=std::move(other.state_);socket_=std::move(other.socket_);reusable_=other.reusable_;}return *this;}
        lease(const lease&)=delete;
        ~lease(){reset();}
        socket_handle get()const noexcept{return socket_.get();}
        void reusable()noexcept{reusable_=true;} // Only after consuming and validating the complete response.
        void reset()noexcept{
            if(!socket_)return;std::lock_guard lock(state_->mutex);
            if(reusable_ && !state_->closed)state_->idle.push_back({std::move(socket_),clock::now()+state_->ttl});
            else{socket_.reset();--state_->live;}
            state_->changed.notify_all();
        }
    };
    socket_pool(std::uint16_t port,std::size_t capacity=2,std::size_t max_waiters=8,std::chrono::milliseconds ttl=std::chrono::seconds(30)) {
        if(!port || !capacity || capacity>16 || !max_waiters || max_waiters>128 || ttl.count()<=0 || ttl>std::chrono::hours(1))throw std::invalid_argument("invalid pool limits");
        state_=std::make_shared<state>(port,capacity,max_waiters,ttl);
    }
    ~socket_pool(){close();}
    socket_pool(const socket_pool&)=delete;
    net_result<lease> acquire(deadline end) {
        auto s=state_;std::unique_lock lock(s->mutex);
        for(;;) {
            if(s->closed)return std::unexpected(std::make_error_code(std::errc::operation_canceled));
            if(clock::now()>=end)return std::unexpected(std::make_error_code(std::errc::timed_out));
            reap(*s,clock::now());
            while(!s->idle.empty()) {
                auto socket=std::move(s->idle.back().socket);s->idle.pop_back();
                char peek{};const auto n=::recv(socket.get(),&peek,1,MSG_PEEK);
                if(n<0 && would_block(socket_error()))return lease(s,std::move(socket));
                --s->live; // EOF, unsolicited bytes, or socket error: never recycle a dirty connection.
            }
            if(s->live<s->capacity) {
                ++s->live;lock.unlock();auto connected=connect_loopback(s->port,end);lock.lock();
                if(!connected || s->closed) {
                    --s->live;s->changed.notify_all();
                    return std::unexpected(connected?std::make_error_code(std::errc::operation_canceled):connected.error());
                }
                ++s->created;return lease(s,std::move(*connected));
            }
            if(s->waiting>=s->max_waiters)return std::unexpected(std::make_error_code(std::errc::no_buffer_space));
            ++s->waiting;s->changed.notify_all();
            bool ready=false;
            try{ready=s->changed.wait_until(lock,end,[&]{return s->closed || !s->idle.empty() || s->live<s->capacity;});}
            catch(...){--s->waiting;throw;}
            --s->waiting;if(!ready)return std::unexpected(std::make_error_code(std::errc::timed_out));
        }
    }
    void expire_idle(deadline now){std::lock_guard lock(state_->mutex);reap(*state_,now);state_->changed.notify_all();}
    bool wait_for_waiters(std::size_t count,deadline end){std::unique_lock lock(state_->mutex);return state_->changed.wait_until(lock,end,[&]{return state_->waiting>=count;});}
    std::size_t created()const{std::lock_guard lock(state_->mutex);return state_->created;}
    bool close(){std::lock_guard lock(state_->mutex);state_->closed=true;state_->live-=state_->idle.size();state_->idle.clear();state_->changed.notify_all();return state_->live==0;}
};
}
