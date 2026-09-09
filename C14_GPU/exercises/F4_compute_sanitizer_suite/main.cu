// ============================================================
// [F4-T01] Exercise F4: Compute Sanitizer suite and bug detection
// [F4-T02] Goals:
//   - 4 intentionally buggy kernels triggering each sanitizer class
//   - memcheck:   out-of-bounds write
//   - racecheck:  shared memory data race (concurrent write w/o sync)
//   - synccheck:  divergent __syncthreads (called inside if branch)
//   - initcheck:  reads uninitialized shared memory
//   - --bug=oob/race/sync/uninit selects which bug to run
// [F4-T03] Build:  cmake --build build --target F4_compute_sanitizer_suite
// [F4-T04] Detect (run separately):
//   compute-sanitizer --tool memcheck   ./F4_compute_sanitizer_suite --bug=oob
//   compute-sanitizer --tool racecheck  ./F4_compute_sanitizer_suite --bug=race
//   compute-sanitizer --tool synccheck  ./F4_compute_sanitizer_suite --bug=sync
//   compute-sanitizer --tool initcheck  ./F4_compute_sanitizer_suite --bug=uninit
// [F4-T05] Run normally (no args): triggers all four
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [F4-T06] Constants
// ------------------------------------------------------------
constexpr int N         = 1024;   // [F4-T07] data size (small, sanitizer reports quickly)
constexpr int BLOCK     = 128;
constexpr int OOB_EXTRA = 1024;   // [F4-T08] OOB offset (intentionally beyond allocation)

// ------------------------------------------------------------
// [F4-T09] INTENTIONAL BUG #1: out-of-bounds write (memcheck)
//
//   Each thread writes global_mem[tid + OOB_EXTRA], where tid < N but
//   the allocation is only N * sizeof(float). OOB_EXTRA puts the write
//   past the end.
//
//   Tool: compute-sanitizer --tool memcheck
//   Expected report:
//     "Invalid __global__ write of size 4"
//     at 0x... / kernel_oob
//     Address 0x... is out of bounds
// ------------------------------------------------------------
__global__ void kernel_oob(float* global_mem, int n)
{
    // [F4-T10] INTENTIONAL BUG #1 -- out-of-bounds write, triggers memcheck
    // [F4-T11] tid is in [0, n) but write address tid + OOB_EXTRA goes past allocation end
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        global_mem[tid + OOB_EXTRA] = static_cast<float>(tid); // BUG: oob write
    }
}

// ------------------------------------------------------------
// [F4-T12] INTENTIONAL BUG #2: shared memory data race (racecheck)
//
//   Two warps write the same __shared__ address with no sync.
//   shared_counter[0] is concurrently incremented by all threads with
//   no atomics or __syncthreads.
//
//   Tool: compute-sanitizer --tool racecheck
//   Expected report:
//     "Shared memory race hazard: write"
//     between threads ... and ...
// ------------------------------------------------------------
__global__ void kernel_race(int* result, int n)
{
    // [F4-T13] INTENTIONAL BUG #2 -- shared mem counter without atomics, triggers racecheck
    __shared__ int shared_counter; // [F4-T14] uninitialized + unsynchronized
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    // [F4-T15] BUG: all threads concurrently write shared_counter, no atomic, data race
    if (tid < n) {
        shared_counter = shared_counter + 1; // BUG: race -- non-atomic RMW
    }
    __syncthreads();

    if (threadIdx.x == 0) {
        result[blockIdx.x] = shared_counter;
    }
}

// ------------------------------------------------------------
// [F4-T16] INTENTIONAL BUG #3: divergent __syncthreads (synccheck)
//
//   Threads with threadIdx.x < 16 enter the if and call __syncthreads();
//   the rest skip it. Only part of the warp participates -- divergent sync.
//
//   Tool: compute-sanitizer --tool synccheck
//   Expected report:
//     "__syncthreads() requires all threads in a CTA to be active"
//     or "divergent __syncthreads"
// ------------------------------------------------------------
__global__ void kernel_div_sync(float* out, int n)
{
    // [F4-T17] INTENTIONAL BUG #3 -- __syncthreads in divergent branch, triggers synccheck
    __shared__ float smem[128];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    smem[threadIdx.x] = (tid < n) ? static_cast<float>(tid) : 0.0f;

    // [F4-T18] BUG: only threadIdx.x < 16 calls __syncthreads, others skip -- divergent
    if (threadIdx.x < 16) {
        __syncthreads(); // BUG: divergent __syncthreads
        out[tid] = smem[threadIdx.x] * 2.0f;
    } else {
        out[tid] = smem[threadIdx.x];
    }
}

// ------------------------------------------------------------
// [F4-T19] INTENTIONAL BUG #4: read uninitialized shared memory (initcheck)
//
//   Only threadIdx.x == 0 initializes smem[0]; the other slots are never written.
//   All threads then read smem[threadIdx.x], so smem[1..127] is uninitialized.
//
//   Tool: compute-sanitizer --tool initcheck
//   Expected report:
//     "Uninitialized __shared__ memory read of size 4"
//     at thread (x, y, z)
// ------------------------------------------------------------
__global__ void kernel_uninit(float* out, int n)
{
    // [F4-T20] INTENTIONAL BUG #4 -- reads uninitialized __shared__, triggers initcheck
    __shared__ float smem[128];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    // [F4-T21] BUG: only smem[0] initialized; the rest read without being written
    if (threadIdx.x == 0) {
        smem[0] = 42.0f; // [F4-T22] only index 0 initialized
    }
    // [F4-T23] note: __syncthreads is intentionally omitted so initcheck still flags BUG #4
    //   (adding __syncthreads would not help because smem[1..127] remains uninitialized)

    if (tid < n) {
        out[tid] = smem[threadIdx.x]; // BUG: smem[1..127] read before being written
    }
}

// ------------------------------------------------------------
// [F4-T24] main
// ------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::puts("[F4_compute_sanitizer_suite]");
    print_device_info(0);

    NVTX_RANGE("F4/main");

    // [F4-T25] parse --bug=xxx arguments
    bool run_oob   = true;
    bool run_race  = true;
    bool run_sync  = true;
    bool run_uninit = true;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--bug=oob") == 0) {
            run_race = run_sync = run_uninit = false;
        } else if (std::strcmp(argv[i], "--bug=race") == 0) {
            run_oob = run_sync = run_uninit = false;
        } else if (std::strcmp(argv[i], "--bug=sync") == 0) {
            run_oob = run_race = run_uninit = false;
        } else if (std::strcmp(argv[i], "--bug=uninit") == 0) {
            run_oob = run_race = run_sync = false;
        }
    }

    // ------------------------------------------------------------
    // [F4-T26] Allocate memory
    // ------------------------------------------------------------
    // [F4-T27] note: only N elements allocated; kernel_oob writes OOB_EXTRA past the end
    float* d_float = nullptr;
    int*   d_int   = nullptr;
    float* d_out   = nullptr;

    CUDA_CHECK(cudaMalloc(&d_float, N * sizeof(float)));          // [F4-T28] BUG #1 will overrun
    CUDA_CHECK(cudaMalloc(&d_int,   (N / BLOCK) * sizeof(int)));  // [F4-T29] BUG #2 result
    CUDA_CHECK(cudaMalloc(&d_out,   N * sizeof(float)));          // [F4-T30] BUG #3 / #4 output

    CUDA_CHECK(cudaMemset(d_float, 0, N * sizeof(float)));
    CUDA_CHECK(cudaMemset(d_out,   0, N * sizeof(float)));

    int grid = (N + BLOCK - 1) / BLOCK;

    // ------------------------------------------------------------
    // [F4-T31] BUG #1: kernel_oob (memcheck)
    // ------------------------------------------------------------
    if (run_oob) {
        printf("\n--- BUG #1: kernel_oob (compute-sanitizer --tool memcheck) ---\n");
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        printf("  expected: Invalid __global__ write of size 4, address out of bounds\n");
        {
            NVTX_RANGE_COLOR("F4/kernel_oob", 0xFFFF4040);
            kernel_oob<<<grid, BLOCK>>>(d_float, N);
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());
        }
        printf("  no-sanitizer run: does not crash (writes neighboring memory) but memcheck flags it\n");
    }

    // ------------------------------------------------------------
    // [F4-T32] BUG #2: kernel_race (racecheck)
    // ------------------------------------------------------------
    if (run_race) {
        printf("\n--- BUG #2: kernel_race (compute-sanitizer --tool racecheck) ---\n");
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        printf("  expected: Shared memory race hazard: non-atomic RMW on shared_counter\n");
        {
            NVTX_RANGE_COLOR("F4/kernel_race", 0xFFFF8040);
            kernel_race<<<grid, BLOCK>>>(d_int, N);
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());
        }
        printf("  no-sanitizer run: result is nondeterministic (data race); racecheck reports hazard\n");
    }

    // ------------------------------------------------------------
    // [F4-T33] BUG #3: kernel_div_sync (synccheck)
    // ------------------------------------------------------------
    if (run_sync) {
        printf("\n--- BUG #3: kernel_div_sync (compute-sanitizer --tool synccheck) ---\n");
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        printf("  expected: __syncthreads() called in divergent branch\n");
        {
            NVTX_RANGE_COLOR("F4/kernel_div_sync", 0xFFFFFF40);
            kernel_div_sync<<<grid, BLOCK>>>(d_out, N);
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());
        }
        printf("  no-sanitizer run: undefined behavior; synccheck reports divergent sync\n");
    }

    // ------------------------------------------------------------
    // [F4-T34] BUG #4: kernel_uninit (initcheck)
    // ------------------------------------------------------------
    if (run_uninit) {
        printf("\n--- BUG #4: kernel_uninit (compute-sanitizer --tool initcheck) ---\n");
        printf("  launch: grid=%d block=%d\n", grid, BLOCK);
        printf("  expected: Uninitialized __shared__ memory read of size 4\n");
        {
            NVTX_RANGE_COLOR("F4/kernel_uninit", 0xFF40FFFF);
            CUDA_CHECK(cudaMemset(d_out, 0, N * sizeof(float)));
            kernel_uninit<<<grid, BLOCK>>>(d_out, N);
            CUDA_CHECK(cudaGetLastError());
            CUDA_CHECK(cudaDeviceSynchronize());
        }
        printf("  no-sanitizer run: reads garbage; initcheck reports uninitialized read\n");
    }

    // ------------------------------------------------------------
    // [F4-T35] TODO [REQUIRED-1] for each bug, record sanitizer key fields:
    //   - error type (Invalid write / race hazard / divergent sync / uninit read)
    //   - block / thread id
    //   - memory address (if any)
    //   Record findings as comments:
    //   // BUG #1 memcheck: "Invalid __global__ write..." at block(?, 0, 0) thread(?, 0, 0)
    //   // BUG #2 racecheck: "race hazard" between thread ... and thread ...
    //   // BUG #3 synccheck: "divergent __syncthreads" at ...
    //   // BUG #4 initcheck: "Uninitialized __shared__ memory read" at thread(?, 0, 0)

    // [F4-T36] TODO [REQUIRED-2] fix each bug, re-run the matching sanitizer, verify the error disappears:
    //   BUG #1 fix: replace global_mem[tid + OOB_EXTRA] with global_mem[tid]
    //   BUG #2 fix: replace with atomicAdd(&shared_counter, 1)
    //   BUG #3 fix: move __syncthreads() out of the if branch
    //   BUG #4 fix: __syncthreads() before the read, and initialize all of smem first

    // [F4-T37] TODO [REQUIRED-3] compute the perf gap between buggy and fixed versions
    //   (sanitizer overhead vs the gain from the fix)

    // [F4-T38] TODO [ADVANCED-1] build a kernel with multiple subtle bugs and run --tool all
    // [F4-T39] TODO [ADVANCED-2] use --log-level info to get more detailed sanitizer logs
    // [F4-T40] TODO [ADVANCED-3] run all four sanitizers on a correct kernel, verify no false positives

    // ------------------------------------------------------------
    // [F4-T41] Cleanup
    // ------------------------------------------------------------
    CUDA_CHECK(cudaFree(d_float));
    CUDA_CHECK(cudaFree(d_int));
    CUDA_CHECK(cudaFree(d_out));

    // [F4-T42]
    printf("\n[F4] done. The kernels above contain intentional bugs; run compute-sanitizer with the matching tool.\n");
    return 0;
}
