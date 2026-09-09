// Implementation Starter. The supplied benchmark lives in benchmark.cpp.
// reference.hpp is used for independent verification, never to execute the answer.
#include "reference.hpp"
#include <array>

float student_light(float /*x*/) noexcept {
    // TODO Part 1: compute x*x+2 for the documented small-integer domain.
    return 0;
}
float student_heavy(float /*x*/) noexcept {
    // TODO Part 2: evaluate sum(k=0..96,x^k) in double, then convert to float.
    return 0;
}
void student_map(cs::numeric::input a,cs::numeric::output out,
                 cs::numeric::policy p,bool /*heavy*/) {
    cs::numeric::unary_shape(a,out);
    cs::check(cs::numeric::policy_available(p),"selected policy unavailable");
    // TODO Part 3: transform using student_light/student_heavy and selected policy.
    // Validate the heavy input domain BEFORE entering a policy callback.
}

int main() try {
    const std::array<float,5> integers{-2,-1,0,1,2};
    for (float x:integers) cs::check(student_light(x)==float(int(x)*int(x)+2),"L3 Part 1: student_light is incomplete");
    const std::array<float,5> fractions{-0.5f,-0.25f,0,0.25f,0.5f};
    std::array<float,5> out{};
    for (std::size_t i=0;i<fractions.size();++i) out[i]=student_heavy(fractions[i]);
    cs::policy_bench::verify(fractions,out,true);
    for (bool heavy:{false,true}) {
        const auto& input=heavy?fractions:integers;
        for (auto p:{cs::numeric::policy::plain,cs::numeric::policy::seq,cs::numeric::policy::par,
                     cs::numeric::policy::unseq,cs::numeric::policy::par_unseq}) {
            if (!cs::numeric::policy_available(p)) continue;
            out.fill(-12345);
            student_map(input,out,p,heavy); // Check the student's path, not numeric::map.
            cs::policy_bench::verify(input,out,heavy);
        }
    }
    std::cout << "L3 student checks passed for these inputs; run the full Reference and study the measurement Parts\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
