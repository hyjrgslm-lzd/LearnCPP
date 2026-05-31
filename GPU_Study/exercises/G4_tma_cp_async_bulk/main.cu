// G4_tma_cp_async_bulk/main.cu
// [G4-T01] Exercise G4: tma_cp_async_bulk - Hopper TMA async bulk data transfer
//
// [G4-T02] Learning goals:
//   - host-side CUtensorMap construction (cuTensorMapEncodeTiled)
//   - kernel-side __grid_constant__ CUtensorMap parameter passing
//   - ptx::cp_async_bulk_tensor (or equivalent inline PTX) global -> shared
//   - cuda::barrier + barrier_arrive_tx tracks transaction byte count
//   - TMA swizzle 128B eliminates bank conflicts
//
// [G4-T03] Build: cmake --build build --target G4_tma_cp_async_bulk
// [G4-T04] Run:   ./G4_tma_cp_async_bulk
// [G4-T05] HW:    sm_90a (Hopper required)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
// [G4-T06] Driver API (CUtensorMap / cuTensorMapEncodeTiled)
#include <cuda.h>
// [G4-T07] libcu++ barrier
#include <cuda/barrier>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [G4-T08] Matrix size and tile parameters
// ------------------------------------------------------------
constexpr int  MAT_ROWS = 128;
constexpr int  MAT_COLS = 128;
constexpr int  TILE_ROWS = 32;
constexpr int  TILE_COLS = 32;
constexpr int  BLOCK_DIM = 128;

// ------------------------------------------------------------
// [G4-T09] __grid_constant__ tensor map (global scope, kernel reads via const ptr)
// NOTE: __grid_constant__ must be declared at global scope
// ------------------------------------------------------------
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
// [G4-T10] Compile-time declaration (host fills value before passing as kernel param)
#endif

// ------------------------------------------------------------
// [G4-T11] Hopper TMA kernel (sm_90a only device code)
// ------------------------------------------------------------
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [G4-T12] TODO [REQUIRED] step 3: TMA cp_async_bulk wrapper (stub)
//
// Real call:
//   cuda::device::memcpy_async_bulk(smem_dst, global_src, byte_count, barrier);
// or inline PTX:
//   cp.async.bulk.tensor.2d.shared::cluster.global.tile.mbarrier::complete_tx::bytes
//     [smem_addr], [tensor_map, {col, row}], [mbar_addr];
//
// [G4-T13] This is a compile-safe stub: element-wise copy.
// ------------------------------------------------------------
__device__ __forceinline__ void tma_cp_async_bulk_stub(
    __half* __restrict__       smem_dst,
    const __half* __restrict__ global_src,
    int                        tile_rows,
    int                        tile_cols,
    int                        global_cols)
{
    // [G4-T14] TODO [REQUIRED] step 3: replace with real TMA cp.async.bulk
    // Real version needs CUtensorMap descriptor and mbarrier transaction count
    int tid = threadIdx.x;
    for (int e = tid; e < tile_rows * tile_cols; e += blockDim.x) {
        int r = e / tile_cols;
        int c = e % tile_cols;
        smem_dst[r * tile_cols + c] = global_src[r * global_cols + c]; // stub
    }
}

// ------------------------------------------------------------
// [G4-T15] Kernel 1: TMA transfer demo (no swizzle, stub)
//
// Each block handles one TILE_ROWS x TILE_COLS matrix tile.
// ------------------------------------------------------------
__global__ void tma_load_kernel(
    const __half* __restrict__ d_src,      // global memory source [MAT_ROWS, MAT_COLS]
    __half*       __restrict__ d_dst,      // global memory destination (validation)
    int mat_rows, int mat_cols,
    int tile_rows, int tile_cols)
{
    int tile_row = blockIdx.y;
    int tile_col = blockIdx.x;
    int tid      = threadIdx.x;

    // ------------------------------------------------------------
    // [G4-T16] shared memory: holds one tile
    // ------------------------------------------------------------
    __shared__ __half smem[TILE_ROWS * TILE_COLS];

    // ------------------------------------------------------------
    // [G4-T17] TODO [REQUIRED] step 4: mbarrier init
    //   __shared__ cuda::barrier<cuda::thread_scope_block> bar;
    //   if (tid == 0) {
    //       init(&bar, BLOCK_DIM);
    //       // After TMA done, notify bar: barrier_arrive_tx tracks bytes
    //       cuda::device::barrier_arrive_tx(bar, 1, tile_rows * tile_cols * sizeof(__half));
    //   }
    //   __syncthreads();
    // ------------------------------------------------------------
    // [G4-T18] (stub uses __syncthreads as substitute for bar.wait)

    // ------------------------------------------------------------
    // [G4-T19] TODO [REQUIRED] step 3: launch TMA transfer (leader thread, tid == 0)
    // ------------------------------------------------------------
    if (tid == 0) {
        const __half* tile_src = d_src
            + tile_row * tile_rows * mat_cols
            + tile_col * tile_cols;
        // [G4-T20] stub: direct copy
        tma_cp_async_bulk_stub(smem, tile_src, tile_rows, tile_cols, mat_cols);
    }
    __syncthreads(); // [G4-T21] stub substitute for bar.wait(...)

    // ------------------------------------------------------------
    // [G4-T22] TODO [REQUIRED] step 4: consumer waits on barrier
    //   auto token = bar.arrive();
    //   bar.wait(std::move(token));
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [G4-T23] Validation: write smem content back to global memory
    // ------------------------------------------------------------
    for (int e = tid; e < tile_rows * tile_cols; e += BLOCK_DIM) {
        int r = e / tile_cols;
        int c = e % tile_cols;
        int gRow = tile_row * tile_rows + r;
        int gCol = tile_col * tile_cols + c;
        if (gRow < mat_rows && gCol < mat_cols)
            d_dst[gRow * mat_cols + gCol] = smem[e];
    }
}

// ------------------------------------------------------------
// [G4-T24] Kernel 2: TMA swizzle 128B demo (stub)
// TODO [REQUIRED] step 5: observe bank-conflict difference with/without swizzle
// ------------------------------------------------------------
__global__ void tma_swizzle_kernel(
    const __half* __restrict__ d_src,
    __half*       __restrict__ d_dst,
    int mat_rows, int mat_cols,
    int tile_rows, int tile_cols)
{
    // [G4-T25] TODO [REQUIRED] step 5: tensor map built with CU_TENSOR_MAP_SWIZZLE_128B
    // stub: same behavior as tma_load_kernel; only difference is tensor map argument
    int tile_row = blockIdx.y;
    int tile_col = blockIdx.x;
    int tid      = threadIdx.x;

    __shared__ __half smem[TILE_ROWS * TILE_COLS];

    if (tid == 0) {
        const __half* tile_src = d_src
            + tile_row * tile_rows * mat_cols
            + tile_col * tile_cols;
        tma_cp_async_bulk_stub(smem, tile_src, tile_rows, tile_cols, mat_cols);
    }
    __syncthreads();

    for (int e = tid; e < tile_rows * tile_cols; e += BLOCK_DIM) {
        int r = e / tile_cols;
        int c = e % tile_cols;
        int gRow = tile_row * tile_rows + r;
        int gCol = tile_col * tile_cols + c;
        if (gRow < mat_rows && gCol < mat_cols)
            d_dst[gRow * mat_cols + gCol] = smem[e];
    }
}

#endif // __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [G4-T26] Host: CUtensorMap construction (stub wrapper)
//
// TODO [REQUIRED] step 1: fill cuTensorMapEncodeTiled parameters
//   - globalAddress  = device pointer (d_src)
//   - rank           = 2
//   - globalDim      = {MAT_COLS, MAT_ROWS}  (note TMA uses [col, row] order)
//   - globalStrides  = {sizeof(__half) * MAT_COLS} (only rank-1 strides)
//   - boxDim         = {TILE_COLS, TILE_ROWS}
//   - elementStrides = {1, 1}
//   - interleave     = NONE
//   - swizzle        = NONE (step 5 changes to 128B)
//   - l2Promotion    = NONE
//   - oobFill        = ZERO_FILL
// ------------------------------------------------------------
static CUtensorMap build_tensor_map_stub(
    const __half*    d_src,
    int              mat_rows,
    int              mat_cols,
    int              tile_rows,
    int              tile_cols,
    CUtensorMapSwizzle swizzle = CU_TENSOR_MAP_SWIZZLE_NONE)
{
    CUtensorMap tmap{};
    std::memset(&tmap, 0, sizeof(tmap));

    // [G4-T27] TODO [REQUIRED] step 1: call cuTensorMapEncodeTiled
    // CUresult res = cuTensorMapEncodeTiled(
    //     &tmap,
    //     CU_TENSOR_MAP_DATA_TYPE_FLOAT16,
    //     2,                              // rank
    //     (void*)d_src,
    //     (const cuuint64_t[]){(cuuint64_t)mat_cols, (cuuint64_t)mat_rows},
    //     (const cuuint64_t[]){(cuuint64_t)mat_cols * sizeof(__half)},
    //     (const cuuint32_t[]){(cuuint32_t)tile_cols, (cuuint32_t)tile_rows},
    //     (const cuuint32_t[]){1, 1},
    //     CU_TENSOR_MAP_INTERLEAVE_NONE,
    //     swizzle,
    //     CU_TENSOR_MAP_L2_PROMOTION_NONE,
    //     CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE
    // );
    // CU_CHECK(res);
    (void)d_src; (void)mat_rows; (void)mat_cols;
    (void)tile_rows; (void)tile_cols; (void)swizzle;
    // [G4-T28]
    std::puts("  [stub] build_tensor_map_stub: cuTensorMapEncodeTiled not invoked (TODO [REQUIRED] step 1)");
    return tmap; // return zero-initialized stub
}

// ------------------------------------------------------------
// [G4-T29] main
// ------------------------------------------------------------
int main()
{
    std::puts("[G4_tma_cp_async_bulk]");
    print_device_info(0);
    NVTX_RANGE("G4_tma_cp_async_bulk/main");

    // ------------------------------------------------------------
    // [G4-T30] Hopper feature detection
    // ------------------------------------------------------------
    if (!has_hopper_features()) {
        std::puts("requires sm_90a Hopper GPU; skipping kernel");
        return 0;
    }

    // ------------------------------------------------------------
    // [G4-T31] Allocate matrix
    // ------------------------------------------------------------
    const size_t sz = MAT_ROWS * MAT_COLS * sizeof(__half);
    __half* h_src = static_cast<__half*>(std::malloc(sz));
    __half* h_dst = static_cast<__half*>(std::malloc(sz));

    for (int i = 0; i < MAT_ROWS * MAT_COLS; ++i)
        h_src[i] = __float2half(static_cast<float>(i % 16) / 16.f);

    __half* d_src = nullptr;
    __half* d_dst = nullptr;
    CUDA_CHECK(cudaMalloc(&d_src, sz));
    CUDA_CHECK(cudaMalloc(&d_dst, sz));
    CUDA_CHECK(cudaMemcpy(d_src, h_src, sz, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_dst, 0, sz));

    // ------------------------------------------------------------
    // [G4-T32] TODO [REQUIRED] steps 1+2: build CUtensorMap and pass to kernel
    // ------------------------------------------------------------
    CUtensorMap tmap_none = build_tensor_map_stub(
        d_src, MAT_ROWS, MAT_COLS, TILE_ROWS, TILE_COLS,
        CU_TENSOR_MAP_SWIZZLE_NONE);
    CUtensorMap tmap_swizzle = build_tensor_map_stub(
        d_src, MAT_ROWS, MAT_COLS, TILE_ROWS, TILE_COLS,
        CU_TENSOR_MAP_SWIZZLE_128B);
    (void)tmap_none; (void)tmap_swizzle; // stub: kernel does not yet receive tensor map argument

    // ------------------------------------------------------------
    // [G4-T33] Kernel 1: TMA without swizzle (stub)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G4/tma_load");
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
        dim3 grid(MAT_COLS / TILE_COLS, MAT_ROWS / TILE_ROWS);
        dim3 block(BLOCK_DIM);
        // [G4-T34]
        std::printf("launch tma_load_kernel: grid=(%d,%d,1)  block=(%d,1,1)\n",
                    grid.x, grid.y, BLOCK_DIM);

        CudaEventTimer timer;
        timer.start();
        tma_load_kernel<<<grid, block>>>(
            d_src, d_dst, MAT_ROWS, MAT_COLS, TILE_ROWS, TILE_COLS);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_dst, d_dst, sz, cudaMemcpyDeviceToHost));
        // [G4-T35] Per-element verification
        int errors = 0;
        for (int i = 0; i < MAT_ROWS * MAT_COLS; ++i) {
            if (__half2float(h_dst[i]) != __half2float(h_src[i])) ++errors;
        }
        // [G4-T36]
        std::printf("[tma_load_kernel] %.3f ms  verify: %s (%d errors)\n\n",
                    timer.elapsed_ms(), errors == 0 ? "PASS" : "FAIL", errors);
#endif
    }

    // ------------------------------------------------------------
    // [G4-T37] Kernel 2: TMA swizzle 128B (stub)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G4/tma_swizzle");
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900
        CUDA_CHECK(cudaMemset(d_dst, 0, sz));
        dim3 grid(MAT_COLS / TILE_COLS, MAT_ROWS / TILE_ROWS);
        dim3 block(BLOCK_DIM);
        // [G4-T38]
        std::printf("launch tma_swizzle_kernel: grid=(%d,%d,1)  block=(%d,1,1)\n",
                    grid.x, grid.y, BLOCK_DIM);

        CudaEventTimer timer;
        timer.start();
        tma_swizzle_kernel<<<grid, block>>>(
            d_src, d_dst, MAT_ROWS, MAT_COLS, TILE_ROWS, TILE_COLS);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        // [G4-T39]
        std::printf("[tma_swizzle_kernel] %.3f ms  (stub - compare bank conflicts after TODO [REQUIRED] step 5)\n\n",
                    timer.elapsed_ms());
#endif
    }

    // ------------------------------------------------------------
    // [G4-T40] TODO [REQUIRED] step 6: Nsight Compute validation
    //   ncu --metrics l1tex__t_bytes.sum,l2_read_transactions
    //       ./G4_tma_cp_async_bulk
    //   Observe whether cp.async.bulk.tensor bypasses L1
    // ------------------------------------------------------------

    // [G4-T41] TODO [ADVANCED] three-stage pipeline: stage0 TMA load A; stage1 TMA B + compute A; stage2 compute B + TMA write back
    // [G4-T42] TODO [ADVANCED] multiple tensor maps (one each for A/B/C), validate grid_constant capacity
    // [G4-T43] TODO [ADVANCED] measure TMA transfer latency vs theoretical bandwidth

    CUDA_CHECK(cudaFree(d_src));
    CUDA_CHECK(cudaFree(d_dst));
    std::free(h_src);
    std::free(h_dst);

    // [G4-T44]
    std::puts("[G4] done. Use ncu --set full ./G4_tma_cp_async_bulk to inspect TMA transactions and L2 hit rate.");
    return 0;
}
