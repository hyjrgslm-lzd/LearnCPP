#include <c12/log_store.hpp>
#include <check.hpp>
#include <array>
#include <chrono>
#include <iostream>
using namespace c12;
std::vector<mutation> batch(std::string label){return {{label+".1","one"},{label+".2","two"}};}
const char* name(fault_point point){
    switch(point){
    case fault_point::half_record:return "half_record";
    case fault_point::before_flush:return "before_flush";
    case fault_point::after_flush:return "after_flush";
    case fault_point::half_snapshot:return "half_snapshot";
    case fault_point::snapshot_flushed:return "snapshot_flushed";
    }return "invalid";
}
int self_test(){
    check(crc32(as_bytes("123456789"))==0xcbf43926u,"independent CRC check vector");
    auto directory=std::filesystem::temp_directory_path()/("c12-store-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(directory),"own test directory");
    {
        auto db=log_store::open(directory);check(db.has_value(),"new database");
        check(!log_store::open(directory),"exclusive database owner");
        auto written=db->write_batch(batch("A"));check(written&&*written==1,"first durable batch");
        auto before=db->get("A.1");check(before&&*before==std::optional<std::string>{"one"},"get owns value");
        check(db->checkpoint().has_value(),"checkpoint");
        check(db->write_batch(batch("B")).has_value(),"batch after snapshot");
        auto invalid=batch("C");invalid.back().key.clear();
        check(!db->write_batch(invalid)&&db->sequence()==2,"reject before I/O");
    }
    {
        auto db=log_store::open(directory);check(db&&db->sequence()==2&&db->checkpoint_sequence==1,"snapshot plus replay");
        check(db->get("B.2").value()==std::optional<std::string>{"two"},"replayed value");
        check(db->checkpoint().has_value(),"rotate checkpoint");
    }
    {
        auto db=log_store::open(directory,[](fault_point p){
            if(p==fault_point::before_flush)throw failure(errc::io,"injected flush failure",123);
        });
        check(db.has_value(),"open injection database");
        auto unknown=db->write_batch(batch("C"));
        check(!unknown&&unknown.error().code==errc::outcome_unknown,"I/O outcome is unknown");
        check(!db->get("C.1")&&!db->write_batch(batch("D"))&&!db->checkpoint(),"poisoned instance refuses operations");
    }
    {
        auto db=log_store::open(directory);check(db&&db->sequence()==3,"complete unacknowledged batch recovered");
        check(db->get("C.1").value()==std::optional<std::string>{"one"},"unknown is not rolled back");
    }
    std::cout<<"log store checks passed\n";return 0;
}
int main(int argc,char** argv){
    try{
        if(argc==1)return self_test();
        if(argc<3){std::cerr<<"usage: store batch|inspect|checkpoint directory [label] [fault]\n";return 2;}
        const std::string mode=argv[1],fault=argc>4?argv[4]:"";
        auto db=log_store::open(argv[2],[&](fault_point point){
            if(fault==name(point)){
                std::cout<<"READY "<<fault<<std::endl;
                std::string release;std::getline(std::cin,release);
            }
        });
        if(!db){std::cout<<"ERROR "<<static_cast<int>(db.error().code)<<" "<<db.error().offset<<"\n";return 3;}
        if(mode=="batch"){
            if(argc<4)return 2;
            auto ack=db->write_batch(batch(argv[3]));
            if(!ack)return 4;
            std::cout<<"ACK "<<*ack<<std::endl;
        }else if(mode=="checkpoint"){
            if(!db->checkpoint())return 4;std::cout<<"CHECKPOINT\n";
        }else if(mode=="get"){
            if(argc!=4)return 2;
            auto value=db->get(argv[3]);if(!value)return 4;
            if(*value)std::cout<<"VALUE "<<**value<<"\n";else std::cout<<"ABSENT\n";
        }else if(mode=="inspect"){
            std::cout<<"LSN "<<db->sequence()<<" SNAP "<<db->checkpoint_sequence<<" REPAIRED "<<db->repaired_tail_bytes<<" BAD "<<db->rejected_snapshots<<"\n";
            for(const auto* label:{"A","B","C"})for(const auto* suffix:{".1",".2"}){
                const auto key=std::string(label)+suffix;auto value=db->get(key);
                if(!value)return 4;
                std::cout<<key<<"="<<(value->has_value()?**value:"<absent>")<<"\n";
            }
        }else return 2;
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 5;}
}
