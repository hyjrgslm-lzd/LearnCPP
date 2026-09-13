#pragma once
#include <c11/socket.hpp>
#include <c11/connection.hpp>
#include <array>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace c11 {
// A single owner drives every connection. poll is deliberately the portable
// readiness baseline; the epoll lab separately exposes edge/rearm semantics.
class reactor {
    struct peer { unique_socket socket; framed_connection protocol; deadline progress; bool drop=false; };
    listener listener_;
    std::vector<peer> peers_;
    bool draining_=false;
    bool clean_=true;
    deadline drain_end_{};
    int send_buffer_;
    std::mutex observation_mutex_;
    std::condition_variable observation_;
    std::size_t pauses_=0;
public:
    std::size_t accepted=0, refused=0, read_bytes=0, written_bytes=0, protocol_errors=0,abandoned_connections=0;
    explicit reactor(int send_buffer=0) : send_buffer_(send_buffer) {
        auto bound=listen_loopback();
        if (!bound) throw std::system_error(bound.error());
        listener_=std::move(*bound); peers_.reserve(16);
    }
    std::uint16_t port() const noexcept { return listener_.port; }
    bool wait_for_backpressure(deadline end) {
        std::unique_lock lock(observation_mutex_);
        return observation_.wait_until(lock,end,[&]{return pauses_>0;});
    }
    void begin_drain(deadline now) {
        if (draining_) return;
        draining_=true; drain_end_=now+std::chrono::seconds(5); listener_.socket.reset();
        for(auto& p:peers_) p.protocol.drain();
    }
    bool run(std::atomic_bool& stop,deadline end) {
        while(clock::now()<end) {
            if(stop.load(std::memory_order_relaxed)) begin_drain(clock::now());
            std::erase_if(peers_,[&](const peer& p){
                const bool remove=p.drop || p.protocol.state()==connection_state::closed;
                if(remove && p.protocol.queued_bytes()) {clean_=false;++abandoned_connections;}
                return remove;
            });
            if(draining_ && peers_.empty()) return clean_;
            if(draining_ && clock::now()>=drain_end_) { peers_.clear(); return false; }
            std::vector<poll_fd> interest;
            const std::size_t offset=listener_.socket?1:0;
            if(listener_.socket) interest.push_back({listener_.socket.get(),POLLRDNORM,0});
            for(auto& p:peers_) {
                short events=p.protocol.can_read()?POLLRDNORM:0;
                if(!p.protocol.output().empty()) events|=POLLWRNORM;
                interest.push_back({p.socket.get(),events,0});
            }
            if(interest.empty()) continue;
            const int n=poll_sockets(interest,20);
            if(n<0) { if(interrupted(socket_error())) continue; throw std::system_error(net_error(socket_error())); }
            const auto existing=peers_.size();
            for(std::size_t i=0;i<existing;++i) {
                auto& p=peers_[i]; const auto events=interest[offset+i].revents;
                if(events & (POLLERR|POLLNVAL)) {p.drop=true;continue;}
                if(clock::now()-p.progress>std::chrono::seconds(5)) {p.drop=true;continue;}
                if(events & (POLLRDNORM|POLLHUP)) {
                    std::size_t budget=16*1024;
                    std::array<char,4096> input{};
                    while(budget && p.protocol.can_read()) {
                        auto r=read_some(p.socket.get(),std::span{input}.first(std::min(input.size(),budget)));
                        if(!r) {if(!would_block(r.error().value())) p.drop=true;break;}
                        if(!*r) {if(!p.protocol.eof()) ++protocol_errors;break;}
                        budget-=*r;read_bytes+=*r;p.progress=clock::now();
                        if(!p.protocol.receive(std::string_view(input.data(),*r))) {p.drop=true;++protocol_errors;break;}
                        if(!p.protocol.can_read()) {
                            {std::lock_guard lock(observation_mutex_);++pauses_;}
                            observation_.notify_all();
                        }
                    }
                }
                if(!p.drop && (events & POLLWRNORM)) {
                    std::size_t budget=16*1024;
                    while(budget && !p.protocol.output().empty()) {
                        auto r=write_some(p.socket.get(),p.protocol.output().first(std::min(budget,p.protocol.output().size())));
                        if(!r) {if(!would_block(r.error().value())) p.drop=true;break;}
                        if(!*r) {p.drop=true;break;}
                        p.protocol.sent(*r);budget-=*r;written_bytes+=*r;p.progress=clock::now();
                    }
                }
            }
            if(offset && (interest[0].revents & POLLRDNORM)) {
                for(unsigned budget=0;budget<16;++budget) {
                    auto socket=accept_socket(listener_.socket.get());
                    if(!socket) {if(would_block(socket.error().value())) break;throw std::system_error(socket.error());}
                    if(peers_.size()==16) {++refused;continue;}
                    if(send_buffer_>0 && ::setsockopt(socket->get(),SOL_SOCKET,SO_SNDBUF,reinterpret_cast<const char*>(&send_buffer_),sizeof(send_buffer_)))
                        throw std::system_error(net_error(socket_error()));
                    peers_.push_back({std::move(*socket),{},clock::now()});++accepted;
                }
            }
        }
        begin_drain(clock::now());
        const bool clean=clean_ && std::all_of(peers_.begin(),peers_.end(),[](const peer& p){return p.protocol.state()==connection_state::closed && p.protocol.queued_bytes()==0;});
        peers_.clear();return clean;
    }
};
}
