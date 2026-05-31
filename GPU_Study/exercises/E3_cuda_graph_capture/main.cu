// [E3-T01] E3_cuda_graph_capture/main.cu
// [E3-T02] Goal: learn the stream capture API for implicit graph construction;
//          compare per-kernel launch vs graph replay overhead.
//
// [E3-T03] Required tasks (Module E exercise E3):
//   [REQUIRED-1] write a lightweight kernel (runtime < 1 ms)
//   [REQUIRED-2] launch it 1000 times one by one, time with cudaEvent, measure baseline launch overhead
//   [REQUIRED-3] cudaStreamBeginCapture -> launch 1000 times in loop -> cudaStreamEndCapture
//   [REQUIRED-4] cudaGraphInstantiate compiles graph into executable form
//   [REQUIRED-5] cudaGraphLaunch replay 100 times, time it
//   [REQUIRED-6] compare both versions, estimate per-launch overhead in microseconds
//   [REQUIRED-7] print single-kernel runtime and average launch overhead estimate
//
// [E3-T04] Advanced tasks (TODO [ADVANCED]):
//   [ADVANCED-A] add cudaMemcpy node and dependencies into graph, validate capture still works
//   [ADVANCED-B] perform host-side ops (logging) during capture, observe whether they get ignored
//   [ADVANCED-C] call cudaStreamSynchronize during capture, observe error behavior

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------------------------
// [E3-T05] Constants
// ---------------------------------------------------------------------------
static constexpr int N_ELEM        = 1 << 20;  // [E3-T06] 1M elems (lightweight kernel)
static constexpr int BLOCK         = 256;
static constexpr int LAUNCH_ITERS  = 1000;     // [E3-T07] per-kernel launch count
static constexpr int GRAPH_REPLAYS = 100;      // [E3-T08] graph replay count

// ---------------------------------------------------------------------------
// [E3-T09] Kernel (TODO area)
// ---------------------------------------------------------------------------

// [E3-T10] kernel_saxpy: lightweight SAXPY, runtime < 1 ms.
// [E3-T11] TODO [REQUIRED-1] implement: out[idx] = a * x[idx] + y[idx].
__global__ void kernel_saxpy(float a,
                              const float* __restrict__ x,
                              const float* __restrict__ y,
                              float* __restrict__ out,
                              int n)
{
    // [E3-T12] TODO [REQUIRED-1] implement kernel_saxpy
    (void)a; (void)x; (void)y; (void)out; (void)n;
}

// ---------------------------------------------------------------------------
// [E3-T13] Version A: per-launch (baseline)
// ---------------------------------------------------------------------------
static float run_individual_launches(float* d_x, float* d_y, float* d_out,
                                      cudaStream_t stream)
{
    NVTX_RANGE("E3/IndividualLaunches");

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    // [E3-T14] warmup once
    kernel_saxpy<<<grid, BLOCK, 0, stream>>>(2.0f, d_x, d_y, d_out, N_ELEM);
    CUDA_CHECK(cudaStreamSynchronize(stream));

    CudaEventTimer timer;
    timer.start(stream);

    // [E3-T15] TODO [REQUIRED-2] launch kernel_saxpy LAUNCH_ITERS times
    for (int i = 0; i < LAUNCH_ITERS; ++i) {
        // kernel_saxpy<<<grid, BLOCK, 0, stream>>>(2.0f, d_x, d_y, d_out, N_ELEM);
        CUDA_CHECK_LAST();
    }

    timer.stop(stream);
    float ms = timer.elapsed_ms();
    // [E3-T16]
    std::fprintf(stdout, "[per-launch] %d total = %.3f ms, "
                 "avg = %.3f us/launch\n",
                 LAUNCH_ITERS, ms, ms * 1000.0f / LAUNCH_ITERS);
    (void)grid;
    return ms;
}

// ---------------------------------------------------------------------------
// [E3-T17] Version B: stream capture + graph replay
// ---------------------------------------------------------------------------
static float run_graph_replay(float* d_x, float* d_y, float* d_out,
                               cudaStream_t stream)
{
    NVTX_RANGE("E3/GraphReplay");

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    // [E3-T18] -- step 1: capture --
    cudaGraph_t graph = nullptr;

    // [E3-T19] TODO [REQUIRED-3] cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)
    {
        NVTX_RANGE("E3/Capture");
        // [E3-T20] TODO [REQUIRED-3] launch kernel_saxpy LAUNCH_ITERS times here
        for (int i = 0; i < LAUNCH_ITERS; ++i) {
            // kernel_saxpy<<<grid, BLOCK, 0, stream>>>(2.0f, d_x, d_y, d_out, N_ELEM);
            (void)i;
        }
    }
    // [E3-T21] TODO [REQUIRED-3] cudaStreamEndCapture(stream, &graph)

    // [E3-T22] print graph node count
    // size_t node_count = 0;
    // CUDA_CHECK(cudaGraphGetNodes(graph, nullptr, &node_count));
    // std::fprintf(stdout, "  captured graph nodes = %zu\n", node_count);

    // [E3-T23] -- step 2: instantiate --
    cudaGraphExec_t graphExec = nullptr;
    // [E3-T24] TODO [REQUIRED-4] cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0)

    // [E3-T25] -- step 3: replay 100 times --
    CudaEventTimer timer;
    timer.start(stream);

    // [E3-T26] TODO [REQUIRED-5] loop GRAPH_REPLAYS times, each cudaGraphLaunch(graphExec, stream)
    for (int r = 0; r < GRAPH_REPLAYS; ++r) {
        // CUDA_CHECK(cudaGraphLaunch(graphExec, stream));
        (void)r;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    timer.stop(stream);

    float ms = timer.elapsed_ms();
    int total_kernels = GRAPH_REPLAYS * LAUNCH_ITERS;
    // [E3-T27]
    std::fprintf(stdout, "[graph replay] %d replays (total %d kernels) = %.3f ms, "
                 "avg = %.3f us/kernel\n",
                 GRAPH_REPLAYS, total_kernels, ms, ms * 1000.0f / total_kernels);

    // [E3-T28] -- cleanup --
    // [E3-T29] TODO [REQUIRED-5] cudaGraphExecDestroy(graphExec)
    // [E3-T30] TODO [REQUIRED-5] cudaGraphDestroy(graph)

    (void)grid; (void)graph; (void)graphExec;
    return ms;
}

// ---------------------------------------------------------------------------
// [E3-T31] main
// ---------------------------------------------------------------------------
int main()
{
    NVTX_RANGE("E3/main");
    print_device_info();

    // [E3-T32]
    std::fprintf(stdout, "=== E3: stream capture and CUDA Graph ===\n");
    // [E3-T33]
    std::fprintf(stdout, "N_ELEM = %d, BLOCK = %d\n", N_ELEM, BLOCK);
    // [E3-T34]
    std::fprintf(stdout, "per-launch count = %d, graph replays = %d\n\n",
                 LAUNCH_ITERS, GRAPH_REPLAYS);

    // [E3-T35] -- allocate device memory --
    float* d_x   = nullptr;
    float* d_y   = nullptr;
    float* d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_x,   N_ELEM * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_y,   N_ELEM * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, N_ELEM * sizeof(float)));

    // [E3-T36] TODO [REQUIRED-1] init d_x, d_y (e.g. cudaMemset or fill kernel)

    // [E3-T37] -- create stream (non-default, avoid legacy default stream interference) --
    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream));

    // [E3-T38] -- run version A --
    float ms_individual = run_individual_launches(d_x, d_y, d_out, stream);

    // [E3-T39] -- run version B --
    float ms_graph = run_graph_replay(d_x, d_y, d_out, stream);

    // [E3-T40] -- summary --
    std::fprintf(stdout, "\n=== compare (host overhead at equal kernel work) ===\n");
    // [E3-T41] per-launch version: LAUNCH_ITERS kernels
    // [E3-T42] graph version: GRAPH_REPLAYS * LAUNCH_ITERS kernels (launch cost amortized)
    // [E3-T43] compare per LAUNCH_ITERS-kernel batch
    float ms_graph_per_batch = ms_graph / GRAPH_REPLAYS;
    // [E3-T44]
    std::fprintf(stdout, "per-launch (%d kernels)         : %.3f ms\n",
                 LAUNCH_ITERS, ms_individual);
    // [E3-T45]
    std::fprintf(stdout, "graph replay (per batch %d kernels): %.3f ms\n",
                 LAUNCH_ITERS, ms_graph_per_batch);
    if (ms_graph_per_batch > 0.0f) {
        // [E3-T46]
        std::fprintf(stdout, "speedup : %.1fx\n", ms_individual / ms_graph_per_batch);
    }

    // [E3-T47] TODO [ADVANCED-A] add cudaMemcpy node into graph, verify capture works

    // [E3-T48] -- cleanup --
    CUDA_CHECK(cudaStreamDestroy(stream));
    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_y));
    CUDA_CHECK(cudaFree(d_out));

    // [E3-T49]
    std::fprintf(stdout, "\nhint: capture with 'nsys profile -o e3_trace --stats=true ./E3_cuda_graph_capture',\n");
    // [E3-T50]
    std::fprintf(stdout, "      compare per-launch vs graph replay launch density on the timeline.\n");
    return 0;
}
