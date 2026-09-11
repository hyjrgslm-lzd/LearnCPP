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
 * Adapted for LearnCPP C10 from stdexec examples/nvexec/launch.cu at
 * commit 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43.
 */

#include <nvexec/stream_context.cuh>
#include <stdexec/execution.hpp>

#include <cuda_runtime_api.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <numeric>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>
#include <vector>

namespace ex = stdexec;

constexpr int n = 1024;
constexpr int scale = 3;
constexpr int block_size = 128;
constexpr int grid_size = (n + block_size - 1) / block_size;

int host_reference() {
  std::vector<int> values(n);
  std::iota(values.begin(), values.end(), 1);
  for (int &value : values) {
    value *= scale;
  }
  return std::accumulate(values.begin(), values.end(), 0);
}

int main() {
  try {
    int devices = 0;
    cudaError_t count_status = cudaGetDeviceCount(&devices);
    if (count_status != cudaSuccess) {
      std::cerr << "cudaGetDeviceCount failed: " << cudaGetErrorString(count_status) << '\n';
      return 1;
    }
    if (devices < 1) {
      std::cerr << "no CUDA device available\n";
      return 1;
    }

    thrust::device_vector<int> input(n);
    thrust::device_vector<int> gpu_marker(1, 0);
    thrust::sequence(input.begin(), input.end(), 1);
    int *first = thrust::raw_pointer_cast(input.data());
    int *last = first + input.size();
    int *marker = thrust::raw_pointer_cast(gpu_marker.data());

    nvexec::stream_context stream;

    auto sender =
        ex::just(first, last, marker) | ex::continues_on(stream.get_scheduler()) |
        nvexec::launch({.grid_size = grid_size, .block_size = block_size},
                       [] __device__(cudaStream_t, int *begin, int *end, int *on_gpu) {
                         int index = blockIdx.x * blockDim.x + threadIdx.x;
                         if (index == 0) {
                           on_gpu[0] = nvexec::is_on_gpu() ? 1 : -1;
                         }
                         if (begin + index < end) {
                           begin[index] *= scale;
                         }
                       }) |
        ex::then([] __device__(int *begin, int *end, int *on_gpu) {
          return nvexec::is_on_gpu() && on_gpu[0] == 1 ? std::accumulate(begin, end, 0) : -1;
        });

    auto [result] = ex::sync_wait(std::move(sender)).value();
    thrust::host_vector<int> marker_host = gpu_marker;
    int expected = host_reference();

    std::cout << "single_gpu_result=" << result << " expected=" << expected
              << " gpu_marker=" << marker_host[0] << " device_count=" << devices << '\n';

    if (marker_host[0] != 1) {
      std::cerr << "device marker was not written on GPU\n";
      return 1;
    }
    return result == expected ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << "single_gpu_exception=" << error.what() << '\n';
    return 1;
  }
}
