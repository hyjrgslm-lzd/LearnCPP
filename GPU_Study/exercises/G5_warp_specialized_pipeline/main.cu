// G5_warp_specialized_pipeline/main.cu
// [G5-T01] Exercise G5: warp_specialized_pipeline - Hopper producer-consumer warp specialization pipeline
//
// [G5-T02] Learning goals:
//   - 1 producer warp (tid 0-31) dedicated to TMA transfers
//   - 7 consumer warps (tid 32-255) dedicated to wgmma compute
//   - 2-stage mbarrier double-buffer for full overlap of compute and transfer
//   - setmaxnreg assigns different register quotas to producer vs consumer
//   - Compare to plain wgmma version of G3
//
// [G5-T03] Build: cmake --build build --target G5_warp_specialized_pipeline
// [G5-T04] Run:   ./G5_warp_specialized_pipeline
// [G5-T05] HW:    sm_90a (Hopper required)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
// [G5-T06] libcu++ barrier
#include <cuda/barrier>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [G5-T07] Pipeline parameters
// ------------------------------------------------------------
constexpr int PIPE_BLOCK_DIM  = 256;  // 1 producer (warp 0) + 7 consumers (warps 1-7)
constexpr int PRODUCER_WARPS  = 1;
constexpr int CONSUMER_WARPS  = 7;
constexpr int NUM_STAGES      = 2;    // double-buffer

constexpr int TILE_M  = 64;
constexpr int TILE_N  = 128;
constexpr int TILE_K  = 16;

// ------------------------------------------------------------
// [G5-T08] Hopper-only device code (sm_90a)
// ------------------------------------------------------------
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [G5-T09] TODO [REQUIRED] step 3: producer warp TMA transfer stub
//   Real version calls cp.async.bulk.tensor + barrier_arrive_tx
// ------------------------------------------------------------
__device__ __forceinline__ void producer_tma_load_stub(
    __half*        smem_tile,       // destination smem buffer (stage A or B)
    const __half*  global_src,      // global memory source address
    int            tile_m,
    int            tile_k,
    int            global_cols)
{
    // [G5-T10] TODO [REQUIRED] step 3: replace with real TMA cp.async.bulk
    // Real version:
    //   cuda::device::memcpy_async_bulk(smem_tile, global_src,
    //       tile_m * tile_k * sizeof(__half), barrier[stage]);
    int lane = threadIdx.x % 32; // producer warp lane
    for (int e = lane; e < tile_m * tile_k; e += 32) {
        int r = e / tile_k;
        int c = e % tile_k;
        smem_tile[r * tile_k + c] = global_src[r * global_cols + c]; // stub
    }
}

// ------------------------------------------------------------
// [G5-T11] TODO [REQUIRED] step 4: consumer wgmma stub
//   Real version calls wgmma.mma_async.sync.aligned.m64n128k16
// ------------------------------------------------------------
__device__ __forceinline__ void consumer_wgmma_stub(
    float*         acc,         // consumer-held accumulator fragment
    const __half*  smemA,       // smem A tile
    const __half*  smemB,       // smem B tile
    int            n_acc)       // accumulator count per thread
{
    // [G5-T12] TODO [REQUIRED] step 4: replace with real wgmma.mma_async call
    // stub: keep acc unchanged (still all zeros)
    (void)smemA; (void)smemB;
    for (int i = 0; i < n_acc; ++i) acc[i] = 0.f; // placeholder
}

// ------------------------------------------------------------
// [G5-T13] Kernel: warp-specialized pipeline
//
// blockDim = (256, 1, 1)
//   threads 0-31   = producer warp (TMA transfer)
//   threads 32-255 = consumer warp group (wgmma compute, 7 warps = 224 threads)
//
// shared memory layout:
//   [stage 0] A: TILE_M x TILE_K  FP16
//   [stage 0] B: TILE_K x TILE_N  FP16
//   [stage 1] A: TILE_M x TILE_K  FP16
//   [stage 1] B: TILE_K x TILE_N  FP16
//   mbarrier[NUM_STAGES]
// ------------------------------------------------------------
__global__
// [G5-T14] TODO [REQUIRED] step 2: blockDim = 256; optionally add __cluster_dims__(1,1,1)
void warp_specialized_pipeline_kernel(
    const __half* __restrict__ A,   // [M, K] row-major
    const __half* __restrict__ B,   // [K, N] row-major
    float*        __restrict__ C,   // [M, N] row-major
    int M_total, int N_total, int K_total)
{
    // [G5-T15] Thread identity
    int tid      = threadIdx.x;
    int warp_id  = tid / 32;
    bool is_producer = (warp_id == 0);  // threads 0-31 are producer

    // [G5-T16] Output tile this block is responsible for
    int tile_row = blockIdx.y; // M direction
    int tile_col = blockIdx.x; // N direction

    // ------------------------------------------------------------
    // [G5-T17] shared memory allocation
    // ------------------------------------------------------------
    __shared__ __half smemA[NUM_STAGES][TILE_M * TILE_K];
    __shared__ __half smemB[NUM_STAGES][TILE_K * TILE_N];

    // ------------------------------------------------------------
    // [G5-T18] TODO [REQUIRED] step 5: mbarrier double-buffer declaration
    //   __shared__ cuda::barrier<cuda::thread_scope_block> mbar[NUM_STAGES];
    //   if (tid == 0) {
    //       for (int s = 0; s < NUM_STAGES; ++s)
    //           init(&mbar[s], PIPE_BLOCK_DIM);
    //   }
    //   __syncthreads();
    // ------------------------------------------------------------
    // [G5-T19] (stub uses __syncthreads as substitute for barrier)
    __syncthreads(); // init sync

    // ------------------------------------------------------------
    // [G5-T20] setmaxnreg assigns different regs to producer/consumer
    // ------------------------------------------------------------
    if (is_producer) {
        // [G5-T21] TODO [REQUIRED] step 3: producer reduces regs (no compute, fewer regs needed)
        // asm volatile("setmaxnreg.dec.sync.aligned.u32 40;");
    } else {
        // [G5-T22] TODO [REQUIRED] step 4: consumer increases regs (wgmma needs more accumulator regs)
        // asm volatile("setmaxnreg.inc.sync.aligned.u32 232;");
    }

    // ------------------------------------------------------------
    // [G5-T23] Per-consumer-thread accumulator (stub: 2 FP32)
    // ------------------------------------------------------------
    constexpr int ACC_PER_THREAD = 2;
    float acc[ACC_PER_THREAD] = {0.f, 0.f};

    int k_tiles = K_total / TILE_K;

    // ------------------------------------------------------------
    // [G5-T24] TODO [REQUIRED] steps 3 + 5: pipeline main loop
    //
    // Pseudocode:
    //   for k_tile in 0..k_tiles:
    //     stage = k_tile % NUM_STAGES
    //     if is_producer:
    //       producer_tma_load_stub(smemA[stage], ..., A tile for k_tile)
    //       producer_tma_load_stub(smemB[stage], ..., B tile for k_tile)
    //       barrier_arrive_tx(mbar[stage], byte_count_A + byte_count_B)
    //     else:
    //       mbar[stage].wait(...)  // wait for buffer ready
    //       consumer_wgmma_stub(acc, smemA[stage], smemB[stage], ...)
    //       mbar[(stage+1)%2].arrive()  // notify producer buffer consumed
    //
    // [G5-T25] Note: pipeline flush (last few stages) needs special handling
    // ------------------------------------------------------------
    for (int k_tile = 0; k_tile < k_tiles; ++k_tile) {
        int stage = k_tile % NUM_STAGES;

        if (is_producer) {
            // [G5-T26] producer: TMA transfer A tile
            const __half* A_tile_ptr = A
                + (tile_row * TILE_M) * K_total
                + k_tile * TILE_K;
            producer_tma_load_stub(smemA[stage], A_tile_ptr, TILE_M, TILE_K, K_total);

            // [G5-T27] producer: TMA transfer B tile
            const __half* B_tile_ptr = B
                + k_tile * TILE_K * N_total
                + tile_col * TILE_N;
            producer_tma_load_stub(smemB[stage], B_tile_ptr, TILE_K, TILE_N, N_total);

            // [G5-T28] TODO [REQUIRED] step 5: barrier_arrive_tx notify byte count
            // cuda::device::barrier_arrive_tx(mbar[stage], 1,
            //     (TILE_M * TILE_K + TILE_K * TILE_N) * sizeof(__half));
        } else {
            // [G5-T29] TODO [REQUIRED] step 5: consumer waits on barrier
            // mbar[stage].wait(...);
        }

        // [G5-T30] stub sync replaces barrier
        __syncthreads();

        if (!is_producer) {
            // [G5-T31] consumer: wgmma compute
            consumer_wgmma_stub(acc, smemA[stage], smemB[stage], ACC_PER_THREAD);
        }

        __syncthreads();
        // [G5-T32] TODO [REQUIRED] step 5: switch buffer (double-buffer stage swap)
    }

    // ------------------------------------------------------------
    // [G5-T33] TODO [REQUIRED] step 3: fence.proxy.async ensures wgmma results visible
    // asm volatile("fence.proxy.async;");
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [G5-T34] Write back C (non-producer warps write results; stub writes zeros)
    // TODO [REQUIRED] step 4: write acc back to correct addresses per wgmma output layout
    // ------------------------------------------------------------
    if (!is_producer) {
        int consumer_tid = tid - 32; // 0..223 (index within consumer warp group)
        for (int i = 0; i < ACC_PER_THREAD; ++i) {
            int out_row = tile_row * TILE_M + consumer_tid / (TILE_N / ACC_PER_THREAD);
            int out_col = tile_col * TILE_N
                        + (consumer_tid % (TILE_N / ACC_PER_THREAD)) * ACC_PER_THREAD + i;
            if (out_row < M_total && out_col < N_total)
                C[out_row * N_total + out_col] = acc[i]; // stub value 0
        }
    }
}

#endif // __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [G5-T35] CPU reference
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
// [G5-T36] main
// ------------------------------------------------------------
int main()
{
    std::puts("[G5_warp_specialized_pipeline]");
    print_device_info(0);
    NVTX_RANGE("G5_warp_specialized_pipeline/main");

    // ------------------------------------------------------------
    // [G5-T37] Hopper feature detection
    // ------------------------------------------------------------
    if (!has_hopper_features()) {
        std::puts("requires sm_90a Hopper GPU; skipping kernel");
        return 0;
    }

    // ------------------------------------------------------------
    // [G5-T38] Matrix size: M=128, N=256, K=64 (2x2 tile grid)
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
    // [G5-T39] warp_specialized_pipeline_kernel launch
    // grid = (N/TILE_N, M/TILE_M)  block = 256
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G5/warp_specialized_pipeline");
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
        dim3 grid(N / TILE_N, M / TILE_M);
        dim3 block(PIPE_BLOCK_DIM, 1, 1);
        // [G5-T40]
        std::printf("launch warp_specialized_pipeline_kernel: grid=(%d,%d,1)  block=(%d,1,1)\n",
                    grid.x, grid.y, PIPE_BLOCK_DIM);
        // [G5-T41]
        std::printf("  producer warp: tid 0-31   (%d warp)\n",  PRODUCER_WARPS);
        // [G5-T42]
        std::printf("  consumer warp: tid 32-255 (%d warps)\n", CONSUMER_WARPS);
        // [G5-T43]
        std::printf("  double-buffer stages: %d\n", NUM_STAGES);

        CudaEventTimer timer;
        timer.start();
        warp_specialized_pipeline_kernel<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_C, d_C, sC, cudaMemcpyDeviceToHost));
        // [G5-T44]
        std::printf("[warp_specialized_pipeline] %.3f ms  (stub - verify result after TODO [REQUIRED] steps 3-5)\n\n",
                    timer.elapsed_ms());
#endif
    }

    // ------------------------------------------------------------
    // [G5-T45] TODO [REQUIRED] step 6: Nsight Compute performance comparison
    //   Compare TFLOPS vs G3 wgmma version (target 30-50% improvement)
    //   ncu --metrics sm__cycles_active.avg,
    //               sm__pipe_tensor_op_hmma_cycles_active.avg
    //       ./G5_warp_specialized_pipeline
    //   Observe interleaving of producer/consumer warp timelines
    // ------------------------------------------------------------

    // [G5-T46] TODO [ADVANCED] triple/quad buffering for further compute/transfer overlap
    // [G5-T47] TODO [ADVANCED] try different tile sizes (32x32 / 64x64 / 128x128), observe buffer/regs/throughput trade-off
    // [G5-T48] TODO [ADVANCED] integrate Blackwell sm_100a MXFP8 MMA, validate producer-consumer pattern generality

    CUDA_CHECK(cudaFree(d_A)); CUDA_CHECK(cudaFree(d_B)); CUDA_CHECK(cudaFree(d_C));
    std::free(h_A); std::free(h_B); std::free(h_C); std::free(h_ref);

    // [G5-T49]
    std::puts("[G5] done. Use ncu --set full ./G5_warp_specialized_pipeline to observe producer/consumer warp scheduling.");
    return 0;
}
