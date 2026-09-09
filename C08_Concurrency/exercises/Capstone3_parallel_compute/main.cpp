// Implementation Starter. benchmark.cpp is the supplied measurement driver.
// Only input generation and the independent oracle are reused from reference.hpp.
#include "reference.hpp"

void student_gemm(cs::numeric::input /*a*/,cs::numeric::input /*b*/,cs::numeric::output /*c*/,
                  std::size_t /*n*/,std::size_t /*block*/,std::string_view /*version*/) {
    // TODO Parts 1-3: naive, tiled, threaded/par GEMM with disjoint output rows.
    // TODO optional SIMD Part: a real explicit vector loop for the sse2 version.
    // All versions overwrite C, handle tail tiles, and propagate manual worker errors.
}
double student_sum(cs::numeric::input /*values*/,std::string_view /*version*/) {
    // TODO Parts 4-5: transform_reduce (plain/par), then manual local sums + join.
    return 0;
}
void student_sort(std::span<int> /*values*/,bool /*parallel*/) {
    // TODO Part 6: ordinary or policy sort according to the selected version.
}

int main(int argc, char** argv) try {
    using namespace cs::numeric;
    const bool with_simd = argc == 2 && std::string_view(argv[1]) == "--with-simd";
    cs::check(argc == 1 || with_simd, "usage: Capstone3_parallel_compute [--with-simd]");
    constexpr std::size_t n=3;
    std::array<float,n*n> a{},b{},c{};
    cs::cap3::fill_matrices(a,b,n);
    for (auto version:{"naive","tiled","threaded","par"}) {
        if (std::string_view(version)=="par" && !policy_available(policy::par)) continue;
        c.fill(-12345);
        student_gemm(a,b,c,n,2,version);
        cs::cap3::verify_matrix(c,n); // Validates the student's actual output.
    }
    const std::array<float,5> values{0.5f,0.5f,0.5f,0.5f,0.5f};
    for (auto version:{"plain","par","threaded"}) {
        if (std::string_view(version)=="par" && !policy_available(policy::par)) continue;
        cs::check(student_sum(values,version)==2.5,"Cap3 Part 4/5: student reduction");
    }
    for (bool parallel:{false,true}) {
        if (parallel && !policy_available(policy::par)) continue;
        std::array<int,5> sort_input{3,-1,2,-1,0};
        student_sort(sort_input,parallel);
        cs::check(sort_input==std::array<int,5>{-1,-1,0,2,3},"Cap3 Part 6: student sort order and multiplicity");
    }
    std::cout << "Cap3 mandatory student small checks passed\n";
    if (with_simd) {
        if (!available(backend::sse2)) {
            std::cerr << "SKIP: requested optional SSE2 path is unavailable\n";
            return 77;
        }
        c.fill(-12345);
        student_gemm(a,b,c,n,2,"sse2");
        cs::cap3::verify_matrix(c,n);
        std::cout << "Optional SIMD output checks passed; inspect the actual vector implementation too\n";
    } else {
        std::cout << "Optional SIMD not requested; use --with-simd after implementing it\n";
    }
    std::cout << "Reference covers more sizes, faults and contracts\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
