#include <c12/log_store.hpp>
#include <check.hpp>
#include <array>
#include <chrono>
#include <cstdlib>
#include <new>
// This executable alone injects a one-shot allocation failure. No production allocator changes.
static long allocation_budget=-1;
void* operator new(std::size_t size){
    if(allocation_budget==0){allocation_budget=-1;throw std::bad_alloc();}
    if(allocation_budget>0)--allocation_budget;
    if(auto* memory=std::malloc(size?size:1))return memory;
    throw std::bad_alloc();
}
void operator delete(void* memory)noexcept{std::free(memory);}
void operator delete(void* memory,std::size_t)noexcept{std::free(memory);}
int main(){
    using namespace c12;
    dictionary initial{{"key","old"}};
    const std::array changes{mutation{"key","new"}};
    allocation_budget=0;
    auto staged=prepare_batch(initial,changes);allocation_budget=-1;
    check(!staged&&staged.error().code==errc::allocation&&initial.at("key")=="old","allocation is not corruption");
    auto base=std::filesystem::temp_directory_path()/("c12-bounds-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {
        auto db=log_store::open(base/"allocation");check(db.has_value(),"allocation fixture");
        for(int n=0;n!=3;++n)check(db->write_batch(changes).has_value(),"allocation fixture history");
    }
    std::size_t rejected=0;
    for(long budget=0;budget!=100;++budget){
        const auto directory=base/"allocation";
        allocation_budget=budget;
        auto db=log_store::open(directory);allocation_budget=-1;
        if(!db){
            ++rejected;check(db.error().code==errc::allocation,"recovery allocation keeps resource error");
        }else check(db->get("key").value()==std::optional<std::string>{"new"},"allocation retry preserves history");
    }
    check(rejected>0,"allocation fault actually exercised");
    std::uint64_t confirmed=0;
    std::string confirmed_value;
    {
        auto db=log_store::open(base/"capacity");check(db.has_value(),"capacity fixture");
        std::vector<mutation> ops;
        for(std::size_t n=0;n!=max_batch;++n)ops.push_back({"key"+std::to_string(n),std::string(max_value,'x')});
        bool refused=false;
        for(int attempt=0;attempt!=128;++attempt){
            ops[0].value=std::string(max_value,static_cast<char>('!'+attempt%90));
            const auto before=std::filesystem::file_size(base/"capacity"/"journal.bin");
            auto written=db->write_batch(ops);
            if(!written){
                check(written.error().code==errc::capacity&&db->sequence()==confirmed,"log limit rejects before I/O");
                check(std::filesystem::file_size(base/"capacity"/"journal.bin")==before,"capacity refusal does not alter log");
                check(db->get("key0").value()==std::optional<std::string>{confirmed_value},"last committed value survives capacity rejection");
                refused=true;break;
            }
            confirmed=*written;confirmed_value=*ops[0].value;
        }
        check(refused&&confirmed>0,"actual 64 MiB log ceiling reached");
        std::cout<<"capacity confirmed_batches="<<confirmed<<" allocation_rejections="<<rejected<<"\n";
    }
    auto again=log_store::open(base/"capacity");
    check(again&&again->sequence()==confirmed&&again->get("key0").value()==std::optional<std::string>{confirmed_value},"full log reopens with last confirmed value");
    std::cout<<"bounds and allocation checks passed\n";
}
