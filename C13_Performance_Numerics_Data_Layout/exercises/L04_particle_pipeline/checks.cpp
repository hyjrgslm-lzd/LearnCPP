#include "solution.hpp"
int main() { return c13::run([] {
    for(std::size_t n:{std::size_t{0},std::size_t{1},std::size_t{257}}) {
        const auto result=student::run_pipeline(n,1234,c13::milliseconds{16});
        const auto input=c13::make_particles(n,1234);
        check(result.positions.size()==n*3,"pipeline output shape");
        double expected_sum=0;
        for(std::size_t i=0;i<n;++i) {
            const auto p=input[i];
            const float x=p.x+p.vx*0.016f,y=p.y+p.vy*0.016f,z=p.z+p.vz*0.016f;
            const std::array<double,3> expected{-double(y),double(x),double(z)};
            for(std::size_t k=0;k<3;++k) {
                check(c13::near(result.positions[3*i+k],expected[k],2e-6,2e-6),"pipeline position includes elapsed time and transform");
                expected_sum+=expected[k];
            }
        }
        check(c13::near(result.sum,expected_sum,1e-10,1e-10),"pipeline reduction covers transformed positions");
    }
    for(double dt:{-1.0,1001.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
        bool rejected=false;
        try { (void)student::run_pipeline(0,1,c13::milliseconds{dt}); }
        catch(const std::invalid_argument&) { rejected=true; }
        check(rejected,"pipeline rejects invalid duration even for empty input");
    }
    std::cout << "pipeline checks passed\n";
}); }
