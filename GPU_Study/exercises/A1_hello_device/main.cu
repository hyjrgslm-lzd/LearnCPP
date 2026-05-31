// ============================================================
// [A1-T01] Exercise A1: hello_device
// [A1-T02] Goal: write your first kernel; feel host/device code
// [A1-T03]       split, and the minimal form of a kernel launch.
// ============================================================
#include <cstdio>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [A1-T04] Helper: compute global thread id
//          (advanced sample, for student reference / completion)
// ------------------------------------------------------------
// [A1-T05] TODO [ADVANCED] complete the __device__ function below
//          and call it from inside hello_kernel.
#if 0
__device__ int global_thread_id() {
    return blockIdx.x * blockDim.x + threadIdx.x;
}
#endif

// ------------------------------------------------------------
// [A1-T06] 1. Define hello_kernel
// ------------------------------------------------------------
// [A1-T07] TODO [REQUIRED] step 1: prefix the function with
//          __global__ so it becomes a kernel.
// [A1-T08] TODO [REQUIRED] step 2: inside the body, use printf:
//   "Device: blockIdx=(%d,%d,%d), threadIdx=(%d,%d,%d)\n"
//          and pass the corresponding blockIdx.x/y/z and
//          threadIdx.x/y/z values.
#if 0
__global__ void hello_kernel() {
    // [A1-T09] TODO [REQUIRED]:
    //   printf("Device: blockIdx=(%d,%d,%d), threadIdx=(%d,%d,%d)\n",
    //          blockIdx.x, blockIdx.y, blockIdx.z,
    //          threadIdx.x, threadIdx.y, threadIdx.z);
}
#endif

__global__ void hello_kernel()
{
    printf("Device: blockIdx=(%u,%u,%u), threadIdx=(%u,%u,%u)\n",
           blockIdx.x, blockIdx.y, blockIdx.z,
           threadIdx.x, threadIdx.y, threadIdx.z);
}

// ------------------------------------------------------------
// [A1-T10] Advanced: skeleton of a shared-memory variant
// ------------------------------------------------------------
// [A1-T11] TODO [ADVANCED] add a hello_kernel_shared: declare a
//          __shared__ array, each thread writes its own
//          threadIdx.x, then __syncthreads() and read+print.
#if 0
__global__ void hello_kernel_shared() {
    // [A1-T12] assume block size <= 32 threads
    __shared__ int sdata[32];
    sdata[threadIdx.x] = threadIdx.x;
    __syncthreads();
    // [A1-T13] only let thread 0 print
    if (threadIdx.x == 0) {
        for (int i = 0; i < blockDim.x; i++) {
            printf("  shared[%d] = %d\n", i, sdata[i]);
        }
    }
}
#endif

// ------------------------------------------------------------
// [A1-T14] main
// ------------------------------------------------------------
int main() {
    std::puts("[A1_hello_device]");

    // [A1-T15] -- device info --
    print_device_info(0);

    // [A1-T16] -- step 3: minimal launch --
    // [A1-T17] TODO [REQUIRED] step 3: launch hello_kernel<<<1, 1>>>()
    //          here. 1D grid with 1 block, 1D block with 1 thread.
    {
        NVTX_RANGE("A1/launch_1x1");

        // [A1-T18] launch config
        dim3 grid(1);
        dim3 block(1);
        std::printf("[launch] grid=(%u,%u,%u)  block=(%u,%u,%u)\n",
                    grid.x, grid.y, grid.z,
                    block.x, block.y, block.z);

        // [A1-T19] TODO [REQUIRED]: hello_kernel<<<grid, block>>>();
        hello_kernel<<<grid, block>>>();

        // [A1-T20] TODO [REQUIRED] step 4: use CUDA_CHECK(cudaGetLastError())
        //          to catch launch errors.
        CUDA_CHECK_LAST();

        // [A1-T21] TODO [REQUIRED] step 4: use
        //          CUDA_CHECK(cudaDeviceSynchronize()) to wait for completion.
        CUDA_CHECK(cudaDeviceSynchronize());
    }

    // [A1-T22]
    std::puts("[A1] minimal launch (1x1) done");

    // ------------------------------------------------------------
    // [A1-T23] Advanced: 2x2 grid, 2x2 block (16 threads total)
    // ------------------------------------------------------------
    // [A1-T24] TODO [ADVANCED] flip the #if 0 below to #if 1 and
    //          observe the output of 16 threads.
#if 0
    {
        NVTX_RANGE("A1/launch_2x2");

        // [A1-T25] 4 blocks
        dim3 grid2(2, 2);
        // [A1-T26] 4 threads / block => 16 threads total
        dim3 block2(2, 2);
        // [A1-T27]
        std::printf("[launch ADV] grid=(%u,%u,%u)  block=(%u,%u,%u)\n",
                    grid2.x, grid2.y, grid2.z,
                    block2.x, block2.y, block2.z);

        hello_kernel<<<grid2, block2>>>();
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());
        // [A1-T28]
        std::puts("[A1] advanced launch (2x2 grid, 2x2 block) done");
    }
#endif

    // ------------------------------------------------------------
    // [A1-T29] Advanced: shared memory variant
    // ------------------------------------------------------------
    // [A1-T30] TODO [ADVANCED] flip the #if 0 below to #if 1.
#if 0
    {
        NVTX_RANGE("A1/launch_shared");
        dim3 gs(1), bs(8);
        hello_kernel_shared<<<gs, bs>>>();
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());
        // [A1-T31]
        std::puts("[A1] advanced launch (shared memory) done");
    }
#endif

    // [A1-T32] -- acceptance assertion --
    // [A1-T33] reaching this line means no CUDA runtime error.
    // [A1-T34]
    std::puts("[A1_hello_device] PASSED (no CUDA runtime error)");
    return 0;
}
