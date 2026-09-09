// G3_hopper_wgmma/main.cu
// [G3-T01] Exercise G3: hopper_wgmma - Hopper-specific warp-group MMA async
//
// [G3-T02] Learning goals:
//   - Understand 128-thread warp group concept (4 warps cooperate on wgmma)
//   - wgmma.mma_async.sync.aligned async execution model
//   - setmaxnreg.inc / setmaxnreg.dec register reallocation demo
//   - FP16 input + FP32 accumulator; FP8 E4M3 extension (stub)
//   - cuda::barrier coordinates warp-group synchronization
//
// [G3-T03] Build: cmake --build build --target G3_hopper_wgmma
// [G3-T04] Run:   ./G3_hopper_wgmma
// [G3-T05] HW:    sm_90a (Hopper required)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>
#include <cuda_fp16.h>

// [G3-T06] cuda::barrier from libcu++
#include <cuda/barrier>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [G3-T07] Hopper wgmma device code: only compile under sm_90a
// ------------------------------------------------------------
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900

// [G3-T08] wgmma tile size (warp group level: 64x128x16, FP16->FP32)
constexpr int WGMMA_M = 64;
constexpr int WGMMA_N = 128;
constexpr int WGMMA_K = 16;

// [G3-T09] block = 128 threads (1 warp group = 4 warps)
constexpr int WGMMA_BLOCK_DIM = 128;

// ------------------------------------------------------------
// [G3-T10] TODO [REQUIRED] step 3: wgmma.mma_async PTX wrapper (stub)
//
// Actual instruction form:
//   wgmma.mma_async.sync.aligned.m64n128k16.f32.f16.f16
//     d{0..63},           // 64 FP32 output regs (per-thread holds 64/128 = 0.5,
//                            wgmma actually has each thread hold 2 f32)
//     descA,              // shared memory descriptor (A matrix)
//     descB,              // shared memory descriptor (B matrix)
//     1, 1, 0;            // scale-D, scale-A, trans-A
//
// [G3-T11] This is a compile-safe stub: writes zeros, no actual wgmma call
// ------------------------------------------------------------
__device__ __forceinline__ void wgmma_m64n128k16_fp16_stub(
    float* acc,   // per warp-group thread accumulator fragment (stub: zero)
    int    n_acc) // accumulator element count
{
    // [G3-T12] TODO [REQUIRED] step 3: replace with real wgmma.mma_async inline PTX
    // Example (requires smem descriptor, omitted here):
    // asm volatile(
    //     "wgmma.mma_async.sync.aligned.m64n128k16.f32.f16.f16 "
    //     "{%0,%1,...},"
    //     "%64, %65,"
    //     "1, 1, 0;"
    //     : ...
    // );
    for (int i = 0; i < n_acc; ++i) acc[i] = 0.0f; // placeholder
}

// ------------------------------------------------------------
// [G3-T13] TODO [REQUIRED] step 3: setmaxnreg demo
//   increase regs: asm("setmaxnreg.inc.sync.aligned.u32 %0;" :: "r"(232));
//   decrease regs: asm("setmaxnreg.dec.sync.aligned.u32 %0;" :: "r"(40));
// ------------------------------------------------------------
__device__ __forceinline__ void setmaxnreg_inc_demo(unsigned n)
{
    // [G3-T14] TODO [REQUIRED] step 3: uncomment to actually set
    // asm volatile("setmaxnreg.inc.sync.aligned.u32 %0;" :: "r"(n));
    (void)n; // stub
}

__device__ __forceinline__ void setmaxnreg_dec_demo(unsigned n)
{
    // [G3-T15] TODO [REQUIRED] step 3: uncomment to actually set
    // asm volatile("setmaxnreg.dec.sync.aligned.u32 %0;" :: "r"(n));
    (void)n; // stub
}

// ------------------------------------------------------------
// [G3-T16] Kernel: wgmma demo (blockDim = 128, 1 warp group)
//
// Each block computes one 64x128 tile of the output matrix.
// Outer loop is K direction.
// ------------------------------------------------------------
__global__
// [G3-T17] TODO [REQUIRED] step 1: keep __cluster_dims__ declaration (warm-up for G5)
// __cluster_dims__(1, 1, 1)
void wgmma_demo_kernel(
    const __half* __restrict__ A,   // [M, K]  row-major
    const __half* __restrict__ B,   // [K, N]  row-major
    float*        __restrict__ C,   // [M, N]  row-major
    int M, int N, int K)
{
    // [G3-T18] Each block handles output tile (tile_row, tile_col) of WGMMA_M x WGMMA_N
    int tile_row = blockIdx.y;
    int tile_col = blockIdx.x;

    int tid = threadIdx.x; // 0..127

    // ------------------------------------------------------------
    // [G3-T19] TODO [REQUIRED] step 3: setmaxnreg increase consumer regs
    //   wgmma consumer typically needs more regs (between 128-232)
    setmaxnreg_inc_demo(232u);

    // ------------------------------------------------------------
    // [G3-T20] TODO [REQUIRED] step 2: shared memory stages A/B tile
    //   Real wgmma needs smem descriptor; here we just allocate smem as placeholder
    // ------------------------------------------------------------
    __shared__ __half smemA[WGMMA_M * WGMMA_K];   // 64x16 FP16
    __shared__ __half smemB[WGMMA_K * WGMMA_N];   // 16x128 FP16

    // [G3-T21] Per-thread accumulator (stub: 2 FP32 = 1/128 of wgmma m64n128k16)
    constexpr int ACC_PER_THREAD = 2;
    float acc[ACC_PER_THREAD] = {0.f, 0.f};

    // ------------------------------------------------------------
    // [G3-T22] TODO [REQUIRED] step 4: mbarrier init (coordinate 128 threads in warp group)
    //   cuda::barrier<cuda::thread_scope_block> bar;
    //   if (tid == 0) init(&bar, WGMMA_BLOCK_DIM);
    //   __syncthreads();
    // ------------------------------------------------------------
    // [G3-T23] (stub uses __syncthreads as substitute)

    // ------------------------------------------------------------
    // [G3-T24] K-direction tile loop
    // ------------------------------------------------------------
    for (int k_tile = 0; k_tile < K / WGMMA_K; ++k_tile) {
        // [G3-T25] Cooperatively load A tile to smem
        // Each thread copies (WGMMA_M * WGMMA_K / 128) half elements
        int elems_A = WGMMA_M * WGMMA_K;
        for (int e = tid; e < elems_A; e += WGMMA_BLOCK_DIM) {
            int row = e / WGMMA_K;
            int col = e % WGMMA_K;
            int gRow = tile_row * WGMMA_M + row;
            int gCol = k_tile   * WGMMA_K + col;
            smemA[e] = (gRow < M && gCol < K) ? A[gRow * K + gCol] : __float2half(0.f);
        }
        // [G3-T26] Cooperatively load B tile to smem
        int elems_B = WGMMA_K * WGMMA_N;
        for (int e = tid; e < elems_B; e += WGMMA_BLOCK_DIM) {
            int row = e / WGMMA_N;
            int col = e % WGMMA_N;
            int gRow = k_tile   * WGMMA_K + row;
            int gCol = tile_col * WGMMA_N + col;
            smemB[e] = (gRow < K && gCol < N) ? B[gRow * N + gCol] : __float2half(0.f);
        }
        __syncthreads();

        // [G3-T27] TODO [REQUIRED] step 3: call wgmma.mma_async (stub)
        wgmma_m64n128k16_fp16_stub(acc, ACC_PER_THREAD);

        __syncthreads();
    }

    // [G3-T28] TODO [REQUIRED] step 3: fence.proxy.async ensures wgmma results visible globally
    // asm volatile("fence.proxy.async;");

    // ------------------------------------------------------------
    // [G3-T29] TODO [REQUIRED] step 3: setmaxnreg returns regs (demo)
    setmaxnreg_dec_demo(40u);

    // [G3-T30] Write back C (stub: zeros)
    // [G3-T31] TODO [REQUIRED] step 3: write acc back to correct addresses per wgmma output layout
    for (int i = 0; i < ACC_PER_THREAD; ++i) {
        int out_idx = (tile_row * WGMMA_M + tid / (WGMMA_N / ACC_PER_THREAD)) * N
                    + (tile_col * WGMMA_N + (tid % (WGMMA_N / ACC_PER_THREAD)) * ACC_PER_THREAD + i);
        if (out_idx < M * N) C[out_idx] = acc[i]; // stub zero
    }
}

// ------------------------------------------------------------
// [G3-T32] TODO [REQUIRED] step 5: FP8 E4M3 wgmma stub
//   wgmma.mma_async.sync.aligned.m64n128k32.f32.e4m3.e4m3
//   FP8 scale factor demo (per-tensor)
// ------------------------------------------------------------
__global__ void wgmma_fp8_stub_kernel(
    const void* __restrict__ A,
    const void* __restrict__ B,
    float*      __restrict__ C,
    int M, int N, int K)
{
    // [G3-T33] TODO [REQUIRED] step 5: FP8 wgmma implementation
    // Just write zeros as placeholder
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < M * N) C[idx] = 0.0f;
}

#endif // __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [G3-T34] CPU reference (FP16 -> FP32 accumulation, row-major x row-major)
// ------------------------------------------------------------
static void cpu_gemm_ref(
    const __half* A, const __half* B, float* C,
    int m, int n, int k)
{
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j) {
            float acc = 0.f;
            for (int l = 0; l < k; ++l)
                acc += __half2float(A[i*k+l]) * __half2float(B[l*n+j]);
            C[i*n+j] = acc;
        }
}

// ------------------------------------------------------------
// [G3-T35] main
// ------------------------------------------------------------
int main()
{
    std::puts("[G3_hopper_wgmma]");
    print_device_info(0);
    NVTX_RANGE("G3_hopper_wgmma/main");

    // ------------------------------------------------------------
    // [G3-T36] Hopper feature detection: skip kernel launch on non-sm_90a HW
    // ------------------------------------------------------------
    if (!has_hopper_features()) {
        std::puts("requires sm_90a Hopper GPU; skipping kernel");
        return 0;
    }

    // ------------------------------------------------------------
    // [G3-T37] Matrix sizes: M=128, N=256, K=64 (4 tile x 2 tile grid)
    // ------------------------------------------------------------
    constexpr int M = 128, N = 256, K = 64;
    const size_t sA = M * K * sizeof(__half);
    const size_t sB = K * N * sizeof(__half);
    const size_t sC = M * N * sizeof(float);

    __half* h_A   = static_cast<__half*>(std::malloc(sA));
    __half* h_B   = static_cast<__half*>(std::malloc(sB));
    float*  h_C   = static_cast<float* >(std::malloc(sC));
    float*  h_ref = static_cast<float* >(std::malloc(sC));

    for (int i = 0; i < M*K; ++i) h_A[i] = __float2half((rand()%8)/8.f - 0.5f);
    for (int i = 0; i < K*N; ++i) h_B[i] = __float2half((rand()%8)/8.f - 0.5f);
    cpu_gemm_ref(h_A, h_B, h_ref, M, N, K);

    __half* d_A = nullptr; __half* d_B = nullptr; float* d_C = nullptr;
    CUDA_CHECK(cudaMalloc(&d_A, sA));
    CUDA_CHECK(cudaMalloc(&d_B, sB));
    CUDA_CHECK(cudaMalloc(&d_C, sC));
    CUDA_CHECK(cudaMemcpy(d_A, h_A, sA, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B, sB, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_C, 0, sC));

    // ------------------------------------------------------------
    // [G3-T38] wgmma_demo_kernel launch
    // grid = (N/WGMMA_N, M/WGMMA_M)  block = 128
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G3/wgmma_demo");
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
        dim3 grid(N / WGMMA_N, M / WGMMA_M);
        dim3 block(WGMMA_BLOCK_DIM, 1, 1);
        // [G3-T39]
        std::printf("launch wgmma_demo_kernel: grid=(%d,%d,1)  block=(%d,1,1)\n",
                    grid.x, grid.y, block.x);

        CudaEventTimer timer;
        timer.start();
        wgmma_demo_kernel<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_C, d_C, sC, cudaMemcpyDeviceToHost));
        // [G3-T40]
        std::printf("[wgmma_demo_kernel] %.3f ms  (stub - result is all zeros, verify after TODO [REQUIRED] step 3)\n\n",
                    timer.elapsed_ms());
#endif
    }

    // ------------------------------------------------------------
    // [G3-T41] TODO [REQUIRED] step 5: FP8 wgmma stub launch
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G3/wgmma_fp8_stub");
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
        dim3 grid2((M * N + 127) / 128);
        dim3 block2(128);
        // [G3-T42]
        std::printf("launch wgmma_fp8_stub_kernel: grid=(%d,1,1)  block=(128,1,1)\n", grid2.x);

        wgmma_fp8_stub_kernel<<<grid2, block2>>>(d_A, d_B, d_C, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        // [G3-T43]
        std::puts("[wgmma_fp8_stub_kernel] done  (stub - TODO [REQUIRED] step 5)\n");
#endif
    }

    // ------------------------------------------------------------
    // [G3-T44] TODO [REQUIRED] step 6: Nsight Compute performance measurement
    //   ncu --metrics sm__pipe_tensor_op_hmma_cycles_active.avg.pct_of_peak_sustained_active
    //       ./G3_hopper_wgmma
    // ------------------------------------------------------------

    // [G3-T45] TODO [ADVANCED] double buffer: warp group 0 loads tile0, warp group 1 computes tile-1
    // [G3-T46] TODO [ADVANCED] FP8 E5M2 vs E4M3 precision/throughput trade-off
    // [G3-T47] TODO [ADVANCED] preview new MMA forms on Blackwell sm_100a (MXFP8)

    CUDA_CHECK(cudaFree(d_A)); CUDA_CHECK(cudaFree(d_B)); CUDA_CHECK(cudaFree(d_C));
    std::free(h_A); std::free(h_B); std::free(h_C); std::free(h_ref);

    // [G3-T48]
    std::puts("[G3] done. Use ncu --set full ./G3_hopper_wgmma to inspect wgmma throughput and warp-group occupancy.");
    return 0;
}
