// D2_vote_ballot/main.cu
// [D2-T01] Exercise D2: Warp Vote & Ballot
// [D2-T02] __any_sync / __all_sync / __ballot_sync
//
// [D2-T03] Learning goals:
// [D2-T04]   - __any_sync / __all_sync: warp-level OR / AND reduction
// [D2-T05]   - __ballot_sync: encode each lane's predicate into a 32-bit mask
// [D2-T06]   - ballot + __popc to implement stream compaction (filter)
// [D2-T07]   - compare ballot vs shared memory gather: correctness and performance
//
// Build: cmake --build build --target D2_vote_ballot
// Run:   ./D2_vote_ballot

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D2-T08] Constants
// ------------------------------------------------------------
constexpr int WARP_SIZE  = 32;
constexpr int BLOCK_SIZE = WARP_SIZE;
constexpr int N          = WARP_SIZE;
constexpr int THRESHOLD  = 16; // [D2-T09] predicate: val > THRESHOLD (half of even-indexed satisfy)

// ------------------------------------------------------------
// [D2-T10] Kernel 1: __any_sync demo
// [D2-T11] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void vote_any_kernel(const int* in, int* out_any)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D2-T12] predicate: any lane has val > THRESHOLD
    // [D2-T13] TODO [REQUIRED-1]:
    // int any_result = __any_sync(0xffffffff, val > THRESHOLD);
    // if (lane_id == 0) out_any[0] = any_result;

    if (lane_id == 0) out_any[0] = 0; // stub
}

// ------------------------------------------------------------
// [D2-T14] Kernel 2: __all_sync demo
// [D2-T15] TODO [REQUIRED-2]
// ------------------------------------------------------------
__global__ void vote_all_kernel(const int* in, int* out_all)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D2-T16] predicate: all lanes have val > 0 (in[] = 1..32, all true)
    // [D2-T17] TODO [REQUIRED-2]:
    // int all_result = __all_sync(0xffffffff, val > 0);
    // if (lane_id == 0) out_all[0] = all_result;

    if (lane_id == 0) out_all[0] = 0; // stub
}

// ------------------------------------------------------------
// [D2-T18] Kernel 3: __ballot_sync produces mask
// [D2-T19] TODO [REQUIRED-3]
// ------------------------------------------------------------
__global__ void ballot_mask_kernel(const int* in, unsigned int* out_mask)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D2-T20] TODO [REQUIRED-3] encode each lane's predicate (val > THRESHOLD) into 32-bit mask
    // unsigned int mask = __ballot_sync(0xffffffff, val > THRESHOLD);
    // if (lane_id == 0) out_mask[0] = mask;

    if (lane_id == 0) out_mask[0] = 0u; // stub
}

// ------------------------------------------------------------
// [D2-T21] Kernel 4: stream compaction - ballot version
// [D2-T22]   Input: in[] (N elements)
// [D2-T23]   Output: out[] (compacted), out_count: number satisfied
// [D2-T24] TODO [REQUIRED-4]
// ------------------------------------------------------------
__global__ void stream_compact_ballot(const int* in, int* out, int* out_count)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    bool cond = (val % 2 == 0); // [D2-T25] predicate: even number

    // [D2-T26] Step 1: ballot to obtain mask
    // [D2-T27] TODO [REQUIRED-4]:
    // unsigned int ballot_mask = __ballot_sync(0xffffffff, cond);

    // [D2-T28] Step 2: popcount yields count of satisfied lanes
    // [D2-T29] TODO [REQUIRED-4]:
    // int total = __popc(ballot_mask);

    // [D2-T30] Step 3: compute output position per lane
    // [D2-T31] (popcount of bits below current lane = exclusive scan)
    // [D2-T32] TODO [REQUIRED-4]:
    // int pos = __popc(ballot_mask & ((1u << lane_id) - 1u));

    // [D2-T33] Step 4: satisfied lanes write output
    // [D2-T34] TODO [REQUIRED-4]:
    // if (cond) out[pos] = val;
    // if (lane_id == 0) *out_count = total;

    if (lane_id == 0) *out_count = 0; // stub
}

// ------------------------------------------------------------
// [D2-T35] Kernel 5: stream compaction - smem gather version (control)
// [D2-T36] TODO [REQUIRED-5]
// ------------------------------------------------------------
__global__ void stream_compact_smem(const int* in, int* out, int* out_count)
{
    __shared__ int smem_out[WARP_SIZE];
    __shared__ int smem_cnt;

    int tid = threadIdx.x;
    int val = in[tid];

    if (tid == 0) smem_cnt = 0;
    __syncthreads();

    if (val % 2 == 0) {
        // [D2-T37] TODO [REQUIRED-5] use atomicAdd to write into smem (not warp-level, demo only)
        // int pos = atomicAdd(&smem_cnt, 1);
        // smem_out[pos] = val;
    }
    __syncthreads();

    // [D2-T38] copy to global
    if (tid < smem_cnt) out[tid] = 0; // stub: replace with smem_out[tid] after fix
    if (tid == 0)       *out_count = 0; // stub
}

// ------------------------------------------------------------
// [D2-T39] CPU reference
// ------------------------------------------------------------
static int cpu_compact(const int* in, int n, int* out)
{
    int cnt = 0;
    for (int i = 0; i < n; ++i)
        if (in[i] % 2 == 0) out[cnt++] = in[i];
    return cnt;
}

// ------------------------------------------------------------
// [D2-T40] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D2/main");

    int h_in[N];
    for (int i = 0; i < N; ++i) h_in[i] = i + 1; // 1..32

    int cpu_out[N], cpu_cnt;
    cpu_cnt = cpu_compact(h_in, N, cpu_out);
    // [D2-T41]
    printf("CPU compact: %d evens, first two: %d %d\n\n",
           cpu_cnt, cpu_out[0], cpu_out[1]);

    int *d_in = nullptr, *d_out = nullptr, *d_count = nullptr;
    unsigned int *d_mask = nullptr;
    int *d_any = nullptr, *d_all = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,    N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out,   N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_count, sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_mask,  sizeof(unsigned int)));
    CUDA_CHECK(cudaMalloc(&d_any,   sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_all,   sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in, N * sizeof(int), cudaMemcpyHostToDevice));

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [D2-T42] Test any
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D2/any");
        CUDA_CHECK(cudaMemset(d_any, 0, sizeof(int)));
        vote_any_kernel<<<1, BLOCK_SIZE>>>(d_in, d_any);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        int h_any; CUDA_CHECK(cudaMemcpy(&h_any, d_any, sizeof(int), cudaMemcpyDeviceToHost));
        // [D2-T43]
        printf("[any]  any(val > %d) = %d  (expected 1, stub=0)\n", THRESHOLD, h_any);
    }

    // ------------------------------------------------------------
    // [D2-T44] Test all
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D2/all");
        CUDA_CHECK(cudaMemset(d_all, 0, sizeof(int)));
        vote_all_kernel<<<1, BLOCK_SIZE>>>(d_in, d_all);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        int h_all; CUDA_CHECK(cudaMemcpy(&h_all, d_all, sizeof(int), cudaMemcpyDeviceToHost));
        // [D2-T45]
        printf("[all]  all(val > 0)  = %d  (expected 1, stub=0)\n", h_all);
    }

    // ------------------------------------------------------------
    // [D2-T46] Test ballot mask
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D2/ballot");
        CUDA_CHECK(cudaMemset(d_mask, 0, sizeof(unsigned int)));
        ballot_mask_kernel<<<1, BLOCK_SIZE>>>(d_in, d_mask);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        unsigned int h_mask;
        CUDA_CHECK(cudaMemcpy(&h_mask, d_mask, sizeof(unsigned int), cudaMemcpyDeviceToHost));
        // [D2-T47]
        printf("[ballot] mask=0x%08X  popcount=%d  (expected %d lanes > %d, stub=0)\n",
               h_mask, static_cast<int>(std::popcount(h_mask)), N - THRESHOLD, THRESHOLD);
    }

    // ------------------------------------------------------------
    // [D2-T48] Test stream compaction (ballot version)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D2/compact_ballot");
        CUDA_CHECK(cudaMemset(d_out,   0, N * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_count, 0, sizeof(int)));
        timer.start();
        stream_compact_ballot<<<1, BLOCK_SIZE>>>(d_in, d_out, d_count);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        int h_cnt; CUDA_CHECK(cudaMemcpy(&h_cnt, d_count, sizeof(int), cudaMemcpyDeviceToHost));
        int h_out[N]; CUDA_CHECK(cudaMemcpy(h_out, d_out, N * sizeof(int), cudaMemcpyDeviceToHost));
        // [D2-T49]
        printf("[ballot compact]  count=%d (expected %d, stub=0)  %.3f ms\n",
               h_cnt, cpu_cnt, timer.elapsed_ms());
        if (h_cnt == cpu_cnt) {
            // [D2-T50]
            printf("  first two: %d %d (expected %d %d)\n",
                   h_out[0], h_out[1], cpu_out[0], cpu_out[1]);
        }
    }

    // ------------------------------------------------------------
    // [D2-T51] Test stream compaction (smem version)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D2/compact_smem");
        CUDA_CHECK(cudaMemset(d_out,   0, N * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_count, 0, sizeof(int)));
        timer.start();
        stream_compact_smem<<<1, BLOCK_SIZE>>>(d_in, d_out, d_count);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        int h_cnt; CUDA_CHECK(cudaMemcpy(&h_cnt, d_count, sizeof(int), cudaMemcpyDeviceToHost));
        // [D2-T52]
        printf("[smem compact]    count=%d (expected %d, stub=0)  %.3f ms\n",
               h_cnt, cpu_cnt, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [D2-T53] TODO [REQUIRED-6] Nsight Compute PTX view: confirm vote.any / ballot
    // [D2-T54] TODO [ADVANCED] warp-level histogram (multi-condition ballot)
    // [D2-T55] TODO [ADVANCED] visualize lane activeness (active lanes per warp)
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_count));
    CUDA_CHECK(cudaFree(d_mask));
    CUDA_CHECK(cudaFree(d_any));
    CUDA_CHECK(cudaFree(d_all));

    // [D2-T56]
    printf("\n[D2] done. After filling TODOs, ballot compact should match CPU reference.\n");
    return 0;
}
