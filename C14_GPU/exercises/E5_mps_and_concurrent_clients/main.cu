// [E5-T01] E5_mps_and_concurrent_clients/main.cu
// [E5-T02] Goal: understand how Multi-Process Service (MPS) works.
//          Demonstrate single-GPU multi-process scenarios; measure kernel
//          latency variance.
//
// [E5-T03] Usage:
//   ./E5_mps_and_concurrent_clients --role=producer   (process 1: keeps issuing kernels)
//   ./E5_mps_and_concurrent_clients --role=consumer   (process 2: issues kernels concurrently, measures variance)
//   ./E5_mps_and_concurrent_clients                   (default: single-process baseline)
//
// [E5-T04] Required tasks (Module E exercise E5):
//   [REQUIRED-1] write a lightweight CUDA program that runs a 1-10 ms kernel and prints PID + runtime
//   [REQUIRED-2] no MPS: script launches 4 processes simultaneously, observe total time (context switch overhead)
//   [REQUIRED-3] start MPS daemon (needs privilege), see README for steps
//   [REQUIRED-4] with MPS: re-run, compare completion time
//   [REQUIRED-5] record before/after MPS comparison data (total time, throughput)
//   [REQUIRED-6] use nvidia-smi or Nsight Systems to observe GPU utilization shift
//
// [E5-T05] Advanced tasks (TODO [ADVANCED]):
//   [ADVANCED-A] MPS active thread percentage limit
//   [ADVANCED-B] compare many small kernels vs single big kernel for MPS gain
//   [ADVANCED-C] dual-GPU comparison (one with MPS enabled, one without)
//
// [E5-T06] Note: MPS is a system-level configuration, not a CUDA API call.
//          Must be enabled at the system layer via nvidia-cuda-mps-control CLI.
//          See README.md for MPS startup steps.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <cuda_runtime.h>

#ifdef _WIN32
  #include <windows.h>
  #define GET_PID() ((int)GetCurrentProcessId())
#else
  #include <unistd.h>
  #define GET_PID() ((int)getpid())
#endif

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------------------------
// [E5-T07] Constants
// ---------------------------------------------------------------------------
static constexpr int   N_ELEM          = 1 << 24;  // [E5-T08] 16M elems
static constexpr int   BLOCK           = 256;
static constexpr int   WARMUP_ITERS    = 5;
static constexpr int   MEASURE_ITERS   = 50;       // [E5-T09] number of latency variance samples
static constexpr int   INNER_LOOP      = 256;      // [E5-T10] controls kernel runtime ~1-10 ms

// ---------------------------------------------------------------------------
// [E5-T11] Kernel (TODO area)
// ---------------------------------------------------------------------------

// [E5-T12] kernel_worker: simulates a real workload (medium compute density, 1-10 ms).
// [E5-T13] TODO [REQUIRED-1] implement: each thread does INNER_LOOP fp ops on its element.
__global__ void kernel_worker(float* __restrict__ buf, int n, int loops)
{
    // [E5-T14] TODO [REQUIRED-1] implement kernel_worker
    // per-thread: float acc = buf[idx]; for (int k=0;k<loops;++k) acc = acc*1.0001f+0.001f;
    //             buf[idx] = acc;
    (void)buf; (void)n; (void)loops;
}

// ---------------------------------------------------------------------------
// [E5-T15] Measure single-process kernel latency (mean + variance)
// ---------------------------------------------------------------------------
static void measure_latency(const char* label, cudaStream_t stream)
{
    NVTX_RANGE("E5/measure_latency");

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    float* d_buf = nullptr;
    CUDA_CHECK(cudaMalloc(&d_buf, N_ELEM * sizeof(float)));
    CUDA_CHECK(cudaMemset(d_buf, 0, N_ELEM * sizeof(float)));

    // [E5-T16] warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        kernel_worker<<<grid, BLOCK, 0, stream>>>(d_buf, N_ELEM, INNER_LOOP);
        CUDA_CHECK_LAST();
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));

    // [E5-T17] measure
    std::vector<float> latencies(MEASURE_ITERS);
    for (int i = 0; i < MEASURE_ITERS; ++i) {
        CudaEventTimer t;
        t.start(stream);
        kernel_worker<<<grid, BLOCK, 0, stream>>>(d_buf, N_ELEM, INNER_LOOP);
        CUDA_CHECK_LAST();
        t.stop(stream);
        latencies[i] = t.elapsed_ms();
    }

    // [E5-T18] stats
    float mean = 0.0f;
    for (float v : latencies) mean += v;
    mean /= MEASURE_ITERS;

    float var = 0.0f;
    for (float v : latencies) var += (v - mean) * (v - mean);
    var /= MEASURE_ITERS;
    float stddev = std::sqrt(var);

    float min_v = *std::min_element(latencies.begin(), latencies.end());
    float max_v = *std::max_element(latencies.begin(), latencies.end());

    // [E5-T19]
    std::fprintf(stdout, "[PID %d][%s] latency stats (%d samples):\n",
                 GET_PID(), label, MEASURE_ITERS);
    // [E5-T20]
    std::fprintf(stdout, "  mean   = %.3f ms\n", mean);
    // [E5-T21]
    std::fprintf(stdout, "  stddev = %.3f ms\n", stddev);
    // [E5-T22]
    std::fprintf(stdout, "  min    = %.3f ms\n", min_v);
    // [E5-T23]
    std::fprintf(stdout, "  max    = %.3f ms\n", max_v);
    // [E5-T24]
    std::fprintf(stdout, "  CV     = %.1f%%\n\n",
                 mean > 0 ? stddev / mean * 100.0f : 0.0f);

    CUDA_CHECK(cudaFree(d_buf));
}

// ---------------------------------------------------------------------------
// [E5-T25] Producer role: keeps launching kernels as background load
// ---------------------------------------------------------------------------
static void run_producer(cudaStream_t stream)
{
    // [E5-T26]
    std::fprintf(stdout, "[PID %d] Producer started, continuously issuing kernels as background load...\n",
                 GET_PID());

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;
    float* d_buf = nullptr;
    CUDA_CHECK(cudaMalloc(&d_buf, N_ELEM * sizeof(float)));

    // [E5-T27] TODO [REQUIRED-2] keep looping to keep GPU busy
    // In real test, producer process should run in background while consumer measures variance.
    for (int i = 0; i < 200; ++i) {
        kernel_worker<<<grid, BLOCK, 0, stream>>>(d_buf, N_ELEM, INNER_LOOP);
        CUDA_CHECK_LAST();
        if (i % 10 == 0) {
            CUDA_CHECK(cudaStreamSynchronize(stream));
            // [E5-T28]
            std::fprintf(stdout, "[Producer] completed %d kernel rounds\n", i);
        }
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    CUDA_CHECK(cudaFree(d_buf));
}

// ---------------------------------------------------------------------------
// [E5-T29] Consumer role: measure kernel latency variance under background load
// ---------------------------------------------------------------------------
static void run_consumer(cudaStream_t stream)
{
    // [E5-T30]
    std::fprintf(stdout, "[PID %d] Consumer started, measuring kernel latency variance...\n",
                 GET_PID());
    // [E5-T31]
    std::fprintf(stdout, "note: when running concurrently with Producer, without MPS\n");
    // [E5-T32]
    std::fprintf(stdout, "      context switch will inflate latency and CV.\n\n");

    measure_latency("consumer_with_background_load", stream);
}

// ---------------------------------------------------------------------------
// [E5-T33] main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    NVTX_RANGE("E5/main");
    print_device_info();

    // [E5-T34] parse --role argument
    const char* role = "standalone";
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--role=", 7) == 0) {
            role = argv[i] + 7;
        }
    }

    // [E5-T35]
    std::fprintf(stdout, "=== E5: MPS and concurrent clients ===\n");
    // [E5-T36]
    std::fprintf(stdout, "PID = %d, role = %s\n\n", GET_PID(), role);
    // [E5-T37]
    std::fprintf(stdout, "grid = %d, block = %d, inner_loop = %d\n",
                 (N_ELEM + BLOCK - 1) / BLOCK, BLOCK, INNER_LOOP);
    // [E5-T38]
    std::fprintf(stdout, "estimated kernel time ~%.0f ms (depends on GPU)\n\n",
                 (double)N_ELEM * INNER_LOOP / (1e9));

    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream));

    if (std::strcmp(role, "producer") == 0) {
        run_producer(stream);
    } else if (std::strcmp(role, "consumer") == 0) {
        run_consumer(stream);
    } else {
        // [E5-T39] standalone: single-process baseline for comparison
        std::fprintf(stdout, "--- single-process baseline (no background load) ---\n");
        measure_latency("standalone_baseline", stream);

        // [E5-T40]
        std::fprintf(stdout, "--- multi-process test instructions ---\n");
        // [E5-T41]
        std::fprintf(stdout, "1. without MPS:\n");
        // [E5-T42]
        std::fprintf(stdout, "   terminal 1: ./E5_mps_and_concurrent_clients --role=producer &\n");
        // [E5-T43]
        std::fprintf(stdout, "   terminal 2: ./E5_mps_and_concurrent_clients --role=consumer\n");
        // [E5-T44]
        std::fprintf(stdout, "   observe consumer latency variance (should be higher than baseline)\n\n");
        // [E5-T45]
        std::fprintf(stdout, "2. enable MPS and re-run:\n");
        // [E5-T46]
        std::fprintf(stdout, "   see MPS startup steps in README.md\n\n");
        // [E5-T47]
        std::fprintf(stdout, "   expected: with MPS, consumer variance should drop below the without-MPS case\n");
    }

    CUDA_CHECK(cudaStreamDestroy(stream));

    // [E5-T48]
    std::fprintf(stdout, "\nhint: capture with 'nsys profile -o e5_trace --stats=true "
                 "./E5_mps_and_concurrent_clients',\n");
    // [E5-T49]
    std::fprintf(stdout, "      observe GPU utilization and context switch behavior.\n");
    return 0;
}
