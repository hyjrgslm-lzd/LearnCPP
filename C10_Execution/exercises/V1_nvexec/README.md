# V1_nvexec: nvexec GPU execution bridge

This unit is explicit about the compiler boundary. CUDA Toolkit and `nvcc` are
not enough for this stdexec `nvexec` path; the pinned upstream examples build
with NVIDIA HPC SDK `nvc++`.

Current observation behavior:

- without `nvc++`, the single-GPU and multi-GPU observations return `SKIP` with
  `NVEXEC_NO_COMPILER`;
- with `nvc++`, configure probes `-std=c++26` together with the real nvexec
  flags. If the mode probe fails, only the selected language mode is lowered to
  `-std=c++23`; body compile or runtime failure is still a failure;
- with `nvc++`, CMake builds `pipeline_single_gpu.cu` and
  `pipeline_multi_gpu.cu`; a compile failure is a build failure;
- CTest runs the built programs. The single-GPU program must check its result;
- if only one CUDA device exists, `pipeline_multi_gpu.cu` returns `77`.

Future command shape:

```powershell
nvc++ -stdpar=gpu -gpu=mem:separate -std=c++26 -x c++ -I%STDEXEC_ROOT%/include pipeline_single_gpu.cu -o pipeline_single_gpu.exe
nvc++ -stdpar=gpu -gpu=mem:separate -std=c++26 -x c++ -I%STDEXEC_ROOT%/include pipeline_multi_gpu.cu -o pipeline_multi_gpu.exe
```

If the `-std=c++26` probe fails, replace only `-std=c++26` with `-std=c++23`.
Do not remove `-stdpar=gpu`, `-gpu=mem:separate`, or `-x c++`; those mirror the
pinned upstream nvexec example setup for `nvc++` compiling `.cu` sources as C++.

The single GPU pipeline owns its device vector until `sync_wait` returns. The
sender moves from host setup, through `continues_on(stream.get_scheduler())`, to
`nvexec::launch`, then reduces on the same stream before returning to the host.
The stream/event details are owned by `nvexec::stream_context`; the operation
state and device buffers must outlive the terminal completion.

`pipeline_single_gpu.cu` initializes `1..N` on the host through Thrust, launches
the GPU transform, writes a device-only marker with `nvexec::is_on_gpu()`, then
copies the marker back after `sync_wait`. Release builds do not rely on
`assert`.

`pipeline_multi_gpu.cu` verifies multi-GPU bulk as a logical-domain operation:
each logical item must be doubled and marked by device code. The program prints
`cuda_device_count`, `multi_gpu_logical_items`, and result totals. That is not a
proof of physical device distribution. To prove which devices actually ran each
slice, record a CUDA trace with Nsight Systems or CUPTI around the same binary.
