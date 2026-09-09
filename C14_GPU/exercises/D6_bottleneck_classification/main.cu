// D6_bottleneck_classification/main.cu
// [D6-T01] Exercise D6: Bottleneck classification
// [D6-T02] memory-bound / compute-bound / latency-bound
//
// [D6-T03] Learning goals:
// [D6-T04]   - 3 contrasting kernels: scan (memory) / matmul_tile (compute) / atomic_hist (latency)
// [D6-T05]   - Nsight Compute Speed-of-Light: SM / L1 / L2 / DRAM Throughput
// [D6-T06]   - roofline model: arithmetic intensity = ops / bytes
// [D6-T07]   - propose targeted optimization based on bottleneck classification
//
// Build: cmake --build build --target D6_bottleneck_classification
// Nsight Compute: ncu --set full -o d6_profile ./D6_bottleneck_classification
// Run: ./D6_bottleneck_classification

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D6-T08] Constants
// ------------------------------------------------------------
constexpr int N          = 1 << 22; // [D6-T09] 4M elements (scan / hist)
constexpr int TILE_DIM   = 16;      // [D6-T10] matmul tile size
constexpr int MAT_N      = 512;     // [D6-T11] matrix size (512x512)
constexpr int BLOCK_SIZE = 256;
constexpr int HIST_BINS  = 256;     // [D6-T12] histogram bins

// ------------------------------------------------------------
// [D6-T13] Kernel 1: memory-bound scan (approximate prefix sum)
// [D6-T14]   high DRAM / L2 bandwidth, very low arithmetic intensity
// [D6-T15]   arithmetic intensity ~= 2 ops / 8 bytes = 0.25 FLOP/byte
// [D6-T16] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void scan_memory_bound(const float* __restrict__ in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D6-T17] simple copy (simulating bandwidth-bound)
        out[tid] = in[tid] * 1.0f; // TODO: change to a more realistic scan
    }
}

// ------------------------------------------------------------
// [D6-T18] Kernel 2: compute-bound matmul tile (simplified)
// [D6-T19]   shared-memory tiling, dense FMA
// [D6-T20]   arithmetic intensity ~= TILE_DIM / 2 FLOP/byte (high)
// [D6-T21] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void matmul_tile_compute_bound(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float* C, int M)
{
    __shared__ float s_A[TILE_DIM][TILE_DIM];
    __shared__ float s_B[TILE_DIM][TILE_DIM];

    int row = blockIdx.y * TILE_DIM + threadIdx.y;
    int col = blockIdx.x * TILE_DIM + threadIdx.x;
    float acc = 0.0f;

    for (int t = 0; t < M / TILE_DIM; ++t) {
        // [D6-T22] load tile
        if (row < M && t * TILE_DIM + threadIdx.x < M)
            s_A[threadIdx.y][threadIdx.x] = A[row * M + t * TILE_DIM + threadIdx.x];
        else
            s_A[threadIdx.y][threadIdx.x] = 0.0f;

        if (col < M && t * TILE_DIM + threadIdx.y < M)
            s_B[threadIdx.y][threadIdx.x] = B[(t * TILE_DIM + threadIdx.y) * M + col];
        else
            s_B[threadIdx.y][threadIdx.x] = 0.0f;

        __syncthreads();

        // [D6-T23] TODO [REQUIRED-1] inner product over tile
        for (int k = 0; k < TILE_DIM; ++k) {
            // acc += s_A[threadIdx.y][k] * s_B[k][threadIdx.x]; // TODO
        }
        __syncthreads();
    }

    if (row < M && col < M) {
        C[row * M + col] = 0.0f; // [D6-T24] stub: replace with acc after fix
    }
}

// ------------------------------------------------------------
// [D6-T25] Kernel 3: latency-bound atomic histogram (irregular access)
// [D6-T26]   many global atomicAdd, random write addresses -> high L2/DRAM latency
// [D6-T27]   arithmetic intensity ~= 1 FLOP / 8 bytes (low), but latency-bound, not bw-bound
// [D6-T28] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void atomic_hist_latency_bound(const int* in, int* hist, int n, int bins)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        int bin = in[tid] % bins; // [D6-T29] random write -> high L2 miss rate
        // [D6-T30] TODO [REQUIRED-1]: atomicAdd(&hist[bin], 1);
    }
}

// ------------------------------------------------------------
// [D6-T31] Helper: print arithmetic intensity analysis
// ------------------------------------------------------------
static void print_roofline_hint(const char* name, float ops, float bytes_accessed)
{
    printf("  %-25s  AI=%.2f FLOP/B  (ops=%.0fM  bytes=%.0fMB)\n",
           name,
           ops / bytes_accessed,
           ops / 1e6f,
           bytes_accessed / 1e6f);
}

// ------------------------------------------------------------
// [D6-T32] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D6/main");

    // [D6-T33] allocate memory
    float *d_scan_in = nullptr, *d_scan_out = nullptr;
    float *d_A = nullptr, *d_B = nullptr, *d_C = nullptr;
    int   *d_hist_in = nullptr, *d_hist = nullptr;

    CUDA_CHECK(cudaMalloc(&d_scan_in,  N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_scan_out, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_A,   MAT_N * MAT_N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_B,   MAT_N * MAT_N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_C,   MAT_N * MAT_N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_hist_in, N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_hist,    HIST_BINS * sizeof(int)));

    // [D6-T34] initialize
    {
        float* h = new float[N];
        for (int i = 0; i < N; ++i) h[i] = static_cast<float>(i % 1024) / 1024.f;
        CUDA_CHECK(cudaMemcpy(d_scan_in, h, N * sizeof(float), cudaMemcpyHostToDevice));

        float* hmat = new float[MAT_N * MAT_N];
        for (int i = 0; i < MAT_N * MAT_N; ++i) hmat[i] = 1.0f / MAT_N;
        CUDA_CHECK(cudaMemcpy(d_A, hmat, MAT_N * MAT_N * sizeof(float), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_B, hmat, MAT_N * MAT_N * sizeof(float), cudaMemcpyHostToDevice));
        delete[] h;
        delete[] hmat;

        int* hi = new int[N];
        for (int i = 0; i < N; ++i) hi[i] = (i * 1234567) % HIST_BINS; // [D6-T35] pseudo-random
        CUDA_CHECK(cudaMemcpy(d_hist_in, hi, N * sizeof(int), cudaMemcpyHostToDevice));
        delete[] hi;
    }

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [D6-T36] Estimate arithmetic intensity
    // ------------------------------------------------------------
    // [D6-T37]
    printf("\n--- Roofline analysis (estimated arithmetic intensity) ---\n");
    print_roofline_hint("scan_memory_bound",
        N * 2.0f,                  // [D6-T38] 2 ops per element
        N * 2.0f * sizeof(float)); // [D6-T39] 1 read + 1 write
    print_roofline_hint("matmul_tile_compute_bound",
        (float)MAT_N * MAT_N * MAT_N * 2, // [D6-T40] 2 FLOP per FMA
        (float)MAT_N * MAT_N * 3 * sizeof(float)); // [D6-T41] A+B+C accesses (with tiling)
    print_roofline_hint("atomic_hist_latency_bound",
        N * 1.0f,                  // [D6-T42] 1 op per element
        N * 1.0f * sizeof(int));   // [D6-T43] 1 read per element

    // ------------------------------------------------------------
    // [D6-T44] Run Kernel 1: scan (memory-bound)
    // ------------------------------------------------------------
    // [D6-T45]
    printf("\n--- Kernel 1: scan (memory-bound) ---\n");
    {
        NVTX_RANGE("D6/scan");
        int grid = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;
        // [D6-T46]
        printf("  launch: grid=%d block=%d\n", grid, BLOCK_SIZE);
        timer.start();
        scan_memory_bound<<<grid, BLOCK_SIZE>>>(d_scan_in, d_scan_out, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();
        float bw = 2.0f * N * sizeof(float) / (ms * 1e6f); // GB/s
        // [D6-T47]
        printf("  time=%.3f ms  bandwidth=%.1f GB/s\n", ms, bw);
        // [D6-T48] TODO [REQUIRED-3] confirm DRAM Throughput near peak in Nsight Compute
    }

    // ------------------------------------------------------------
    // [D6-T49] Run Kernel 2: matmul tile (compute-bound)
    // ------------------------------------------------------------
    // [D6-T50]
    printf("\n--- Kernel 2: matmul_tile (compute-bound) ---\n");
    {
        NVTX_RANGE("D6/matmul");
        dim3 block(TILE_DIM, TILE_DIM);
        dim3 grid((MAT_N + TILE_DIM - 1) / TILE_DIM, (MAT_N + TILE_DIM - 1) / TILE_DIM);
        // [D6-T51]
        printf("  launch: grid=(%d,%d) block=(%d,%d)\n",
               grid.x, grid.y, block.x, block.y);
        CUDA_CHECK(cudaMemset(d_C, 0, MAT_N * MAT_N * sizeof(float)));
        timer.start();
        matmul_tile_compute_bound<<<grid, block>>>(d_A, d_B, d_C, MAT_N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms  = timer.elapsed_ms();
        float ops = (float)MAT_N * MAT_N * MAT_N * 2; // FMA
        // [D6-T52]
        printf("  time=%.3f ms  GFLOP/s=%.1f (stub: correct value after TODO)\n",
               ms, ops / (ms * 1e6f));
        // [D6-T53] TODO [REQUIRED-3] confirm SM Throughput near peak in Nsight Compute
    }

    // ------------------------------------------------------------
    // [D6-T54] Run Kernel 3: atomic histogram (latency-bound)
    // ------------------------------------------------------------
    // [D6-T55]
    printf("\n--- Kernel 3: atomic_hist (latency-bound) ---\n");
    {
        NVTX_RANGE("D6/atomic_hist");
        CUDA_CHECK(cudaMemset(d_hist, 0, HIST_BINS * sizeof(int)));
        int grid = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;
        // [D6-T56]
        printf("  launch: grid=%d block=%d\n", grid, BLOCK_SIZE);
        timer.start();
        atomic_hist_latency_bound<<<grid, BLOCK_SIZE>>>(d_hist_in, d_hist, N, HIST_BINS);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();
        // [D6-T57]
        printf("  time=%.3f ms  (latency-bound: high L2 miss and atomic contention)\n", ms);
        // [D6-T58] TODO [REQUIRED-3] confirm high pipeline stall in Nsight Compute
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    // ------------------------------------------------------------
    // [D6-T59] Bottleneck classification guide
    // ------------------------------------------------------------
    // [D6-T60]
    printf("\n--- Bottleneck classification (Speed-of-Light interpretation) ---\n");
    // [D6-T61]
    printf("  kernel          | expected bottleneck | Nsight metric\n");
    // [D6-T62]
    printf("  scan            | memory-bound        | DRAM Throughput ~ peak\n");
    // [D6-T63]
    printf("  matmul_tile     | compute-bound       | SM Throughput ~ peak\n");
    // [D6-T64]
    printf("  atomic_hist     | latency-bound       | high L2 miss, high pipeline stall\n");

    // ------------------------------------------------------------
    // [D6-T65] TODO [REQUIRED-4] propose one optimization per kernel (in comments)
    // [D6-T66] TODO [REQUIRED-5] mark the three kernels on a roofline plot
    // [D6-T67] TODO [REQUIRED-6] optimize atomic_hist (e.g. shared memory partial histogram), re-measure
    // [D6-T68] TODO [ADVANCED]   hybrid kernel (compute + memory), analyze complex bottleneck
    // [D6-T69] TODO [ADVANCED]   compare bottleneck class on sm_80 vs sm_90a for same kernel
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_scan_in));
    CUDA_CHECK(cudaFree(d_scan_out));
    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));
    CUDA_CHECK(cudaFree(d_hist_in));
    CUDA_CHECK(cudaFree(d_hist));

    // [D6-T70]
    printf("\n[D6] done. Use ncu --set full -o d6_profile, then analyze in Nsight Compute GUI.\n");
    return 0;
}
