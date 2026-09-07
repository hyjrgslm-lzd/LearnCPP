#include "reference.hpp"
#include <array>
int main() {
    for (bool heavy:{false,true})
        for (std::size_t n:{0,1,7,65,257}) {
            std::vector<float> input(n),out(n);
            for (std::size_t i=0;i<n;++i) input[i]=heavy?float(int(i%65)-32)/64.0f:float(int(i%33)-16);
            for (auto p:{cs::numeric::policy::plain,cs::numeric::policy::seq,cs::numeric::policy::par,
                         cs::numeric::policy::unseq,cs::numeric::policy::par_unseq}) {
                if (!cs::numeric::policy_available(p)) continue;
                std::fill(out.begin(),out.end(),-12345.0f);
                if (n>0) {
                    bool caught=false;
                    try { cs::policy_bench::verify(input,out,heavy); } catch (const std::runtime_error&) { caught=true; }
                    cs::check(caught,"L3 skip-write fault must fail");
                }
                cs::numeric::map(input,out,p,heavy);
                cs::policy_bench::verify(input,out,heavy);
            }
        }
    for (float invalid:{0.75f,std::numeric_limits<float>::quiet_NaN()}) {
        const std::array<float,1> input{invalid}; std::array<float,1> out{};
        bool caught=false;
        try { cs::numeric::map(input,out,cs::numeric::policy::plain,true); } catch (const std::runtime_error&) { caught=true; }
        cs::check(caught,"heavy domain rejected before callback");
    }
    char program[]="L3_reference", size[]="--size", n[]="17", variant[]="--variant", all[]="all";
    char* argv[]{program,size,n,variant,all};
    cs::check(cs::policy_bench::run(5,argv)==0,"one checked sample for each available policy");
    bool rejected=false;
    char invalid[]="not_a_policy";
    argv[4]=invalid;
    try { cs::policy_bench::run(5,argv); } catch (const std::invalid_argument&) { rejected=true; }
    cs::check(rejected,"invalid benchmark variant rejected");
}
