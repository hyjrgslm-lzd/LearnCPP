#include <c12/log_store.hpp>
#include <check.hpp>
#include <chrono>
#include <iostream>
int main(int argc,char** argv){
    using namespace c12;
    const std::string mode=argc>1?argv[1]:"full_replay";
    if(mode!="full_replay"&&mode!="checkpoint")return 2;
    const auto path=mode=="checkpoint"?recovery_path::checkpoint:recovery_path::full_replay;
    auto directory=std::filesystem::temp_directory_path()/("c12-replay-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    check(std::filesystem::create_directory(directory),"fresh replay workload");
    for(int stage=0;stage!=2;++stage){
        if(stage==0){
            auto db=log_store::open(directory);check(db.has_value(),"workload database");
            for(int revision=0;revision!=256;++revision){
                std::vector<mutation> ops;
                for(int key=0;key!=16;++key)ops.push_back({"key"+std::to_string(key),std::to_string(revision)+std::string(64,'x')});
                check(db->write_batch(ops).has_value(),"workload commit");
                if(revision==239)check(db->checkpoint().has_value(),"workload checkpoint at 240");
            }
        }else{
            const auto start=std::chrono::steady_clock::now();
            auto db=log_store::open(directory,{},path);
            const auto end=std::chrono::steady_clock::now();
            check(db&&db->sequence()==256&&db->checkpoint_sequence==240,"workload recovered history");
            for(int key=0;key!=16;++key)
                check(db->get("key"+std::to_string(key)).value()==std::optional<std::string>{std::string("255")+std::string(64,'x')},"independent final value");
            check(db->applied_batches==(mode=="checkpoint"?16u:256u),"measured replay operations");
            const auto seconds=std::chrono::duration<double>(end-start).count();
            std::cout<<"{\"mode\":\""<<mode<<"\",\"batches\":256,\"checkpoint\":240,\"scanned_bytes\":"<<db->scanned_bytes
                     <<",\"decoded_batches\":"<<db->decoded_batches<<",\"applied_batches\":"<<db->applied_batches
                     <<",\"recovery_seconds\":"<<seconds<<",\"valid\":true}\n";
        }
    }
}
