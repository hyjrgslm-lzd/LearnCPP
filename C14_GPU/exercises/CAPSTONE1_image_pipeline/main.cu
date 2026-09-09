// CAPSTONE1_image_pipeline/main.cu
// [CAP1-T01] Capstone Project 1: data-parallel pipeline and performance baseline.
//
// [CAP1-T02] This file is a complete scaffold. Every kernel body has TODO
//            markers; students must fill in the actual algorithms. The host
//            code compiles and runs as-is, kernels emit zero placeholders, and
//            acceptance assertions fail visibly to remind students to finish
//            the implementation.
//
// [CAP1-T03] Pipeline stages:
//   Stage 1  normalize_kernel        -- Welford online mean/variance + normalize
//   Stage 2  histogram_kernel        -- atomic version vs privatized version
//   Stage 3  conv2d_kernel           -- 3x3 Gaussian + shared-mem halo tile
//   Stage 4  scan_kernel             -- Hillis-Steele vs Blelloch
//   Stage 5  radix_sort_kernel       -- digit histogram + scan + scatter
//
// [CAP1-T04] Build:
//   cmake --preset vs2026
//   cmake --build build-vs2026 --config Release --target CAPSTONE1_image_pipeline
// [CAP1-T05] Run:
//   build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 4096 --height 4096
//   build-vs2026\Release\CAPSTONE1_image_pipeline.exe --use-graph --iters 50

#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------
// [CAP1-T06] Compile-time constants
// ---------------------------------------------------------
constexpr int   CHANNELS        = 3;       // [CAP1-T07] RGB three channels
constexpr int   HIST_BINS       = 256;     // [CAP1-T08] histogram bin count
constexpr int   BLOCK_DIM_X     = 16;      // [CAP1-T09] conv2d / generic 2D tile width
constexpr int   BLOCK_DIM_Y     = 16;      // [CAP1-T10] conv2d / generic 2D tile height
constexpr int   BLOCK_1D        = 256;     // [CAP1-T11] default block size for 1D kernels
constexpr int   CONV_KERNEL_R   = 1;       // [CAP1-T12] convolution kernel radius, 3x3 => R=1
constexpr int   CONV_KERNEL_SIZE= 2 * CONV_KERNEL_R + 1;   // = 3
constexpr float NORM_EPS        = 1e-6f;   // [CAP1-T13] denominator guard for normalization
constexpr int   RADIX_BITS      = 4;       // [CAP1-T14] bits processed per radix-sort pass
constexpr int   RADIX_BUCKETS   = 1 << RADIX_BITS; // = 16

// ---------------------------------------------------------
// [CAP1-T15] 3x3 Gaussian convolution kernel (host-side; copy to device
//            constant memory or pass as a kernel argument).
// ---------------------------------------------------------
static const float h_gaussian3x3[CONV_KERNEL_SIZE * CONV_KERNEL_SIZE] = {
    1.0f/16, 2.0f/16, 1.0f/16,
    2.0f/16, 4.0f/16, 2.0f/16,
    1.0f/16, 2.0f/16, 1.0f/16,
};

// ---------------------------------------------------------
// [CAP1-T16] CLI argument struct
// ---------------------------------------------------------
struct CliArgs {
    int  width        = 4096;
    int  height       = 4096;
    int  iters        = 10;
    bool use_graph    = false;
    int  stream_count = 2;
};

// ---------------------------------------------------------
// [CAP1-T17] Per-stage performance record
// ---------------------------------------------------------
struct StagePerf {
    const char* name;
    float       ms;
    double      bytes_moved;   // [CAP1-T18] bytes read + written
    double      gbs;           // [CAP1-T19] effective bandwidth in GB/s
    double      ai;            // [CAP1-T20] arithmetic intensity FLOP/byte (filled by roofline helper)
};

// ---------------------------------------------------------
// [CAP1-T21] Print usage instructions
// ---------------------------------------------------------
static void print_usage(const char* prog)
{
    // [CAP1-T22]
    printf("Usage: %s [options]\n\n", prog);
    printf("Options:\n");
    // [CAP1-T23]
    printf("  --width  N        image width in pixels, default 4096\n");
    // [CAP1-T24]
    printf("  --height N        image height in pixels, default 4096\n");
    // [CAP1-T25]
    printf("  --iters  N        repeat count (used for CUDA Graph replay), default 10\n");
    // [CAP1-T26]
    printf("  --use-graph       wrap pipeline in a CUDA Graph (default false)\n");
    // [CAP1-T27]
    printf("  --stream-count N  concurrent stream count, default 2\n");
    // [CAP1-T28]
    printf("\nExamples:\n");
    printf("  %s --width 4096 --height 4096 --use-graph\n", prog);
    printf("  %s --width 8192 --height 8192 --iters 50 --stream-count 4\n", prog);
}

// ---------------------------------------------------------
// [CAP1-T29] CLI parsing
// ---------------------------------------------------------
static CliArgs parse_args(int argc, char* argv[])
{
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            print_usage(argv[0]);
            exit(0);
        } else if (a == "--width" && i + 1 < argc) {
            args.width = std::atoi(argv[++i]);
        } else if (a == "--height" && i + 1 < argc) {
            args.height = std::atoi(argv[++i]);
        } else if (a == "--iters" && i + 1 < argc) {
            args.iters = std::atoi(argv[++i]);
        } else if (a == "--use-graph") {
            args.use_graph = true;
        } else if (a == "--stream-count" && i + 1 < argc) {
            args.stream_count = std::atoi(argv[++i]);
        } else {
            // [CAP1-T30]
            printf("[warning] unknown argument: %s\n", a.c_str());
        }
    }
    return args;
}

// ---------------------------------------------------------
// [CAP1-T31] Host-side random data generation.
//            Produces a height x width x CHANNELS float32 image with values in [0, 1).
// ---------------------------------------------------------
static void generate_image(float* data, int height, int width)
{
    std::mt19937_64 rng(42ULL);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const long long total = (long long)height * width * CHANNELS;
    for (long long i = 0; i < total; ++i) {
        data[i] = dist(rng);
    }
}

// ---------------------------------------------------------
// [CAP1-T32] Stage 1: normalization kernel (Welford online algorithm).
//
//   Inputs:  in              -- raw float image, [H, W, C] packed
//            H, W, C         -- dimensions
//   Outputs: out             -- normalized image (same dims)
//            channel_mean    -- per-channel mean, length C
//            channel_var     -- per-channel variance, length C
//
//   Maps to: 08-capstone-project-1 required step 2.
// ---------------------------------------------------------
__global__ void normalize_kernel(
    const float* __restrict__ in,
    float*       __restrict__ out,
    int H, int W, int C,
    float* __restrict__ channel_mean,
    float* __restrict__ channel_var)
{
    // [CAP1-T33] TODO [REQUIRED] step 2-a: determine the pixel coordinate
    //   (row, col, ch) handled by the current thread.
    //   int row = blockIdx.y * blockDim.y + threadIdx.y;
    //   int col = blockIdx.x * blockDim.x + threadIdx.x;
    //   int ch  = blockIdx.z;   // each z-block handles one channel

    // [CAP1-T34] TODO [REQUIRED] step 2-b: single-pass Welford reduce -- each
    //   thread keeps a local (count, mean, M2).
    //   Welford update formulas:
    //     count += 1
    //     delta  = x - mean
    //     mean  += delta / count
    //     delta2 = x - mean
    //     M2    += delta * delta2
    //   variance = M2 / count (population) or M2 / (count - 1) (sample).
    //
    //   Use warp shuffle + shared memory for a two-level reduce; thread 0
    //   finally writes channel_mean[ch] and channel_var[ch].

    // [CAP1-T35] TODO [REQUIRED] step 2-c: __syncthreads() so channel_mean /
    //   channel_var are visible, then normalize each pixel:
    //     out[idx] = (in[idx] - mean) / sqrtf(var + NORM_EPS)

    // [CAP1-T36] TODO [ADVANCED] FP16 rewrite: replace float with __half /
    //   __half2 and observe the throughput change.

    // [CAP1-T37] Placeholder: outputs all zero; the acceptance assertion will
    //   visibly fail until students fill this in.
    (void)in; (void)out; (void)H; (void)W; (void)C;
    (void)channel_mean; (void)channel_var;
}

// ---------------------------------------------------------
// [CAP1-T38] Stage 2-V1: atomic histogram (global-memory atomicAdd; baseline).
//
//   Maps to: 08-capstone-project-1 required step 3 (V1 baseline).
// ---------------------------------------------------------
__global__ void histogram_atomic_kernel(
    const float* __restrict__ data,
    int N_elements,
    int* __restrict__ hist,
    int num_bins)
{
    // [CAP1-T39] TODO [REQUIRED] step 3-a: compute the global thread index idx.
    //   int idx = blockIdx.x * blockDim.x + threadIdx.x;
    //   if (idx >= N_elements) return;

    // [CAP1-T40] TODO [REQUIRED] step 3-b: map a float pixel to a bin index.
    //   After normalization the values sit roughly in [-3, 3] (about +/- 3 sigma);
    //   formula: bin = (int)((data[idx] + 3.0f) / 6.0f * num_bins),
    //   clamped to [0, num_bins - 1].

    // [CAP1-T41] TODO [REQUIRED] step 3-c: global atomic accumulation.
    //   atomicAdd(&hist[bin], 1);
    //
    //   Observation: inspect l2_global_atomic_store in Nsight Compute; under
    //   high contention this version serializes badly.

    (void)data; (void)N_elements; (void)hist; (void)num_bins;
}

// ---------------------------------------------------------
// [CAP1-T42] Stage 2-V2: privatized histogram (per-block shared mem +
//            final atomic merge).
//
//   Maps to: 08-capstone-project-1 required step 3 (V2 optimized).
// ---------------------------------------------------------
__global__ void histogram_privatized_kernel(
    const float* __restrict__ data,
    int N_elements,
    int* __restrict__ hist,
    int num_bins)
{
    // [CAP1-T43] TODO [REQUIRED] step 3-d: declare the per-block local
    //   histogram in shared memory.
    //   extern __shared__ int s_hist[];
    //   (launch with num_bins * sizeof(int) bytes of dynamic shared memory)

    // [CAP1-T44] TODO [REQUIRED] step 3-e: zero-initialize s_hist.
    //   for (int i = threadIdx.x; i < num_bins; i += blockDim.x)
    //       s_hist[i] = 0;
    //   __syncthreads();

    // [CAP1-T45] TODO [REQUIRED] step 3-f: each thread atomicAdds into s_hist
    //   (shared-memory atomics are dramatically faster).

    // [CAP1-T46] TODO [REQUIRED] step 3-g: __syncthreads() and merge s_hist
    //   into the global hist.
    //   for (int i = threadIdx.x; i < num_bins; i += blockDim.x)
    //       atomicAdd(&hist[i], s_hist[i]);
    //
    //   Observation: compare V1's l2_global_atomic_store against V2's
    //   shared_load/store metrics.

    (void)data; (void)N_elements; (void)hist; (void)num_bins;
}

// ---------------------------------------------------------
// [CAP1-T47] Stage 3: 2D convolution kernel (3x3 Gaussian, shared-mem halo tile).
//
//   Maps to: 08-capstone-project-1 required step 4.
// ---------------------------------------------------------
__global__ void conv2d_kernel(
    const float* __restrict__ in,
    float*       __restrict__ out,
    int H, int W,
    const float* __restrict__ kernel_weights,
    int K_radius)
{
    // [CAP1-T48] TODO [REQUIRED] step 4-a: compute tile dimensions (with halo).
    //   tile_w = BLOCK_DIM_X + 2 * K_radius
    //   tile_h = BLOCK_DIM_Y + 2 * K_radius
    //   Compute shared-memory size dynamically at launch from K_radius, or
    //   declare statically:
    //   __shared__ float smem[(BLOCK_DIM_Y + 2*CONV_KERNEL_R)
    //                       * (BLOCK_DIM_X + 2*CONV_KERNEL_R)];

    // [CAP1-T49] TODO [REQUIRED] step 4-b: cooperatively load halo + tile
    //   pixels into shared memory. Boundary handling: zero-padding for
    //   out-of-range pixels. Multiple threads share halo row/column loads to
    //   avoid divergence.

    // [CAP1-T50] TODO [REQUIRED] step 4-c: __syncthreads() and run the 3x3
    //   convolution.
    //   float sum = 0.0f;
    //   for (int ky = -K_radius; ky <= K_radius; ++ky)
    //       for (int kx = -K_radius; kx <= K_radius; ++kx)
    //           sum += kernel_weights[(ky+K_radius)*CONV_KERNEL_SIZE + (kx+K_radius)]
    //                  * smem[(ty+K_radius+ky) * tile_w + (tx+K_radius+kx)];
    //   out[row*W + col] = sum;

    // [CAP1-T51] TODO [ADVANCED] 5x5 Gaussian: K_radius=2; adjust shared mem
    //   size and observe smem occupancy changes.

    (void)in; (void)out; (void)H; (void)W;
    (void)kernel_weights; (void)K_radius;
}

// ---------------------------------------------------------
// [CAP1-T52] Stage 4-V1: Hillis-Steele inclusive scan (O(n log n) work,
//            high parallelism).
//
//   Maps to: 08-capstone-project-1 required step 5 (V1).
// ---------------------------------------------------------
__global__ void scan_hillis_steele_kernel(
    const float* __restrict__ in,
    float*       __restrict__ out,
    int N)
{
    // [CAP1-T53] TODO [REQUIRED] step 5-a: each block handles one segment;
    //   this is the single-block demo.
    //   extern __shared__ float s[];

    // [CAP1-T54] TODO [REQUIRED] step 5-b: load data into shared memory.
    //   int tid = threadIdx.x;
    //   s[tid] = (tid < N) ? in[tid] : 0.0f;
    //   __syncthreads();

    // [CAP1-T55] TODO [REQUIRED] step 5-c: Hillis-Steele iteration.
    //   for (int stride = 1; stride < blockDim.x; stride <<= 1) {
    //       float val = (tid >= stride) ? s[tid - stride] : 0.0f;
    //       __syncthreads();
    //       s[tid] += val;
    //       __syncthreads();
    //   }
    //   Note: Hillis-Steele needs double-buffered shared memory to avoid
    //   reading and writing the same buffer, or use the "read-old / sync /
    //   write-new" two-step approach above.

    // [CAP1-T56] TODO [REQUIRED] step 5-d: write back to out.
    //   if (tid < N) out[tid] = s[tid];

    // [CAP1-T57] TODO [ADVANCED] multi-block version: store each block's sum
    //   in an auxiliary array, scan it, then scatter back into each block.

    (void)in; (void)out; (void)N;
}

// ---------------------------------------------------------
// [CAP1-T58] Stage 4-V2: Blelloch work-efficient inclusive scan (O(n) work).
//
//   Maps to: 08-capstone-project-1 required step 5 (V2).
// ---------------------------------------------------------
__global__ void scan_blelloch_kernel(
    const float* __restrict__ in,
    float*       __restrict__ out,
    int N)
{
    // [CAP1-T59] TODO [REQUIRED] step 5-e: load into shared mem (same as Hillis-Steele).

    // [CAP1-T60] TODO [REQUIRED] step 5-f: up-sweep (reduce) phase.
    //   for (int stride = 1; stride < blockDim.x; stride <<= 1) {
    //       int idx = (tid + 1) * 2 * stride - 1;
    //       if (idx < blockDim.x)
    //           s[idx] += s[idx - stride];
    //       __syncthreads();
    //   }

    // [CAP1-T61] TODO [REQUIRED] step 5-g: set the root to identity (exclusive
    //   scan) or keep it (inclusive scan).
    //   if (tid == 0) s[blockDim.x - 1] = 0.0f;  // exclusive scan
    //   __syncthreads();

    // [CAP1-T62] TODO [REQUIRED] step 5-h: down-sweep (distribution) phase.
    //   for (int stride = blockDim.x >> 1; stride >= 1; stride >>= 1) {
    //       int idx = (tid + 1) * 2 * stride - 1;
    //       if (idx < blockDim.x) {
    //           float t = s[idx - stride];
    //           s[idx - stride] = s[idx];
    //           s[idx] += t;
    //       }
    //       __syncthreads();
    //   }

    // [CAP1-T63] TODO [REQUIRED] step 5-i: write back to out (inclusive =
    //   exclusive + original value).

    (void)in; (void)out; (void)N;
}

// ---------------------------------------------------------
// [CAP1-T64] Stage 5: radix sort -- digit histogram kernel (step 1).
//
//   Maps to: 08-capstone-project-1 required step 6 (manual route).
// ---------------------------------------------------------
__global__ void radix_digit_hist_kernel(
    const unsigned int* __restrict__ keys,
    int N,
    int* __restrict__ digit_hist,
    int bit_shift)
{
    // [CAP1-T65] TODO [REQUIRED] step 6-a: each thread reads one key and
    //   extracts the digit for the current pass.
    //   int idx = blockIdx.x * blockDim.x + threadIdx.x;
    //   if (idx >= N) return;
    //   unsigned int digit = (keys[idx] >> bit_shift) & (RADIX_BUCKETS - 1);

    // [CAP1-T66] TODO [REQUIRED] step 6-b: per-block shared-memory histogram
    //   (similar to Stage 2 privatized).
    //   __shared__ int s_hist[RADIX_BUCKETS];
    //   if (threadIdx.x < RADIX_BUCKETS) s_hist[threadIdx.x] = 0;
    //   __syncthreads();
    //   atomicAdd(&s_hist[digit], 1);
    //   __syncthreads();

    // [CAP1-T67] TODO [REQUIRED] step 6-c: merge the block histogram into the
    //   global one. digit_hist shape: [gridDim.x, RADIX_BUCKETS], one segment
    //   per block.
    //   if (threadIdx.x < RADIX_BUCKETS)
    //       digit_hist[blockIdx.x * RADIX_BUCKETS + threadIdx.x] = s_hist[threadIdx.x];

    (void)keys; (void)N; (void)digit_hist; (void)bit_shift;
}

// ---------------------------------------------------------
// [CAP1-T68] Stage 5: radix sort -- scatter kernel (step 3).
//
//   Maps to: 08-capstone-project-1 required step 6.
// ---------------------------------------------------------
__global__ void radix_scatter_kernel(
    const unsigned int* __restrict__ keys_in,
    const unsigned int* __restrict__ vals_in,
    unsigned int*       __restrict__ keys_out,
    unsigned int*       __restrict__ vals_out,
    int N,
    const int* __restrict__ prefix_sums,   // [CAP1-T69] scan result: global start offset for each digit
    int bit_shift)
{
    // [CAP1-T70] TODO [REQUIRED] step 6-d: each thread reads its key, derives
    //   the digit, looks up prefix_sums[digit] for the bucket start, and
    //   atomicAdds within the bucket to grab a slot (or uses the exclusive
    //   scan result directly).

    // [CAP1-T71] TODO [REQUIRED] step 6-e: write into keys_out / vals_out.
    //   scatter: keys_out[dst] = keys_in[idx]

    // [CAP1-T72] TODO [ADVANCED] swap in cub::DeviceRadixSort::SortPairs;
    //   compare against the manual implementation and explain why the library
    //   version is usually faster (better warp-level prefix and memory
    //   access pattern).

    (void)keys_in; (void)vals_in; (void)keys_out; (void)vals_out;
    (void)N; (void)prefix_sums; (void)bit_shift;
}

// ---------------------------------------------------------
// [CAP1-T73] CPU-side correctness checks (acceptance point 3).
//            Students fill in CPU references to compare against GPU output.
// ---------------------------------------------------------
static void verify_normalize_cpu(
    const float* input,
    const float* gpu_output,
    int H, int W, int C,
    float tol = 1e-5f)
{
    // [CAP1-T74] TODO [REQUIRED] step 2-d (acceptance):
    //   1. For each channel, compute CPU mean and variance with the standard
    //      two-pass formula.
    //   2. Normalize each pixel on the CPU.
    //   3. Compare element-wise against gpu_output; relative error must be <= tol.
    //
    //   Suggestion:
    //     double sum = 0, sum2 = 0;
    //     for (int i = 0; i < H*W; ++i) sum  += input[i*C + ch];
    //     for (int i = 0; i < H*W; ++i) sum2 += input[i*C + ch] * input[i*C + ch];
    //     double mean = sum / (H*W);
    //     double var  = sum2 / (H*W) - mean * mean;
    //     double cpu_out = (input[idx] - mean) / sqrt(var + NORM_EPS);
    //
    //   Current placeholder: print a warning that the check is skipped.
    // [CAP1-T75]
    printf("[verify_normalize] TODO [REQUIRED]: fill in the CPU reference; verification skipped.\n");
    (void)input; (void)gpu_output; (void)H; (void)W; (void)C; (void)tol;
}

static void verify_histogram_cpu(
    const float* data,
    int N_elements,
    const int* gpu_hist,
    int num_bins)
{
    // [CAP1-T76] TODO [REQUIRED] step 3-h (acceptance):
    //   Compute the histogram on the CPU and compare against gpu_hist exactly
    //   (int values, no tolerance). Confirm sum(gpu_hist) == N_elements so no
    //   pixel was dropped.
    // [CAP1-T77]
    printf("[verify_histogram] TODO [REQUIRED]: fill in the CPU reference; verification skipped.\n");
    (void)data; (void)N_elements; (void)gpu_hist; (void)num_bins;
}

static void verify_sort_cpu(
    const unsigned int* keys_sorted,
    int N)
{
    // [CAP1-T78] TODO [REQUIRED] step 6-f (acceptance):
    //   Confirm keys_sorted[i] <= keys_sorted[i+1] holds for every i.
    // [CAP1-T79]
    printf("[verify_sort] TODO [REQUIRED]: fill in the ordering check; verification skipped.\n");
    (void)keys_sorted; (void)N;
}

// ---------------------------------------------------------
// [CAP1-T80] Roofline helper: compute per-stage arithmetic intensity and
//            print a comparison table.
// ---------------------------------------------------------
static void print_roofline_table(
    const StagePerf* stages,
    int num_stages,
    int peak_bw_gbps)
{
    // [CAP1-T81] Theoretical FP32 peak (TFLOPS): pull from device properties;
    //   approximate value used here, students should refine.
    // [CAP1-T82] TODO [REQUIRED] step 8 (Roofline):
    //   Read multiProcessorCount * clockRate * 2 (FMA) * 32 (warp) from
    //   cudaDeviceProp to obtain the theoretical FP32 peak (TFLOPS), then
    //   plot the roofline together with peak_bw_gbps.
    // [CAP1-T83] Placeholder: Hopper H100 ~60 TFLOPS FP32.
    const double peak_fp32_tflops = 60.0;
    printf("\n");
    // [CAP1-T84]
    printf("+--------------------------+-----------+----------+----------+----------+----------+\n");
    printf("| stage                    |  time(ms) |  GB/s    | %%peak BW | AI(F/B)  | bound    |\n");
    printf("+--------------------------+-----------+----------+----------+----------+----------+\n");

    for (int i = 0; i < num_stages; ++i) {
        const StagePerf& s = stages[i];
        double pct_bw = (peak_bw_gbps > 0) ? (s.gbs / peak_bw_gbps * 100.0) : 0.0;
        // [CAP1-T85] Rough rule: AI < ridge_point => memory-bound, else compute-bound.
        //   ridge point = peak_fp32_tflops * 1000 / peak_bw_gbps (FLOP/byte)
        double ridge = (peak_bw_gbps > 0)
                       ? (peak_fp32_tflops * 1e3 / peak_bw_gbps)
                       : 0.0;
        const char* bound = (s.ai < ridge) ? "mem-bound" : "compute-bound";
        printf("| %-24s | %9.3f | %8.2f | %8.1f | %8.4f | %-8s |\n",
               s.name,
               (double)s.ms,
               s.gbs,
               pct_bw,
               s.ai,
               bound);
    }
    printf("+--------------------------+-----------+----------+----------+----------+----------+\n");
    // [CAP1-T86]
    printf("  theoretical peak BW: %d GB/s    theoretical FP32 peak: %.0f TFLOPS\n",
           peak_bw_gbps, peak_fp32_tflops);
    // [CAP1-T87]
    printf("  Roofline ridge point: %.2f FLOP/byte\n",
           (peak_bw_gbps > 0) ? (peak_fp32_tflops * 1e3 / peak_bw_gbps) : 0.0);
}

// ---------------------------------------------------------
// [CAP1-T88] Device-side pointer bundle (pipeline stage inputs/outputs).
// ---------------------------------------------------------
struct DeviceBuffers {
    float*        d_input       = nullptr;  // [CAP1-T89] raw image  [H*W*C]
    float*        d_normalized  = nullptr;  // normalized result [H*W*C]
    float*        d_channel_mean= nullptr;  // per-channel mean  [C]
    float*        d_channel_var = nullptr;  // per-channel variance [C]
    int*          d_hist_v1     = nullptr;  // histogram V1 [HIST_BINS]
    int*          d_hist_v2     = nullptr;  // histogram V2 [HIST_BINS]
    float*        d_conv_out    = nullptr;  // convolution output [H*W] (single channel)
    float*        d_conv_kernel = nullptr;  // 3x3 conv kernel [9]
    float*        d_scan_hs     = nullptr;  // Hillis-Steele scan output [H*W]
    float*        d_scan_bl     = nullptr;  // Blelloch scan output [H*W]
    unsigned int* d_sort_keys_in  = nullptr; // sort input  [H*W]
    unsigned int* d_sort_keys_out = nullptr; // sort output [H*W]
    unsigned int* d_sort_vals_in  = nullptr; // original indices [H*W]
    unsigned int* d_sort_vals_out = nullptr; // sorted indices   [H*W]
    int*          d_digit_hist    = nullptr; // radix digit hist (per-block)
    int*          d_prefix_sums   = nullptr; // scan result
};

static void alloc_device_buffers(DeviceBuffers& b, int H, int W)
{
    long long img_size  = (long long)H * W * CHANNELS * sizeof(float);
    long long single_ch = (long long)H * W * sizeof(float);
    long long keys_size = (long long)H * W * sizeof(unsigned int);

    // [CAP1-T90] Image buffers
    CUDA_CHECK(cudaMalloc(&b.d_input,        img_size));
    CUDA_CHECK(cudaMalloc(&b.d_normalized,   img_size));
    CUDA_CHECK(cudaMalloc(&b.d_channel_mean, CHANNELS * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&b.d_channel_var,  CHANNELS * sizeof(float)));

    // [CAP1-T91] Histograms
    CUDA_CHECK(cudaMalloc(&b.d_hist_v1, HIST_BINS * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&b.d_hist_v2, HIST_BINS * sizeof(int)));

    // [CAP1-T92] Convolution (single-channel conv on channel 0)
    CUDA_CHECK(cudaMalloc(&b.d_conv_out,    single_ch));
    CUDA_CHECK(cudaMalloc(&b.d_conv_kernel, CONV_KERNEL_SIZE * CONV_KERNEL_SIZE * sizeof(float)));

    // [CAP1-T93] Scan
    CUDA_CHECK(cudaMalloc(&b.d_scan_hs, single_ch));
    CUDA_CHECK(cudaMalloc(&b.d_scan_bl, single_ch));

    // [CAP1-T94] Radix sort
    CUDA_CHECK(cudaMalloc(&b.d_sort_keys_in,  keys_size));
    CUDA_CHECK(cudaMalloc(&b.d_sort_keys_out, keys_size));
    CUDA_CHECK(cudaMalloc(&b.d_sort_vals_in,  keys_size));
    CUDA_CHECK(cudaMalloc(&b.d_sort_vals_out, keys_size));

    // [CAP1-T95] Digit histogram: at most 65536 blocks * RADIX_BUCKETS.
    int max_blocks = ((H * W) + BLOCK_1D - 1) / BLOCK_1D;
    CUDA_CHECK(cudaMalloc(&b.d_digit_hist,   (long long)max_blocks * RADIX_BUCKETS * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&b.d_prefix_sums,  RADIX_BUCKETS * sizeof(int)));
}

static void free_device_buffers(DeviceBuffers& b)
{
    CUDA_CHECK(cudaFree(b.d_input));
    CUDA_CHECK(cudaFree(b.d_normalized));
    CUDA_CHECK(cudaFree(b.d_channel_mean));
    CUDA_CHECK(cudaFree(b.d_channel_var));
    CUDA_CHECK(cudaFree(b.d_hist_v1));
    CUDA_CHECK(cudaFree(b.d_hist_v2));
    CUDA_CHECK(cudaFree(b.d_conv_out));
    CUDA_CHECK(cudaFree(b.d_conv_kernel));
    CUDA_CHECK(cudaFree(b.d_scan_hs));
    CUDA_CHECK(cudaFree(b.d_scan_bl));
    CUDA_CHECK(cudaFree(b.d_sort_keys_in));
    CUDA_CHECK(cudaFree(b.d_sort_keys_out));
    CUDA_CHECK(cudaFree(b.d_sort_vals_in));
    CUDA_CHECK(cudaFree(b.d_sort_vals_out));
    CUDA_CHECK(cudaFree(b.d_digit_hist));
    CUDA_CHECK(cudaFree(b.d_prefix_sums));
}

// ---------------------------------------------------------
// [CAP1-T96] Single pipeline run organized over multiple streams.
//   stream 0: normalize -> histogram -> conv2d -> scan -> sort
//   stream 1: (optional) concurrent checksum kernel
//
//   Maps to: required step 7 (multi-stream concurrency).
// ---------------------------------------------------------
static void run_pipeline_streams(
    const DeviceBuffers& b,
    int H, int W,
    int stream_count,
    StagePerf* perfs)  // [CAP1-T97] output per-stage perf, length 5
{
    // [CAP1-T98] TODO [REQUIRED] step 7-a: create stream_count streams.
    //   cudaStream_t streams[stream_count];
    //   for (int i = 0; i < stream_count; ++i)
    //       CUDA_CHECK(cudaStreamCreate(&streams[i]));

    // [CAP1-T99] Use the default stream as a placeholder so the host code
    //   compiles before students wire up real streams.
    cudaStream_t stream0 = 0;

    const int HW  = H * W;
    const int HWC = H * W * CHANNELS;

    // ---------------------------------------
    // [CAP1-T100] Stage 1 -- Normalize
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/stage1_normalize");
        CudaEventTimer timer;

        // [CAP1-T101] Launch config
        dim3 block(BLOCK_DIM_X, BLOCK_DIM_Y);
        dim3 grid((W + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                  (H + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y,
                  CHANNELS);
        printf("[Stage1] normalize  grid=(%d,%d,%d) block=(%d,%d)\n",
               grid.x, grid.y, grid.z, block.x, block.y);

        timer.start(stream0);
        normalize_kernel<<<grid, block, 0, stream0>>>(
            b.d_input, b.d_normalized,
            H, W, CHANNELS,
            b.d_channel_mean, b.d_channel_var);
        CUDA_CHECK_LAST();
        timer.stop(stream0);

        float ms = timer.elapsed_ms();
        // [CAP1-T102] Read: HWC floats; write: HWC floats + 2*C floats (mean/var negligible).
        double bytes = 2.0 * HWC * sizeof(float);
        // [CAP1-T103] Per pixel: roughly 7 FLOPs (Welford update). TODO: fill in exact value.
        double flops = 7.0 * HWC;
        perfs[0] = {"normalize", ms, bytes, bytes / ms * 1e-6, flops / bytes};
    }

    // [CAP1-T104] TODO [REQUIRED] step 7-b: insert an event after Stage 1 so
    //   Stage 2 can wait on it.
    //   cudaEvent_t ev_norm_done;
    //   CUDA_CHECK(cudaEventCreate(&ev_norm_done));
    //   CUDA_CHECK(cudaEventRecord(ev_norm_done, stream0));
    //   CUDA_CHECK(cudaStreamWaitEvent(streams[1], ev_norm_done));

    // ---------------------------------------
    // [CAP1-T105] Stage 2 -- Histogram (V1 first, then V2; time V2 as the "optimized" version).
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/stage2_histogram");
        CudaEventTimer timer;

        // [CAP1-T106] After normalization pixels sit roughly in [-3, 3]; flatten to 1D and
        //   build a histogram over all channels.
        dim3 block(BLOCK_1D);
        dim3 grid((HWC + BLOCK_1D - 1) / BLOCK_1D);
        printf("[Stage2] histogram  grid=(%d) block=(%d)  shared_mem_v2=%zu B\n",
               grid.x, block.x, (size_t)HIST_BINS * sizeof(int));

        // [CAP1-T107] V1
        CUDA_CHECK(cudaMemsetAsync(b.d_hist_v1, 0, HIST_BINS * sizeof(int), stream0));
        histogram_atomic_kernel<<<grid, block, 0, stream0>>>(
            b.d_normalized, HWC, b.d_hist_v1, HIST_BINS);
        CUDA_CHECK_LAST();

        // [CAP1-T108] V2 (timed)
        CUDA_CHECK(cudaMemsetAsync(b.d_hist_v2, 0, HIST_BINS * sizeof(int), stream0));
        timer.start(stream0);
        histogram_privatized_kernel<<<grid, block, HIST_BINS * sizeof(int), stream0>>>(
            b.d_normalized, HWC, b.d_hist_v2, HIST_BINS);
        CUDA_CHECK_LAST();
        timer.stop(stream0);

        float ms = timer.elapsed_ms();
        double bytes = (double)HWC * sizeof(float) + HIST_BINS * sizeof(int);
        double flops = 3.0 * HWC;  // [CAP1-T109] mapping + clamp + atomicAdd (approx).
        perfs[1] = {"histogram_priv", ms, bytes, bytes / ms * 1e-6, flops / bytes};
    }

    // ---------------------------------------
    // [CAP1-T110] Stage 3 -- Conv2D (single-channel conv on channel 0).
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/stage3_conv2d");
        CudaEventTimer timer;

        dim3 block(BLOCK_DIM_X, BLOCK_DIM_Y);
        dim3 grid((W + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                  (H + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);
        // [CAP1-T111] Dynamic shared mem: tile with halo.
        size_t smem = (size_t)(BLOCK_DIM_X + 2 * CONV_KERNEL_R)
                    * (BLOCK_DIM_Y + 2 * CONV_KERNEL_R) * sizeof(float);
        printf("[Stage3] conv2d     grid=(%d,%d) block=(%d,%d) smem=%zu B\n",
               grid.x, grid.y, block.x, block.y, smem);

        timer.start(stream0);
        conv2d_kernel<<<grid, block, smem, stream0>>>(
            b.d_normalized,   // [CAP1-T112] take channel 0 (stride=CHANNELS, interleaved layout).
            b.d_conv_out,
            H, W,
            b.d_conv_kernel,
            CONV_KERNEL_R);
        CUDA_CHECK_LAST();
        timer.stop(stream0);

        float ms = timer.elapsed_ms();
        // [CAP1-T113] Each output pixel reads (3x3) input pixels + writes 1 output.
        double bytes = (double)H * W * (CONV_KERNEL_SIZE * CONV_KERNEL_SIZE + 1) * sizeof(float);
        double flops = (double)H * W * CONV_KERNEL_SIZE * CONV_KERNEL_SIZE * 2; // [CAP1-T114] MAC
        perfs[2] = {"conv2d_3x3", ms, bytes, bytes / ms * 1e-6, flops / bytes};
    }

    // ---------------------------------------
    // [CAP1-T115] Stage 4 -- Scan (Hillis-Steele first, then Blelloch; Blelloch is timed).
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/stage4_scan");
        CudaEventTimer timer;

        // [CAP1-T116] Single-block version: only scans min(HW, BLOCK_1D) elements as a demo.
        // [CAP1-T117] TODO [REQUIRED]: implement a multi-block version (using an auxiliary
        //   array) to support 4K x 4K.
        int scan_N = std::min(HW, BLOCK_1D);
        size_t smem = scan_N * sizeof(float);
        printf("[Stage4] scan       N=%d  smem=%zu B\n", scan_N, smem);

        // [CAP1-T118] Hillis-Steele
        scan_hillis_steele_kernel<<<1, scan_N, smem * 2, stream0>>>(
            b.d_conv_out, b.d_scan_hs, scan_N);
        CUDA_CHECK_LAST();

        // [CAP1-T119] Blelloch (timed)
        timer.start(stream0);
        scan_blelloch_kernel<<<1, scan_N, smem, stream0>>>(
            b.d_conv_out, b.d_scan_bl, scan_N);
        CUDA_CHECK_LAST();
        timer.stop(stream0);

        float ms = timer.elapsed_ms();
        double bytes = 2.0 * scan_N * sizeof(float);  // [CAP1-T120] read + write
        double flops = (double)scan_N * std::log2((double)scan_N); // [CAP1-T121] Blelloch O(n) approx
        perfs[3] = {"scan_blelloch", ms, bytes, bytes / ms * 1e-6, flops / bytes};
    }

    // ---------------------------------------
    // [CAP1-T122] Stage 5 -- Radix Sort (sort the conv output by float bit pattern).
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/stage5_radix_sort");
        CudaEventTimer timer;

        // [CAP1-T123] Reinterpret float as uint32 for sorting (demo only; real code must
        //   handle the sign bit).
        // [CAP1-T124] TODO [REQUIRED]: convert negative-float bit patterns to keep the
        //   ordering correct.
        int sort_N = HW;
        int grid_x = (sort_N + BLOCK_1D - 1) / BLOCK_1D;
        printf("[Stage5] radix_sort N=%d  grid=(%d) block=(%d)  passes=%d\n",
               sort_N, grid_x, BLOCK_1D, 32 / RADIX_BITS);

        timer.start(stream0);
        // [CAP1-T125] Each pass handles RADIX_BITS bits; total 32/RADIX_BITS = 8 passes.
        for (int pass = 0; pass < 32 / RADIX_BITS; ++pass) {
            int bit_shift = pass * RADIX_BITS;

            // [CAP1-T126] Step 1: digit histogram (per block).
            CUDA_CHECK(cudaMemsetAsync(b.d_digit_hist, 0,
                (long long)grid_x * RADIX_BUCKETS * sizeof(int), stream0));
            radix_digit_hist_kernel<<<grid_x, BLOCK_1D, 0, stream0>>>(
                b.d_sort_keys_in, sort_N, b.d_digit_hist, bit_shift);
            CUDA_CHECK_LAST();

            // [CAP1-T127] Step 2: scan (global prefix sum over digit_hist).
            // [CAP1-T128] TODO [REQUIRED]: call scan_blelloch_kernel or
            //   scan_hillis_steele_kernel; store each digit's global start offset
            //   in d_prefix_sums.

            // [CAP1-T129] Step 3: scatter
            radix_scatter_kernel<<<grid_x, BLOCK_1D, 0, stream0>>>(
                b.d_sort_keys_in, b.d_sort_vals_in,
                b.d_sort_keys_out, b.d_sort_vals_out,
                sort_N, b.d_prefix_sums, bit_shift);
            CUDA_CHECK_LAST();

            // [CAP1-T130] Swap in/out pointers (ping-pong buffer).
            // [CAP1-T131] TODO [REQUIRED]: swap d_sort_keys_in / d_sort_keys_out etc.
        }
        timer.stop(stream0);

        float ms = timer.elapsed_ms();
        double bytes = 2.0 * (32 / RADIX_BITS) * sort_N * sizeof(unsigned int) * 2; // [CAP1-T132] read + write
        double flops = (double)sort_N * (32 / RADIX_BITS) * 3; // [CAP1-T133] hist + scan + scatter approx
        perfs[4] = {"radix_sort", ms, bytes, bytes / ms * 1e-6, flops / bytes};
    }

    // [CAP1-T134] TODO [REQUIRED] step 7-c: destroy streams and events.
    //   for (int i = 0; i < stream_count; ++i)
    //       CUDA_CHECK(cudaStreamDestroy(streams[i]));

    (void)stream_count;
}

// ---------------------------------------------------------
// [CAP1-T135] CUDA Graph wrapped pipeline: stream capture + replay.
//
//   Maps to: required step 7 (CUDA Graph).
//   Advanced: branch C (explicit graph API).
// ---------------------------------------------------------
static void build_and_run_graph(
    const DeviceBuffers& b,
    int H, int W,
    int iters,
    StagePerf* perfs)
{
    // [CAP1-T136]
    printf("\n[Graph] beginning stream capture...\n");

    cudaStream_t capture_stream;
    CUDA_CHECK(cudaStreamCreate(&capture_stream));

    // [CAP1-T137] TODO [REQUIRED] step 7-d: begin capture on capture_stream.
    //   CUDA_CHECK(cudaStreamBeginCapture(capture_stream,
    //                                     cudaStreamCaptureModeGlobal));

    // [CAP1-T138] TODO [REQUIRED] step 7-e: launch every kernel in order on
    //   capture_stream. During capture you must not call cudaStreamSynchronize
    //   or any other host-sync API.

    // [CAP1-T139] TODO [REQUIRED] step 7-f: end capture and obtain the graph.
    //   cudaGraph_t graph;
    //   CUDA_CHECK(cudaStreamEndCapture(capture_stream, &graph));

    // [CAP1-T140] TODO [REQUIRED] step 7-g: instantiate the graph (JIT-compile to executable).
    //   cudaGraphExec_t graph_exec;
    //   CUDA_CHECK(cudaGraphInstantiate(&graph_exec, graph, nullptr, nullptr, 0));

    // [CAP1-T141] TODO [REQUIRED] step 7-h: replay iters times and measure total time.
    //   CudaEventTimer total_timer;
    //   total_timer.start(0);
    //   for (int i = 0; i < iters; ++i)
    //       CUDA_CHECK(cudaGraphLaunch(graph_exec, capture_stream));
    //   CUDA_CHECK(cudaStreamSynchronize(capture_stream));
    //   total_timer.stop(0);
    //   printf("[Graph] %d iters total %.3f ms, average %.3f ms/iter\n",
    //          iters, total_timer.elapsed_ms(), total_timer.elapsed_ms() / iters);

    // [CAP1-T142] TODO [ADVANCED] branch C: switch to the explicit graph API
    //   (cudaGraphCreate / cudaGraphAddKernelNode), build the DAG by hand,
    //   and use cudaGraphExecKernelNodeSetParams to update parameters dynamically.

    // [CAP1-T143] TODO [ADVANCED] branch D: replace Stage 5 with
    //   cub::DeviceRadixSort and compare the performance gap.

    // [CAP1-T144] Placeholder: run sequentially (replace once capture logic is filled in).
    printf("[Graph] TODO [REQUIRED]: fill in stream capture; falling back to sequential execution.\n");
    run_pipeline_streams(b, H, W, 1, perfs);

    CUDA_CHECK(cudaStreamDestroy(capture_stream));
    (void)iters;
}

// ---------------------------------------------------------
// [CAP1-T145] main
// ---------------------------------------------------------
int main(int argc, char* argv[])
{
    // ---------------------------------------
    // [CAP1-T146] Parse arguments
    // ---------------------------------------
    CliArgs args = parse_args(argc, argv);
    // [CAP1-T147]
    printf("=== CAPSTONE1 image statistics pipeline ===\n");
    // [CAP1-T148]
    printf("  resolution:  %d x %d  (%.1f MP)\n",
           args.width, args.height,
           (double)args.width * args.height / 1e6);
    // [CAP1-T149]
    printf("  channels:    %d (RGB)\n", CHANNELS);
    // [CAP1-T150]
    printf("  iterations:  %d\n", args.iters);
    // [CAP1-T151]
    printf("  CUDA Graph:  %s\n", args.use_graph ? "on" : "off");
    // [CAP1-T152]
    printf("  streams:     %d\n\n", args.stream_count);

    NVTX_RANGE("capstone/main");

    // ---------------------------------------
    // [CAP1-T153] Device info
    // ---------------------------------------
    print_device_info(0);
    int peak_bw = get_peak_memory_bandwidth_gbps(0);

    // ---------------------------------------
    // [CAP1-T154] Host-side data generation
    // ---------------------------------------
    {
        NVTX_RANGE("capstone/host_data_gen");
        long long total_floats = (long long)args.height * args.width * CHANNELS;
        long long total_bytes  = total_floats * sizeof(float);
        // [CAP1-T155]
        printf("[Host] allocating and generating random image %.2f MB...\n",
               (double)total_bytes / (1 << 20));

        std::vector<float> h_image(total_floats);
        generate_image(h_image.data(), args.height, args.width);

        // ---------------------------------------
        // [CAP1-T156] Device buffer allocation
        // ---------------------------------------
        DeviceBuffers bufs;
        alloc_device_buffers(bufs, args.height, args.width);

        // [CAP1-T157] H2D copy of the raw image
        CUDA_CHECK(cudaMemcpy(bufs.d_input, h_image.data(),
                              total_bytes, cudaMemcpyHostToDevice));

        // [CAP1-T158] Upload the convolution kernel
        CUDA_CHECK(cudaMemcpy(bufs.d_conv_kernel, h_gaussian3x3,
                              CONV_KERNEL_SIZE * CONV_KERNEL_SIZE * sizeof(float),
                              cudaMemcpyHostToDevice));

        // [CAP1-T159] Initialize the sort input by reinterpreting the normalized
        //   image's float bit pattern as uint32.
        // [CAP1-T160] TODO [REQUIRED]: feed the real normalize_kernel output as the
        //   sort input; the raw image is used as a placeholder for now.
        CUDA_CHECK(cudaMemcpy(bufs.d_sort_keys_in, h_image.data(),
                              (long long)args.height * args.width * sizeof(unsigned int),
                              cudaMemcpyHostToDevice));
        // [CAP1-T161] Initialize the index value array 0,1,2,...
        {
            std::vector<unsigned int> h_vals((size_t)args.height * args.width);
            for (unsigned int i = 0; i < (unsigned int)h_vals.size(); ++i)
                h_vals[i] = i;
            CUDA_CHECK(cudaMemcpy(bufs.d_sort_vals_in, h_vals.data(),
                                  h_vals.size() * sizeof(unsigned int),
                                  cudaMemcpyHostToDevice));
        }

        // ---------------------------------------
        // [CAP1-T162] Run the pipeline
        // ---------------------------------------
        StagePerf perfs[5] = {};

        if (args.use_graph) {
            NVTX_RANGE("capstone/graph_pipeline");
            build_and_run_graph(bufs, args.height, args.width,
                                args.iters, perfs);
        } else {
            NVTX_RANGE("capstone/stream_pipeline");
            run_pipeline_streams(bufs, args.height, args.width,
                                 args.stream_count, perfs);
        }

        // [CAP1-T163] Wait for all GPU work to finish
        CUDA_CHECK(cudaDeviceSynchronize());

        // ---------------------------------------
        // [CAP1-T164] Correctness verification (results visible once students
        //             fill in the verify functions).
        // ---------------------------------------
        {
            NVTX_RANGE("capstone/verify");
            // [CAP1-T165] D2H copy back of the normalized result
            std::vector<float> h_norm((long long)args.height * args.width * CHANNELS);
            CUDA_CHECK(cudaMemcpy(h_norm.data(), bufs.d_normalized,
                                  h_norm.size() * sizeof(float),
                                  cudaMemcpyDeviceToHost));
            verify_normalize_cpu(h_image.data(), h_norm.data(),
                                 args.height, args.width, CHANNELS);

            std::vector<int> h_hist(HIST_BINS);
            CUDA_CHECK(cudaMemcpy(h_hist.data(), bufs.d_hist_v2,
                                  HIST_BINS * sizeof(int),
                                  cudaMemcpyDeviceToHost));
            verify_histogram_cpu(h_norm.data(),
                                 args.height * args.width * CHANNELS,
                                 h_hist.data(), HIST_BINS);

            std::vector<unsigned int> h_sorted(args.height * args.width);
            CUDA_CHECK(cudaMemcpy(h_sorted.data(), bufs.d_sort_keys_out,
                                  h_sorted.size() * sizeof(unsigned int),
                                  cudaMemcpyDeviceToHost));
            verify_sort_cpu(h_sorted.data(), (int)h_sorted.size());
        }

        // ---------------------------------------
        // [CAP1-T166] Performance report
        // ---------------------------------------
        print_roofline_table(perfs, 5, peak_bw);

        // ---------------------------------------
        // [CAP1-T167] Cleanup
        // ---------------------------------------
        free_device_buffers(bufs);
    }

    // ---------------------------------------
    // [CAP1-T168] TODO [ADVANCED] branch A: FP16 rewrite.
    //   Replace float with __half in normalize / conv2d, vectorize with
    //   __half2, and compare against FP32 throughput.
    //
    // [CAP1-T169] TODO [ADVANCED] branch B: multi-GPU version.
    //   Use cudaMemcpyPeerAsync or NCCL to shard the image across multiple
    //   GPUs; each GPU processes its tile and GPU-0 aggregates the histogram
    //   and sort results.
    //
    // [CAP1-T170] TODO [ADVANCED] branch C: end-to-end explicit CUDA Graph.
    //   Use cudaGraphCreate / cudaGraphAddKernelNode to hand-build the DAG
    //   and cudaGraphExecKernelNodeSetParams to support dynamic resolution updates.
    //
    // [CAP1-T171] TODO [ADVANCED] branch D: Thrust/CUB comparison.
    //   Rewrite normalize with thrust::transform + thrust::reduce; replace
    //   Stage 5 with cub::DeviceRadixSort::SortPairs; compare manual kernels
    //   against the library implementations.
    // ---------------------------------------

    // [CAP1-T172]
    printf("\n[CAPSTONE1] done.\n");
    // [CAP1-T173]
    printf("  Tip: nsys profile --trace=cuda,nvtx for a full timeline,\n");
    // [CAP1-T174]
    printf("  and ncu --set full to collect detailed metrics per kernel.\n");
    return 0;
}
