// ============================================================
// [A3-T01] Exercise A3: nvcc_ptx_sass
// [A3-T02] Goal: use nvcc -ptx to emit PTX intermediate code,
// [A3-T03]       use cuobjdump to extract SASS machine code,
// [A3-T04]       and observe the fatbin compilation chain.
// [A3-T05]
// [A3-T06] Required workflow (run on the command line after
// [A3-T07] this file finishes building):
// [A3-T08]   1. Build (Release):
// [A3-T09]      cmake --build . --config Release --target A3_nvcc_ptx_sass
// [A3-T10]   2. Emit PTX:
// [A3-T11]      nvcc -ptx main.cu -o main.ptx -arch=sm_80
// [A3-T12]   3. Extract SASS (from the executable):
// [A3-T13]      cuobjdump --dump-sass A3_nvcc_ptx_sass.exe 2>nul | findstr /A 50 "fma_kernel"
// [A3-T14]   4. Recompile with line-info:
// [A3-T15]      nvcc --generate-line-info -ptx main.cu -o main_lineinfo.ptx -arch=sm_80
// [A3-T16]   5. Compare the file sizes of main.ptx and main_lineinfo.ptx
// ============================================================
#include <cstdio>
#include <cmath>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [A3-T17] Target kernel: do some meaningful float work so
// [A3-T18] the PTX/SASS output contains readable instructions
// [A3-T19] (fma, ld, st, etc.).
// ------------------------------------------------------------
// [A3-T20] TODO [REQUIRED] step 1: this is the kernel whose PTX/SASS
//          you will inspect. Do not modify it. Build and run first,
//          then emit PTX, then compare SASS.
__global__ void fma_kernel(const float* __restrict__ a,
                           const float* __restrict__ b,
                           float*       __restrict__ c,
                           int n)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // [A3-T21] fused multiply-add: c = a * b + c
        // [A3-T22] this becomes an fma.rn.f32 instruction in PTX
        c[idx] = a[idx] * b[idx] + c[idx];
    }
}

// ------------------------------------------------------------
// [A3-T23] Advanced: -O3 vs -O0 -- compare PTX code length
// ------------------------------------------------------------
// [A3-T24] TODO [ADVANCED] re-emit PTX with nvcc -O0 -ptx and
//          nvcc -O3 -ptx, compare line count and instruction mix.
// [A3-T25] TODO [ADVANCED] use cuobjdump --dump-elf A3_nvcc_ptx_sass.exe
//          to see which compute capabilities are bundled in the fatbin.

// ------------------------------------------------------------
// [A3-T26] main
// ------------------------------------------------------------
int main() {
    std::puts("[A3_nvcc_ptx_sass]");
    print_device_info(0);

    // [A3-T27] -- workflow hints for the student --
    std::puts("------------------------------------------------------------");
    // [A3-T28]
    std::puts("  [step 2] After building, in the build directory run:");
    // [A3-T29]
    std::puts("    nvcc -ptx main.cu -o main.ptx -arch=sm_80");
    // [A3-T30]
    std::puts("  [step 4] View SASS:");
    // [A3-T31]
    std::puts("    cuobjdump --dump-sass A3_nvcc_ptx_sass.exe");
    // [A3-T32]
    std::puts("  [step 7] With line info:");
    // [A3-T33]
    std::puts("    nvcc --generate-line-info -ptx main.cu -o main_li.ptx -arch=sm_80");
    std::puts("------------------------------------------------------------");

    // [A3-T34] -- prepare data --
    constexpr int N    = 1 << 20;  // [A3-T35] 1M elements
    constexpr int BYTES = N * sizeof(float);

    float *h_a = new float[N];
    float *h_b = new float[N];
    float *h_c = new float[N];

    for (int i = 0; i < N; i++) {
        h_a[i] = static_cast<float>(i) * 0.001f;
        h_b[i] = static_cast<float>(i) * 0.002f;
        h_c[i] = 1.0f;
    }

    float *d_a = nullptr, *d_b = nullptr, *d_c = nullptr;
    CUDA_CHECK(cudaMalloc(&d_a, BYTES));
    CUDA_CHECK(cudaMalloc(&d_b, BYTES));
    CUDA_CHECK(cudaMalloc(&d_c, BYTES));

    CUDA_CHECK(cudaMemcpy(d_a, h_a, BYTES, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_b, h_b, BYTES, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_c, h_c, BYTES, cudaMemcpyHostToDevice));

    // [A3-T36] -- launch fma_kernel --
    {
        NVTX_RANGE("A3/fma_kernel");

        dim3 block(256);
        dim3 grid((N + block.x - 1) / block.x);
        std::printf("[launch] grid=%u  block=%u\n", grid.x, block.x);

        CudaEventTimer timer;
        timer.start();
        fma_kernel<<<grid, block>>>(d_a, d_b, d_c, N);
        CUDA_CHECK_LAST();
        timer.stop();

        // [A3-T37]
        std::printf("[A3] fma_kernel elapsed: %.3f ms\n", timer.elapsed_ms());
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    // [A3-T38] copy results back and do a simple sanity check
    CUDA_CHECK(cudaMemcpy(h_c, d_c, BYTES, cudaMemcpyDeviceToHost));
    {
        // [A3-T39] verify element 0: c[0] = a[0]*b[0] + 1.0f = 0 + 1 = 1.0f
        float expected0 = h_a[0] * h_b[0] + 1.0f;
        bool ok = (h_c[0] == expected0);
        std::printf("[A3] c[0]=%.6f  expected=%.6f  %s\n",
                    h_c[0], expected0, ok ? "OK" : "MISMATCH");
    }

    // [A3-T40] -- cleanup --
    CUDA_CHECK(cudaFree(d_a));
    CUDA_CHECK(cudaFree(d_b));
    CUDA_CHECK(cudaFree(d_c));
    delete[] h_a;
    delete[] h_b;
    delete[] h_c;

    // [A3-T41] -- remind the student of the next steps --
    std::puts("------------------------------------------------------------");
    // [A3-T42]
    std::puts("  TODO [REQUIRED] step 3: open main.ptx, find the fma_kernel");
    // [A3-T43]
    std::puts("              function declaration, copy 10-20 representative lines.");
    // [A3-T44]
    std::puts("  TODO [REQUIRED] step 5: locate the matching block in the SASS dump.");
    // [A3-T45]
    std::puts("  TODO [REQUIRED] step 6: compare PTX vs SASS and write down the differences.");
    std::puts("------------------------------------------------------------");

    std::puts("[A3_nvcc_ptx_sass] DONE");
    return 0;
}
