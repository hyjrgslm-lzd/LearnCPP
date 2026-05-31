// ============================================================
// [B1-T01] Exercise B1: global_coalescing
// [B1-T02] Goal: observe how memory coalescing affects global memory throughput.
//          Compare sequential vs strided access bandwidth, and AoS vs SoA layout.
//
// [B1-T03] Acceptance:
//   sequential throughput >> stride-32 throughput (5-10x or more)
//   Nsight Compute: ncu --set memory_l1_l2 -o b1.ncu-rep ./B1_global_coalescing.exe
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
// [B1-T04] Constants
// ------------------------------------------------------------
static constexpr int   N_ELEM    = 1 << 24;   // ~100 MB of float (16M * 4B = 64MB)
static constexpr int   BLOCK_SZ  = 256;
static constexpr int   WARMUP    = 3;         // [B1-T05] warmup iterations

// ------------------------------------------------------------
// [B1-T06] AoS (Array of Structures) layout
// ------------------------------------------------------------
struct ParticleAoS {
    float x, y, z, w;   // [B1-T07] interleaved storage; reading .x has stride sizeof(ParticleAoS)=16
};

// ------------------------------------------------------------
// [B1-T08] SoA (Structure of Arrays) layout
// ------------------------------------------------------------
struct ParticleSoA {
    float* x;   // [B1-T09] all particles' x stored contiguously -> coalesced access
    float* y;
    float* z;
    float* w;
};

// ------------------------------------------------------------
// [B1-T10] Kernel 1: sequential coalesced read
// ------------------------------------------------------------
// [B1-T11] TODO [REQUIRED] step 1: implement the sequential read kernel
//   - int idx = blockIdx.x * blockDim.x + threadIdx.x;
//   - if (idx < N) data_out[idx] = data_in[idx];
__global__ void coalesce_read_sequential(const float* __restrict__ data_in,
                                         float*       __restrict__ data_out,
                                         int n)
{
    // [B1-T12] TODO [REQUIRED]: int idx = blockIdx.x * blockDim.x + threadIdx.x;
    // [B1-T13] TODO [REQUIRED]: if (idx < n) { data_out[idx] = data_in[idx]; }
    (void)data_in; (void)data_out; (void)n;
}

// ------------------------------------------------------------
// [B1-T14] Kernel 2: stride-32 access (uncoalesced, scattered)
// ------------------------------------------------------------
// [B1-T15] TODO [REQUIRED] step 3: implement the stride-32 read kernel
//   - int idx = (blockIdx.x * blockDim.x + threadIdx.x) * 32;
//   - if (idx < N) data_out[idx] = data_in[idx];
__global__ void coalesce_read_strided(const float* __restrict__ data_in,
                                      float*       __restrict__ data_out,
                                      int n)
{
    // [B1-T16] TODO [REQUIRED]: int idx = (blockIdx.x * blockDim.x + threadIdx.x) * 32;
    // [B1-T17] TODO [REQUIRED]: if (idx < n) { data_out[idx] = data_in[idx]; }
    (void)data_in; (void)data_out; (void)n;
}

// ------------------------------------------------------------
// [B1-T18] Kernel 3: AoS read (only the .x field, uncoalesced)
// ------------------------------------------------------------
// [B1-T19] TODO [REQUIRED] step 7 (AoS): implement AoS read.
//   address gap between threads in a warp = sizeof(ParticleAoS) = 16 bytes -> not coalesced
__global__ void read_aos(const ParticleAoS* __restrict__ particles,
                         float*             __restrict__ out,
                         int n)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // [B1-T20] TODO [REQUIRED]: out[idx] = particles[idx].x;
        (void)particles; (void)out;
    }
}

// ------------------------------------------------------------
// [B1-T21] Kernel 4: SoA read (only the .x field, coalesced)
// ------------------------------------------------------------
// [B1-T22] TODO [REQUIRED] step 7 (SoA): implement SoA read.
//   threads in a warp access x[i], x[i+1], ... contiguously -> coalesced
__global__ void read_soa(const float* __restrict__ soa_x,
                         float*       __restrict__ out,
                         int n)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        // [B1-T23] TODO [REQUIRED]: out[idx] = soa_x[idx];
        (void)soa_x; (void)out;
    }
}

// ------------------------------------------------------------
// [B1-T24] Advanced kernel: random access
// ------------------------------------------------------------
// [B1-T25] TODO [ADVANCED] implement a random-access kernel and observe throughput drop
#if 0
__global__ void coalesce_read_random(const float* data_in, float* data_out, int n) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        int idx = (tid * 12345) % n;
        data_out[tid] = data_in[idx];
    }
}
#endif

// ------------------------------------------------------------
// [B1-T26] Timing helper: run kernel N_RUNS times, return average GB/s
// ------------------------------------------------------------
template<typename KernelFn>
static float bench_gbps(KernelFn fn, long long bytes, int n_runs = 5) {
    // [B1-T27] warmup
    for (int i = 0; i < WARMUP; i++) fn();
    CUDA_CHECK(cudaDeviceSynchronize());

    CudaEventTimer timer;
    timer.start();
    for (int i = 0; i < n_runs; i++) fn();
    timer.stop();

    float ms = timer.elapsed_ms() / n_runs;
    return static_cast<float>(bytes) / ms / 1e6f;   // [B1-T28] GB/s = bytes / ms / 1e6
}

// ------------------------------------------------------------
// [B1-T29] main
// ------------------------------------------------------------
int main() {
    std::puts("[B1_global_coalescing]");
    print_device_info(0);

    int peak_bw = get_peak_memory_bandwidth_gbps(0);
    // [B1-T30]
    std::printf("[B1] theoretical peak bandwidth: %d GB/s\n", peak_bw);

    const long long BYTES = (long long)N_ELEM * sizeof(float);

    // [B1-T31] -- allocate device memory --
    float *d_in = nullptr, *d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,  BYTES));
    CUDA_CHECK(cudaMalloc(&d_out, BYTES));

    // [B1-T32] initialize input (just fill with arbitrary values)
    {
        int* tmp = (int*)std::malloc(BYTES);
        for (int i = 0; i < N_ELEM; i++) tmp[i] = i;
        CUDA_CHECK(cudaMemcpy(d_in, tmp, BYTES, cudaMemcpyHostToDevice));
        std::free(tmp);
    }

    dim3 block(BLOCK_SZ);
    dim3 grid_seq((N_ELEM + BLOCK_SZ - 1) / BLOCK_SZ);
    // [B1-T33] stride-32 needs a smaller grid (each thread accesses elements with stride 32)
    dim3 grid_str((N_ELEM / 32 + BLOCK_SZ - 1) / BLOCK_SZ);

    std::printf("[launch seq]    grid=%u  block=%u  elements=%d\n",
                grid_seq.x, block.x, N_ELEM);
    std::printf("[launch stride] grid=%u  block=%u  elements=%d\n",
                grid_str.x, block.x, N_ELEM / 32);

    // [B1-T34] -- timing sequential access --
    // [B1-T35] TODO [REQUIRED] step 2: fill in coalesce_read_sequential and time it
    float gbps_seq;
    {
        NVTX_RANGE("B1/sequential");
        gbps_seq = bench_gbps([&] {
            coalesce_read_sequential<<<grid_seq, block>>>(d_in, d_out, N_ELEM);
        }, BYTES * 2LL);   // [B1-T36] 1 read + 1 write = 2 * BYTES
        // [B1-T37]
        std::printf("[B1] sequential throughput:  %.1f GB/s\n", gbps_seq);
    }

    // [B1-T38] -- timing stride-32 access --
    // [B1-T39] TODO [REQUIRED] step 4: fill in coalesce_read_strided and time it
    float gbps_str;
    {
        NVTX_RANGE("B1/strided");
        // [B1-T40] stride-32 only touches N_ELEM/32 elements
        long long bytes_str = (long long)(N_ELEM / 32) * sizeof(float) * 2;
        gbps_str = bench_gbps([&] {
            coalesce_read_strided<<<grid_str, block>>>(d_in, d_out, N_ELEM);
        }, bytes_str);
        // [B1-T41]
        std::printf("[B1] stride-32 throughput:   %.1f GB/s\n", gbps_str);
    }

    // [B1-T42] TODO [REQUIRED] step 5: record and compare
    std::printf("[B1] seq / stride ratio:     %.1fx\n", gbps_seq / (gbps_str + 1e-6f));
    // [B1-T43]
    std::printf("[B1] seq vs peak:            %.0f%%\n",
                gbps_seq / static_cast<float>(peak_bw) * 100.0f);

    // [B1-T44] -- AoS vs SoA comparison --
    // [B1-T45] TODO [REQUIRED] step 7
    {
        NVTX_RANGE("B1/AoS_vs_SoA");

        int np = N_ELEM / 4;  // [B1-T46] particle count
        long long bytes_p = (long long)np * sizeof(float);

        ParticleAoS* d_aos = nullptr;
        float *d_soa_x = nullptr, *d_out_p = nullptr;
        CUDA_CHECK(cudaMalloc(&d_aos,    (long long)np * sizeof(ParticleAoS)));
        CUDA_CHECK(cudaMalloc(&d_soa_x,  bytes_p));
        CUDA_CHECK(cudaMalloc(&d_out_p,  bytes_p));

        dim3 grid_p((np + BLOCK_SZ - 1) / BLOCK_SZ);

        float gbps_aos = bench_gbps([&] {
            read_aos<<<grid_p, block>>>(d_aos, d_out_p, np);
        }, bytes_p * 2LL);

        float gbps_soa = bench_gbps([&] {
            read_soa<<<grid_p, block>>>(d_soa_x, d_out_p, np);
        }, bytes_p * 2LL);

        // [B1-T47]
        std::printf("[B1] AoS read .x throughput: %.1f GB/s\n", gbps_aos);
        // [B1-T48]
        std::printf("[B1] SoA read .x throughput: %.1f GB/s\n", gbps_soa);
        // [B1-T49]
        std::printf("[B1] SoA / AoS ratio:        %.1fx\n", gbps_soa / (gbps_aos + 1e-6f));

        CUDA_CHECK(cudaFree(d_aos));
        CUDA_CHECK(cudaFree(d_soa_x));
        CUDA_CHECK(cudaFree(d_out_p));
    }

    // [B1-T50] -- Nsight Compute hint --
    std::puts("------------------------------------------------------------");
    // [B1-T51]
    std::puts("  TODO [REQUIRED] step 6: run Nsight Compute:");
    std::puts("    ncu --set memory_l1_l2 -o b1.ncu-rep ./B1_global_coalescing.exe");
    // [B1-T52]
    std::puts("  Inspect L1 hit rate, L2 miss rate, HBM bandwidth utilization.");
    std::puts("------------------------------------------------------------");

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));

    std::puts("[B1_global_coalescing] DONE");
    return 0;
}
