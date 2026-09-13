#include <solution.hpp>
#include <check.hpp>
#include <array>
#include <iostream>
int main() {
  try {
    const std::array<std::uint8_t,16> packet{3,'w','w','w',4,'t','e','s','t',0,3,'a','p','i',0xc0,4};
    auto a=exercise::decode_name(packet,0), b=exercise::decode_name(packet,10);
    check(a && a->text=="www.test" && a->next==10,"uncompressed name and consumed location");
    check(b && b->text=="api.test" && b->next==16,"compressed name and original consumed location");
    const std::array<std::uint8_t,2> cycle{0xc0,0};
    auto c=exercise::decode_name(cycle,0);
    check(!c && c.error()==c11::dns::error::pointer_loop,"compression cycles rejected");
    const std::array<std::uint8_t,2> outside{0xc0,0xff};
    check(!exercise::decode_name(outside,0),"pointer outside packet rejected");
    const std::array<std::uint8_t,18> chain{3,'w','w','w',4,'t','e','s','t',0,3,'a','p','i',0xc0,4,0xc0,10};
    auto nested=exercise::decode_name(chain,16);
    check(nested && nested->text=="api.test" && nested->next==18,"nested pointers preserve original consumed position");
    const std::array<std::uint8_t,5> forward{0xc0,2,1,'a',0};
    check(!exercise::decode_name(forward,0),"forward compression pointer rejected");
    for(const auto last:{61u,63u}) {
        std::vector<std::uint8_t> long_name;
        for(auto size:{63u,63u,63u,last}) {long_name.push_back(static_cast<std::uint8_t>(size));long_name.insert(long_name.end(),size,'a');}
        long_name.push_back(0);auto decoded=exercise::decode_name(long_name,0);
        if(last==61)check(decoded && decoded->text.size()==253,"maximum hostname accepted");
        else check(!decoded,"overlong complete hostname rejected");
    }
    for (std::size_t n=0;n<10;++n) check(!exercise::decode_name(std::span{packet}.first(n),0),"all truncated label boundaries rejected");
    auto query=c11::dns::query(0x1234,"api.test");
    check(query.has_value(),"query built");
    auto answer=*query; answer[2]=0x81; answer[3]=0x80; answer[7]=1;
    answer.insert(answer.end(),{0xc0,12,0,1,0,1,0,0,0,60,0,4,127,0,0,1});
    auto ip=c11::dns::parse_a(answer,0x1234,"api.test");
    check(ip && *ip=="127.0.0.1","bounded A answer parsed");
    check(c11::dns::parse_a(answer,0x1234,"API.TEST").has_value(),"DNS names compared case-insensitively");
    for(auto count_field:{7u,9u,11u}) {
        auto malformed=answer;++malformed[count_field];
        check(!c11::dns::parse_a(malformed,0x1234,"api.test"),"every declared answer authority and additional record validated");
    }
    auto trailing=answer;trailing.push_back(0);
    check(!c11::dns::parse_a(trailing,0x1234,"api.test"),"unexplained trailing datagram bytes rejected");
    check(!c11::dns::parse_a(answer,0x1235,"api.test"),"DNS transaction id checked");
    check(!c11::dns::parse_a(answer,0x1234,"wrong.test"),"DNS question identity checked");
    answer.pop_back(); check(!c11::dns::parse_a(answer,0x1234,"api.test"),"truncated RDATA rejected");
    check(!c11::dns::query(1,"a..test"),"empty interior label rejected");
    std::cout<<"DNS checks passed\n";
  } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
