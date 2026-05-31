// ============================================================
// [B3-T01] Exercise B3: bank_conflict_and_swizzle
// [B3-T02] Goal: gain a deep understanding of the 32-bank shared memory model.
//          Observe how bank conflicts cause serialization, and learn to remove
//          them with padding.
//
// [B3-T03] Acceptance:
//   stride-32 throughput << stride-1 throughput (~30x slower).
//   Nsight Compute: ncu --set full -o b3.ncu-rep ./B3_bank_conflict_and_swizzle.exe
//   Inspect the shared_ld_bank_conflict / shared_st_bank_conflict metrics.
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [B3-T04] Constants
// ------------------------------------------------------------
static constexpr int BLOCK_SZ   = 32;     // [B3-T05] one warp
static constexpr int INNER_LOOP = 10000;  // [B3-T06] loop count to amplify timing differences
static constexpr int GRID_SZ    = 1024;   // [B3-T07] enough blocks to amortize latency

// ------------------------------------------------------------
// [B3-T08] Kernel 1: stride-1 access (no bank conflict)
// ------------------------------------------------------------
// [B3-T09] TODO [REQUIRED] step 1: understand why this kernel has no conflict
//   thread i accesses sdata[i], bank_id = (i * 4B / 4) % 32 = i % 32
//   32 threads -> 32 distinct banks -> no conflict
__global__ void access_stride_1(float* global_out, int n) {
    __shared__ float sdata[BLOCK_SZ];
    int tid = threadIdx.x;

    // [B3-T10] TODO [REQUIRED]: inside INNER_LOOP iterations:
    //   sdata[tid] = (float)tid;    // stride-1 write
    //   __syncthreads();
    //   float v = sdata[tid];       // stride-1 read
    //   __syncthreads();
    float acc = 0.0f;
    for (int iter = 0; iter < INNER_LOOP; iter++) {
        sdata[tid] = static_cast<float>(tid + iter);
        __syncthreads();
        acc += sdata[tid];           // [B3-T11] TODO [REQUIRED]: use sdata[tid] (stride-1)
        __syncthreads();
    }

    if (tid == 0 && blockIdx.x < n) global_out[blockIdx.x] = acc;
}

// ------------------------------------------------------------
// [B3-T12] Kernel 2: stride-32 access (worst-case bank conflict)
// ------------------------------------------------------------
// [B3-T13] TODO [REQUIRED] step 2: understand why this kernel always conflicts
//   thread i accesses sdata[(i * 32) % 32] = sdata[0]
//   all 32 threads hit bank 0 -> 32-way conflict -> serialized 32 times
__global__ void access_stride_32(float* global_out, int n) {
    __shared__ float sdata[BLOCK_SZ * 32];  // [B3-T14] big enough to hold the stride
    int tid = threadIdx.x;

    float acc = 0.0f;
    for (int iter = 0; iter < INNER_LOOP; iter++) {
        // [B3-T15] TODO [REQUIRED]: int idx = tid * 32;   // stride 32 -> all threads hit bank 0
        int idx = tid * 32;
        sdata[idx] = static_cast<float>(tid + iter);
        __syncthreads();
        acc += sdata[idx];
        __syncthreads();
    }

    if (tid == 0 && blockIdx.x < n) global_out[blockIdx.x] = acc;
}

// ------------------------------------------------------------
// [B3-T16] Kernel 3: padding eliminates the conflict
// ------------------------------------------------------------
// [B3-T17] TODO [REQUIRED] step 5: understand the padding trick
//   Declare sdata[32 + 1] (one extra float as padding).
//   thread i accesses sdata[i], address offset = i * 4B
//   bank_id = (address / 4) % 32
//   The padding changes the address spacing between consecutive threads,
//   making them land on different banks.
__global__ void access_with_padding(float* global_out, int n) {
    __shared__ float sdata[BLOCK_SZ + 1];  // [B3-T18] +1 padding
    int tid = threadIdx.x;

    float acc = 0.0f;
    for (int iter = 0; iter < INNER_LOOP; iter++) {
        // [B3-T19] TODO [REQUIRED]: same writes/reads as stride-1, but with padding
        sdata[tid] = static_cast<float>(tid + iter);
        __syncthreads();
        acc += sdata[tid];
        __syncthreads();
    }

    if (tid == 0 && blockIdx.x < n) global_out[blockIdx.x] = acc;
}

// ------------------------------------------------------------
// [B3-T20] Advanced: 32x32 matrix transpose -- naive vs padded
// ------------------------------------------------------------
// [B3-T21] TODO [ADVANCED] implement three versions of a 32x32 transpose:
//   1. naive (using __shared__ float tile[32][32], with bank conflict)
//   2. padded (__shared__ float tile[32][33], conflict eliminated)
//   3. compare shared_ld_bank_conflict between the two using Nsight Compute
#if 0
static constexpr int TILE = 32;

__global__ void transpose_naive(const float* __restrict__ in,
                                float*       __restrict__ out,
                                int width, int height)
{
    __shared__ float tile[TILE][TILE];  // bank conflict!
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    if (x < width && y < height)
        tile[threadIdx.y][threadIdx.x] = in[y * width + x];
    __syncthreads();
    // [B3-T22] write transposed output
    int ox = blockIdx.y * TILE + threadIdx.x;
    int oy = blockIdx.x * TILE + threadIdx.y;
    if (ox < height && oy < width)
        out[oy * height + ox] = tile[threadIdx.x][threadIdx.y];
}

__global__ void transpose_padded(const float* __restrict__ in,
                                 float*       __restrict__ out,
                                 int width, int height)
{
    __shared__ float tile[TILE][TILE + 1];  // [B3-T23] +1 padding eliminates conflict
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    if (x < width && y < height)
        tile[threadIdx.y][threadIdx.x] = in[y * width + x];
    __syncthreads();
    int ox = blockIdx.y * TILE + threadIdx.x;
    int oy = blockIdx.x * TILE + threadIdx.y;
    if (ox < height && oy < width)
        out[oy * height + ox] = tile[threadIdx.x][threadIdx.y];
}
#endif

// ------------------------------------------------------------
// [B3-T24] Timing helper
// ------------------------------------------------------------
template<typename Fn>
static float bench_ms(Fn fn, int n_runs = 5) {
    // [B3-T25] warmup
    for (int i = 0; i < 3; i++) fn();
    CUDA_CHECK(cudaDeviceSynchronize());

    CudaEventTimer timer;
    timer.start();
    for (int i = 0; i < n_runs; i++) fn();
    timer.stop();
    return timer.elapsed_ms() / n_runs;
}

// ------------------------------------------------------------
// [B3-T26] main
// ------------------------------------------------------------
int main() {
    std::puts("[B3_bank_conflict_and_swizzle]");
    print_device_info(0);

    // [B3-T27] Bank ID formula explanation
    std::puts("  Bank ID = (address_in_bytes / 4) % 32");
    // [B3-T28]
    std::puts("  stride-1:  thread i -> bank i           (no conflict)");
    // [B3-T29]
    std::puts("  stride-32: thread i -> bank 0           (32-way conflict)");
    // [B3-T30]
    std::puts("  padding:   shifts offsets, thread i -> bank i+1 (no conflict)");

    float* d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_out, GRID_SZ * sizeof(float)));

    dim3 grid(GRID_SZ);
    dim3 block(BLOCK_SZ);
    std::printf("[launch] grid=%d  block=%d  inner_loop=%d\n",
                GRID_SZ, BLOCK_SZ, INNER_LOOP);

    // [B3-T31] -- test stride-1 (no conflict) --
    float ms_s1;
    {
        NVTX_RANGE("B3/stride1");
        ms_s1 = bench_ms([&] {
            access_stride_1<<<grid, block>>>(d_out, GRID_SZ);
        });
        // [B3-T32]
        std::printf("[B3] stride-1  time: %.3f ms/run\n", ms_s1);
    }

    // [B3-T33] -- test stride-32 (full conflict) --
    float ms_s32;
    {
        NVTX_RANGE("B3/stride32");
        ms_s32 = bench_ms([&] {
            access_stride_32<<<grid, block>>>(d_out, GRID_SZ);
        });
        // [B3-T34]
        std::printf("[B3] stride-32 time: %.3f ms/run\n", ms_s32);
    }

    // [B3-T35] -- test padding (conflict eliminated) --
    float ms_pad;
    {
        NVTX_RANGE("B3/padding");
        ms_pad = bench_ms([&] {
            access_with_padding<<<grid, block>>>(d_out, GRID_SZ);
        });
        // [B3-T36]
        std::printf("[B3] padding   time: %.3f ms/run\n", ms_pad);
    }

    // [B3-T37]
    std::printf("[B3] stride-32 / stride-1 ratio: %.1fx (expected ~30x)\n",
                ms_s32 / (ms_s1 + 1e-6f));
    // [B3-T38]
    std::printf("[B3] padding   / stride-1 ratio: %.1fx (expected ~1x)\n",
                ms_pad  / (ms_s1 + 1e-6f));

    // [B3-T39] -- Nsight Compute hint --
    std::puts("------------------------------------------------------------");
    // [B3-T40]
    std::puts("  TODO [REQUIRED] step 4/6: run Nsight Compute:");
    std::puts("    ncu --set full -o b3.ncu-rep ./B3_bank_conflict_and_swizzle.exe");
    // [B3-T41]
    std::puts("  Inspect shared_ld_bank_conflict / shared_st_bank_conflict metrics.");
    std::puts("------------------------------------------------------------");

    CUDA_CHECK(cudaFree(d_out));
    std::puts("[B3_bank_conflict_and_swizzle] DONE");
    return 0;
}
