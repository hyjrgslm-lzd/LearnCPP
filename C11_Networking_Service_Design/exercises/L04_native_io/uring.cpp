// Optional Linux single-shot network operations; not a C10 file backend.
#include <c11/socket.hpp>
#include <check.hpp>
#include <liburing.h>
#include <array>
#include <iostream>
using namespace std::chrono_literals;
int main(){
    io_uring ring{};const int initialized=io_uring_queue_init(8,&ring,0);
    if(initialized==-ENOSYS || initialized==-EPERM){std::cout<<"SKIP: io_uring unavailable or policy denied\n";return 77;}
    check(initialized==0,"uring queue initialization");struct cleanup{io_uring* r;~cleanup(){io_uring_queue_exit(r);}}guard{&ring};
    auto listener=c11::listen_loopback();check(bool(listener),"uring listener");const auto end=c11::clock::now()+5s;
    auto client=c11::connect_loopback(listener->port,end);check(bool(client),"uring client");
    check(bool(c11::wait_ready(listener->socket.get(),POLLRDNORM,end)),"uring accept");auto peer=c11::accept_socket(listener->socket.get());check(bool(peer),"uring peer");
    std::array<char,32> buffer{};
    auto submit_read=[&](std::uint64_t id){auto* sqe=io_uring_get_sqe(&ring);check(sqe!=nullptr,"uring SQ capacity");io_uring_prep_recv(sqe,peer->get(),buffer.data(),buffer.size(),0);io_uring_sqe_set_data64(sqe,id);check(io_uring_submit(&ring)==1,"uring read submitted");};
    auto next=[&]{io_uring_cqe* cqe=nullptr;__kernel_timespec timeout{2,0};check(io_uring_wait_cqe_timeout(&ring,&cqe,&timeout)==0,"uring completion bounded");const auto result=std::pair{io_uring_cqe_get_data64(cqe),cqe->res};io_uring_cqe_seen(&ring,cqe);return result;};
    submit_read(1);check(bool(c11::send_all(client->get(),std::string_view("u"),end)),"uring send");auto first=next();check(first.first==1 && first.second==1 && buffer[0]=='u',"uring actual socket completion");
    submit_read(2);auto* cancel=io_uring_get_sqe(&ring);check(cancel!=nullptr,"uring cancel SQ capacity");io_uring_prep_cancel64(cancel,2,0);io_uring_sqe_set_data64(cancel,3);check(io_uring_submit(&ring)==1,"uring cancel submitted");
    bool target_done=false,cancel_done=false;
    for(int i=0;i<2;++i){auto result=next();if(result.first==2){check(!target_done && result.second==-ECANCELED,"target final canceled CQE");target_done=true;}else{check(result.first==3 && !cancel_done && result.second==0,"cancel CQE");cancel_done=true;}}
    check(target_done&&cancel_done,"buffer retained until target and cancel complete");std::cout<<"io_uring socket recv and separate cancellation completions passed\n";
}
