// [C4-T01] C4_hopper_cluster_launch/main.cu
// [C4-T02] Exercise C4: Hopper Thread Block Cluster - compile-time decl + runtime launch + distributed smem
//
// [C4-T03] Hardware requirement: sm_90a (Hopper)
//   This target sets CUDA_ARCHITECTURES "90a" explicitly in CMakeLists.txt
//
// [C4-T04] Goals:
//   - __cluster_dims__(2,1,1) compile-time declaration
//   - cudaLaunchKernelEx + cudaLaunchConfig_t runtime launch
//   - cluster.map_shared_rank to access neighboring block's smem
//   - cluster.sync() cross-block barrier
//
// [C4-T05] Build: cmake --build build --target C4_hopper_cluster_launch
// Run:   ./C4_hopper_cluster_launch
// Nsight Compute: ncu --set full -o c4_profile ./C4_hopper_cluster_launch

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
// [C4-T06] cooperative_groups provides the cluster group abstraction
#include <cooperative_groups.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

namespace cg = cooperative_groups;

// ------------------------------------------------------------
// [C4-T07] Constants
// ------------------------------------------------------------
constexpr int BLOCK_SIZE   = 128;
constexpr int SMEM_WORDS   = 32; // [C4-T08] number of int words used per block in smem
constexpr int CLUSTER_DIM_X = 2; // [C4-T09] cluster has 2 blocks (along x)

// ------------------------------------------------------------
// [C4-T10] Kernel: cluster launch demo
//   Compile-time __cluster_dims__(2,1,1) declares cluster size
//   Runtime cudaLaunchKernelEx specifies it (the two must match)
//
// [C4-T11] TODO [REQUIRED-1] __cluster_dims__ declaration
// [C4-T12] TODO [REQUIRED-3] read clusterIdx / clusterSize
// [C4-T13] TODO [REQUIRED-4] distributed smem access
// ------------------------------------------------------------

// [C4-T14] TODO [REQUIRED-1] uncomment the next line to add the compile-time cluster declaration:
// __cluster_dims__(2, 1, 1)
__global__ void cluster_kernel(int* out, int n)
{
    // [C4-T15] TODO [REQUIRED-3] obtain the cluster group
    // auto cluster = cg::this_cluster();
    // auto block   = cg::this_thread_block();

    __shared__ int smem[SMEM_WORDS];

    int tid      = threadIdx.x;
    int block_id = blockIdx.x; // [C4-T16] block's global index in the grid

    // [C4-T17] initialize smem: each block writes block_id * 100 + tid
    if (tid < SMEM_WORDS) {
        smem[tid] = block_id * 100 + tid;
    }
    __syncthreads();

    // ------------------------------------------------------------
    // [C4-T18] TODO [REQUIRED-4] cluster.sync(): wait for all blocks in cluster to finish smem init
    // cluster.sync();
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [C4-T19] TODO [REQUIRED-4] access neighbor block's smem (XOR 1)
    // int  peer_rank = block_id ^ 1;           // neighboring block in the cluster
    // int* peer_smem = cluster.map_shared_rank(smem, peer_rank);
    // int  peer_val  = peer_smem[tid % SMEM_WORDS];
    // ------------------------------------------------------------

    // [C4-T20] TODO [REQUIRED-5] verify distributed smem address calculation
    // Expected: peer_val == (peer_rank * 100 + tid % SMEM_WORDS)

    int global_tid = block_id * blockDim.x + tid;
    if (global_tid < n) {
        // [C4-T21] TODO: after the fix, write peer_val; for now write stub=0 to keep the check visible
        out[global_tid] = 0;
    }

    // [C4-T22] TODO [REQUIRED-3] print cluster / block info (lane 0 of each block)
    if (tid == 0) {
        // [C4-T23]
        printf("block_id=%d  (cluster info: TODO [REQUIRED-3])\n", block_id);
    }
}

// ------------------------------------------------------------
// [C4-T24] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);

    // [C4-T25] Hopper feature detection
    if (!has_hopper_features(0)) {
        // [C4-T26]
        printf("[SKIP] sm_90a not available on this device. Hopper cluster skipped.\n");
        return 0;
    }

    NVTX_RANGE("C4/main");

    constexpr int N = BLOCK_SIZE * CLUSTER_DIM_X; // [C4-T27] 2 blocks, total N threads
    int* d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_out, N * sizeof(int)));
    CUDA_CHECK(cudaMemset(d_out, 0, N * sizeof(int)));

    // ------------------------------------------------------------
    // [C4-T28] TODO [REQUIRED-2] use cudaLaunchKernelEx (replacing <<<...>>>)
    //
    // cudaLaunchConfig_t cfg    = {};
    // cfg.gridDim               = dim3(CLUSTER_DIM_X, 1, 1); // 2 blocks in grid
    // cfg.blockDim              = dim3(BLOCK_SIZE,    1, 1);
    // cfg.dynamicSmemBytes      = 0;
    // cfg.stream                = 0;
    //
    // cudaLaunchAttribute attr[1];
    // attr[0].id                       = cudaLaunchAttributeClusterDimension;
    // attr[0].val.clusterDim.x         = CLUSTER_DIM_X;
    // attr[0].val.clusterDim.y         = 1;
    // attr[0].val.clusterDim.z         = 1;
    // cfg.attrs                        = attr;
    // cfg.numAttrs                     = 1;
    //
    // void* args[] = { &d_out, const_cast<int*>(&N) };  // kernel arguments
    // CUDA_CHECK(cudaLaunchKernelEx(&cfg, cluster_kernel, args, nullptr));
    // ------------------------------------------------------------

    // [C4-T29] use the legacy launch for now (replace with cudaLaunchKernelEx after the TODO)
    // [C4-T30]
    printf("Launch config: grid=%d block=%d (cluster=%dx1x1)\n",
           CLUSTER_DIM_X, BLOCK_SIZE, CLUSTER_DIM_X);
    cluster_kernel<<<CLUSTER_DIM_X, BLOCK_SIZE>>>(d_out, N);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // ------------------------------------------------------------
    // [C4-T31] verify the output (after the TODO, check distributed smem data)
    // ------------------------------------------------------------
    int h_out[N];
    CUDA_CHECK(cudaMemcpy(h_out, d_out, N * sizeof(int), cudaMemcpyDeviceToHost));
    // [C4-T32]
    printf("out[0]=%d out[1]=%d ... (stub=0; after the TODO, should be peer smem values)\n",
           h_out[0], h_out[1]);

    // ------------------------------------------------------------
    // [C4-T33] TODO [REQUIRED-6] verify cluster launch in Nsight Compute
    //   ncu --set full -o c4_profile ./C4_hopper_cluster_launch
    //   In "Launch Statistics", check the "Cluster Size" field
    //   In "Memory Workload Analysis", confirm the distributed shared memory address range
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [C4-T34] Advanced kernel 2: cluster all-reduce stub
    // [C4-T35] TODO [ADVANCED] clusterDim=(2,2,1), 4 blocks share data
    // ------------------------------------------------------------
    // [C4-T36]
    printf("\n--- Advanced: cluster all-reduce stub ---\n");
    {
        // [C4-T37] Each block contributes a value, the cluster gathers it into block 0's smem[0]
        // Steps:
        //   1. each block initializes smem[0] = blockIdx.x + 1
        //   2. cluster.sync()
        //   3. block 0 walks the other blocks' smem[0] and accumulates locally
        //   4. cluster.sync()
        //   5. block 0 writes the result to out[0]
        //
        // [C4-T38] TODO [ADVANCED]: implement the logic above; expected result = 1 + 2 = 3 (2-block cluster)

        // [C4-T39]
        printf("  [advanced stub] cluster all-reduce not implemented; expected out[0]=3\n");
    }

    // ------------------------------------------------------------
    // [C4-T40] Advanced kernel 3: clusterDim=(2,2,1) - 4 blocks share
    // [C4-T41] TODO [ADVANCED]
    // ------------------------------------------------------------
    // [C4-T42]
    printf("\n--- Advanced: 2x2 cluster (4 blocks) ---\n");
    {
        // [C4-T43] Launch config: gridDim=(4,1,1), clusterDim=(2,2,1)
        // each block writes smem, cluster.sync(), then reads diagonal block's smem
        //
        // [C4-T44] TODO [ADVANCED]: validate distributed smem addresses in a 4-block cluster
        //   block (0,0) reads smem of block (1,1)
        //   block (0,1) reads smem of block (1,0)

        // [C4-T45]
        printf("  [advanced stub] 2x2 cluster not implemented\n");
    }

    // ------------------------------------------------------------
    // [C4-T46] Manually verify distributed smem address math (no GPU, just print formula)
    // ------------------------------------------------------------
    // [C4-T47]
    printf("\n--- Distributed smem address calculation ---\n");
    // [C4-T48]
    printf("  base address of smem in cluster block rank 0 = smem_ptr (local)\n");
    // [C4-T49]
    printf("  base address of smem in cluster block rank 1 = cluster.map_shared_rank(smem_ptr, 1)\n");
    // [C4-T50]
    printf("  access element k of peer block smem: peer_smem[k]\n");
    // [C4-T51]
    printf("  Note: peer_smem may only be read safely after cluster.sync()\n");

    CUDA_CHECK(cudaFree(d_out));
    // [C4-T52]
    printf("\n[C4] done. Use Nsight Compute to verify cluster launch and distributed smem.\n");
    return 0;
}
