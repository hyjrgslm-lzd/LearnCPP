#include "reference.hpp"
#include <sstream>
int main() {
    using namespace cs::numeric;
    for (std::size_t n:{0,1,3,7,17}) {
        std::vector<float> a(square_size(n)),b(a.size()),c(a.size(),999);
        cs::cap3::fill_matrices(a,b,n);
        for (auto name:{"naive","tiled","threaded","par","sse2"}) {
            if (std::string_view(name)=="par" && !policy_available(policy::par)) continue;
            if (std::string_view(name)=="sse2" && !available(backend::sse2)) continue;
            for (std::size_t block:{1,4,32}) {
                cs::cap3::compute(name,a,b,c,n,block,3);
                cs::cap3::verify_matrix(c,n);
                std::fill(c.begin(),c.end(),999.0f); // Detect accidental C += A*B contract.
            }
        }
    }
    bool caught=false;
    try { parallel_chunks(7,3,[](auto t,auto,auto) { if(t==1) throw std::runtime_error("worker sentinel"); }); }
    catch (const std::runtime_error& e) { caught=std::string_view(e.what())=="worker sentinel"; }
    cs::check(caught,"worker exception reaches caller after join");
    char program[]="Cap3_reference",size[]="--size",n[]="7",items[]="--items",count[]="257";
    char* argv[]{program,size,n,items,count};
    cs::check(cs::cap3::run(5,argv)==0,"all measured variants validated");
    // Check the actual emitted CSV, including the known-zero-worker case.
    for (auto variant:{std::string("threaded"),std::string("reduce_threaded")})
        for (auto extent:{std::string("0"),std::string("1"),std::string("3")}) {
            std::string program_name="metadata_reference",size_key="--size",items_key="--items",variant_key="--variant";
            char* options[]{program_name.data(),size_key.data(),extent.data(),items_key.data(),extent.data(),variant_key.data(),variant.data()};
            std::ostringstream captured;
            auto* original=std::cout.rdbuf(captured.rdbuf());
            try { cs::check(cs::cap3::run(7,options)==0,"metadata driver succeeds"); }
            catch (...) { std::cout.rdbuf(original); throw; }
            std::cout.rdbuf(original);
            std::istringstream row(captured.str());
            std::string field;
            for (int column=0;column<4;++column) std::getline(row,field,',');
            const std::string reported=extent=="3"?"2":"1";
            const std::string workers=extent=="0"?"0":extent=="1"?"1":"2";
            cs::check(field==reported,"CSV threads counts workers or the empty caller");
            cs::check(captured.str().find("workers="+workers+";")!=std::string::npos &&
                      captured.str().find("caller=1;")!=std::string::npos,"CSV reports known worker count and caller");
        }
}
