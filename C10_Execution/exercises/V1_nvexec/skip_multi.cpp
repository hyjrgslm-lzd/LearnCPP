#include <c10/test.hpp>

#include <iostream>

int main() {
  return c10::test_main([] {
    std::cout << "NVEXEC_NO_COMPILER: nvc++ not found; CUDA/NVCC is not nvexec support\n";
    throw c10::skip("multi-GPU nvexec body requires nvc++ from NVIDIA HPC SDK");
  });
}
