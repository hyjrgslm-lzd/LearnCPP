// ============================================================
// [F3-T01] Exercise F3: Roofline model and performance analysis
// [F3-T02] Goals:
//   - 3 contrasting kernels: K1 low AI (memory-bound) / K2 medium AI / K3 high AI (compute-bound)
//   - Plus latency-bound K4: atomic histogram
//   - Compute AI manually or extract from Nsight Compute for each kernel
//   - Plot 4 points on roofline chart, observe distance to roof
//   - Give optimization advice based on roofline position
// [F3-T03] Build:  cmake --build build --target F3_roofline_and_bottleneck
// [F3-T04] Capture: ncu --set full -o f3_report ./F3_roofline_and_bottleneck
// [F3-T05] Run:    ./F3_roofline_and_bottleneck
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [F3-T06] Constants
// ------------------------------------------------------------
constexpr int N        = 1 << 22;   // [F3-T07] 4M elements (K1 / K4)
constexpr int MAT_N    = 128;       // [F3-T08] matmul size (K2 / K3)
constexpr int TILE     = 16;
constexpr int BLOCK    = 256;
constexpr int HIST_BINS = 256;

// ------------------------------------------------------------
// [F3-T09] K1: low AI copy (memory-bound)
//   AI = 2 ops / 8 bytes = 0.25 FLOP/byte
//   expected: lands near memory slope on roofline
// [F3-T10] TODO [REQUIRED-1] complete kernel body
// ------------------------------------------------------------
__global__ void k1_copy_low_ai(
    const float* __restrict__ in,
    float* __restrict__ out,
    int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [F3-T11] TODO [REQUIRED-1]: out[tid] = in[tid] * 1.0f + 0.0f;
    // AI ~ 2 FLOP / 8 bytes = 0.25 FLOP/byte (memory-bound)
    out[tid] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F3-T12] K2: medium AI -- vector fused multiply-add (MAD chain)
//   each thread runs CHAIN_LEN FMA, reads/writes 2 floats
//   AI = 2*CHAIN_LEN ops / 8 bytes ~ 8 FLOP/byte (medium)
//   expected: lands near intersection of memory slope and compute roof
// [F3-T13] TODO [REQUIRED-1] complete kernel body
// ------------------------------------------------------------
constexpr int CHAIN_LEN = 32; // [F3-T14] AI = 2*32/8 = 8 FLOP/byte

__global__ void k2_fma_chain_medium_ai(
    const float* __restrict__ in,
    float* __restrict__ out,
    float alpha,
    int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    float val = in[tid];
    // [F3-T15] TODO [REQUIRED-1]: unroll FMA chain to artificially raise compute
    //   for (int i = 0; i < CHAIN_LEN; ++i) val = val * alpha + in[tid];
    //   out[tid] = val;
    // AI = 2*CHAIN_LEN / 8 bytes ~ 8 FLOP/byte
    out[tid] = val; // stub
}

// ------------------------------------------------------------
// [F3-T16] K3: high AI -- naive matmul 128x128 (compute-bound)
//   AI = 2*N^3 / (N^2*12 bytes) ~ 2*128/12 ~ 21 FLOP/byte (high)
//   expected: lands near compute roof
// [F3-T17] TODO [REQUIRED-1] complete kernel body
// ------------------------------------------------------------
__global__ void k3_matmul_high_ai(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float* __restrict__ C,
    int M)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= M || col >= M) return;

    float acc = 0.0f;
    // [F3-T18] TODO [REQUIRED-1]:
    //   for (int k = 0; k < M; ++k) acc += A[row * M + k] * B[k * M + col];
    //   C[row * M + col] = acc;
    // AI ~ 2*M / (3*4) = 2*128/12 ~ 21 FLOP/byte (compute-bound)
    C[row * M + col] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F3-T19] K4: latency-bound atomic histogram (random write)
//   AI ~ 1 FLOP / 4 bytes = 0.25 FLOP/byte (close to K1) but NOT BW-bound;
//   atomic contention + random L2 miss = latency-bound
//   expected: low AI and low throughput, sits below both roofs
// [F3-T20] TODO [REQUIRED-1] complete kernel body
// ------------------------------------------------------------
__global__ void k4_atomic_hist_latency(
    const int* __restrict__ in,
    int* hist,
    int n,
    int bins)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [F3-T21] TODO [REQUIRED-1]: atomicAdd(&hist[in[tid] % bins], 1);
    // random write address -> high L2 miss -> high latency -> latency-bound
    (void)hist; (void)bins; // stub -- remove when implementing
}

// ------------------------------------------------------------
// [F3-T22] Helper: print roofline parameters
// ------------------------------------------------------------
static void print_roofline_point(
    const char* kernel_name,
    double ops,
    double bytes_accessed,
    float elapsed_ms)
{
    double ai         = ops / bytes_accessed;          // FLOP/byte
    double throughput = ops / (elapsed_ms * 1e6);      // GFLOP/s
    double bandwidth  = bytes_accessed / (elapsed_ms * 1e6); // GB/s
    printf("  %-30s  AI=%.2f FLOP/B  Tput=%.1f GFLOP/s  BW=%.1f GB/s  t=%.3f ms\n",
           kernel_name, ai, throughput, bandwidth, elapsed_ms);
}

// ------------------------------------------------------------
// [F3-T23] main
// ------------------------------------------------------------
int main()
{
    std::puts("[F3_roofline_and_bottleneck]");
    print_device_info(0);

    NVTX_RANGE("F3/main");

    // ------------------------------------------------------------
    // [F3-T24] Allocate memory
    // ------------------------------------------------------------
    const size_t bytes_vec = static_cast<size_t>(N) * sizeof(float);
    const size_t bytes_mat = static_cast<size_t>(MAT_N) * MAT_N * sizeof(float);
    const size_t bytes_int = static_cast<size_t>(N) * sizeof(int);

    float *d_in = nullptr, *d_out = nullptr;
    float *d_A = nullptr, *d_B = nullptr, *d_C = nullptr;
    int   *d_hist_in = nullptr, *d_hist = nullptr;

    CUDA_CHECK(cudaMalloc(&d_in,      bytes_vec));
    CUDA_CHECK(cudaMalloc(&d_out,     bytes_vec));
    CUDA_CHECK(cudaMalloc(&d_A,       bytes_mat));
    CUDA_CHECK(cudaMalloc(&d_B,       bytes_mat));
    CUDA_CHECK(cudaMalloc(&d_C,       bytes_mat));
    CUDA_CHECK(cudaMalloc(&d_hist_in, bytes_int));
    CUDA_CHECK(cudaMalloc(&d_hist,    HIST_BINS * sizeof(int)));

    // [F3-T25] initialize
    {
        float* hv = new float[N];
        for (int i = 0; i < N; ++i) hv[i] = static_cast<float>(i % 1024) / 1024.0f;
        CUDA_CHECK(cudaMemcpy(d_in, hv, bytes_vec, cudaMemcpyHostToDevice));
        delete[] hv;

        float* hm = new float[MAT_N * MAT_N];
        for (int i = 0; i < MAT_N * MAT_N; ++i) hm[i] = 1.0f / MAT_N;
        CUDA_CHECK(cudaMemcpy(d_A, hm, bytes_mat, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_B, hm, bytes_mat, cudaMemcpyHostToDevice));
        delete[] hm;

        int* hi = new int[N];
        for (int i = 0; i < N; ++i) hi[i] = (i * 1234567) % HIST_BINS;
        CUDA_CHECK(cudaMemcpy(d_hist_in, hi, bytes_int, cudaMemcpyHostToDevice));
        delete[] hi;
    }

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [F3-T26] Theoretical AI estimate (for roofline reference points)
    // ------------------------------------------------------------
    printf("\n--- Roofline parameters: theoretical arithmetic intensity (AI) ---\n");
    printf("  %-30s  AI = 0.25 FLOP/B  (2 ops / 8 bytes)\n",
           "k1_copy_low_ai");
    printf("  %-30s  AI = %.1f FLOP/B  (2*%d ops / 8 bytes)\n",
           "k2_fma_chain_medium_ai", 2.0 * CHAIN_LEN / 8.0, CHAIN_LEN);
    printf("  %-30s  AI = %.1f FLOP/B  (2*%d^3 / %d^2*12 bytes)\n",
           "k3_matmul_high_ai",
           2.0 * MAT_N / 12.0, MAT_N, MAT_N);
    printf("  %-30s  AI = 0.25 FLOP/B  but latency-bound (atomic + random write)\n",
           "k4_atomic_hist_latency");

    // ------------------------------------------------------------
    // [F3-T27] Run K1: low AI copy (memory-bound)
    // ------------------------------------------------------------
    printf("\n--- K1: copy (memory-bound, AI~0.25) ---\n");
    {
        NVTX_RANGE_COLOR("F3/K1_copy", 0xFF4080FF);
        int grid = (N + BLOCK - 1) / BLOCK;
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        timer.start();
        k1_copy_low_ai<<<grid, BLOCK>>>(d_in, d_out, N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        print_roofline_point("k1_copy_low_ai",
            static_cast<double>(N) * 2,
            static_cast<double>(N) * 2 * sizeof(float),
            timer.elapsed_ms());
        // [F3-T28] TODO [REQUIRED-2] capture with ncu, fill measured AI and throughput
    }

    // ------------------------------------------------------------
    // [F3-T29] Run K2: medium AI FMA chain
    // ------------------------------------------------------------
    printf("\n--- K2: FMA chain (medium AI~8) ---\n");
    {
        NVTX_RANGE_COLOR("F3/K2_fma", 0xFF40FF40);
        int grid = (N + BLOCK - 1) / BLOCK;
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        timer.start();
        k2_fma_chain_medium_ai<<<grid, BLOCK>>>(d_in, d_out, 1.0001f, N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        print_roofline_point("k2_fma_chain_medium_ai",
            static_cast<double>(N) * 2 * CHAIN_LEN,
            static_cast<double>(N) * 2 * sizeof(float),
            timer.elapsed_ms());
        // [F3-T30] TODO [REQUIRED-2] record measured data
    }

    // ------------------------------------------------------------
    // [F3-T31] Run K3: high AI naive matmul (compute-bound)
    // ------------------------------------------------------------
    printf("\n--- K3: naive matmul %dx%d (compute-bound, AI~21) ---\n", MAT_N, MAT_N);
    {
        NVTX_RANGE_COLOR("F3/K3_matmul", 0xFFFF8040);
        dim3 block(TILE, TILE);
        dim3 grid((MAT_N + TILE - 1) / TILE, (MAT_N + TILE - 1) / TILE);
        printf("  launch: grid=(%d,%d) block=(%d,%d)\n",
               grid.x, grid.y, block.x, block.y);
        CUDA_CHECK(cudaMemset(d_C, 0, bytes_mat));
        timer.start();
        k3_matmul_high_ai<<<grid, block>>>(d_A, d_B, d_C, MAT_N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        print_roofline_point("k3_matmul_high_ai",
            static_cast<double>(MAT_N) * MAT_N * MAT_N * 2,
            static_cast<double>(MAT_N) * MAT_N * 3 * sizeof(float),
            timer.elapsed_ms());
        // [F3-T32] TODO [REQUIRED-2] record measured data
    }

    // ------------------------------------------------------------
    // [F3-T33] Run K4: latency-bound atomic histogram
    // ------------------------------------------------------------
    printf("\n--- K4: atomic hist (latency-bound, AI~0.25 but not BW-bound) ---\n");
    {
        NVTX_RANGE_COLOR("F3/K4_hist", 0xFFFF4040);
        CUDA_CHECK(cudaMemset(d_hist, 0, HIST_BINS * sizeof(int)));
        int grid = (N + BLOCK - 1) / BLOCK;
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        timer.start();
        k4_atomic_hist_latency<<<grid, BLOCK>>>(d_hist_in, d_hist, N, HIST_BINS);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        print_roofline_point("k4_atomic_hist_latency",
            static_cast<double>(N) * 1,
            static_cast<double>(N) * sizeof(int),
            timer.elapsed_ms());
        // [F3-T34] TODO [REQUIRED-2] record measured data; K4 AI close to K1 but tput notably lower => latency-bound
    }

    // ------------------------------------------------------------
    // [F3-T35] TODO [REQUIRED-3] manually plot the roofline (Python matplotlib or Excel):
    //   - x-axis: arithmetic intensity (FLOP/byte), log scale
    //   - y-axis: achieved throughput (GFLOP/s), log scale
    //   - draw two boundary lines: compute roof (horizontal) / memory roof (slope)
    //   - mark K1/K2/K3/K4 points on the chart
    //   - annotate each point's distance to the roof (optimization headroom)

    // [F3-T36] TODO [REQUIRED-4] for each kernel, give roofline position + optimization advice:
    //   // K1: memory-bound, near memory roof, vectorize/prefetch to push closer
    //   // K2: medium AI, between two lines, raise CHAIN_LEN to increase compute density
    //   // K3: compute-bound, use shared memory tiling to reduce global accesses
    //   // K4: latency-bound, roofline does not directly apply -- reduce atomic contention (per-block local hist)

    // [F3-T37] TODO [REQUIRED-5] verify gap between measured AI and theoretical AI from ncu (cache effect)

    // [F3-T38] TODO [ADVANCED-1] optimize K4 (per-block local histogram), re-measure roofline position
    // [F3-T39] TODO [ADVANCED-2] plot roofline on sm_80 vs sm_90a, compare roof heights
    // [F3-T40] TODO [ADVANCED-3] use Nsight Compute built-in roofline chart, compare to manual plot

    // ------------------------------------------------------------
    // [F3-T41] Cleanup
    // ------------------------------------------------------------
    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));
    CUDA_CHECK(cudaFree(d_hist_in));
    CUDA_CHECK(cudaFree(d_hist));

    // [F3-T42]
    printf("\n[F3] done. ncu --set full -o f3_report and plot the roofline.\n");
    return 0;
}
