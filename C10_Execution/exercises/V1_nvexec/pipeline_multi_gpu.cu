/*
 * Copyright (c) 2022 NVIDIA Corporation
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Adapted for LearnCPP C10 from stdexec examples/nvexec/bulk.cu and
 * include/nvexec/multi_gpu_context.cuh at commit
 * 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43.
 */

#include <nvexec/multi_gpu_context.cuh>
#include <stdexec/execution.hpp>

#include <cuda_runtime_api.h>

#include <exception>
#include <iostream>
#include <numeric>

namespace ex = stdexec;

constexpr int items_per_device = 64;

int main() {
  try {
    int devices = 0;
    cudaError_t status = cudaGetDeviceCount(&devices);
    if (status != cudaSuccess) {
      std::cerr << "cudaGetDeviceCount failed: " << cudaGetErrorString(status) << '\n';
      return 1;
    }
    if (devices < 2) {
      std::cout << "SKIP: multi-GPU extension requires at least two CUDA devices\n";
      return 77;
    }

    int const item_count = devices * items_per_device;
    int *values = nullptr;
    int *gpu_markers = nullptr;
    if (cudaMallocManaged(&values, sizeof(int) * item_count) != cudaSuccess ||
        cudaMallocManaged(&gpu_markers, sizeof(int) * item_count) != cudaSuccess) {
      std::cerr << "cudaMallocManaged failed\n";
      cudaFree(values);
      cudaFree(gpu_markers);
      return 1;
    }

    for (int i = 0; i < item_count; ++i) {
      values[i] = i + 1;
      gpu_markers[i] = 0;
    }

    nvexec::multi_gpu_stream_context context;
    auto scheduler = context.get_scheduler();
    auto sender = ex::schedule(scheduler) | ex::bulk(ex::par, item_count, [=] __device__(int i) {
                    if (nvexec::is_on_gpu()) {
                      values[i] = values[i] * 2;
                      gpu_markers[i] = 1;
                    } else {
                      gpu_markers[i] = -1;
                    }
                  });
    ex::sync_wait(std::move(sender));

    status = cudaDeviceSynchronize();
    if (status != cudaSuccess) {
      std::cerr << "cudaDeviceSynchronize failed: " << cudaGetErrorString(status) << '\n';
      cudaFree(values);
      cudaFree(gpu_markers);
      return 1;
    }

    int expected_sum = item_count * (item_count + 1);
    int observed_sum = std::accumulate(values, values + item_count, 0);
    int gpu_marked = std::accumulate(gpu_markers, gpu_markers + item_count, 0);

    std::cout << "multi_gpu_logical_items=" << item_count << " cuda_device_count=" << devices
              << " observed_sum=" << observed_sum << " expected_sum=" << expected_sum
              << " gpu_marked_items=" << gpu_marked << '\n';

    bool ok = observed_sum == expected_sum && gpu_marked == item_count;
    cudaFree(values);
    cudaFree(gpu_markers);
    return ok ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << "multi_gpu_exception=" << error.what() << '\n';
    return 1;
  }
}
