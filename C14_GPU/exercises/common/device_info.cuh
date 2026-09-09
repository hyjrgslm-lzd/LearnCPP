#pragma once
// [CMN-T10] device_info.cuh -- CUDA device property query and print helpers
//
// [CMN-T11] Provides three categories of helpers:
//   print_device_info()              : print full device info to stdout
//                                      (debugging / boot self-check)
//   get_peak_memory_bandwidth_gbps() : compute theoretical peak global-memory
//                                      bandwidth (GB/s)
//   has_hopper_features()            : true iff device has Hopper (CC 9.0+)
//   has_blackwell_features()         : true iff device has Blackwell (CC 10.0+)

#include <cstdio>
#include <cuda_runtime.h>

// --------------------------------------------------------------------------
// [CMN-T12] Print full device info
// --------------------------------------------------------------------------
inline void print_device_info(int device = 0) {
    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, device);

    // [CMN-T13] Basic identification
    std::fprintf(stdout, "=== Device %d: %s ===\n", device, p.name);
    std::fprintf(stdout, "  Compute Capability : %d.%d", p.major, p.minor);
    // [CMN-T14] Hopper and newer use the sm_XXa accelerated-arch tag.
    if (p.major >= 9) {
        std::fprintf(stdout, "  (sm_%d%da)", p.major, p.minor);
    }
    std::fprintf(stdout, "\n");

    // [CMN-T15] Execution resources
    std::fprintf(stdout, "  SM Count           : %d\n",  p.multiProcessorCount);
    std::fprintf(stdout, "  Max Threads/Block  : %d\n",  p.maxThreadsPerBlock);
    std::fprintf(stdout, "  Warp Size          : %d\n",  p.warpSize);

    // [CMN-T16] Shared memory
    // [CMN-T17] Default value (compile-time static upper bound)
    std::fprintf(stdout, "  Shared Mem/Block   : %zu KB (default)\n",
                 p.sharedMemPerBlock / 1024);
    std::fprintf(stdout, "  Shared Mem/SM      : %zu KB\n",
                 p.sharedMemPerMultiprocessor / 1024);
    // [CMN-T18] Opt-in dynamic upper bound (cudaFuncSetAttribute can raise
    // the per-block limit up to this value).
    {
        int val = 0;
        cudaDeviceGetAttribute(&val, cudaDevAttrMaxSharedMemoryPerBlockOptin, device);
        std::fprintf(stdout, "  Shared Mem/Block   : %d KB (opt-in max)\n", val / 1024);
    }

    // [CMN-T19] Registers
    std::fprintf(stdout, "  Registers/Block    : %d\n",  p.regsPerBlock);

    // [CMN-T20] Global memory and L2 cache
    std::fprintf(stdout, "  Global Memory      : %.1f GB\n",
                 static_cast<double>(p.totalGlobalMem) / (1 << 30));
    std::fprintf(stdout, "  L2 Cache Size      : %d MB\n",
                 p.l2CacheSize / (1 << 20));

    // [CMN-T21] Theoretical peak global-memory bandwidth
    // [CMN-T22] In CUDA 13 cudaDeviceProp::memoryClockRate was removed; use
    // cudaDeviceGetAttribute(cudaDevAttrMemoryClockRate, ...) instead.
    int memClockKHz = 0;
    cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, device);
    // [CMN-T23] BW (GB/s) = clock(Hz) * busWidth(bit) / 8 * 2(DDR) / 1e9
    double bw = static_cast<double>(memClockKHz) * 1e3       // Hz
                * (p.memoryBusWidth / 8.0)                    // bytes / cycle
                * 2.0                                         // DDR double-rate
                / 1e9;                                        // -> GB/s
    std::fprintf(stdout, "  Mem Clock          : %d MHz\n",   memClockKHz / 1000);
    std::fprintf(stdout, "  Mem Bus Width      : %d bit\n",   p.memoryBusWidth);
    std::fprintf(stdout, "  Peak Bandwidth     : %.1f GB/s\n", bw);

    // [CMN-T24] PCIe identification
    std::fprintf(stdout, "  PCIe Bus ID        : %d:%d.%d\n",
                 p.pciBusID, p.pciDeviceID, p.pciDomainID);

    // [CMN-T25] Hopper+ feature: thread-block cluster scheduling
    {
        int clusterLaunch = 0;
        // [CMN-T26] cudaDevAttrClusterLaunch is available since CUDA 11.8 / sm_90+
        cudaDeviceGetAttribute(&clusterLaunch, cudaDevAttrClusterLaunch, device);
        std::fprintf(stdout, "  Cluster Launch     : %s\n",
                     clusterLaunch ? "supported (sm_90+)" : "not supported");
    }

    // [CMN-T27] Async copy engine count
    std::fprintf(stdout, "  Async Engine Count : %d\n",  p.asyncEngineCount);

    std::fprintf(stdout, "\n");
}

// --------------------------------------------------------------------------
// [CMN-T28] Return theoretical peak global-memory bandwidth (GB/s, floored).
// --------------------------------------------------------------------------
inline int get_peak_memory_bandwidth_gbps(int device = 0) {
    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, device);
    int memClockKHz = 0;
    cudaDeviceGetAttribute(&memClockKHz, cudaDevAttrMemoryClockRate, device);
    // [CMN-T29] Same formula as in print_device_info above.
    double bw = static_cast<double>(memClockKHz) * 1e3
                * (p.memoryBusWidth / 8.0)
                * 2.0
                / 1e9;
    return static_cast<int>(bw);
}

// --------------------------------------------------------------------------
// [CMN-T30] Hopper (CC >= 9.0) feature detection.
// [CMN-T31] Implies wgmma / TMA / distributed shared memory / cluster.
// --------------------------------------------------------------------------
inline bool has_hopper_features(int device = 0) {
    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, device);
    return p.major >= 9;
}

// --------------------------------------------------------------------------
// [CMN-T32] Blackwell (CC >= 10.0) feature detection.
// [CMN-T33] Implies MXFP8 / FP4, 5th-gen Tensor Core, new TMA extensions.
// --------------------------------------------------------------------------
inline bool has_blackwell_features(int device = 0) {
    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, device);
    return p.major >= 10;
}
