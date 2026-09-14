#include "solution.hpp"
#include "c13/common.hpp"
#include <array>
int main() { return c13::run([] {
    for (std::size_t n : {std::size_t{0},std::size_t{1},std::size_t{17}}) {
        auto original=c13::make_particles(n);
        auto soa=solution::convert(original);
        c13::validate(soa);
        c13::check(soa.x.size()==n,"conversion retains all elements");
        for (std::size_t i=0; i<n; ++i) {
            c13::check(soa.x[i]==original[i].x && soa.y[i]==original[i].y && soa.z[i]==original[i].z,
                  "conversion retains positions");
            c13::check(soa.vx[i]==original[i].vx && soa.vy[i]==original[i].vy && soa.vz[i]==original[i].vz,
                  "conversion retains velocities");
        }
        solution::advance(soa,0.25f);
        for (std::size_t i=0; i<n; ++i) {
            const auto& p=original[i];
            c13::check(c13::near(soa.x[i],double(p.x)+double(p.vx)*0.25,2e-7,2e-7)
               && c13::near(soa.y[i],double(p.y)+double(p.vy)*0.25,2e-7,2e-7)
               && c13::near(soa.z[i],double(p.z)+double(p.vz)*0.25,2e-7,2e-7),"position update includes the tail");
            c13::check(soa.vx[i]==p.vx && soa.vy[i]==p.vy && soa.vz[i]==p.vz,"update preserves velocities");
        }
    }
    auto malformed=solution::convert(c13::make_particles(2));
    malformed.vz.pop_back();
    const auto before=malformed;
    bool rejected=false;
    try { solution::advance(malformed,0.25f); } catch(const std::invalid_argument&) { rejected=true; }
    c13::check(rejected,"mismatched lengths rejected before access");
    c13::check(malformed.x==before.x && malformed.y==before.y && malformed.z==before.z
        && malformed.vx==before.vx && malformed.vy==before.vy && malformed.vz==before.vz,
        "invalid shape leaves every component unchanged");
    auto p=solution::convert(c13::make_particles(1));
    const auto unchanged=p;
    for(float dt:{-1.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        rejected=false;
        try { solution::advance(p,dt); } catch(const std::invalid_argument&) { rejected=true; }
        c13::check(rejected,"nonfinite or negative time rejected");
        c13::check(p.x==unchanged.x && p.y==unchanged.y && p.z==unchanged.z
            && p.vx==unchanged.vx && p.vy==unchanged.vy && p.vz==unchanged.vz,
            "invalid time leaves every component unchanged");
    }
    std::cout << "layout checks passed\n";
}); }
