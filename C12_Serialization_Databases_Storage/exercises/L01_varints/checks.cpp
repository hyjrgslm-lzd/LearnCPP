#include <solution.hpp>
#include <check.hpp>
#include <array>
int main(){
    using namespace c12;
    bytes redundant{std::byte{0x80},std::byte{0}};
    std::size_t at=0;auto refused=solution::read(redundant,at);
    if(!refused&&refused.error().code==errc::unfinished){std::cerr<<"UNFINISHED\n";return 1;}
    check(!refused&&at==0,"reject redundant representation");
    const bytes golden{std::byte{0xac},std::byte{0x02}};
    auto decoded=solution::read(golden,at);check(decoded&&*decoded==300&&at==2,"independent 300 fixture");
    check(varuint(300)==golden,"writer against golden fixture");
    for(auto value:std::array<std::uint64_t,8>{0,1,127,128,16383,16384,UINT64_C(1)<<63,UINT64_MAX}){
        auto data=varuint(value);at=0;
        auto roundtrip=solution::read(data,at);check(roundtrip&&*roundtrip==value&&at==data.size(),"integer boundaries");
        for(std::size_t cut=0;cut<data.size();++cut){at=0;check(!solution::read(std::span(data).first(cut),at)&&at==0,"truncation is transactional");}
    }
    const bytes stream{std::byte{0x55},std::byte{0xac},std::byte{2},std::byte{127}};
    at=1;auto first=solution::read(stream,at);
    check(first&&*first==300&&at==3,"nonzero cursor reads the intended field");
    auto second=solution::read(stream,at);
    check(second&&*second==127&&at==4,"sequential integers preserve field boundaries");
    at=1;const bytes bad_at_offset{std::byte{0x55},std::byte{0x80}};
    check(!solution::read(bad_at_offset,at)&&at==1,"nonzero failure cursor is preserved");
    bytes overflow(10,std::byte{0xff});overflow.back()=std::byte{2};at=0;
    check(!solution::read(overflow,at)&&at==0,"tenth byte overflow");
    at=golden.size()+1;check(!solution::read(golden,at)&&at==golden.size()+1,"invalid starting cursor");
    for(auto value:std::array<std::int64_t,7>{INT64_MIN,-300,-1,0,1,300,INT64_MAX})
        check(unzigzag(zigzag(value))==value,"ZigZag signed limits");
    std::cout<<"varint checks passed\n";
}
