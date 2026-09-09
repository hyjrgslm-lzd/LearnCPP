// [C2-T01] C2_syncthreads_and_divergence/main.cu
// [C2-T02] Exercise C2: correct use of __syncthreads and divergent-branch pitfalls
//
// [C2-T03] Goals:
//   - Understand __syncthreads as a block-level barrier
//   - Calling __syncthreads inside a divergent branch is undefined behavior
//   - Each step of a shared-memory reduction needs a sync
//   - Use compute-sanitizer --tool synccheck to find sync issues
//
// [C2-T04] Build:    cmake --build build --target C2_syncthreads_and_divergence
// Run:      ./C2_syncthreads_and_divergence
// Sanitizer: compute-sanitizer --tool synccheck ./C2_syncthreads_and_divergence

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [C2-T05] Constants
// ------------------------------------------------------------
constexpr int BLOCK_SIZE = 256;
constexpr int N          = BLOCK_SIZE; // [C2-T06] single-block demo, N == blockDim.x

// ------------------------------------------------------------
// [C2-T07] Kernel 1: write to and read from shared memory (basic check)
// [C2-T08] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void smem_write_read_kernel(int* out)
{
    __shared__ int smem[BLOCK_SIZE];

    int tid = threadIdx.x;

    // [C2-T09] each thread writes its own threadIdx
    smem[tid] = tid;

    // [C2-T10] TODO [REQUIRED-1] add __syncthreads() before reading from another location
    // __syncthreads();

    // [C2-T11] read from a neighboring slot (wrap-around)
    int neighbor = smem[(tid + 1) % BLOCK_SIZE]; // TODO: reading before sync gives wrong value
    out[tid] = neighbor;
}

// ------------------------------------------------------------
// [C2-T12] Kernel 2: __syncthreads inside a divergent branch (deliberate bug)
// [C2-T13] TODO [REQUIRED-2] - detect this kernel with compute-sanitizer --tool synccheck
// [C2-T14] Warning: may hang on a real GPU; while debugging, use synccheck --report-api-errors
// ------------------------------------------------------------
__global__ void bad_divergent_sync_kernel(int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;
    smem[tid] = tid;

    // [C2-T15] TODO [REQUIRED-2] the if branch below is divergent:
    //   odd tids call __syncthreads, even tids do not -> UB / hang
    // Student task: observe diagnostics with compute-sanitizer --tool synccheck,
    // then move __syncthreads outside the if to fix it.

    // if (tid % 2 == 0) {
    //     __syncthreads(); // BUG: only even threads arrive, odd threads never reach here
    // }

    // [C2-T16] write 0 for now so the code still compiles
    out[tid] = 0; // TODO: after the fix, read smem[...] correctly
}

// ------------------------------------------------------------
// [C2-T17] Kernel 3: incorrect reduction (missing a sync)
// [C2-T18] TODO [REQUIRED-3/4] - bad_reduce: students must find the missing __syncthreads
// ------------------------------------------------------------
__global__ void bad_reduce_kernel(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;

    smem[tid] = (tid < N) ? in[tid] : 0;

    // [C2-T19] TODO [REQUIRED-3] the reduction below is missing __syncthreads, results are wrong
    // Student task: figure out where each step needs a sync
    for (int s = BLOCK_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) {
            smem[tid] += smem[tid + s]; // BUG: no __syncthreads()
        }
        // [C2-T20] TODO [REQUIRED-3]: add __syncthreads() here
    }

    if (tid == 0) out[0] = 0; // [C2-T21] stub: write 0; after the fix, write smem[0]
}

// ------------------------------------------------------------
// [C2-T22] Kernel 4: correct reduction (sync at every step)
// [C2-T23] TODO [REQUIRED-3] - good_reduce: reference implementation
// ------------------------------------------------------------
__global__ void good_reduce_kernel(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;

    smem[tid] = (tid < N) ? in[tid] : 0;
    __syncthreads();

    // [C2-T24] TODO [REQUIRED-3] complete the strided reduction correctly
    for (int s = BLOCK_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) {
            // TODO: smem[tid] += smem[tid + s];
        }
        __syncthreads(); // [C2-T25] each step must sync
    }

    if (tid == 0) out[0] = 0; // [C2-T26] stub: after the fix, store smem[0]
}

// ------------------------------------------------------------
// [C2-T27] Kernel 5: bank-conflict-aware reduction
// [C2-T28] TODO [REQUIRED-5]
// ------------------------------------------------------------
__global__ void bc_aware_reduce_kernel(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;

    smem[tid] = (tid < N) ? in[tid] : 0;
    __syncthreads();

    // [C2-T29] TODO [REQUIRED-5] use offset access to reduce bank conflicts
    // The standard strided reduce has bank conflicts; students should analyze and mark them
    for (int s = 1; s < BLOCK_SIZE; s <<= 1) {
        int idx = 2 * s * tid;
        if (idx < BLOCK_SIZE) {
            // TODO: smem[idx] += smem[idx + s];
        }
        __syncthreads();
    }

    if (tid == 0) out[0] = 0; // [C2-T30] stub
}

// ------------------------------------------------------------
// [C2-T31] CPU reference reduction
// ------------------------------------------------------------
static int cpu_reduce(const int* data, int n)
{
    int sum = 0;
    for (int i = 0; i < n; ++i) sum += data[i];
    return sum;
}

// ------------------------------------------------------------
// [C2-T32] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("C2/main");

    // ------------------------------------------------------------
    // [C2-T33] Prepare data
    // ------------------------------------------------------------
    int h_in[N], h_out_bad[1], h_out_good[1], h_out_smem[1];
    for (int i = 0; i < N; ++i) h_in[i] = i + 1; // [C2-T34] 1..256

    int cpu_result = cpu_reduce(h_in, N);
    // [C2-T35]
    printf("CPU reduce reference result = %d\n\n", cpu_result);

    int *d_in = nullptr, *d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,  N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out, sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in, N * sizeof(int), cudaMemcpyHostToDevice));

    // ------------------------------------------------------------
    // [C2-T36] Test 1: smem write+read
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C2/smem_write_read");
        int* d_tmp = nullptr;
        CUDA_CHECK(cudaMalloc(&d_tmp, N * sizeof(int)));
        smem_write_read_kernel<<<1, BLOCK_SIZE>>>(d_tmp);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        // [C2-T37]
        printf("[REQUIRED-1] smem_write_read_kernel done (TODO: validate after adding sync)\n");
        CUDA_CHECK(cudaFree(d_tmp));
    }

    // ------------------------------------------------------------
    // [C2-T38] Test 2: bad divergent sync (do not run outside sanitizer)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C2/bad_divergent");
        // [C2-T39] TODO [REQUIRED-2] uncomment, then run with compute-sanitizer --tool synccheck
        // bad_divergent_sync_kernel<<<1, BLOCK_SIZE>>>(d_out);
        // CUDA_CHECK(cudaGetLastError());
        // CUDA_CHECK(cudaDeviceSynchronize());
        // [C2-T40]
        printf("[REQUIRED-2] bad_divergent_sync_kernel: observe UB with synccheck (kernel commented out)\n");
    }

    // ------------------------------------------------------------
    // [C2-T41] Test 3: bad reduce vs good reduce
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C2/reduce_compare");

        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        bad_reduce_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out_bad, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C2-T42]
        printf("[REQUIRED-3] bad_reduce  result = %d (expected %d, currently stub=0)\n",
               h_out_bad[0], cpu_result);

        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        good_reduce_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out_good, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C2-T43]
        printf("[REQUIRED-3] good_reduce result = %d (expected %d, currently stub=0)\n",
               h_out_good[0], cpu_result);
    }

    // ------------------------------------------------------------
    // [C2-T44] Test 4: bank-conflict-aware reduce
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C2/bc_aware_reduce");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        bc_aware_reduce_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out_smem, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C2-T45]
        printf("[REQUIRED-5] bc_aware_reduce result = %d (expected %d, currently stub=0)\n",
               h_out_smem[0], cpu_result);
    }

    // ------------------------------------------------------------
    // [C2-T46] TODO [ADVANCED] three-level reduction: in-block -> across-blocks + atomic
    // [C2-T47] TODO [ADVANCED] replace one reduction layer with warp shuffle
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));

    // [C2-T48]
    printf("\n[C2] done. Tip: compute-sanitizer --tool synccheck ./C2_syncthreads_and_divergence\n");
    return 0;
}
