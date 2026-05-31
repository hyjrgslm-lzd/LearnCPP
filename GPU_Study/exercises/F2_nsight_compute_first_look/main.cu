// ============================================================
// [F2-T01] Exercise F2: Nsight Compute and kernel-level metrics
// [F2-T02] Goals:
//   - memory-bound copy kernel: high DRAM bandwidth, low compute intensity
//   - compute-bound naive 128x128 matmul: high FMA density
//   - ncu --set full -o report; read Speed-Of-Light page (Compute% / Memory%)
//   - Memory Workload Analysis: L1/L2/DRAM transactions, coalescing efficiency
//   - Source-SASS line correlation (requires GPU_STUDY_LINEINFO=ON)
// [F2-T03] Build:  cmake --build build --target F2_nsight_compute_first_look
// [F2-T04] Capture: ncu --set full -o f2_report ./F2_nsight_compute_first_look
// [F2-T05] Run:    ./F2_nsight_compute_first_look
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [F2-T06] Constants
// ------------------------------------------------------------
constexpr int N_COPY    = 1 << 24;   // [F2-T07] 16M elements, 64 MB (copy kernel data volume)
constexpr int MAT_N     = 128;       // [F2-T08] matrix size 128x128
constexpr int TILE      = 16;        // [F2-T09] matmul tile size
constexpr int BLOCK     = 256;       // [F2-T10] copy kernel block size

// ------------------------------------------------------------
// [F2-T11] Kernel 1: memory-bound copy (bandwidth limited)
//   arithmetic intensity = 2 ops / 8 bytes = 0.25 FLOP/byte
//   expected: Speed-Of-Light Memory% high, Compute% low
// [F2-T12] TODO [REQUIRED-1] complete kernel body: out[tid] = in[tid]
//   (with simple arithmetic to preserve compute character)
// ------------------------------------------------------------
__global__ void kernel_copy_memory_bound(
    const float* __restrict__ in,
    float* __restrict__ out,
    int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [F2-T13] TODO [REQUIRED-1]:
    //   out[tid] = in[tid] * 1.0f + 0.0f;
    //   keep memory-bound character: 2 ops/element, 8 bytes accessed
    out[tid] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F2-T14] Kernel 2: compute-bound naive matmul (128x128)
//   arithmetic intensity = 2*N^3 / (3*N^2*4 bytes) = 2N/12 ~ 21 FLOP/byte (high)
//   expected: Speed-Of-Light Compute% high, Memory% low
//   note: naive (no tiling) clearly exhibits compute-bound character
// [F2-T15] TODO [REQUIRED-1] complete kernel body: C[row][col] = sum(A[row][k] * B[k][col])
// ------------------------------------------------------------
__global__ void kernel_matmul_naive(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float* __restrict__ C,
    int M)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= M || col >= M) return;

    float acc = 0.0f;
    // [F2-T16] TODO [REQUIRED-1]: unroll inner-product loop
    //   for (int k = 0; k < M; ++k) acc += A[row * M + k] * B[k * M + col];
    //   C[row * M + col] = acc;
    C[row * M + col] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F2-T17] Helper: print arithmetic intensity estimate
//   AI = ops / bytes_accessed
// ------------------------------------------------------------
static void print_arithmetic_intensity(
    const char* kernel_name,
    double ops,
    double bytes_accessed)
{
    printf("  %-30s  AI = %.2f FLOP/B  (%.0f MFLOP / %.0f MB)\n",
           kernel_name,
           ops / bytes_accessed,
           ops / 1e6,
           bytes_accessed / 1e6);
}

// ------------------------------------------------------------
// [F2-T18] main
// ------------------------------------------------------------
int main()
{
    std::puts("[F2_nsight_compute_first_look]");
    print_device_info(0);

    NVTX_RANGE("F2/main");

    // ------------------------------------------------------------
    // [F2-T19] Allocate memory
    // ------------------------------------------------------------
    const size_t bytes_copy = static_cast<size_t>(N_COPY) * sizeof(float);
    const size_t bytes_mat  = static_cast<size_t>(MAT_N) * MAT_N * sizeof(float);

    float *d_copy_in  = nullptr, *d_copy_out = nullptr;
    float *d_A = nullptr, *d_B = nullptr, *d_C = nullptr;

    CUDA_CHECK(cudaMalloc(&d_copy_in,  bytes_copy));
    CUDA_CHECK(cudaMalloc(&d_copy_out, bytes_copy));
    CUDA_CHECK(cudaMalloc(&d_A, bytes_mat));
    CUDA_CHECK(cudaMalloc(&d_B, bytes_mat));
    CUDA_CHECK(cudaMalloc(&d_C, bytes_mat));

    // [F2-T20] initialize device memory
    CUDA_CHECK(cudaMemset(d_copy_in,  0, bytes_copy));
    CUDA_CHECK(cudaMemset(d_A, 0, bytes_mat));
    CUDA_CHECK(cudaMemset(d_B, 0, bytes_mat));

    // [F2-T21] initialize matrix on host (diag = 1/MAT_N for numeric stability)
    {
        float* h = new float[MAT_N * MAT_N];
        for (int i = 0; i < MAT_N * MAT_N; ++i) h[i] = 1.0f / MAT_N;
        CUDA_CHECK(cudaMemcpy(d_A, h, bytes_mat, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_B, h, bytes_mat, cudaMemcpyHostToDevice));
        delete[] h;

        float* hv = new float[N_COPY];
        for (int i = 0; i < N_COPY; ++i) hv[i] = static_cast<float>(i % 1024) / 1024.0f;
        CUDA_CHECK(cudaMemcpy(d_copy_in, hv, bytes_copy, cudaMemcpyHostToDevice));
        delete[] hv;
    }

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [F2-T22] Theoretical AI estimate (verify in Nsight Compute)
    // ------------------------------------------------------------
    printf("\n--- Theoretical arithmetic intensity (AI) estimate ---\n");
    print_arithmetic_intensity(
        "kernel_copy_memory_bound",
        static_cast<double>(N_COPY) * 2,                  // [F2-T23] 1 mul + 1 add = 2 ops
        static_cast<double>(N_COPY) * 2 * sizeof(float)); // [F2-T24] 1 read + 1 write
    print_arithmetic_intensity(
        "kernel_matmul_naive",
        static_cast<double>(MAT_N) * MAT_N * MAT_N * 2,   // [F2-T25] 2 FLOP per FMA
        static_cast<double>(MAT_N) * MAT_N * 3 * sizeof(float)); // [F2-T26] A+B+C (naive: no cache reuse)

    // ------------------------------------------------------------
    // [F2-T27] Run Kernel 1: memory-bound copy
    // ------------------------------------------------------------
    printf("\n--- Kernel 1: memory-bound copy (ncu expected: Memory%% high) ---\n");
    {
        NVTX_RANGE_COLOR("F2/kernel_copy", 0xFF4080FF);
        int grid = (N_COPY + BLOCK - 1) / BLOCK;
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        timer.start();
        kernel_copy_memory_bound<<<grid, BLOCK>>>(d_copy_in, d_copy_out, N_COPY);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        float ms = timer.elapsed_ms();
        float bw = static_cast<float>(bytes_copy) * 2.0f / (ms * 1e6f); // GB/s
        printf("  time=%.3f ms  BW=%.1f GB/s\n", ms, bw);
        // [F2-T28] TODO [REQUIRED-2] Nsight Compute: Memory% ~ achieved BW / theoretical peak x100
        // [F2-T29] TODO [REQUIRED-2] record L1/L2/DRAM transactions (Memory Workload Analysis)
    }

    // ------------------------------------------------------------
    // [F2-T30] Run Kernel 2: compute-bound naive matmul
    // ------------------------------------------------------------
    printf("\n--- Kernel 2: compute-bound naive matmul %dx%d (ncu expected: Compute%% high) ---\n",
           MAT_N, MAT_N);
    {
        NVTX_RANGE_COLOR("F2/kernel_matmul", 0xFF40FF40);
        dim3 block(TILE, TILE);
        dim3 grid((MAT_N + TILE - 1) / TILE, (MAT_N + TILE - 1) / TILE);
        printf("  launch: grid=(%d,%d) block=(%d,%d)\n",
               grid.x, grid.y, block.x, block.y);
        CUDA_CHECK(cudaMemset(d_C, 0, bytes_mat));
        timer.start();
        kernel_matmul_naive<<<grid, block>>>(d_A, d_B, d_C, MAT_N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        float ms   = timer.elapsed_ms();
        double ops = static_cast<double>(MAT_N) * MAT_N * MAT_N * 2;
        printf("  time=%.3f ms  GFLOP/s=%.1f (stub: TODO fill correct value when complete)\n",
               ms, static_cast<float>(ops) / (ms * 1e6f));
        // [F2-T31] TODO [REQUIRED-3] Nsight Compute: Compute% ~ achieved FMA tput / theoretical peak x100
        // [F2-T32] TODO [REQUIRED-3] navigate to Source-SASS, correlate inner-product line
    }

    // ------------------------------------------------------------
    // [F2-T33] TODO [REQUIRED-4] capture with ncu --set full -o f2_report,
    //   then in GUI:
    //   - open Speed Of Light, record Compute% and Memory%
    //   - judge bottleneck class (compute-bound / memory-bound)
    //   - Memory Workload Analysis: record L1/L2/DRAM read/write transactions
    //   Record findings as comments:
    //   // kernel_copy: Compute%=??? Memory%=???  => memory-bound
    //   // kernel_matmul: Compute%=??? Memory%=??? => compute-bound

    // [F2-T34] TODO [REQUIRED-5] rebuild with GPU_STUDY_LINEINFO=ON, re-sample,
    //   verify Source-SASS clearly correlates inner-product source lines

    // [F2-T35] TODO [ADVANCED-1] use ncu --set speedoflight to quickly compare bottleneck classes
    // [F2-T36] TODO [ADVANCED-2] add shared memory tiling to matmul, compare AI and Memory% changes
    // [F2-T37] TODO [ADVANCED-3] use Nsight Compute rule feature, view automatic optimization advice

    // ------------------------------------------------------------
    // [F2-T38] Cleanup
    // ------------------------------------------------------------
    CUDA_CHECK(cudaFree(d_copy_in));
    CUDA_CHECK(cudaFree(d_copy_out));
    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));

    // [F2-T39]
    printf("\n[F2] done. ncu --set full -o f2_report and open report in Nsight Compute GUI.\n");
    return 0;
}
