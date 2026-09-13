// Linux-only observation branch; not executed in the Windows validation.
#include <c11/socket.hpp>
#include <check.hpp>
#include <sys/epoll.h>
#include <array>
#include <iostream>
using namespace std::chrono_literals;
int main(){
 for(unsigned mode=0;mode<3;++mode){
    auto listener=c11::listen_loopback();check(bool(listener),"epoll listener");auto end=c11::clock::now()+5s;
    auto client=c11::connect_loopback(listener->port,end);check(bool(client),"epoll client");
    check(bool(c11::wait_ready(listener->socket.get(),POLLRDNORM,end)),"epoll accept");auto peer=c11::accept_socket(listener->socket.get());check(bool(peer),"epoll peer");
    struct owner{int fd;~owner(){if(fd>=0)::close(fd);}}ep{epoll_create1(EPOLL_CLOEXEC)};check(ep.fd>=0,"epoll create");
    epoll_event interest{};interest.events=EPOLLIN|(mode==1?EPOLLET:mode==2?EPOLLONESHOT:0);interest.data.u64=1;
    check(epoll_ctl(ep.fd,EPOLL_CTL_ADD,peer->get(),&interest)==0,"epoll register");
    const std::string payload(24*1024,'e');check(bool(c11::send_all(client->get(),payload,end)),"epoll payload queued");
    std::size_t total=0;bool runnable=false;unsigned continuations=0;std::array<char,4096> bytes{};
    while(total<payload.size()){
        check(c11::clock::now()<end,"epoll bounded deadline");
        if(!runnable){epoll_event event{};check(epoll_wait(ep.fd,&event,1,1000)==1,"epoll readiness");check(event.data.u64==1,"epoll identity");}
        else ++continuations;
        runnable=false;std::size_t budget=16*1024;
        while(budget){auto n=c11::read_some(peer->get(),std::span{bytes}.first(std::min(bytes.size(),budget)));
            if(!n){check(c11::would_block(n.error().value()),"epoll recv error");break;}
            check(*n>0,"epoll no early EOF");for(std::size_t i=0;i<*n;++i)check(bytes[i]=='e',"epoll bytes");total+=*n;budget-=*n;
        }
        if(mode==1 && budget==0)runnable=true; // ET continuation: do not wait for another edge.
        if(mode==2 && total<payload.size())check(epoll_ctl(ep.fd,EPOLL_CTL_MOD,peer->get(),&interest)==0,"oneshot rearm");
    }
    check(total==payload.size(),"epoll exact payload");if(mode==1)check(continuations>0,"ET budget continuation exercised");
 }
 std::cout<<"epoll LT/ET/oneshot with bounded dispatch passed\n";
}
