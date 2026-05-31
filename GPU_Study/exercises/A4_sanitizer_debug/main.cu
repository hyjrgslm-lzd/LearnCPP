// ============================================================
// [A4-T01] Exercise A4: sanitizer_debug
// [A4-T02] Goal: use compute-sanitizer to locate memory access
// [A4-T03]       errors (out-of-bounds) and data races.
// [A4-T04]
// [A4-T05] Required workflow:
// [A4-T06]   memcheck:
// [A4-T07]     compute-sanitizer memcheck A4_sanitizer_debug.exe
// [A4-T08]   racecheck:
// [A4-T09]     compute-sanitizer --tool racecheck A4_sanitizer_debug.exe
// [A4-T10]   synccheck:
// [A4-T11]     compute-sanitizer --tool synccheck A4_sanitizer_debug.exe
// [A4-T12]
// [A4-T13] Note: this file deliberately contains 3 buggy kernels.
// [A4-T14]       The exercise is to find them with the sanitizer
// [A4-T15]       and then fix them. After fixing, the sanitizer
// [A4-T16]       should report "no errors detected".
// ============================================================
#include <cstdio>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [A4-T17] Bug Kernel 1: deliberate out-of-bounds write
// ------------------------------------------------------------
// [A4-T18] TODO [REQUIRED] step 1: find the OOB error in this kernel.
//   hint: look at the boundary in the if condition.
__global__ void buggy_oob_kernel(int* data, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // [A4-T19] BUG: using n+1 makes the last thread write data[n] (OOB)
    if (idx < n + 1) {
        data[idx] = idx;
    }
}

// [A4-T20] TODO [REQUIRED] step 4: fixed version -- replace n+1 with n
#if 0
__global__ void fixed_oob_kernel(int* data, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {   // [A4-T21] correct boundary
        data[idx] = idx;
    }
}
#endif

// ------------------------------------------------------------
// [A4-T22] Bug Kernel 2: shared memory data race
// ------------------------------------------------------------
// [A4-T23] TODO [REQUIRED] step 5: find the data race in this kernel.
//   hint: multiple threads write the same global address with no atomic.
__global__ void buggy_race_kernel(int* flag) {
    int idx = threadIdx.x;
    if (idx < 2) {
        // [A4-T24] BUG: two threads both write flag[0] -> data race
        for (int i = 0; i < 1000; i++) {
            flag[0]++;  // [A4-T25] non-atomic read-modify-write -> race!
        }
    }
}

// [A4-T26] TODO [REQUIRED] step 7: fixed version -- use atomicAdd
#if 0
__global__ void fixed_race_kernel(int* flag) {
    int idx = threadIdx.x;
    if (idx < 2) {
        for (int i = 0; i < 1000; i++) {
            atomicAdd(&flag[0], 1);  // [A4-T27] atomic op, no race
        }
    }
}
#endif

// ------------------------------------------------------------
// [A4-T28] Bug Kernel 3: missing __syncthreads (sync error)
// ------------------------------------------------------------
// [A4-T29] TODO [REQUIRED] (advanced) step: when __syncthreads() is
//   called inside a divergent branch and only some threads reach it,
//   synccheck will report an error.
__global__ void buggy_sync_kernel(int* data, int n) {
    __shared__ int sdata[256];
    int idx = threadIdx.x;

    sdata[idx] = (idx < n) ? data[idx] : 0;

    // [A4-T30] BUG: __syncthreads inside a conditional branch --
    //          only some threads reach it.
    if (idx % 2 == 0) {
        __syncthreads();  // [A4-T31] odd threads never reach here -> deadlock!
    }

    if (idx == 0) {
        int sum = 0;
        for (int i = 0; i < blockDim.x; i++) sum += sdata[i];
        data[0] = sum;
    }
}

// [A4-T32] TODO [ADVANCED] fixed version: move __syncthreads() out of the branch
#if 0
__global__ void fixed_sync_kernel(int* data, int n) {
    __shared__ int sdata[256];
    int idx = threadIdx.x;

    sdata[idx] = (idx < n) ? data[idx] : 0;
    __syncthreads();  // [A4-T33] correct: every thread reaches it

    if (idx == 0) {
        int sum = 0;
        for (int i = 0; i < blockDim.x; i++) sum += sdata[i];
        data[0] = sum;
    }
}
#endif

// ------------------------------------------------------------
// [A4-T34] main
// ------------------------------------------------------------
int main() {
    std::puts("[A4_sanitizer_debug]");
    print_device_info(0);

    std::puts("------------------------------------------------------------");
    // [A4-T35]
    std::puts("  This program contains 3 deliberately buggy kernels.");
    // [A4-T36]
    std::puts("  Use compute-sanitizer to locate them, then fix them.");
    // [A4-T37]
    std::puts("  Fix recipe: enable the fixed_xxx variant inside the");
    // [A4-T38]
    std::puts("              #if 0 / #endif block, and replace the");
    // [A4-T39]
    std::puts("              buggy_xxx call sites with fixed_xxx.");
    std::puts("------------------------------------------------------------");

    // ============================================================
    // [A4-T40] Test 1: out-of-bounds
    // ============================================================
    {
        NVTX_RANGE("A4/oob_test");

        constexpr int N = 1000;
        int* d_data = nullptr;
        CUDA_CHECK(cudaMalloc(&d_data, N * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_data, 0, N * sizeof(int)));

        // [A4-T41] TODO [REQUIRED] step 2: launch with at least 1001 threads
        //          (to trigger the OOB).
        // [A4-T42] grid/block such that ceil(1001/256)*256 = 1024 >= 1001 threads
        dim3 block_oob(256);
        // [A4-T43] note: (N + block_oob.x) / block_oob.x = 5 blocks = 1280 threads > 1001
        dim3 grid_oob((N + block_oob.x) / block_oob.x);
        std::printf("[launch oob] grid=%u block=%u  total_threads=%u  N=%d\n",
                    grid_oob.x, block_oob.x, grid_oob.x * block_oob.x, N);

        // [A4-T44] TODO [REQUIRED] step 3: run under compute-sanitizer memcheck and observe
        buggy_oob_kernel<<<grid_oob, block_oob>>>(d_data, N);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());

        // [A4-T45]
        std::puts("[A4] OOB kernel finished (sanitizer should report OOB)");
        CUDA_CHECK(cudaFree(d_data));
    }

    // ============================================================
    // [A4-T46] Test 2: data race
    // ============================================================
    {
        NVTX_RANGE("A4/race_test");

        int h_flag = 0;
        int* d_flag = nullptr;
        CUDA_CHECK(cudaMalloc(&d_flag, sizeof(int)));
        CUDA_CHECK(cudaMemcpy(d_flag, &h_flag, sizeof(int), cudaMemcpyHostToDevice));

        // [A4-T47] 32 threads; threads 0 and 1 race on flag[0]
        dim3 block_race(32);
        dim3 grid_race(1);
        std::printf("[launch race] grid=%u block=%u\n",
                    grid_race.x, block_race.x);

        // [A4-T48] TODO [REQUIRED] step 6: run under compute-sanitizer --tool racecheck
        buggy_race_kernel<<<grid_race, block_race>>>(d_flag);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());

        CUDA_CHECK(cudaMemcpy(&h_flag, d_flag, sizeof(int), cudaMemcpyDeviceToHost));
        // [A4-T49]
        std::printf("[A4] Race kernel done: flag=%d (expected 2000, but race makes the result undefined)\n",
                    h_flag);

        CUDA_CHECK(cudaFree(d_flag));
    }

    // ============================================================
    // [A4-T50] Test 3: sync error
    // [A4-T51] (note: outside of the sanitizer this kernel may hang/timeout
    // [A4-T52]  the GPU; only enable the #if 0 below when running under
    // [A4-T53]  the sanitizer.)
    // ============================================================
    // [A4-T54] TODO [ADVANCED] flip #if 0 to #if 1 and run with
    //          compute-sanitizer --tool synccheck.
#if 0
    {
        NVTX_RANGE("A4/sync_test");
        constexpr int M = 256;
        int* d_sync = nullptr;
        CUDA_CHECK(cudaMalloc(&d_sync, M * sizeof(int)));
        for (int i = 0; i < M; i++) { int v=i; cudaMemcpy(d_sync+i,&v,4,cudaMemcpyHostToDevice); }

        buggy_sync_kernel<<<1, M>>>(d_sync, M);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());
        // [A4-T55]
        std::puts("[A4] sync kernel done (compute-sanitizer synccheck should report)");

        CUDA_CHECK(cudaFree(d_sync));
    }
#endif

    std::puts("------------------------------------------------------------");
    // [A4-T56]
    std::puts("  TODO [REQUIRED] when finished:");
    // [A4-T57]
    std::puts("    1. Uncomment the fixed_xxx kernel variants");
    // [A4-T58]
    std::puts("    2. Replace buggy_xxx with fixed_xxx in main()");
    // [A4-T59]
    std::puts("    3. Re-run compute-sanitizer and confirm 'no errors'");
    std::puts("------------------------------------------------------------");

    std::puts("[A4_sanitizer_debug] DONE");
    return 0;
}
