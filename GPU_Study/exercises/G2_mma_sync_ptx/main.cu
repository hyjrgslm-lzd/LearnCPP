// G2_mma_sync_ptx/main.cu
// [G2-T01] Exercise G2: mma_sync_ptx - Ampere+ PTX mma.sync direct usage
//
// [G2-T02] Learning goals:
//   - Understand register layout for PTX mma.sync.aligned.m16n8k16
//   - Use inline PTX or <cuda/ptx> wrapper to call mma.sync (FP16 / BF16 / INT8)
//   - Pairing of ldmatrix with mma.sync
//   - Validate instruction generation via Nsight Compute PTX view
//
// [G2-T03] Build: cmake --build build --target G2_mma_sync_ptx
// [G2-T04] Run:   ./G2_mma_sync_ptx
// [G2-T05] HW:    sm_80+ (Ampere / Ada / Hopper)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda_bf16.h>
#include <mma.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [G2-T06] Constants for m16n8k16 matrix sizes
//   A: M*K = 16x16  (FP16, row-major)
//   B: K*N = 16x8   (FP16, col-major)
//   C: M*N = 16x8   (FP32)
// ------------------------------------------------------------
constexpr int WMMA_M = 16;
constexpr int WMMA_N =  8;
constexpr int WMMA_K = 16;

// ------------------------------------------------------------
// [G2-T07] Helper: FP16 CPU GEMM (row-major A x col-major B)
// ------------------------------------------------------------
static void cpu_mma_ref_fp16(
    const __half* A,     // [M, K]  row-major
    const __half* B,     // [K, N]  col-major (B[k,n] = B_ptr[n*K + k])
    float*        C,     // [M, N]
    int m, int n, int k)
{
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            float acc = 0.0f;
            for (int l = 0; l < k; ++l) {
                acc += __half2float(A[i * k + l]) * __half2float(B[j * k + l]);
            }
            C[i * n + j] = acc;
        }
    }
}

static void cpu_mma_ref_s8(
    const int8_t* A,  // [M, K] row-major
    const int8_t* B,  // [K, N] col-major
    int32_t*      C,  // [M, N]
    int m, int n, int k)
{
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            int32_t acc = 0;
            for (int l = 0; l < k; ++l) {
                acc += static_cast<int32_t>(A[i * k + l]) *
                       static_cast<int32_t>(B[j * k + l]);
            }
            C[i * n + j] = acc;
        }
    }
}

// ------------------------------------------------------------
// [G2-T08] TODO [REQUIRED] step 1: PTX mma.sync wrapper
//
// mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32
//
// [G2-T09] Register layout (per warp, per-thread fragment):
//   A: 8 .f16x2 registers (16 half = 1/32 of M16xK16)
//   B: 4 .f16x2 registers ( 8 half = 1/32 of K16xN8)
//   C: 4 .f32   registers ( 4 float = 1/32 of M16xN8)
// ------------------------------------------------------------
__device__ __forceinline__ void mma_m16n8k16_fp16(
    // [G2-T10] Output C (FP32, 4 registers/thread)
    float& d0, float& d1, float& d2, float& d3,
    // [G2-T11] Input A (FP16, 8 unsigneds = 8 .f16x2 regs)
    unsigned a0, unsigned a1, unsigned a2, unsigned a3,
    unsigned a4, unsigned a5, unsigned a6, unsigned a7,
    // [G2-T12] Input B (FP16, 4 unsigneds = 4 .f16x2 regs)
    unsigned b0, unsigned b1, unsigned b2, unsigned b3,
    // [G2-T13] Input C (FP32 accumulator initial value)
    float c0, float c1, float c2, float c3)
{
    // [G2-T14] TODO [REQUIRED] step 1: inline PTX call mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32
    // Format:
    //   mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32
    //     {%0,%1,%2,%3},          // D (output FP32 regs)
    //     {%4,%5,%6,%7,%8,%9,%10,%11},  // A (FP16 regs)
    //     {%12,%13,%14,%15},      // B (FP16 regs)
    //     {%16,%17,%18,%19};      // C (FP32 accumulator)
    asm volatile(
        "mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32 "
        "{%0,%1,%2,%3},"
        "{%4,%5,%6,%7,%8,%9,%10,%11},"
        "{%12,%13,%14,%15},"
        "{%16,%17,%18,%19};"
        : "=f"(d0), "=f"(d1), "=f"(d2), "=f"(d3)
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3),
          "r"(a4), "r"(a5), "r"(a6), "r"(a7),
          "r"(b0), "r"(b1), "r"(b2), "r"(b3),
          "f"(c0), "f"(c1), "f"(c2), "f"(c3)
    );
}

// ------------------------------------------------------------
// [G2-T15] TODO [REQUIRED] step 4: BF16 wrapper
// mma.sync.aligned.m16n8k16.row.col.f32.bf16.bf16.f32
// ------------------------------------------------------------
__device__ __forceinline__ void mma_m16n8k16_bf16(
    float& d0, float& d1, float& d2, float& d3,
    unsigned a0, unsigned a1, unsigned a2, unsigned a3,
    unsigned a4, unsigned a5, unsigned a6, unsigned a7,
    unsigned b0, unsigned b1, unsigned b2, unsigned b3,
    float c0, float c1, float c2, float c3)
{
    // [G2-T16] TODO [REQUIRED] step 4: replace .f16 with .bf16
    asm volatile(
        "mma.sync.aligned.m16n8k16.row.col.f32.bf16.bf16.f32 "
        "{%0,%1,%2,%3},"
        "{%4,%5,%6,%7,%8,%9,%10,%11},"
        "{%12,%13,%14,%15},"
        "{%16,%17,%18,%19};"
        : "=f"(d0), "=f"(d1), "=f"(d2), "=f"(d3)
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3),
          "r"(a4), "r"(a5), "r"(a6), "r"(a7),
          "r"(b0), "r"(b1), "r"(b2), "r"(b3),
          "f"(c0), "f"(c1), "f"(c2), "f"(c3)
    );
}

// ------------------------------------------------------------
// [G2-T17] TODO [REQUIRED] step 5: INT8 wrapper
// mma.sync.aligned.m16n8k16.row.col.s32.s8.s8.s32
// ------------------------------------------------------------
__device__ __forceinline__ void mma_m16n8k16_s8(
    int32_t& d0, int32_t& d1, int32_t& d2, int32_t& d3,
    unsigned a0, unsigned a1,               // A: 2 .b32 (each holds 4 s8)
    unsigned b0,                            // B: 1 .b32 (holds 4 s8)
    int32_t c0, int32_t c1, int32_t c2, int32_t c3)
{
    // [G2-T18] TODO [REQUIRED] step 5: INT8 mma.sync
    // Note: m16n8k16 s8 layout uses 2 regs for A, 1 reg for B
    asm volatile(
        "mma.sync.aligned.m16n8k16.row.col.s32.s8.s8.s32 "
        "{%0,%1,%2,%3},"
        "{%4,%5},"
        "{%6},"
        "{%7,%8,%9,%10};"
        : "=r"(d0), "=r"(d1), "=r"(d2), "=r"(d3)
        : "r"(a0), "r"(a1),
          "r"(b0),
          "r"(c0), "r"(c1), "r"(c2), "r"(c3)
    );
}

// ------------------------------------------------------------
// [G2-T19] Kernel: FP16 mma.sync (m16n8k16, single warp, single tile)
//
// Each thread holds 8 half of A, 4 half of B, 4 float of C.
// Use ldmatrix to load A/B from shared memory (smem address must be aligned).
// ------------------------------------------------------------
__global__ void mma_fp16_kernel(
    const __half* __restrict__ A,   // [16, 16] row-major
    const __half* __restrict__ B,   // [16,  8] col-major
    float*        __restrict__ C,   // [16,  8]
    int lda, int ldb, int ldc)
{
    // [G2-T20] TODO [REQUIRED] step 2: stage A/B into shared memory (ldmatrix needs smem)
    __shared__ __half smemA[WMMA_M * WMMA_K];   // 16x16 FP16
    __shared__ __half smemB[WMMA_K * WMMA_N];   // 16x8  FP16

    int tid = threadIdx.x;

    // [G2-T21] Cooperative load to smem (each thread copies 16/32 * 2 = 1 half4 chunk)
    // [G2-T22] TODO [REQUIRED] step 2: ldmatrix requires data in shared memory before loading
    if (tid < WMMA_M * WMMA_K) smemA[tid] = A[tid];
    if (tid < WMMA_K * WMMA_N) smemB[tid] = B[tid];
    __syncthreads();

    // ------------------------------------------------------------
    // [G2-T23] TODO [REQUIRED] step 3: ldmatrix loads A/B
    //   ldmatrix.sync.aligned.m8n8.x4.shared.b16 loads A (4 .b32)
    //   ldmatrix.sync.aligned.m8n8.x2.shared.b16 loads B (2 .b32)
    //   Note: thread t loads smem row = t/4 (A), or specific permutation
    // ------------------------------------------------------------
    unsigned a0, a1, a2, a3, a4, a5, a6, a7;
    unsigned b0, b1, b2, b3;

    {
        // [G2-T24] A: m16n8k16 -> each thread holds 8 half, ldmatrix x4
        // Each group of 8 threads shares one row (ldmatrix internal repack)
        uint32_t smemA_addr = __cvta_generic_to_shared(smemA) + (tid % 16) * WMMA_K * sizeof(__half);
        asm volatile(
            "ldmatrix.sync.aligned.m8n8.x4.shared.b16 {%0,%1,%2,%3}, [%4];"
            : "=r"(a0), "=r"(a1), "=r"(a2), "=r"(a3)
            : "r"(smemA_addr)
        );
        // [G2-T25] Upper half (K = 8..15)
        uint32_t smemA_hi_addr = __cvta_generic_to_shared(smemA) + (tid % 16) * WMMA_K * sizeof(__half) + 8 * sizeof(__half);
        asm volatile(
            "ldmatrix.sync.aligned.m8n8.x4.shared.b16 {%0,%1,%2,%3}, [%4];"
            : "=r"(a4), "=r"(a5), "=r"(a6), "=r"(a7)
            : "r"(smemA_hi_addr)
        );
    }

    {
        // [G2-T26] B: m16n8k16 -> each thread holds 4 half, ldmatrix x2
        uint32_t smemB_addr = __cvta_generic_to_shared(smemB) + (tid % 8) * WMMA_N * sizeof(__half);
        asm volatile(
            "ldmatrix.sync.aligned.m8n8.x2.trans.shared.b16 {%0,%1}, [%2];"
            : "=r"(b0), "=r"(b1)
            : "r"(smemB_addr)
        );
        uint32_t smemB_hi_addr = smemB_addr + 8 * WMMA_N * sizeof(__half);
        asm volatile(
            "ldmatrix.sync.aligned.m8n8.x2.trans.shared.b16 {%0,%1}, [%2];"
            : "=r"(b2), "=r"(b3)
            : "r"(smemB_hi_addr)
        );
    }

    // [G2-T27] Zero accumulator
    float d0 = 0.f, d1 = 0.f, d2 = 0.f, d3 = 0.f;

    // [G2-T28] TODO [REQUIRED] step 3: call FP16 mma.sync
    mma_m16n8k16_fp16(d0, d1, d2, d3,
                      a0, a1, a2, a3, a4, a5, a6, a7,
                      b0, b1, b2, b3,
                      d0, d1, d2, d3);

    // [G2-T29] Write back C (each thread writes 2 elements: row = tid/4, col = (tid%4)*2 + 0/1)
    // [G2-T30] TODO [REQUIRED] step 3: write back per mma.sync output register layout
    int row0 = tid / 4;
    int row1 = row0 + 8;
    int col  = (tid % 4) * 2;
    if (row0 < WMMA_M && col < WMMA_N) {
        C[row0 * ldc + col]     = d0;
        C[row0 * ldc + col + 1] = d1;
    }
    if (row1 < WMMA_M && col < WMMA_N) {
        C[row1 * ldc + col]     = d2;
        C[row1 * ldc + col + 1] = d3;
    }
}

// ------------------------------------------------------------
// [G2-T31] Kernel: INT8 mma.sync (stub)
// TODO [REQUIRED] step 5: INT8 version, just write zeros as placeholder
// ------------------------------------------------------------
__global__ void mma_s8_kernel(
    const int8_t* __restrict__ A,   // [16, 16] row-major
    const int8_t* __restrict__ B,   // [16,  8] col-major
    int32_t*      __restrict__ C,   // [16,  8]
    int lda, int ldb, int ldc)
{
    // [G2-T32] TODO [REQUIRED] step 5: load A/B (s8), call mma_m16n8k16_s8, write back C
    // stub: write zeros
    int tid = threadIdx.x;
    int row = tid / 4;
    int col = (tid % 4) * 2;
    if (row < WMMA_M && col < WMMA_N) {
        C[row * ldc + col]     = 0; // stub
        C[row * ldc + col + 1] = 0; // stub
    }
}

// ------------------------------------------------------------
// [G2-T33] main
// ------------------------------------------------------------
int main()
{
    std::puts("[G2_mma_sync_ptx]");
    print_device_info(0);
    NVTX_RANGE("G2_mma_sync_ptx/main");

    // ------------------------------------------------------------
    // [G2-T34] TODO [REQUIRED] step 2: build 16x16 (FP16) A and 16x8 (FP16) B
    //   A row-major, B col-major (mma.sync.row.col convention)
    // ------------------------------------------------------------
    const int szA = WMMA_M * WMMA_K;
    const int szB = WMMA_K * WMMA_N;
    const int szC = WMMA_M * WMMA_N;

    __half*  h_A    = static_cast<__half* >(std::malloc(szA * sizeof(__half)));
    __half*  h_B    = static_cast<__half* >(std::malloc(szB * sizeof(__half)));
    float*   h_C    = static_cast<float*  >(std::malloc(szC * sizeof(float)));
    float*   h_ref  = static_cast<float*  >(std::malloc(szC * sizeof(float)));
    int8_t*  h_As8  = static_cast<int8_t* >(std::malloc(szA * sizeof(int8_t)));
    int8_t*  h_Bs8  = static_cast<int8_t* >(std::malloc(szB * sizeof(int8_t)));
    int32_t* h_Cs32 = static_cast<int32_t*>(std::malloc(szC * sizeof(int32_t)));

    for (int i = 0; i < szA; ++i) { h_A[i]   = __float2half((rand() % 16) / 16.f - 0.5f); h_As8[i] = static_cast<int8_t>(rand() % 11 - 5); }
    for (int i = 0; i < szB; ++i) { h_B[i]   = __float2half((rand() % 16) / 16.f - 0.5f); h_Bs8[i] = static_cast<int8_t>(rand() % 11 - 5); }

    // [G2-T35] CPU reference
    cpu_mma_ref_fp16(h_A, h_B, h_ref, WMMA_M, WMMA_N, WMMA_K);

    __half*  d_A    = nullptr;
    __half*  d_B    = nullptr;
    float*   d_C    = nullptr;
    int8_t*  d_As8  = nullptr;
    int8_t*  d_Bs8  = nullptr;
    int32_t* d_Cs32 = nullptr;
    CUDA_CHECK(cudaMalloc(&d_A,    szA * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_B,    szB * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_C,    szC * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_As8,  szA * sizeof(int8_t)));
    CUDA_CHECK(cudaMalloc(&d_Bs8,  szB * sizeof(int8_t)));
    CUDA_CHECK(cudaMalloc(&d_Cs32, szC * sizeof(int32_t)));
    CUDA_CHECK(cudaMemcpy(d_A,   h_A,   szA * sizeof(__half),  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B,   h_B,   szB * sizeof(__half),  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_As8, h_As8, szA * sizeof(int8_t),  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_Bs8, h_Bs8, szB * sizeof(int8_t),  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_C,    0, szC * sizeof(float)));
    CUDA_CHECK(cudaMemset(d_Cs32, 0, szC * sizeof(int32_t)));

    // ------------------------------------------------------------
    // [G2-T36] Launch FP16 mma.sync kernel (single warp, single tile)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G2/mma_fp16");
        dim3 grid(1);
        dim3 block(32);
        // [G2-T37]
        std::printf("launch mma_fp16_kernel: grid=(1,1,1)  block=(32,1,1)\n");

        CudaEventTimer timer;
        timer.start();
        mma_fp16_kernel<<<grid, block>>>(d_A, d_B, d_C, WMMA_K, WMMA_N, WMMA_N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_C, d_C, szC * sizeof(float), cudaMemcpyDeviceToHost));
        // [G2-T38] Simple first-row verification
        bool ok = true;
        for (int j = 0; j < WMMA_N && ok; ++j) {
            if (std::fabsf(h_C[j] - h_ref[j]) > 0.05f * (std::fabsf(h_ref[j]) + 1e-4f)) ok = false;
        }
        // [G2-T39]
        std::printf("[mma_fp16_kernel] %.3f ms  first-row verify: %s\n\n",
                    timer.elapsed_ms(), ok ? "PASS" : "FAIL(stub or layout pending)");
    }

    // ------------------------------------------------------------
    // [G2-T40] Launch INT8 mma.sync kernel (stub)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G2/mma_s8");
        dim3 grid(1);
        dim3 block(32);
        // [G2-T41]
        std::printf("launch mma_s8_kernel: grid=(1,1,1)  block=(32,1,1)\n");

        CudaEventTimer timer;
        timer.start();
        mma_s8_kernel<<<grid, block>>>(d_As8, d_Bs8, d_Cs32, WMMA_K, WMMA_N, WMMA_N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        // [G2-T42]
        std::printf("[mma_s8_kernel]   %.3f ms  (stub - verify after TODO [REQUIRED] step 5)\n\n",
                    timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [G2-T43] TODO [REQUIRED] step 6: validate via Nsight Compute PTX view
    //   nvcc --keep --ptx G2_mma_sync_ptx/main.cu
    //   grep 'mma.sync' *.ptx
    // ------------------------------------------------------------

    // [G2-T44] TODO [ADVANCED] implement m16n8k32 (K=32), watch ldmatrix load count change
    // [G2-T45] TODO [ADVANCED] compare mma.sync vs nvcuda::wmma PTX, understand wrapper overhead
    // [G2-T46] TODO [ADVANCED] mixed precision: BF16 input + FP32 output, validate auto conversion

    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));
    CUDA_CHECK(cudaFree(d_As8));
    CUDA_CHECK(cudaFree(d_Bs8));
    CUDA_CHECK(cudaFree(d_Cs32));
    std::free(h_A); std::free(h_B); std::free(h_C); std::free(h_ref);
    std::free(h_As8); std::free(h_Bs8); std::free(h_Cs32);

    // [G2-T47]
    std::puts("\n[G2] done. Use nvcc --keep --ptx + grep 'mma.sync' to validate PTX generation.");
    return 0;
}
