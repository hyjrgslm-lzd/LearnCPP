// ============================================================
// [B4-T01] Exercise B4: pinned_vs_pageable_and_unified
// [B4-T02] Goal: understand the H2D bandwidth differences between pinned memory
//          (DMA capable), pageable memory (regular malloc), and UVM (Unified
//          Virtual Memory).
//
// [B4-T03] Acceptance:
//   pinned throughput >= 2x pageable throughput.
//   Nsight Systems: nsys profile --trace cuda ./B4_pinned_vs_pageable_and_unified.exe
// ============================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/timer.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [B4-T04] Constants
// ------------------------------------------------------------
static constexpr size_t DATA_MB  = 256;                          // [B4-T05] transfer size
static constexpr size_t DATA_SZ  = DATA_MB * 1024 * 1024;        // [B4-T06] in bytes
static constexpr int    RUNS     = 5;                            // [B4-T07] repeat count (averaged)
static constexpr int    WARMUP   = 2;

// ------------------------------------------------------------
// [B4-T08] Placeholder kernel (used to warm up the GPU; no real work)
// ------------------------------------------------------------
__global__ void warmup_kernel() { /* [B4-T09] only used to trigger GPU context init */ }

// ------------------------------------------------------------
// [B4-T10] Compute H2D throughput (GB/s)
// ------------------------------------------------------------
static float measure_h2d_gbps(void* h_src, void* d_dst, size_t bytes, int runs) {
    // [B4-T11] warmup
    for (int i = 0; i < WARMUP; i++) {
        CUDA_CHECK(cudaMemcpy(d_dst, h_src, bytes, cudaMemcpyHostToDevice));
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    CudaEventTimer timer;
    timer.start();
    for (int i = 0; i < runs; i++) {
        CUDA_CHECK(cudaMemcpy(d_dst, h_src, bytes, cudaMemcpyHostToDevice));
    }
    timer.stop();

    float ms = timer.elapsed_ms() / runs;
    return static_cast<float>(bytes) / ms / 1e6f;  // [B4-T12] GB/s
}

// ------------------------------------------------------------
// [B4-T13] main
// ------------------------------------------------------------
int main() {
    std::puts("[B4_pinned_vs_pageable_and_unified]");
    print_device_info(0);

    // [B4-T14]
    std::printf("[B4] data size: %zu MB  runs: %d\n", DATA_MB, RUNS);

    // [B4-T15] GPU warmup (eliminate first-launch overhead)
    warmup_kernel<<<1, 1>>>();
    CUDA_CHECK(cudaDeviceSynchronize());

    // [B4-T16] -- allocate device-side destination buffer --
    void* d_buf = nullptr;
    CUDA_CHECK(cudaMalloc(&d_buf, DATA_SZ));

    // ============================================================
    // [B4-T17] Version 1: pageable (regular malloc)
    // ============================================================
    // [B4-T18] TODO [REQUIRED] step 1: allocate host memory with malloc and time the H2D copy
    float gbps_pageable = 0.0f;
    {
        NVTX_RANGE("B4/pageable");

        // [B4-T19] TODO [REQUIRED]: float* h_pageable = (float*)std::malloc(DATA_SZ);
        // [B4-T20] TODO [REQUIRED]: // initialize (avoid lazy allocation)
        // [B4-T21] TODO [REQUIRED]: std::memset(h_pageable, 0, DATA_SZ);
        // [B4-T22] TODO [REQUIRED]: gbps_pageable = measure_h2d_gbps(h_pageable, d_buf, DATA_SZ, RUNS);
        // [B4-T23] TODO [REQUIRED]: std::free(h_pageable);

        float* h_pageable = (float*)std::malloc(DATA_SZ);
        std::memset(h_pageable, 0, DATA_SZ);      // [B4-T24] force physical allocation
        gbps_pageable = measure_h2d_gbps(h_pageable, d_buf, DATA_SZ, RUNS);
        // [B4-T25]
        std::printf("[B4] pageable H2D:  %.1f GB/s\n", gbps_pageable);
        std::free(h_pageable);
    }

    // ============================================================
    // [B4-T26] Version 2: pinned (cudaMallocHost)
    // ============================================================
    // [B4-T27] TODO [REQUIRED] step 3: allocate with cudaMallocHost and time the copy
    float gbps_pinned = 0.0f;
    {
        NVTX_RANGE("B4/pinned");

        float* h_pinned = nullptr;
        // [B4-T28] TODO [REQUIRED]: CUDA_CHECK(cudaMallocHost(&h_pinned, DATA_SZ));
        // [B4-T29] TODO [REQUIRED]: std::memset(h_pinned, 0, DATA_SZ);
        // [B4-T30] TODO [REQUIRED]: gbps_pinned = measure_h2d_gbps(h_pinned, d_buf, DATA_SZ, RUNS);
        // [B4-T31] TODO [REQUIRED]: CUDA_CHECK(cudaFreeHost(h_pinned));

        CUDA_CHECK(cudaMallocHost(&h_pinned, DATA_SZ));
        std::memset(h_pinned, 0, DATA_SZ);
        gbps_pinned = measure_h2d_gbps(h_pinned, d_buf, DATA_SZ, RUNS);
        // [B4-T32]
        std::printf("[B4] pinned H2D:    %.1f GB/s\n", gbps_pinned);
        CUDA_CHECK(cudaFreeHost(h_pinned));
    }

    // ============================================================
    // [B4-T33] Version 3: UVM (cudaMallocManaged)
    // ============================================================
    // [B4-T34] TODO [REQUIRED] step 5: use cudaMallocManaged and measure
    float gbps_uvm = 0.0f;
    {
        NVTX_RANGE("B4/uvm");

        float* um_data = nullptr;
        // [B4-T35] TODO [REQUIRED]: CUDA_CHECK(cudaMallocManaged(&um_data, DATA_SZ));
        // [B4-T36] TODO [REQUIRED]: std::memset(um_data, 0, DATA_SZ);  // host-side init
        // [B4-T37] TODO [REQUIRED]: // option A: no prefetch (lazy; first kernel access triggers a page fault)
        // [B4-T38] TODO [REQUIRED]: // option B: explicit prefetch
        // [B4-T39] TODO [REQUIRED]: // int dev; CUDA_CHECK(cudaGetDevice(&dev));
        // [B4-T40] TODO [REQUIRED]: // CUDA_CHECK(cudaMemPrefetchAsync(um_data, DATA_SZ, dev));
        // [B4-T41] TODO [REQUIRED]: CUDA_CHECK(cudaFree(um_data));

        CUDA_CHECK(cudaMallocManaged(&um_data, DATA_SZ));
        std::memset(um_data, 0, DATA_SZ);

        // [B4-T42] prefetch path (uncomment to enable)
        // int dev; CUDA_CHECK(cudaGetDevice(&dev));
        // CUDA_CHECK(cudaMemPrefetchAsync(um_data, DATA_SZ, dev));

        // [B4-T43] UVM has no traditional cudaMemcpy timing path; wrap a prefetch with events to estimate
        {
            int dev = 0;
            CUDA_CHECK(cudaGetDevice(&dev));
            // CUDA 13: cudaMemPrefetchAsync now takes a cudaMemLocation struct (was int dev).
            cudaMemLocation loc{};
            loc.type = cudaMemLocationTypeDevice;
            loc.id   = dev;
            CudaEventTimer t;
            t.start();
            for (int i = 0; i < RUNS; i++) {
                CUDA_CHECK(cudaMemPrefetchAsync(um_data, DATA_SZ, loc, /*flags*/0));
                CUDA_CHECK(cudaDeviceSynchronize());
            }
            t.stop();
            float ms = t.elapsed_ms() / RUNS;
            gbps_uvm = static_cast<float>(DATA_SZ) / ms / 1e6f;
        }

        // [B4-T44]
        std::printf("[B4] UVM+prefetch H2D: %.1f GB/s\n", gbps_uvm);
        CUDA_CHECK(cudaFree(um_data));
    }

    // [B4-T45] -- summary --
    std::puts("------------------------------------------------------------");
    std::printf("  pageable : %.1f GB/s\n", gbps_pageable);
    std::printf("  pinned   : %.1f GB/s  (%.1fx vs pageable)\n",
                gbps_pinned, gbps_pinned / (gbps_pageable + 1e-6f));
    std::printf("  UVM+pre  : %.1f GB/s  (%.1fx vs pageable)\n",
                gbps_uvm,    gbps_uvm    / (gbps_pageable + 1e-6f));
    std::puts("------------------------------------------------------------");
    // [B4-T46]
    std::puts("  TODO [REQUIRED] step 7: run Nsight Systems:");
    std::puts("    nsys profile --trace cuda ./B4_pinned_vs_pageable_and_unified.exe");
    // [B4-T47]
    std::puts("  Inspect the PCIe transfer timeline for the three versions.");
    std::puts("------------------------------------------------------------");

    CUDA_CHECK(cudaFree(d_buf));
    std::puts("[B4_pinned_vs_pageable_and_unified] DONE");
    return 0;
}
