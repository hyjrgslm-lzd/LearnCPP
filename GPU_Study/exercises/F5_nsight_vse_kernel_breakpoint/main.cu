// ============================================================
// [F5-T01] Exercise F5: Nsight VSE kernel breakpoints + register inspection
// [F5-T02] Goals:
//   - 2D Gaussian blur kernel (512x512), good for IDE breakpoint demo
//   - Set breakpoints on kernel hot lines in Nsight VSE
//   - Step through, view warp/lane register values ($r0, $r1 ...)
//   - Inspect CUDA Warp Info / Lane Info windows
// [F5-T03] Build (Debug):  cmake --build build --config Debug --target F5_nsight_vse_kernel_breakpoint
// [F5-T04] Run:            ./F5_nsight_vse_kernel_breakpoint
// [F5-T05] Nsight VSE debug flow (see README.md for details):
//   1. compile in Debug (no Release optimizations)
//   2. set breakpoint at line ~90 in main.cu (gaussian_blur inner-product loop)
//   3. menu Extensions -> Nsight -> Start CUDA Debugging
//   4. once stopped on breakpoint, open CUDA Warp Info / Lane Info windows
//   5. inspect register values for the current warp, single-step
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [F5-T06] Constants
// ------------------------------------------------------------
constexpr int IMG_W     = 512;  // [F5-T07] image width (pixels)
constexpr int IMG_H     = 512;  // [F5-T08] image height (pixels)
constexpr int RADIUS    = 3;    // [F5-T09] Gaussian conv radius (kernel size = 2*RADIUS+1 = 7)
constexpr int TILE_W    = 16;   // [F5-T10] tile width (with halo = TILE_W + 2*RADIUS)
constexpr int TILE_H    = 16;   // [F5-T11] tile height

// ------------------------------------------------------------
// [F5-T12] Device-side: 2D Gaussian blur kernel
//
//   Uses shared memory tiling to reduce global accesses:
//   - each block produces TILE_W x TILE_H output region
//   - loads (TILE_W + 2*RADIUS) x (TILE_H + 2*RADIUS) input tile
//   - each thread computes one output pixel: weighted sum over (2R+1)^2
//
//   Recommended breakpoint: the inner-product line (TODO [REQUIRED-2])
//   -> Lane Info window will show acc register changing over k iterations
//
// [F5-T13] TODO [REQUIRED-1] complete smem load + convolution inner product
// ------------------------------------------------------------
__global__ void gaussian_blur(
    const float* __restrict__ src,  // [F5-T14] input image, row-major, IMG_H x IMG_W
    float* __restrict__       dst,  // [F5-T15] output image
    const float* __restrict__ kern, // [F5-T16] Gaussian kernel, (2R+1)^2 coefficients
    int width,
    int height,
    int radius)
{
    // [F5-T17] shared memory tile (with halo)
    extern __shared__ float smem[]; // [F5-T18] size = (TILE_W+2*radius)*(TILE_H+2*radius)
    const int smem_w = blockDim.x + 2 * radius;
    const int smem_h = blockDim.y + 2 * radius;

    int out_x = blockIdx.x * blockDim.x + threadIdx.x;
    int out_y = blockIdx.y * blockDim.y + threadIdx.y;

    // [F5-T19] --- Load shared memory tile (with boundary clamp) ---
    // [F5-T20] TODO [REQUIRED-1]: load (smem_w x smem_h) region from src into smem
    //   each thread loads one or more smem elements (multi-iter to cover halo)
    //   boundary handling: clamp to [0, width-1] / [0, height-1]
    //
    //   Reference skeleton:
    //   for (int dy = threadIdx.y; dy < smem_h; dy += blockDim.y) {
    //       for (int dx = threadIdx.x; dx < smem_w; dx += blockDim.x) {
    //           int sx = blockIdx.x * blockDim.x + dx - radius;
    //           int sy = blockIdx.y * blockDim.y + dy - radius;
    //           sx = max(0, min(sx, width  - 1));
    //           sy = max(0, min(sy, height - 1));
    //           smem[dy * smem_w + dx] = src[sy * width + sx];
    //       }
    //   }
    (void)smem_w; (void)smem_h; // stub -- remove when implementing

    __syncthreads();

    if (out_x >= width || out_y >= height) return;

    float acc = 0.0f;
    // [F5-T21] TODO [REQUIRED-2]: convolution inner product -- set the breakpoint HERE,
    //   observe acc register changes
    //   int ksize = 2 * radius + 1;
    //   for (int ky = 0; ky < ksize; ++ky) {
    //       for (int kx = 0; kx < ksize; ++kx) {
    //           float pixel = smem[(threadIdx.y + ky) * smem_w + (threadIdx.x + kx)];
    //           float weight = kern[ky * ksize + kx];
    //           acc += pixel * weight;  // <-- recommended breakpoint line
    //       }
    //   }
    //   dst[out_y * width + out_x] = acc;
    dst[out_y * width + out_x] = acc; // stub
}

// ------------------------------------------------------------
// [F5-T22] Host: build normalized Gaussian kernel (sigma=1.0)
// ------------------------------------------------------------
static void make_gaussian_kernel(float* kern, int radius, float sigma)
{
    int ksize = 2 * radius + 1;
    float sum = 0.0f;
    for (int ky = 0; ky < ksize; ++ky) {
        for (int kx = 0; kx < ksize; ++kx) {
            float dy = static_cast<float>(ky - radius);
            float dx = static_cast<float>(kx - radius);
            float val = std::exp(-(dx * dx + dy * dy) / (2.0f * sigma * sigma));
            kern[ky * ksize + kx] = val;
            sum += val;
        }
    }
    // [F5-T23] normalize
    for (int i = 0; i < ksize * ksize; ++i) kern[i] /= sum;
}

// ------------------------------------------------------------
// [F5-T24] main
// ------------------------------------------------------------
int main()
{
    std::puts("[F5_nsight_vse_kernel_breakpoint]");
    print_device_info(0);

    NVTX_RANGE("F5/main");

    // ------------------------------------------------------------
    // [F5-T25] Allocate memory
    // ------------------------------------------------------------
    const size_t img_bytes  = static_cast<size_t>(IMG_W) * IMG_H * sizeof(float);
    const int    ksize      = 2 * RADIUS + 1;
    const size_t kern_bytes = static_cast<size_t>(ksize) * ksize * sizeof(float);

    float* h_src  = new float[IMG_W * IMG_H];
    float* h_dst  = new float[IMG_W * IMG_H];
    float* h_kern = new float[ksize * ksize];

    // [F5-T26] generate test image (gradient + a few bright spots, easy to eyeball when debugging)
    for (int y = 0; y < IMG_H; ++y) {
        for (int x = 0; x < IMG_W; ++x) {
            float base = (static_cast<float>(x + y) / (IMG_W + IMG_H));
            // [F5-T27] a few bright spots
            float spot = 0.0f;
            if ((x - 128) * (x - 128) + (y - 128) * (y - 128) < 25) spot = 1.0f;
            if ((x - 384) * (x - 384) + (y - 256) * (y - 256) < 16) spot = 0.8f;
            h_src[y * IMG_W + x] = base * 0.5f + spot;
        }
    }

    // [F5-T28] build Gaussian kernel (sigma=1.0)
    make_gaussian_kernel(h_kern, RADIUS, 1.0f);

    printf("\n  image: %dx%d  Gaussian radius=%d  kernel size=%dx%d\n",
           IMG_W, IMG_H, RADIUS, ksize, ksize);

    float* d_src  = nullptr;
    float* d_dst  = nullptr;
    float* d_kern = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src,  img_bytes));
    CUDA_CHECK(cudaMalloc(&d_dst,  img_bytes));
    CUDA_CHECK(cudaMalloc(&d_kern, kern_bytes));

    CUDA_CHECK(cudaMemcpy(d_src,  h_src,  img_bytes,  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_kern, h_kern, kern_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_dst, 0, img_bytes));

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [F5-T29] Launch Gaussian blur kernel
    // ------------------------------------------------------------
    printf("\n--- Gaussian blur kernel (512x512, radius=%d) ---\n", RADIUS);
    {
        NVTX_RANGE_COLOR("F5/gaussian_blur", 0xFF4080FF);

        dim3 block(TILE_W, TILE_H);
        dim3 grid((IMG_W + TILE_W - 1) / TILE_W,
                  (IMG_H + TILE_H - 1) / TILE_H);

        // [F5-T30] shared memory size = (tile + 2*radius)^2 * sizeof(float)
        size_t smem_size = static_cast<size_t>(TILE_W + 2 * RADIUS)
                         * (TILE_H + 2 * RADIUS)
                         * sizeof(float);

        printf("  launch: grid=(%d,%d) block=(%d,%d) smem=%zu B\n",
               grid.x, grid.y, block.x, block.y, smem_size);
        printf("  Nsight VSE breakpoint hint: line ~90 in this file (acc += ... inside inner product)\n");

        timer.start();
        // [F5-T31] TODO [REQUIRED-2]: set breakpoint on gaussian_blur inner product line,
        //   start CUDA Debugging in Nsight VSE; once stopped:
        //   - open CUDA Warp Info, observe currently active warp
        //   - open Lane Info, view per-lane acc register values
        //   - single-step a few times, watch acc accumulate over k
        gaussian_blur<<<grid, block, smem_size>>>(
            d_src, d_dst, d_kern, IMG_W, IMG_H, RADIUS);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();
        printf("  time=%.3f ms\n", timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [F5-T32] Result check (output should not be all zeros)
    // ------------------------------------------------------------
    CUDA_CHECK(cudaMemcpy(h_dst, d_dst, img_bytes, cudaMemcpyDeviceToHost));
    float sum = 0.0f;
    for (int i = 0; i < IMG_W * IMG_H; ++i) sum += h_dst[i];
    printf("  output pixel sum = %.4f (stub expects 0; should be > 0 once complete)\n", sum);

    // ------------------------------------------------------------
    // [F5-T33] TODO [REQUIRED-1] complete gaussian_blur (smem load + inner product)
    // [F5-T34] TODO [REQUIRED-2] in Nsight VSE, set breakpoint and run CUDA Debugging, record:
    //   - block(x,y) / warp / lane info at the hit
    //   - initial acc value and after a few iterations
    //   - smem pixel values vs corresponding h_src region
    // [F5-T35] TODO [REQUIRED-3] switch between warps (CUDA Warp Info), compare register diffs

    // [F5-T36] TODO [ADVANCED-1] conditional breakpoint (stop only when out_x==256 && out_y==256)
    // [F5-T37] TODO [ADVANCED-2] do the same flow with CUDA-GDB (Linux), compare CLI vs GUI
    // [F5-T38] TODO [ADVANCED-3] modify a pixel in h_src, observe whether smem load reflects it correctly

    // ------------------------------------------------------------
    // [F5-T39] Cleanup
    // ------------------------------------------------------------
    CUDA_CHECK(cudaFree(d_src));
    CUDA_CHECK(cudaFree(d_dst));
    CUDA_CHECK(cudaFree(d_kern));
    delete[] h_src;
    delete[] h_dst;
    delete[] h_kern;

    // [F5-T40]
    printf("\n[F5] done. Build with Debug config; follow README.md to debug the kernel in Nsight VSE.\n");
    return 0;
}
