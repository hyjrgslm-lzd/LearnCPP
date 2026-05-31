// [E4-T01] E4_cuda_graph_explicit/main.cu
// [E4-T02] Goal: learn the explicit graph API to build a DAG; use
//          cudaGraphExecKernelNodeSetParams to update kernel params at
//          runtime without re-capturing or re-instantiating.
//
// [E4-T03] Required tasks (Module E exercise E4):
//   [REQUIRED-1] cudaGraphCreate to make an empty graph
//   [REQUIRED-2] define three kernels: A (transform input), B (reduction),
//                C (normalize/output)
//   [REQUIRED-3] cudaGraphAddKernelNode to add the three nodes
//   [REQUIRED-4] cudaGraphAddDependencies to build B->A, C->B
//   [REQUIRED-5] cudaGraphInstantiate to compile graph
//   [REQUIRED-6] loop 10x: update nodeA input pointer ->
//                cudaGraphExecKernelNodeSetParams -> launch
//   [REQUIRED-7] compare "re-capture every iter" vs "dynamic param update"
//
// [E4-T04] Advanced tasks (TODO [ADVANCED]):
//   [ADVANCED-A] cudaGraphAddMemcpyNode: DtoH copy result after kernel C
//   [ADVANCED-B] cudaGraphAddEmptyNode as sync point
//   [ADVANCED-C] try removing nodes (cudaGraphRemoveNode), observe dangling deps

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------------------------
// [E4-T05] Constants
// ---------------------------------------------------------------------------
static constexpr int N_ELEM      = 1 << 22; // [E4-T06] 4M elems
static constexpr int BLOCK       = 256;
static constexpr int LOOP_ITERS  = 10;      // [E4-T07] dynamic update iter count
static constexpr int N_DATASETS  = LOOP_ITERS; // [E4-T08] one input array per iter

// ---------------------------------------------------------------------------
// [E4-T09] Kernel declarations (TODO area)
// ---------------------------------------------------------------------------

// [E4-T10] kernel_A: read input, write intermediate (e.g. elementwise square + bias).
// [E4-T11] TODO [REQUIRED-2] implement: out[idx] = in[idx] * in[idx] + bias.
__global__ void kernel_A(const float* __restrict__ in,
                          float* __restrict__ out,
                          int n,
                          float bias)
{
    // [E4-T12] TODO [REQUIRED-2] implement kernel_A
    (void)in; (void)out; (void)n; (void)bias;
}

// [E4-T13] kernel_B: block-level reduce of A's output (write into partial_sums).
// [E4-T14] TODO [REQUIRED-2] implement: each block sums its slice into partial_sums[blockIdx.x].
__global__ void kernel_B(const float* __restrict__ in,
                          float* __restrict__ partial_sums,
                          int n)
{
    // [E4-T15] TODO [REQUIRED-2] implement kernel_B (shared memory reduce ok)
    (void)in; (void)partial_sums; (void)n;
}

// [E4-T16] kernel_C: final reduction of partial_sums and normalized output.
// [E4-T17] TODO [REQUIRED-2] implement: total of partial_sums into result[0].
__global__ void kernel_C(const float* __restrict__ partial_sums,
                          float* __restrict__ result,
                          int num_blocks,
                          int n_total)
{
    // [E4-T18] TODO [REQUIRED-2] implement kernel_C (single thread or small block reduce)
    (void)partial_sums; (void)result; (void)num_blocks; (void)n_total;
}

// ---------------------------------------------------------------------------
// [E4-T19] Build explicit graph (one-time, fixed topology)
// ---------------------------------------------------------------------------
struct GraphBundle {
    cudaGraph_t     graph     = nullptr;
    cudaGraphExec_t graphExec = nullptr;
    cudaGraphNode_t nodeA     = nullptr;
    cudaGraphNode_t nodeB     = nullptr;
    cudaGraphNode_t nodeC     = nullptr;
    cudaKernelNodeParams paramsA{};
    cudaKernelNodeParams paramsB{};
    cudaKernelNodeParams paramsC{};
};

static GraphBundle build_explicit_graph(float* d_input,
                                         float* d_inter,
                                         float* d_partial,
                                         float* d_result,
                                         int grid)
{
    GraphBundle gb;

    // [E4-T20] TODO [REQUIRED-1] cudaGraphCreate(&gb.graph, 0)

    // [E4-T21] -- fill kernel_A param struct --
    float bias_val = 1.0f;
    // [E4-T22] TODO [REQUIRED-3] fill gb.paramsA:
    //   .func           = (void*)kernel_A
    //   .gridDim        = { (unsigned)grid, 1, 1 }
    //   .blockDim       = { BLOCK, 1, 1 }
    //   .sharedMemBytes = 0
    //   .kernelParams   = pointer-array (addresses of d_input, d_inter, N_ELEM, bias_val)
    //   .extra          = nullptr
    void* argsA[] = { &d_input, &d_inter, (void*)&N_ELEM, &bias_val };
    (void)argsA;

    // [E4-T23] TODO [REQUIRED-3] cudaGraphAddKernelNode(&gb.nodeA, gb.graph, nullptr, 0, &gb.paramsA)

    // [E4-T24] -- fill kernel_B param struct --
    void* argsB[] = { &d_inter, &d_partial, (void*)&N_ELEM };
    (void)argsB;
    // [E4-T25] TODO [REQUIRED-3] fill gb.paramsB, cudaGraphAddKernelNode(&gb.nodeB, ...)

    // [E4-T26] -- fill kernel_C param struct --
    int num_blocks = grid;
    void* argsC[] = { &d_partial, &d_result, &num_blocks, (void*)&N_ELEM };
    (void)argsC;
    // [E4-T27] TODO [REQUIRED-3] fill gb.paramsC, cudaGraphAddKernelNode(&gb.nodeC, ...)

    // [E4-T28] -- build deps: B depends on A, C depends on B --
    // [E4-T29] TODO [REQUIRED-4] cudaGraphAddDependencies(gb.graph, &gb.nodeA, &gb.nodeB, 1)
    // [E4-T30] TODO [REQUIRED-4] cudaGraphAddDependencies(gb.graph, &gb.nodeB, &gb.nodeC, 1)

    // [E4-T31] -- instantiate --
    // [E4-T32] TODO [REQUIRED-5] cudaGraphInstantiate(&gb.graphExec, gb.graph, NULL, NULL, 0)

    (void)bias_val; (void)d_input; (void)d_inter; (void)d_partial; (void)d_result;
    (void)grid; (void)num_blocks;
    return gb;
}

// ---------------------------------------------------------------------------
// [E4-T33] Dynamic param update loop
// ---------------------------------------------------------------------------
static float run_dynamic_update(GraphBundle& gb,
                                 float** d_inputs, // [E4-T34] N_DATASETS input arrays
                                 float* d_result,
                                 cudaStream_t stream)
{
    NVTX_RANGE("E4/DynamicParamUpdate");
    // [E4-T35]
    std::fprintf(stdout, "--- dynamic param update (%d iters) ---\n", LOOP_ITERS);

    CudaEventTimer timer;
    timer.start(stream);

    for (int iter = 0; iter < LOOP_ITERS; ++iter) {
        // [E4-T36] TODO [REQUIRED-6] point paramsA.kernelParams[0] at d_inputs[iter]
        //          gb.paramsA.kernelParams[0] = &d_inputs[iter];

        // [E4-T37] TODO [REQUIRED-6] cudaGraphExecKernelNodeSetParams(gb.graphExec, gb.nodeA, &gb.paramsA)

        // [E4-T38] TODO [REQUIRED-6] cudaGraphLaunch(gb.graphExec, stream)

        // [E4-T39] TODO [REQUIRED-6] wait + verify d_result[0] (optional, cudaMemcpyAsync)
        (void)iter;
    }

    CUDA_CHECK(cudaStreamSynchronize(stream));
    timer.stop(stream);

    float ms = timer.elapsed_ms();
    // [E4-T40]
    std::fprintf(stdout, "  dynamic update total = %.3f ms, avg = %.3f ms/iter\n\n",
                 ms, ms / LOOP_ITERS);
    (void)d_inputs; (void)d_result;
    return ms;
}

// ---------------------------------------------------------------------------
// [E4-T41] Comparison: re-capture each iteration (slow)
// ---------------------------------------------------------------------------
static float run_recapture_each_iter(float** d_inputs,
                                      float* d_inter,
                                      float* d_partial,
                                      float* d_result,
                                      int grid,
                                      cudaStream_t stream)
{
    NVTX_RANGE("E4/RecaptureEachIter");
    // [E4-T42]
    std::fprintf(stdout, "--- re-capture every iter (%d iters) ---\n", LOOP_ITERS);

    CudaEventTimer timer;
    timer.start(stream);

    for (int iter = 0; iter < LOOP_ITERS; ++iter) {
        float* cur_input = d_inputs[iter];
        float bias_val   = 1.0f;

        cudaGraph_t     g    = nullptr;
        cudaGraphExec_t exec = nullptr;

        // [E4-T43] TODO [REQUIRED-7] cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)
        // kernel_A<<<grid, BLOCK, 0, stream>>>(cur_input, d_inter, N_ELEM, bias_val)
        // kernel_B<<<grid, BLOCK, 0, stream>>>(d_inter, d_partial, N_ELEM)
        // kernel_C<<<1, BLOCK, 0, stream>>>(d_partial, d_result, grid, N_ELEM)
        // [E4-T44] TODO [REQUIRED-7] cudaStreamEndCapture(stream, &g)
        // [E4-T45] TODO [REQUIRED-7] cudaGraphInstantiate(&exec, g, NULL, NULL, 0)
        // [E4-T46] TODO [REQUIRED-7] cudaGraphLaunch(exec, stream)
        // [E4-T47] TODO [REQUIRED-7] cleanup: cudaGraphExecDestroy / cudaGraphDestroy

        (void)cur_input; (void)bias_val; (void)g; (void)exec;
        (void)d_inter; (void)d_partial; (void)d_result; (void)grid;
    }

    CUDA_CHECK(cudaStreamSynchronize(stream));
    timer.stop(stream);

    float ms = timer.elapsed_ms();
    // [E4-T48]
    std::fprintf(stdout, "  recapture total = %.3f ms, avg = %.3f ms/iter\n\n",
                 ms, ms / LOOP_ITERS);
    return ms;
}

// ---------------------------------------------------------------------------
// [E4-T49] main
// ---------------------------------------------------------------------------
int main()
{
    NVTX_RANGE("E4/main");
    print_device_info();

    // [E4-T50]
    std::fprintf(stdout, "=== E4: explicit Graph API and dynamic param update ===\n");
    // [E4-T51]
    std::fprintf(stdout, "N_ELEM = %d, grid = %d, LOOP_ITERS = %d\n\n",
                 N_ELEM, (N_ELEM + BLOCK - 1) / BLOCK, LOOP_ITERS);

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    // [E4-T52] -- allocate device memory --
    float* d_inter   = nullptr;
    float* d_partial = nullptr;
    float* d_result  = nullptr;
    CUDA_CHECK(cudaMalloc(&d_inter,   N_ELEM  * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_partial, grid    * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_result,  1       * sizeof(float)));

    // [E4-T53] allocate per-iter input arrays (simulates batches)
    float* d_inputs[N_DATASETS] = {};
    for (int i = 0; i < N_DATASETS; ++i) {
        CUDA_CHECK(cudaMalloc(&d_inputs[i], N_ELEM * sizeof(float)));
        // [E4-T54] TODO [REQUIRED-2] init d_inputs[i] with distinct values (cudaMemset or fill kernel)
    }

    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream));

    // [E4-T55] -- build explicit graph --
    GraphBundle gb = build_explicit_graph(d_inputs[0], d_inter, d_partial, d_result, grid);

    // [E4-T56] -- run dynamic-update version --
    float ms_dynamic = run_dynamic_update(gb, d_inputs, d_result, stream);

    // [E4-T57] -- run re-capture version --
    float ms_capture = run_recapture_each_iter(d_inputs, d_inter, d_partial, d_result, grid, stream);

    // [E4-T58] -- compare --
    std::fprintf(stdout, "=== compare ===\n");
    // [E4-T59]
    std::fprintf(stdout, "dynamic param update : %.3f ms total\n", ms_dynamic);
    // [E4-T60]
    std::fprintf(stdout, "recapture each iter  : %.3f ms total\n", ms_capture);
    if (ms_dynamic > 0.0f) {
        // [E4-T61]
        std::fprintf(stdout, "capture overhead ratio : %.2fx\n", ms_capture / ms_dynamic);
    }

    // [E4-T62] TODO [ADVANCED-A] cudaGraphAddMemcpyNode: DtoH copy node after nodeC

    // [E4-T63] -- cleanup --
    // [E4-T64] TODO release gb.graphExec, gb.graph
    for (int i = 0; i < N_DATASETS; ++i) {
        CUDA_CHECK(cudaFree(d_inputs[i]));
    }
    CUDA_CHECK(cudaFree(d_inter));
    CUDA_CHECK(cudaFree(d_partial));
    CUDA_CHECK(cudaFree(d_result));
    CUDA_CHECK(cudaStreamDestroy(stream));

    return 0;
}
