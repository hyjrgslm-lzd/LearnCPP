// [E1-T01] E1_streams_and_events/main.cu
// [E1-T02] Goal: understand cudaStream_t concurrency semantics, plus
//          cudaEventRecord / cudaStreamWaitEvent. Compare two versions
//          (default stream vs multi stream) and validate with Nsight Systems.
//
// [E1-T03] Required tasks (Module E exercise E1):
//   [REQUIRED-1] allocate >=256 MB host/device memory
//   [REQUIRED-2] write a kernel that runs 5-50 ms (large reduce loop)
//   [REQUIRED-3] default-stream version: two kernels + HtoD + DtoH,
//                cudaDeviceSynchronize, record total time
//   [REQUIRED-4] multi-stream version: stream A runs two kernels, stream B
//                runs HtoD+kernel, build dependency with cudaEventRecord +
//                cudaStreamWaitEvent
//   [REQUIRED-5] compare timings, verify multi-stream version is faster
//   [REQUIRED-6] capture with nsys profile, view GUI timeline overlap
//
// [E1-T04] Advanced tasks (TODO [ADVANCED]):
//   [ADVANCED-A] third stream, three kernels concurrent without dependency
//   [ADVANCED-B] cudaStreamNonBlocking flag, observe scheduling latency
//   [ADVANCED-C] manual task graph: A||B -> C -> D (events + cudaStreamWaitEvent)

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------------------------
// [E1-T05] Constants
// ---------------------------------------------------------------------------
static constexpr int   N_ELEM   = 256 * 1024 * 1024 / sizeof(float); // 256 MB
static constexpr int   BLOCK    = 256;
static constexpr int   INNER_LOOP = 512; // [E1-T06] controls kernel runtime

// ---------------------------------------------------------------------------
// [E1-T07] Kernel declarations (TODO area, implementation left to student)
// ---------------------------------------------------------------------------

// [E1-T08] kernel_heavy: simulates a heavy kernel, reduces a big array.
// [E1-T09] Target runtime 5-50 ms (tune via INNER_LOOP).
// [E1-T10] TODO [REQUIRED-2] implement: each thread walks a slice of data,
//          performs INNER_LOOP iterations of accumulation / atomic ops.
__global__ void kernel_heavy(const float* __restrict__ in,
                              float* __restrict__ out,
                              int n,
                              int loops)
{
    // [E1-T11] TODO [REQUIRED-2] implement kernel_heavy
    // Reference: int idx = blockIdx.x * blockDim.x + threadIdx.x;
    //            for (int i = 0; i < loops; ++i) { ... }
    (void)in; (void)out; (void)n; (void)loops;
}

// [E1-T12] kernel_transform: lightweight transform kernel
//          (runs on stream B concurrently with kernel_heavy).
// [E1-T13] TODO [REQUIRED-4] implement: simple element multiply by 2.0f.
__global__ void kernel_transform(const float* __restrict__ in,
                                  float* __restrict__ out,
                                  int n)
{
    // [E1-T14] TODO [REQUIRED-4] implement kernel_transform
    (void)in; (void)out; (void)n;
}

// ---------------------------------------------------------------------------
// [E1-T15] Version A: default stream (fully serialized)
// ---------------------------------------------------------------------------
static float run_default_stream(float* h_in, float* d_in, float* d_out,
                                 float* h_out_check)
{
    NVTX_RANGE("E1/DefaultStream");

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    CudaEventTimer timer;
    timer.start(0); // [E1-T16] stream 0 = default stream

    // [E1-T17] TODO [REQUIRED-3] on the default stream, run sequentially:
    //   1. HtoD copy h_in -> d_in (cudaMemcpy, blocking)
    //   2. kernel_heavy<<<grid, BLOCK>>>(d_in, d_out, N_ELEM, INNER_LOOP)
    //   3. kernel_transform<<<grid, BLOCK>>>(d_out, d_in, N_ELEM)
    //   4. DtoH copy d_in -> h_out_check (cudaMemcpy)
    // [E1-T18] TODO [REQUIRED-3] all four steps run on the default stream,
    //          strictly serial.

    CUDA_CHECK(cudaDeviceSynchronize());
    timer.stop(0);

    float ms = timer.elapsed_ms();
    // [E1-T19]
    std::fprintf(stdout, "[default stream] total = %.3f ms\n", ms);
    return ms;
}

// ---------------------------------------------------------------------------
// [E1-T20] Version B: multi-stream (kernel + DtoH concurrent)
// ---------------------------------------------------------------------------
static float run_multi_stream(float* h_in,
                               float* d_in_A, float* d_out_A,
                               float* d_in_B, float* d_out_B,
                               float* h_out_check)
{
    NVTX_RANGE("E1/MultiStream");

    // [E1-T21] TODO [REQUIRED-4] create two non-default streams.
    cudaStream_t streamA = nullptr, streamB = nullptr;
    // CUDA_CHECK(cudaStreamCreate(&streamA));
    // CUDA_CHECK(cudaStreamCreate(&streamB));

    // [E1-T22] TODO [REQUIRED-4] create events to build dependencies.
    cudaEvent_t evHtoDDone = nullptr, evKernelADone = nullptr;
    // CUDA_CHECK(cudaEventCreate(&evHtoDDone));
    // CUDA_CHECK(cudaEventCreate(&evKernelADone));

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    CudaEventTimer timer;
    timer.start(streamA);

    // [E1-T23] TODO [REQUIRED-4] on stream A:
    //   1. cudaMemcpyAsync HtoD h_in -> d_in_A (use streamA)
    //   2. cudaEventRecord(evHtoDDone, streamA)
    //   3. kernel_heavy<<<grid, BLOCK, 0, streamA>>>(d_in_A, d_out_A, N_ELEM, INNER_LOOP)
    //   4. cudaEventRecord(evKernelADone, streamA)

    // [E1-T24] TODO [REQUIRED-4] on stream B (concurrent with stream A):
    //   1. cudaStreamWaitEvent(streamB, evHtoDDone, 0)  // wait for HtoD before reading
    //   2. kernel_transform<<<grid, BLOCK, 0, streamB>>>(d_in_A, d_out_B, N_ELEM)
    //   3. cudaMemcpyAsync DtoH d_out_B -> h_out_check (use streamB)

    // [E1-T25] TODO [REQUIRED-5] wait for everything to finish
    // CUDA_CHECK(cudaDeviceSynchronize());

    timer.stop(streamA);
    float ms = timer.elapsed_ms();
    // [E1-T26]
    std::fprintf(stdout, "[multi stream] total = %.3f ms\n", ms);

    // [E1-T27] TODO cleanup: cudaEventDestroy / cudaStreamDestroy
    (void)h_in; (void)d_in_A; (void)d_out_A;
    (void)d_in_B; (void)d_out_B; (void)h_out_check;
    (void)streamA; (void)streamB;
    (void)evHtoDDone; (void)evKernelADone;
    (void)grid;

    return ms;
}

// ---------------------------------------------------------------------------
// [E1-T28] main
// ---------------------------------------------------------------------------
int main()
{
    NVTX_RANGE("E1/main");
    print_device_info();

    // [E1-T29]
    std::fprintf(stdout, "=== E1: stream and event concurrency ===\n");
    // [E1-T30]
    std::fprintf(stdout, "array size = %d elems (%.0f MB)\n",
                 N_ELEM, N_ELEM * sizeof(float) / 1048576.0);
    // [E1-T31]
    std::fprintf(stdout, "grid = %d, block = %d, inner_loop = %d\n\n",
                 (N_ELEM + BLOCK - 1) / BLOCK, BLOCK, INNER_LOOP);

    // [E1-T32] -- allocate host memory (pinned, allows async copy) --
    float* h_in       = nullptr;
    float* h_out_chk  = nullptr;
    // [E1-T33] TODO [REQUIRED-1] cudaMallocHost / malloc to allocate pinned host
    // CUDA_CHECK(cudaMallocHost(&h_in,      N_ELEM * sizeof(float)));
    // CUDA_CHECK(cudaMallocHost(&h_out_chk, N_ELEM * sizeof(float)));

    // [E1-T34] TODO [REQUIRED-1] initialize h_in (e.g. h_in[i] = (float)i * 0.001f)

    // [E1-T35] -- allocate device memory --
    float* d_in_A  = nullptr;
    float* d_out_A = nullptr;
    float* d_in_B  = nullptr;
    float* d_out_B = nullptr;
    // CUDA_CHECK(cudaMalloc(&d_in_A,  N_ELEM * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_out_A, N_ELEM * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_in_B,  N_ELEM * sizeof(float)));
    // CUDA_CHECK(cudaMalloc(&d_out_B, N_ELEM * sizeof(float)));

    // [E1-T36] -- run both versions --
    float ms_default = run_default_stream(h_in, d_in_A, d_out_A, h_out_chk);
    float ms_multi   = run_multi_stream(h_in, d_in_A, d_out_A, d_in_B, d_out_B, h_out_chk);

    // [E1-T37] -- compare results --
    std::fprintf(stdout, "\n=== compare ===\n");
    // [E1-T38]
    std::fprintf(stdout, "default stream : %.3f ms\n", ms_default);
    // [E1-T39]
    std::fprintf(stdout, "multi  stream  : %.3f ms\n", ms_multi);
    if (ms_default > 0.0f && ms_multi > 0.0f) {
        // [E1-T40]
        std::fprintf(stdout, "speedup        : %.2fx\n", ms_default / ms_multi);
    }
    std::fprintf(stdout, "\n");
    // [E1-T41]
    std::fprintf(stdout, "hint: capture with 'nsys profile -o e1_trace --stats=true ./E1_streams_and_events',\n");
    // [E1-T42]
    std::fprintf(stdout, "      then open the Nsight Systems GUI to see concurrency overlap on the timeline.\n");

    // [E1-T43] TODO [ADVANCED-A] third stream, three kernels concurrent

    // [E1-T44] TODO [ADVANCED-C] manual task graph: kernel A || kernel B -> kernel C -> kernel D

    // [E1-T45] -- cleanup --
    // [E1-T46] TODO free all memory
    (void)ms_default; (void)ms_multi;

    return 0;
}
