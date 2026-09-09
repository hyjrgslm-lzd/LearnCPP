// ============================================================
// [B6-T01] Exercise B6: texture_and_constant
// [B6-T02] Goal: learn two specialized memory spaces:
//   __constant__ for broadcasting small data (convolution kernels)
//   texture memory for spatially local accesses (2D image sampling)
//
// [B6-T03] Acceptance:
//   constant-memory convolution throughput > global-memory version
//   (when the same kernel coefficients are accessed many times).
//   Texture sampling output is correct; bilinear interpolation precision passes.
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [B6-T04] Constants
// ------------------------------------------------------------
static constexpr int IMG_W   = 1024;
static constexpr int IMG_H   = 1024;
static constexpr int KER_SZ  = 5;          // [B6-T05] 5x5 convolution kernel
static constexpr int BLOCK_X = 16;
static constexpr int BLOCK_Y = 16;
static constexpr int WARMUP  = 3;

// ------------------------------------------------------------
// [B6-T06] Constant memory: stores the convolution coefficients
// ------------------------------------------------------------
// [B6-T07] TODO [REQUIRED] step 1: declare a __constant__ variable at file scope
//   Note: __constant__ must be declared at global scope.
__constant__ float c_kernel[KER_SZ][KER_SZ];

// ------------------------------------------------------------
// [B6-T08] Kernel 1: convolution kernel using global memory (baseline)
// ------------------------------------------------------------
// [B6-T09] TODO [REQUIRED] step 4: implement the global-memory convolution to compare
__global__ void conv_global_kernel(const float* __restrict__ input,
                                   float*       __restrict__ output,
                                   const float* __restrict__ kernel_g,
                                   int width, int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float result = 0.0f;
    int half = KER_SZ / 2;
    for (int ky = -half; ky <= half; ky++) {
        for (int kx = -half; kx <= half; kx++) {
            int ix = x + kx;
            int iy = y + ky;
            // [B6-T10] clamp to boundary
            ix = max(0, min(width  - 1, ix));
            iy = max(0, min(height - 1, iy));
            // [B6-T11] TODO [REQUIRED]: result += kernel_g[(ky+half)*KER_SZ+(kx+half)] * input[iy*width+ix];
            result += kernel_g[(ky + half) * KER_SZ + (kx + half)] * input[iy * width + ix];
        }
    }
    output[y * width + x] = result;
}

// ------------------------------------------------------------
// [B6-T12] Kernel 2: convolution kernel using __constant__ memory (optimized)
// ------------------------------------------------------------
// [B6-T13] TODO [REQUIRED] step 2: implement the constant-memory convolution
//   Reference c_kernel[ky+half][kx+half] directly (broadcast read; cache shared by all threads).
__global__ void conv_constant_kernel(const float* __restrict__ input,
                                     float*       __restrict__ output,
                                     int width, int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    float result = 0.0f;
    int half = KER_SZ / 2;
    // [B6-T14] TODO [REQUIRED]: same as the global version but read c_kernel[ky+half][kx+half]
    for (int ky = -half; ky <= half; ky++) {
        for (int kx = -half; kx <= half; kx++) {
            int ix = max(0, min(width  - 1, x + kx));
            int iy = max(0, min(height - 1, y + ky));
            result += c_kernel[ky + half][kx + half] * input[iy * width + ix];
        }
    }
    output[y * width + x] = result;
}

// ------------------------------------------------------------
// [B6-T15] Kernel 3: bilinear texture sampling
// ------------------------------------------------------------
// [B6-T16] TODO [REQUIRED] step 5: implement texture sampling kernel
//   Use tex2D<float>(texObj, x_norm, y_norm) with normalized coordinates.
__global__ void texture_sample_kernel(cudaTextureObject_t texObj,
                                      float*              __restrict__ output,
                                      int out_w, int out_h,
                                      int src_w, int src_h)
{
    int ox = blockIdx.x * blockDim.x + threadIdx.x;
    int oy = blockIdx.y * blockDim.y + threadIdx.y;
    if (ox >= out_w || oy >= out_h) return;

    // [B6-T17] normalized coords in [0,1] (CUDA texture normalized coords are pixel-centered)
    float u = (ox + 0.5f) / out_w;
    float v = (oy + 0.5f) / out_h;

    // [B6-T18] TODO [REQUIRED]: output[oy * out_w + ox] = tex2D<float>(texObj, u, v);
    output[oy * out_w + ox] = tex2D<float>(texObj, u, v);
}

// ------------------------------------------------------------
// [B6-T19] Timing helper
// ------------------------------------------------------------
template<typename Fn>
static float bench_gbps(Fn fn, long long bytes, int n_runs = 5) {
    for (int i = 0; i < WARMUP; i++) fn();
    CUDA_CHECK(cudaDeviceSynchronize());

    CudaEventTimer timer;
    timer.start();
    for (int i = 0; i < n_runs; i++) fn();
    timer.stop();

    float ms = timer.elapsed_ms() / n_runs;
    return static_cast<float>(bytes) / ms / 1e6f;
}

// ------------------------------------------------------------
// [B6-T20] main
// ------------------------------------------------------------
int main() {
    std::puts("[B6_texture_and_constant]");
    print_device_info(0);

    const int total = IMG_W * IMG_H;
    const long long BYTES = (long long)total * sizeof(float);

    // [B6-T21] -- initialize the convolution kernel (Gaussian approximation) --
    float h_kernel[KER_SZ][KER_SZ] = {
        {1, 4,  6,  4,  1},
        {4, 16, 24, 16, 4},
        {6, 24, 36, 24, 6},
        {4, 16, 24, 16, 4},
        {1, 4,  6,  4,  1}
    };
    // [B6-T22] normalize (sum = 256)
    for (int i = 0; i < KER_SZ; i++)
        for (int j = 0; j < KER_SZ; j++)
            h_kernel[i][j] /= 256.0f;

    // [B6-T23] TODO [REQUIRED] step 1: copy h_kernel into c_kernel via cudaMemcpyToSymbol
    CUDA_CHECK(cudaMemcpyToSymbol(c_kernel, h_kernel, sizeof(h_kernel)));

    // [B6-T24] -- allocate input image and output --
    float *h_input = (float*)std::malloc(BYTES);
    for (int i = 0; i < total; i++) h_input[i] = static_cast<float>(i % 256) / 255.0f;

    float *d_input = nullptr, *d_output = nullptr, *d_kernel_g = nullptr;
    CUDA_CHECK(cudaMalloc(&d_input,    BYTES));
    CUDA_CHECK(cudaMalloc(&d_output,   BYTES));
    CUDA_CHECK(cudaMalloc(&d_kernel_g, KER_SZ * KER_SZ * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_input,    h_input, BYTES, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_kernel_g, h_kernel, KER_SZ * KER_SZ * sizeof(float),
                          cudaMemcpyHostToDevice));

    dim3 block(BLOCK_X, BLOCK_Y);
    dim3 grid((IMG_W + BLOCK_X - 1) / BLOCK_X,
              (IMG_H + BLOCK_Y - 1) / BLOCK_Y);
    std::printf("[launch] grid=(%u,%u)  block=(%u,%u)\n",
                grid.x, grid.y, block.x, block.y);

    // [B6-T25] -- Part 1: global memory convolution --
    float gbps_global;
    {
        NVTX_RANGE("B6/conv_global");
        gbps_global = bench_gbps([&] {
            conv_global_kernel<<<grid, block>>>(d_input, d_output, d_kernel_g,
                                               IMG_W, IMG_H);
        }, BYTES * 2LL);
        // [B6-T26]
        std::printf("[B6] global mem conv throughput:   %.1f GB/s\n", gbps_global);
    }

    // [B6-T27] -- Part 2: constant memory convolution --
    float gbps_const;
    {
        NVTX_RANGE("B6/conv_constant");
        gbps_const = bench_gbps([&] {
            conv_constant_kernel<<<grid, block>>>(d_input, d_output, IMG_W, IMG_H);
        }, BYTES * 2LL);
        // [B6-T28]
        std::printf("[B6] constant     conv throughput: %.1f GB/s\n", gbps_const);
        // [B6-T29]
        std::printf("[B6] constant / global ratio:      %.2fx\n",
                    gbps_const / (gbps_global + 1e-6f));
    }

    // [B6-T30] -- Part 3: texture object sampling --
    // [B6-T31] TODO [REQUIRED] step 5: build a 2D texture object and sample it
    {
        NVTX_RANGE("B6/texture_sample");

        // [B6-T32] 3a: create the CUDA array and copy data
        cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float>();
        cudaArray_t cu_array = nullptr;
        CUDA_CHECK(cudaMallocArray(&cu_array, &channelDesc, IMG_W, IMG_H));
        CUDA_CHECK(cudaMemcpy2DToArray(cu_array, 0, 0,
                                      h_input, IMG_W * sizeof(float),
                                      IMG_W * sizeof(float), IMG_H,
                                      cudaMemcpyHostToDevice));

        // [B6-T33] 3b: resource descriptor
        struct cudaResourceDesc resDesc = {};
        resDesc.resType         = cudaResourceTypeArray;
        resDesc.res.array.array = cu_array;

        // [B6-T34] 3c: texture descriptor (bilinear filter + normalized coords + clamp)
        struct cudaTextureDesc texDesc = {};
        texDesc.addressMode[0]   = cudaAddressModeClamp;
        texDesc.addressMode[1]   = cudaAddressModeClamp;
        texDesc.filterMode       = cudaFilterModeLinear;   // [B6-T35] bilinear filter
        texDesc.readMode         = cudaReadModeElementType;
        texDesc.normalizedCoords = 1;                      // [B6-T36] normalized coords

        // [B6-T37] 3d: create the texture object
        cudaTextureObject_t texObj = 0;
        // TODO [REQUIRED]: CUDA_CHECK(cudaCreateTextureObject(&texObj, &resDesc, &texDesc, NULL));
        CUDA_CHECK(cudaCreateTextureObject(&texObj, &resDesc, &texDesc, NULL));

        // [B6-T38] output (same size, used for verification)
        float* d_tex_out = nullptr;
        CUDA_CHECK(cudaMalloc(&d_tex_out, BYTES));

        float gbps_tex = bench_gbps([&] {
            texture_sample_kernel<<<grid, block>>>(texObj, d_tex_out,
                                                   IMG_W, IMG_H,
                                                   IMG_W, IMG_H);
        }, BYTES);

        // [B6-T39]
        std::printf("[B6] texture sample throughput:    %.1f GB/s\n", gbps_tex);

        // [B6-T40] TODO [REQUIRED] step 5: verify sampling result (compare with direct read)
        float* h_tex_out = (float*)std::malloc(BYTES);
        CUDA_CHECK(cudaMemcpy(h_tex_out, d_tex_out, BYTES, cudaMemcpyDeviceToHost));
        float max_err = 0.0f;
        for (int i = 0; i < total; i++) {
            float err = fabsf(h_tex_out[i] - h_input[i]);
            if (err > max_err) max_err = err;
        }
        // [B6-T41]
        std::printf("[B6] texture sample max error:     %.6f (small bilinear interp error allowed)\n", max_err);
        std::free(h_tex_out);

        // [B6-T42] 3e: cleanup texture resources
        CUDA_CHECK(cudaDestroyTextureObject(texObj));
        CUDA_CHECK(cudaFreeArray(cu_array));
        CUDA_CHECK(cudaFree(d_tex_out));
    }

    // [B6-T43] -- advanced hints --
    std::puts("------------------------------------------------------------");
    // [B6-T44]
    std::puts("  TODO [ADVANCED] use constant memory to store an LUT for table-lookup acceleration.");
    // [B6-T45]
    std::puts("  TODO [ADVANCED] try border/repeat addressing modes in texture sampling.");
    // [B6-T46]
    std::puts("  TODO [ADVANCED] compare cudaTextureObject_t with the legacy texture reference API.");
    std::puts("------------------------------------------------------------");

    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
    CUDA_CHECK(cudaFree(d_kernel_g));
    std::free(h_input);

    std::puts("[B6_texture_and_constant] DONE");
    return 0;
}
