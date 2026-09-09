// Provided benchmark driver; implementation exercises live in main.cpp.
#include "reference.hpp"
#include <iostream>
int main(int argc,char** argv) try { return cs::policy_bench::run(argc,argv); }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
