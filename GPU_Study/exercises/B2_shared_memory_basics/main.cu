// ============================================================
// [B2-T01] Exercise B2: shared_memory_basics
// [B2-T02] Goal: learn how to declare and use shared memory inside a block.
//          Compare static vs dynamic shared memory; write a block-level reduce kernel.
//
// [B2-T03] Acceptance:
//   shared-memory reduce throughput > global-memory-only version.
//   Nsight Compute: ncu --set full -o b2.ncu-rep ./B2_shared_memory_basics.exe
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [B2-T04] Constants
// ------------------------------------------------------------
static constexpr int N_ELEM   = 1 << 20;  // [B2-T05] 1M ints
static constexpr int BLOCK_SZ = 256;
static constexpr int WARMUP   = 3;

// ------------------------------------------------------------
// [B2-T06] Kernel 1: pure global-memory reduce (baseline)
// ------------------------------------------------------------
// [B2-T07] TODO [REQUIRED] step 1: implement reduce_global_only
//   Each thread reads one input element; accumulate within the block (no shared memory),
//   write the result to output[blockIdx.x].
//
//   Note: this is the baseline; atomic writes or segmented accumulation are fine,
//         keep the logic simple.
__global__ void reduce_global_only(const int* __restrict__ input,
                                   int*       __restrict__ output,
                                   int n)
{
    // [B2-T08] TODO [REQUIRED]: int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // [B2-T09] TODO [REQUIRED]: // accumulate this thread's contribution in a register, then atomicAdd to output[blockIdx.x]
    (void)input; (void)output; (void)n;
}

// ------------------------------------------------------------
// [B2-T10] Kernel 2: shared-memory reduce (dynamic smem)
// ------------------------------------------------------------
// [B2-T11] TODO [REQUIRED] step 3: implement reduce_with_smem
//   Step 1: coalesced global read -> sdata[threadIdx.x]
//   Step 2: __syncthreads()
//   Step 3: tree-reduce in shared memory (loop with halving stride)
//   Step 4: thread 0 writes back output[blockIdx.x]
//
// Launch:
//   reduce_with_smem<<<grid, block, block.x * sizeof(int)>>>(input, output, n);
__global__ void reduce_with_smem(const int* __restrict__ input,
                                 int*       __restrict__ output,
                                 int n)
{
    extern __shared__ int sdata[];   // [B2-T12] size specified at launch

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + tid;

    // [B2-T13] TODO [REQUIRED]: sdata[tid] = (idx < n) ? input[idx] : 0;
    // [B2-T14] TODO [REQUIRED]: __syncthreads();
    sdata[tid] = 0;  // [B2-T15] placeholder; replace once student fills the TODO
    __syncthreads();

    // [B2-T16] TODO [REQUIRED]: implement tree-reduce:
    //   for (int s = blockDim.x / 2; s > 0; s >>= 1) {
    //       if (tid < s) { sdata[tid] += sdata[tid + s]; }
    //       __syncthreads();
    //   }

    // [B2-T17] TODO [REQUIRED]: if (tid == 0) { output[blockIdx.x] = sdata[0]; }
    (void)input; (void)output; (void)n;
}

// ------------------------------------------------------------
// [B2-T18] Kernel 3: static shared memory variant (advanced comparison)
// ------------------------------------------------------------
// [B2-T19] TODO [ADVANCED] implement the static shared-memory variant and compare performance
#if 0
__global__ void reduce_static_smem(const int* __restrict__ input,
                                   int*       __restrict__ output,
                                   int n)
{
    __shared__ int sdata[BLOCK_SZ];  // [B2-T20] static declaration; size known at compile time
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + tid;

    sdata[tid] = (idx < n) ? input[idx] : 0;
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }

    if (tid == 0) output[blockIdx.x] = sdata[0];
}
#endif

// ------------------------------------------------------------
// [B2-T21] Host-side: final sum over the block-level results
// ------------------------------------------------------------
static int host_final_sum(const int* block_sums, int num_blocks) {
    int total = 0;
    for (int i = 0; i < num_blocks; i++) total += block_sums[i];
    return total;
}

// ------------------------------------------------------------
// [B2-T22] Timing helper
// ------------------------------------------------------------
template<typename Fn>
static float bench_gbps(Fn fn, long long bytes, int n_runs = 5) {
    for (int i = 0; i < WARMUP; i++) fn();
    CUDA_CHECK(cudaDeviceSynchronize());

    CudaEventTimer timer;
    timer.start();
    for (int i = 0; i < n_runs; i++) fn();
    timer.stop();

    float ms = timer.elapsed_ms() / n_runs;
    return static_cast<float>(bytes) / ms / 1e6f;
}

// ------------------------------------------------------------
// [B2-T23] main
// ------------------------------------------------------------
int main() {
    std::puts("[B2_shared_memory_basics]");
    print_device_info(0);

    const long long BYTES = (long long)N_ELEM * sizeof(int);
    const int num_blocks  = (N_ELEM + BLOCK_SZ - 1) / BLOCK_SZ;

    // [B2-T24] -- allocate host data --
    int* h_input = (int*)std::malloc(BYTES);
    int  expected_sum = 0;
    for (int i = 0; i < N_ELEM; i++) {
        h_input[i]   = 1;  // [B2-T25] all ones; expected reduce result = N_ELEM
        expected_sum += h_input[i];
    }
    std::printf("[B2] N=%d  expected_sum=%d\n", N_ELEM, expected_sum);

    // [B2-T26] -- allocate device memory --
    int *d_input = nullptr, *d_output = nullptr;
    CUDA_CHECK(cudaMalloc(&d_input,  BYTES));
    CUDA_CHECK(cudaMalloc(&d_output, num_blocks * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_input, h_input, BYTES, cudaMemcpyHostToDevice));

    dim3 block(BLOCK_SZ);
    dim3 grid(num_blocks);

    // [B2-T27] -- Part 1: global-memory reduce (baseline) --
    {
        NVTX_RANGE("B2/reduce_global");

        CUDA_CHECK(cudaMemset(d_output, 0, num_blocks * sizeof(int)));
        std::printf("[launch global-only] grid=%u  block=%u  smem=0B\n",
                    grid.x, block.x);

        // [B2-T28] TODO [REQUIRED] step 2: time the global-memory version
        float gbps_global = bench_gbps([&] {
            reduce_global_only<<<grid, block>>>(d_input, d_output, N_ELEM);
        }, BYTES);   // [B2-T29] only reads, no write back (simplified accounting)

        // [B2-T30] copy back result and verify
        int* h_out = (int*)std::malloc(num_blocks * sizeof(int));
        CUDA_CHECK(cudaMemcpy(h_out, d_output, num_blocks * sizeof(int),
                              cudaMemcpyDeviceToHost));
        int actual = host_final_sum(h_out, num_blocks);
        // [B2-T31]
        std::printf("[B2] global reduce: %.1f GB/s  result=%d  %s\n",
                    gbps_global, actual,
                    (actual == expected_sum) ? "OK" : "WRONG(TODO unfilled?)");
        std::free(h_out);
    }

    // [B2-T32] -- Part 2: shared-memory reduce --
    {
        NVTX_RANGE("B2/reduce_smem");

        CUDA_CHECK(cudaMemset(d_output, 0, num_blocks * sizeof(int)));
        int smem_bytes = BLOCK_SZ * (int)sizeof(int);
        std::printf("[launch smem-reduce] grid=%u  block=%u  smem=%dB\n",
                    grid.x, block.x, smem_bytes);

        // [B2-T33] TODO [REQUIRED] step 4: specify dynamic shared-memory size (3rd launch parameter)
        float gbps_smem = bench_gbps([&] {
            reduce_with_smem<<<grid, block, smem_bytes>>>(d_input, d_output, N_ELEM);
        }, BYTES);

        int* h_out2 = (int*)std::malloc(num_blocks * sizeof(int));
        CUDA_CHECK(cudaMemcpy(h_out2, d_output, num_blocks * sizeof(int),
                              cudaMemcpyDeviceToHost));
        int actual2 = host_final_sum(h_out2, num_blocks);
        // [B2-T34]
        std::printf("[B2] SMEM reduce:   %.1f GB/s  result=%d  %s\n",
                    gbps_smem, actual2,
                    (actual2 == expected_sum) ? "OK" : "WRONG(TODO unfilled?)");
        std::free(h_out2);
    }

    // [B2-T35] -- Nsight Compute hint --
    std::puts("------------------------------------------------------------");
    // [B2-T36]
    std::puts("  TODO [REQUIRED] step 7: run Nsight Compute:");
    std::puts("    ncu --set full -o b2.ncu-rep ./B2_shared_memory_basics.exe");
    // [B2-T37]
    std::puts("  Compare achieved occupancy and smem utilization between the two kernels.");
    std::puts("------------------------------------------------------------");

    // [B2-T38] -- cleanup --
    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
    std::free(h_input);

    std::puts("[B2_shared_memory_basics] DONE");
    return 0;
}
