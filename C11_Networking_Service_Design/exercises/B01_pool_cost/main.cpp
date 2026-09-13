#include <c11/socket_pool.hpp>
#include <c11/reactor.hpp>
#include <check.hpp>
#include <thread>
#include <iostream>
#include <charconv>
using namespace std::chrono_literals;
int main(int argc,char** argv){
    const bool pooled=argc>1 && std::string_view(argv[1])=="pool";unsigned count=100;
    if(argc>2){std::string_view text=argv[2];auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),count);check(error==std::errc{} && end==text.data()+text.size() && count>0 && count<=1000,"bounded benchmark request count");}
    c11::socket_runtime network;c11::reactor server;std::atomic_bool stop=false;bool drained=false;
    const auto deadline=c11::clock::now()+20s;std::jthread owner([&]{drained=server.run(stop,deadline);});
    struct cleanup{std::atomic_bool& s;std::jthread& t;~cleanup(){s=true;if(t.joinable())t.join();}}guard{stop,owner};
    c11::socket_pool pool(server.port(),1,1,10s);const auto wire=*c11::encode_frame(std::string(128,'b'));
    std::chrono::nanoseconds acquisition{},transfer{};const auto begin=c11::clock::now();
    for(unsigned i=0;i<count;++i){
        const auto a=c11::clock::now();
        if(pooled){auto lease=pool.acquire(deadline);check(bool(lease),"bench pool acquisition");const auto b=c11::clock::now();acquisition+=b-a;
            check(bool(c11::send_all(lease->get(),wire,deadline)),"bench send");auto response=c11::receive_exact(lease->get(),wire.size(),deadline);check(response && *response==wire,"bench response checksum");transfer+=c11::clock::now()-b;lease->reusable();
        }else{auto socket=c11::connect_loopback(server.port(),deadline);check(bool(socket),"bench connection");const auto b=c11::clock::now();acquisition+=b-a;
            check(bool(c11::send_all(socket->get(),wire,deadline)),"bench send");auto response=c11::receive_exact(socket->get(),wire.size(),deadline);check(response && *response==wire,"bench response checksum");transfer+=c11::clock::now()-b;}
    }
    const auto finish=c11::clock::now();check(pool.close(),"bench pool closed");stop=true;owner.join();check(drained,"bench reactor drained");const auto closed=c11::clock::now();
    auto us=[](auto d){return std::chrono::duration<double,std::micro>(d).count();};
    std::cout<<"{\"mode\":\""<<(pooled?"pool":"fresh")<<"\",\"requests\":"<<count<<",\"connections\":"<<(pooled?pool.created():count)<<",\"acquire_us\":"<<us(acquisition)<<",\"transfer_us\":"<<us(transfer)<<",\"work_us\":"<<us(finish-begin)<<",\"drain_us\":"<<us(closed-finish)<<"}\n";
}
