// [C5-T01] C5_mbarrier_async_barrier/main.cu
// [C5-T02] Exercise C5: cuda::barrier (libcu++) - producer/consumer asynchronous sync pattern
//
// [C5-T03] Goals:
//   - Declare and initialize cuda::barrier<cuda::thread_scope_block> in smem
//   - Producer warp uses arrive_and_drop / arrive; consumer uses wait
//   - barrier_arrive_tx passes the expected byte count (warm-up for TMA interaction)
//   - Compare full-block wait via __syncthreads vs partial-thread wait via mbarrier
//
// [C5-T04] Build: cmake --build build --target C5_mbarrier_async_barrier
// Run:   ./C5_mbarrier_async_barrier
// Note: cuda::barrier needs sm_80+; Hopper sm_90a additionally supports barrier_arrive_tx

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
// [C5-T05] libcu++ barrier (ships with the CUDA Toolkit)
#include <cuda/barrier>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [C5-T06] Constants
// ------------------------------------------------------------
constexpr int BLOCK_SIZE     = 128;
constexpr int SMEM_WORDS     = BLOCK_SIZE; // [C5-T07] smem array size
constexpr int PRODUCER_WARPS = 1;          // [C5-T08] first 1 warp is producer
constexpr int CONSUMER_WARPS = BLOCK_SIZE / 32 - PRODUCER_WARPS;

// ------------------------------------------------------------
// [C5-T09] Kernel 1: __syncthreads version (control)
// ------------------------------------------------------------
__global__ void producer_consumer_sync_kernel(int* out)
{
    __shared__ int smem[SMEM_WORDS];
    int tid = threadIdx.x;

    // [C5-T10] all threads write (simulating producer)
    smem[tid] = tid * 2;
    __syncthreads(); // [C5-T11] full-block wait

    // [C5-T12] all threads read (simulating consumer)
    out[tid] = smem[tid] + 1;
}

// ------------------------------------------------------------
// [C5-T13] Kernel 2: cuda::barrier version
//   First PRODUCER_WARPS warps write smem, then arrive_and_drop
//   Remaining warps (consumers) wait at bar.arrive_and_wait
//
// [C5-T14] TODO [REQUIRED-1] declare smem barrier
// [C5-T15] TODO [REQUIRED-2] producer arrive_and_drop / consumer arrive_and_wait
// [C5-T16] TODO [REQUIRED-3] barrier_arrive_tx (expected byte count)
// ------------------------------------------------------------
__global__ void producer_consumer_barrier_kernel(int* out)
{
    // [C5-T17] TODO [REQUIRED-1] declare a barrier in smem; initialize to blockDim.x
    // __shared__ cuda::barrier<cuda::thread_scope_block> bar;
    // Note: the barrier must be initialized by a single thread (typically tid == 0)
    // if (threadIdx.x == 0) {
    //     init(&bar, blockDim.x); // expects blockDim.x arrives
    // }
    // __syncthreads(); // ensure barrier init is visible to all threads

    __shared__ int smem[SMEM_WORDS];
    int tid    = threadIdx.x;
    int warp_id = tid / 32;

    if (warp_id < PRODUCER_WARPS) {
        // [C5-T18] -- producer warp: write data, then arrive
        smem[tid] = tid * 3; // simulate write

        // [C5-T19] TODO [REQUIRED-2] producer arrive_and_drop after finishing (no wait)
        // bar.arrive_and_drop(); // this thread no longer participates in subsequent phases
    } else {
        // [C5-T20] -- consumer warp: arrive and wait for the producer
        // [C5-T21] TODO [REQUIRED-2] consumer arrive_and_wait
        // bar.arrive_and_wait();

        // [C5-T22] read the data the producer wrote
        // TODO: out[tid] = smem[tid - PRODUCER_WARPS * 32] + 100;
    }

    // [C5-T23] Stub: write 0; after completing the TODO students should see the right value
    out[tid] = 0;
}

// ------------------------------------------------------------
// [C5-T24] Kernel 3: barrier_arrive_tx demo (Hopper feature)
// [C5-T25] TODO [REQUIRED-3] - set the expected byte count for a TMA-like scenario
// ------------------------------------------------------------
__global__ void barrier_arrive_tx_demo(int* out)
{
    // [C5-T26] TODO [REQUIRED-3] declare a barrier that supports arrive_tx
    // __shared__ cuda::barrier<cuda::thread_scope_block> bar;
    // if (threadIdx.x == 0) init(&bar, 1); // only 1 arrive expected
    // __syncthreads();

    __shared__ int smem[SMEM_WORDS];
    int tid = threadIdx.x;

    if (tid == 0) {
        // [C5-T27] simulate a TMA copy: announce N bytes will be written
        // constexpr int expected_bytes = SMEM_WORDS * sizeof(int);
        // TODO [REQUIRED-3]:
        // cuda::device::barrier_arrive_tx(bar, 1, expected_bytes);
        // perform the actual write (with real TMA the hardware handles this; we simulate manually)
        for (int i = 0; i < SMEM_WORDS; ++i) smem[i] = i * 5;
    }

    // [C5-T28] consumer waits
    // TODO [REQUIRED-3]:
    // if (tid != 0) { bar.arrive_and_wait(); }
    __syncthreads(); // [C5-T29] fallback: use __syncthreads, replace once TODO is done

    out[tid] = smem[tid]; // [C5-T30] TODO: after the fix, validate the value is tid * 5
    // [C5-T31] Stub
    out[tid] = 0;
}

// ------------------------------------------------------------
// [C5-T32] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("C5/main");

    int *d_out_sync = nullptr, *d_out_bar = nullptr, *d_out_tx = nullptr;
    CUDA_CHECK(cudaMalloc(&d_out_sync, BLOCK_SIZE * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_bar,  BLOCK_SIZE * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out_tx,   BLOCK_SIZE * sizeof(int)));

    CudaEventTimer timer;
    int h_sync[BLOCK_SIZE], h_bar[BLOCK_SIZE], h_tx[BLOCK_SIZE];

    // ------------------------------------------------------------
    // [C5-T33] Control: __syncthreads version
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C5/sync_version");
        timer.start();
        producer_consumer_sync_kernel<<<1, BLOCK_SIZE>>>(d_out_sync);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(h_sync, d_out_sync, BLOCK_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [C5-T34]
        printf("[sync]  out[0]=%d out[1]=%d  %.3f ms\n",
               h_sync[0], h_sync[1], timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [C5-T35] mbarrier version
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C5/barrier_version");
        CUDA_CHECK(cudaMemset(d_out_bar, 0, BLOCK_SIZE * sizeof(int)));
        timer.start();
        producer_consumer_barrier_kernel<<<1, BLOCK_SIZE>>>(d_out_bar);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(h_bar, d_out_bar, BLOCK_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [C5-T36]
        printf("[mbar]  out[0]=%d out[32]=%d  %.3f ms  (stub=0)\n",
               h_bar[0], h_bar[32], timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [C5-T37] barrier_arrive_tx demo
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C5/arrive_tx");
        CUDA_CHECK(cudaMemset(d_out_tx, 0, BLOCK_SIZE * sizeof(int)));
        barrier_arrive_tx_demo<<<1, BLOCK_SIZE>>>(d_out_tx);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_tx, d_out_tx, BLOCK_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [C5-T38]
        printf("[tx]    out[0]=%d out[1]=%d  (expected 0, 5 - stub=0)\n",
               h_tx[0], h_tx[1]);
    }

    // ------------------------------------------------------------
    // [C5-T39] TODO [REQUIRED-4] compare performance (sync vs mbarrier)
    // [C5-T40] TODO [REQUIRED-5] view barrier.arrive / barrier.wait in Nsight Compute PTX view
    // [C5-T41] TODO [REQUIRED-6] verify behavior when transaction count is wrong
    // [C5-T42] TODO [ADVANCED] multi-round pipeline: alternate phases with two mbarriers
    // [C5-T43] TODO [ADVANCED] precisely predict TMA byte count in barrier_arrive_tx
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_out_sync));
    CUDA_CHECK(cudaFree(d_out_bar));
    CUDA_CHECK(cudaFree(d_out_tx));

    // [C5-T44]
    printf("\n[C5] done. Refer to libcu++ <cuda/barrier> docs to complete the TODOs.\n");
    return 0;
}
