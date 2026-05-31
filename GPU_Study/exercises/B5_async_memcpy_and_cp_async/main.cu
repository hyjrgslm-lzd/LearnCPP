// ============================================================
// [B5-T01] Exercise B5: async_memcpy_and_cp_async
// [B5-T02] Goal: use cudaMemcpyAsync + streams to build a double-buffered pipeline
//          that overlaps H2D transfer with kernel compute. Master the sm_80+
//          cuda::memcpy_async path.
//
// [B5-T03] Acceptance:
//   double-buffered version is 1.5-2x faster than the sequential version.
//   Nsight Systems: nsys profile --trace cuda,nvtx ./B5_async_memcpy_and_cp_async.exe
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>

// [B5-T04] sm_80+ cp.async / cuda::memcpy_async needs the headers below
// #include <cuda/barrier>
// #include <cuda/pipeline>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [B5-T05] Constants
// ------------------------------------------------------------
static constexpr int   BATCH_ELEMS  = 1 << 18;   // [B5-T06] 1M floats per batch = 4MB
static constexpr int   NUM_BATCHES  = 8;
static constexpr int   TOTAL_ELEMS  = BATCH_ELEMS * NUM_BATCHES;
static constexpr int   BLOCK_SZ     = 256;
static constexpr int   ITERATIONS   = 500;        // [B5-T07] inner kernel loops, amplifies compute work

// ------------------------------------------------------------
// [B5-T08] Kernel: a compute-bound kernel that occupies the GPU
//          (used so that H2D and kernel can overlap)
// ------------------------------------------------------------
// [B5-T09] TODO [REQUIRED] step 1: understand this kernel -- it consumes compute
//          resources so that H2D and kernel execution can truly run concurrently.
__global__ void compute_kernel(const float* __restrict__ input,
                               float*       __restrict__ output,
                               int n,
                               int iterations)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    // [B5-T10] TODO [REQUIRED]: do iterative work (sin/cos to keep the GPU busy)
    float val = input[idx];
    for (int i = 0; i < iterations; i++) {
        val = sinf(val) * cosf(val) + 1.0f;
    }
    output[idx] = val;
}

// ------------------------------------------------------------
// [B5-T11] Sequential version (baseline): memcpy then kernel, in series
// ------------------------------------------------------------
static float run_sequential(const float* h_src, float* d_buf, float* d_out,
                             int batch, int num_batches) {
    NVTX_RANGE("B5/sequential");

    dim3 block(BLOCK_SZ);
    dim3 grid((batch + BLOCK_SZ - 1) / BLOCK_SZ);

    CudaEventTimer timer;
    timer.start();

    for (int b = 0; b < num_batches; b++) {
        const float* src = h_src + (size_t)b * batch;

        // [B5-T12] TODO [REQUIRED]: sequential -- synchronous memcpy first
        CUDA_CHECK(cudaMemcpy(d_buf, src, batch * sizeof(float),
                              cudaMemcpyHostToDevice));

        // [B5-T13] then the kernel (default stream, serialized)
        compute_kernel<<<grid, block>>>(d_buf, d_out, batch, ITERATIONS);
        CUDA_CHECK_LAST();
        CUDA_CHECK(cudaDeviceSynchronize());
    }

    timer.stop();
    return timer.elapsed_ms();
}

// ------------------------------------------------------------
// [B5-T14] Double-buffered version: stream1 does H2D, stream2 runs kernel,
//          alternating to overlap.
// ------------------------------------------------------------
static float run_double_buffered(const float* h_src,
                                 float* d_buf0, float* d_buf1,
                                 float* d_out0, float* d_out1,
                                 int batch, int num_batches)
{
    NVTX_RANGE("B5/double_buffer");

    // [B5-T15] TODO [REQUIRED] step 2: create two streams
    cudaStream_t stream1 = nullptr, stream2 = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream1));
    CUDA_CHECK(cudaStreamCreate(&stream2));

    dim3 block(BLOCK_SZ);
    dim3 grid((batch + BLOCK_SZ - 1) / BLOCK_SZ);

    CudaEventTimer timer;
    timer.start();

    // [B5-T16] TODO [REQUIRED] step 3: double-buffer main loop
    //   even batch: d_buf0 receives data, d_buf1 computes
    //   odd  batch: d_buf1 receives data, d_buf0 computes
    for (int b = 0; b < num_batches; b++) {
        const float* src  = h_src + (size_t)b * batch;
        float* cur_buf    = (b % 2 == 0) ? d_buf0 : d_buf1;
        float* prev_buf   = (b % 2 == 0) ? d_buf1 : d_buf0;
        float* prev_out   = (b % 2 == 0) ? d_out1 : d_out0;

        // [B5-T17] stream1: async copy of the current batch
        // TODO [REQUIRED]: CUDA_CHECK(cudaMemcpyAsync(cur_buf, src,
        //     batch * sizeof(float), cudaMemcpyHostToDevice, stream1));
        CUDA_CHECK(cudaMemcpyAsync(cur_buf, src,
                                   batch * sizeof(float),
                                   cudaMemcpyHostToDevice, stream1));

        // [B5-T18] stream2: process the previous batch (batch 0 has none)
        if (b > 0) {
            // TODO [REQUIRED]: compute_kernel<<<grid, block, 0, stream2>>>(
            //     prev_buf, prev_out, batch, ITERATIONS);
            compute_kernel<<<grid, block, 0, stream2>>>(
                prev_buf, prev_out, batch, ITERATIONS);
            CUDA_CHECK_LAST();
        }
    }

    // [B5-T19] handle the last batch (process num_batches-1 on stream2)
    {
        float* last_buf = (num_batches % 2 == 0) ? d_buf0 : d_buf1;
        float* last_out = (num_batches % 2 == 0) ? d_out0 : d_out1;
        // [B5-T20] wait for stream1's final memcpy
        CUDA_CHECK(cudaStreamSynchronize(stream1));
        compute_kernel<<<grid, block, 0, stream2>>>(
            last_buf, last_out, batch, ITERATIONS);
        CUDA_CHECK_LAST();
    }

    CUDA_CHECK(cudaStreamSynchronize(stream2));
    timer.stop();

    CUDA_CHECK(cudaStreamDestroy(stream1));
    CUDA_CHECK(cudaStreamDestroy(stream2));

    return timer.elapsed_ms();
}

// ------------------------------------------------------------
// [B5-T21] cp.async kernel skeleton (sm_80+)
// ------------------------------------------------------------
// [B5-T22] TODO [REQUIRED] step 5 (sm_80+ hardware): uncomment and fill the cp.async kernel
//   needs #include <cuda/barrier> or <cuda/pipeline>
//   Idea: asynchronously load from global to shared memory without blocking the warp.
#if 0
#include <cuda/barrier>

__global__ void cp_async_kernel(const float* __restrict__ global_in,
                                float*       __restrict__ global_out,
                                int n)
{
    // [B5-T23] each block processes blockDim.x elements
    extern __shared__ float smem[];
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;

    // [B5-T24] TODO [REQUIRED]: use cuda::memcpy_async to async-load from global_in to smem
    //   cuda::pipeline<cuda::thread_scope_thread> pipe = cuda::make_pipeline();
    //   cuda::memcpy_async(smem + threadIdx.x, global_in + idx,
    //                      sizeof(float), pipe);
    //   pipe.producer_commit();
    //   // ... do other work while waiting ...
    //   pipe.consumer_wait();
    //   float v = smem[threadIdx.x];
    //   global_out[idx] = v * 2.0f;

    (void)global_in; (void)global_out; (void)smem;
}
#endif

// ------------------------------------------------------------
// [B5-T25] main
// ------------------------------------------------------------
int main() {
    std::puts("[B5_async_memcpy_and_cp_async]");
    print_device_info(0);

    std::printf("[B5] BATCH=%d floats (%.1f MB)  NUM_BATCHES=%d  TOTAL=%.1f MB\n",
                BATCH_ELEMS, BATCH_ELEMS * 4.0f / 1024 / 1024,
                NUM_BATCHES,
                (float)TOTAL_ELEMS * 4.0f / 1024 / 1024);

    // [B5-T26] -- allocate pinned host memory (best companion for cudaMemcpyAsync) --
    float* h_src = nullptr;
    CUDA_CHECK(cudaMallocHost(&h_src, (size_t)TOTAL_ELEMS * sizeof(float)));
    for (int i = 0; i < TOTAL_ELEMS; i++) h_src[i] = static_cast<float>(i) * 0.001f;

    // [B5-T27] -- allocate device buffers (double buffer needs 2 batch-sized bufs) --
    float *d_buf0 = nullptr, *d_buf1 = nullptr;
    float *d_out0 = nullptr, *d_out1 = nullptr;
    size_t batch_bytes = (size_t)BATCH_ELEMS * sizeof(float);
    CUDA_CHECK(cudaMalloc(&d_buf0, batch_bytes));
    CUDA_CHECK(cudaMalloc(&d_buf1, batch_bytes));
    CUDA_CHECK(cudaMalloc(&d_out0, batch_bytes));
    CUDA_CHECK(cudaMalloc(&d_out1, batch_bytes));

    // [B5-T28] -- sequential version --
    float ms_seq = run_sequential(h_src, d_buf0, d_out0, BATCH_ELEMS, NUM_BATCHES);
    // [B5-T29]
    std::printf("[B5] sequential total:    %.1f ms\n", ms_seq);

    // [B5-T30] -- double-buffer version --
    float ms_db = run_double_buffered(h_src,
                                      d_buf0, d_buf1,
                                      d_out0, d_out1,
                                      BATCH_ELEMS, NUM_BATCHES);
    // [B5-T31]
    std::printf("[B5] double-buffer total: %.1f ms\n", ms_db);
    // [B5-T32]
    std::printf("[B5] speedup:             %.2fx (expected 1.5-2x)\n",
                ms_seq / (ms_db + 1e-6f));

    // [B5-T33] -- Nsight Systems hint --
    std::puts("------------------------------------------------------------");
    // [B5-T34]
    std::puts("  TODO [REQUIRED] step 4: run Nsight Systems:");
    std::puts("    nsys profile --trace cuda,nvtx ./B5_async_memcpy_and_cp_async.exe");
    // [B5-T35]
    std::puts("  Inspect the two streams' timeline; confirm H2D overlaps with kernel.");
    // [B5-T36]
    std::puts("  TODO [REQUIRED] step 5 (sm_80+): uncomment the cp.async kernel and test.");
    std::puts("------------------------------------------------------------");

    CUDA_CHECK(cudaFree(d_buf0));
    CUDA_CHECK(cudaFree(d_buf1));
    CUDA_CHECK(cudaFree(d_out0));
    CUDA_CHECK(cudaFree(d_out1));
    CUDA_CHECK(cudaFreeHost(h_src));

    std::puts("[B5_async_memcpy_and_cp_async] DONE");
    return 0;
}
