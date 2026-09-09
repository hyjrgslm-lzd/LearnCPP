// ============================================================
// [A2-T01] Exercise A2: index_mapping
// [A2-T02] Goal: master 1D/2D grid and block index mapping;
// [A2-T03]       learn how to break a 2D problem into thread
// [A2-T04]       indices, and vice versa.
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [A2-T05] Constants
// ------------------------------------------------------------
static constexpr int N_1D   = 1024;    // [A2-T06] 1D array size
static constexpr int ROWS   = 256;     // [A2-T07] 2D matrix rows
static constexpr int COLS   = 32;      // [A2-T08] 2D matrix cols

// ------------------------------------------------------------
// [A2-T09] Kernel 1: 1D thread index mapping
// ------------------------------------------------------------
// [A2-T10] TODO [REQUIRED] step 1: implement map_index_1d
//   - compute int idx = blockIdx.x * blockDim.x + threadIdx.x;
//   - if idx < n, set output[idx] = idx (proves the thread
//     accessed the correct element)
//   - printf is optional (with many threads it floods the
//     printf buffer; you may print only when idx==0)
__global__ void map_index_1d(int* output, int n) {
    // [A2-T11] TODO [REQUIRED]: int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // [A2-T12] TODO [REQUIRED]: if (idx < n) { output[idx] = idx; }
    (void)output; (void)n;  // [A2-T13] placeholder; remove after filling in
}

// ------------------------------------------------------------
// [A2-T14] Kernel 2: 2D thread index mapping
// ------------------------------------------------------------
// [A2-T15] TODO [REQUIRED] step 5: implement map_index_2d
//   - int row = blockIdx.y * blockDim.y + threadIdx.y;
//   - int col = blockIdx.x * blockDim.x + threadIdx.x;
//   - int idx = row * cols + col; (row major)
//   - bounds check row < rows && col < cols
__global__ void map_index_2d(int* output, int rows, int cols) {
    // [A2-T16] TODO [REQUIRED]: int row = blockIdx.y * blockDim.y + threadIdx.y;
    // [A2-T17] TODO [REQUIRED]: int col = blockIdx.x * blockDim.x + threadIdx.x;
    // [A2-T18] TODO [REQUIRED]: if (row < rows && col < cols) {
    // [A2-T19] TODO [REQUIRED]:     output[row * cols + col] = row * cols + col;
    // [A2-T20] TODO [REQUIRED]: }
    (void)output; (void)rows; (void)cols;  // [A2-T21] placeholder
}

// ------------------------------------------------------------
// [A2-T22] Advanced kernel: grid stride loop
// ------------------------------------------------------------
// [A2-T23] TODO [ADVANCED] implement map_index_grid_stride:
//   one thread processes multiple elements; the grid size can
//   be smaller than the data size.
#if 0
__global__ void map_index_grid_stride(int* output, int n) {
    int stride = gridDim.x * blockDim.x;
    for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < n; idx += stride) {
        output[idx] = idx;
    }
}
#endif

// ------------------------------------------------------------
// [A2-T24] Host verification: 1D mapping result
// ------------------------------------------------------------
static bool verify_1d(const int* host_out, int n) {
    for (int i = 0; i < n; i++) {
        if (host_out[i] != i) {
            std::printf("[FAIL] 1D: host_out[%d] = %d, expected %d\n",
                        i, host_out[i], i);
            return false;
        }
    }
    return true;
}

// ------------------------------------------------------------
// [A2-T25] Host verification: 2D mapping result
// ------------------------------------------------------------
static bool verify_2d(const int* host_out, int rows, int cols) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int expected = r * cols + c;
            if (host_out[expected] != expected) {
                std::printf("[FAIL] 2D: [%d][%d] = %d, expected %d\n",
                            r, c, host_out[expected], expected);
                return false;
            }
        }
    }
    return true;
}

// ------------------------------------------------------------
// [A2-T26] main
// ------------------------------------------------------------
int main() {
    std::puts("[A2_index_mapping]");
    print_device_info(0);

    // ============================================================
    // [A2-T27] Part 1: 1D index mapping
    // ============================================================
    {
        NVTX_RANGE("A2/1D_mapping");

        // [A2-T28] TODO [REQUIRED] step 2: allocate and zero-init
        //          the output array on the host.
        int* h_out = (int*)std::calloc(N_1D, sizeof(int));  // [A2-T29] zero-initialised

        // [A2-T30] device-side allocation
        int* d_out = nullptr;
        CUDA_CHECK(cudaMalloc(&d_out, N_1D * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_out, 0, N_1D * sizeof(int)));

        // [A2-T31] TODO [REQUIRED] step 3: launch <<<32, 32>>> (1024 threads total)
        dim3 grid1d(32);
        dim3 block1d(32);
        std::printf("[launch 1D] grid=(%u)  block=(%u)  total_threads=%u\n",
                    grid1d.x, block1d.x, grid1d.x * block1d.x);

        map_index_1d<<<grid1d, block1d>>>(d_out, N_1D);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());

        // [A2-T32] TODO [REQUIRED] step 4: cudaMemcpy back to host and verify element-wise
        CUDA_CHECK(cudaMemcpy(h_out, d_out, N_1D * sizeof(int), cudaMemcpyDeviceToHost));

        if (verify_1d(h_out, N_1D)) {
            // [A2-T33]
            std::puts("[A2] 1D mapping verification PASSED");
        } else {
            // [A2-T34] failure means the exercise is not done; do not abort
            //          (so students can observe the state)
            std::puts("[A2] 1D mapping verification FAILED (TODO not filled in?)");
        }

        CUDA_CHECK(cudaFree(d_out));
        std::free(h_out);
    }

    // ============================================================
    // [A2-T35] Part 2: 2D index mapping  256x32 matrix
    // ============================================================
    {
        NVTX_RANGE("A2/2D_mapping");

        int total = ROWS * COLS;
        int* h_out2d = (int*)std::calloc(total, sizeof(int));
        int* d_out2d = nullptr;
        CUDA_CHECK(cudaMalloc(&d_out2d, total * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_out2d, 0, total * sizeof(int)));

        // [A2-T36] TODO [REQUIRED] step 6: launch <<<dim3(8,4), dim3(32,8)>>>
        //   grid  8x4  = 32 blocks
        //   block 32x8 = 256 threads/block
        //   total threads = 8192 >= 256*32=8192, exact coverage
        dim3 grid2d(8, 4);
        dim3 block2d(32, 8);
        std::printf("[launch 2D] grid=(%u,%u)  block=(%u,%u)  total=%u\n",
                    grid2d.x, grid2d.y, block2d.x, block2d.y,
                    grid2d.x * grid2d.y * block2d.x * block2d.y);

        map_index_2d<<<grid2d, block2d>>>(d_out2d, ROWS, COLS);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());

        CUDA_CHECK(cudaMemcpy(h_out2d, d_out2d, total * sizeof(int), cudaMemcpyDeviceToHost));

        if (verify_2d(h_out2d, ROWS, COLS)) {
            // [A2-T37]
            std::puts("[A2] 2D mapping verification PASSED");
        } else {
            // [A2-T38]
            std::puts("[A2] 2D mapping verification FAILED (TODO not filled in?)");
        }

        CUDA_CHECK(cudaFree(d_out2d));
        std::free(h_out2d);
    }

    // ------------------------------------------------------------
    // [A2-T39] TODO [ADVANCED] grid stride loop variant
    // ------------------------------------------------------------
#if 0
    {
        NVTX_RANGE("A2/grid_stride");
        int* d_gs = nullptr;
        CUDA_CHECK(cudaMalloc(&d_gs, N_1D * sizeof(int)));
        // [A2-T40] launch far fewer threads than N_1D; the grid
        //          stride loop fills in the rest.
        map_index_grid_stride<<<4, 32>>>(d_gs, N_1D);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());
        // [A2-T41] ... verify ...
        CUDA_CHECK(cudaFree(d_gs));
    }
#endif

    std::puts("[A2_index_mapping] DONE");
    return 0;
}
