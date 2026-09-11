# 12. 异构执行：把 sender 图接到 `nvexec`

本章只主讲 Execution 边界。CUDA 的线程层级、显存层级、kernel 优化、Nsight、CUDA Graph、多设备通信属于 C14；这里要解决的问题是：一个 CPU 侧 sender 图怎样把一段工作交给 GPU stream，并在完成后回到 `set_value` / `set_error` / `set_stopped` 这三条 completion channel。

先把三条边界分开。

第一条是对象边界。`thrust::device_vector`、`cudaMallocManaged` 得到的指针、host buffer、stream context 都是普通 C++ 对象或资源句柄。sender 里可以传裸指针，但裸指针不拥有资源。拥有资源的对象必须活到 `sync_wait` 返回，或者更一般地说，必须活到 operation state 发出终结信号之后。`connect` 创建“一次执行”的 operation state，`start` 才提交实际工作。终结回调允许销毁 operation state，所以终结之后后端代码不能再读写 operation state。

第二条是执行位置边界。`continues_on(stream.get_scheduler())` 不是“换一条 CPU 线程”，而是把后续能被 GPU stream domain 接管的 adaptor 放到这个 stream 的完成域里。`nvexec::launch` 会把 kernel 提交到 CUDA stream。`nvexec` 的 stream context 负责 stream 和 event 的串联；课程代码只验证 sender 的值、错误和生命周期，不把 stream/event 的所有细节变成 C10 主课。

第三条是工具链边界。固定参考实现是 stdexec `nvhpc-26.05`，commit `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。上游 README 说明 `nvexec::stream_scheduler` 和 `nvexec::multi_gpu_stream_scheduler` 需要 NVIDIA HPC SDK `nvc++` 配合 `-stdpar=gpu`。本机有 CUDA Toolkit 或 `nvcc` 不等于能验证 `nvexec`。如果缺 `nvc++`，本章 GPU 程序必须记为 `NVEXEC_NO_COMPILER`，不能用 CPU 模型或 NVCC 编译结果冒充通过。

## 单 GPU pipeline

单 GPU 实验在 `exercises/V1_nvexec/pipeline_single_gpu.cu`。它的形状来自固定上游 `examples/nvexec/launch.cu`，保留了上游许可，并把教学检查补完整。

程序先在 host 侧建立两个 device vector：

```cpp
thrust::device_vector<int> input(n);
thrust::device_vector<int> gpu_marker(1, 0);
thrust::sequence(input.begin(), input.end(), 1);
```

`input` 是真实数据，`gpu_marker` 是执行位置标记。`thrust::sequence` 在 host 侧发起初始化；初始化完成后，程序拿到两个 device pointer，并把 pointer 当作 sender value 传下去：

```cpp
auto sender =
    stdexec::just(first, last, marker)
  | stdexec::continues_on(stream.get_scheduler())
  | nvexec::launch(config, kernel)
  | stdexec::then(device_reduce);
```

这里的所有权很关键。`first`、`last`、`marker` 都是借用值。它们指向的 `device_vector` 在 `main` 的作用域里，晚于 `sync_wait` 销毁。这样 operation state 在执行期间不会拿着悬空地址。课程要求写清这个关系，是因为 sender 图看起来像值传递，但设备内存仍然有外部 owner。

kernel 做两件事。第一，按 index 把每个元素乘以 `scale`。第二，只有 `index == 0` 的线程写 `gpu_marker[0]`：

```cpp
on_gpu[0] = nvexec::is_on_gpu() ? 1 : -1;
```

不能只写 `assert(nvexec::is_on_gpu())`。Release 构建可能关闭 assert；而且 assert 失败通常只告诉你程序炸了，不给检查器一个稳定的值。这里用设备端写 marker，然后在 `sync_wait` 返回后通过 `thrust::host_vector` 回读。marker 是运行期证据，检查器可以在 Release 下拒绝“没有真的在 GPU 上执行”的路径。

`then` 仍在 GPU stream domain 中执行，返回 device 侧 reduce 结果：

```cpp
return nvexec::is_on_gpu() && on_gpu[0] == 1
         ? std::accumulate(begin, end, 0)
         : -1;
```

host 侧独立计算 `host_reference()`，再比较 `single_gpu_result == expected` 且 `gpu_marker == 1`。这两项缺一不可：结果对但 marker 错，说明位置证据不成立；marker 对但结果错，说明工作本身不成立。

## 多 GPU extension

多 GPU 实验在 `pipeline_multi_gpu.cu`。它使用 `nvexec::multi_gpu_stream_context` 和 `ex::bulk(ex::par, item_count, ...)`。固定上游 `include/nvexec/stream/bulk.cuh` 的 multi-GPU bulk 会按设备数把 logical shape 切成 share，并给每个设备建立 stream。课程代码验证的是这个协议层面的可观察结果：

- 至少两个 CUDA device，否则只跳过多 GPU extension；
- 每个 logical item 都被 device 代码访问；
- 每个 item 的值都从 `i + 1` 变成 `2 * (i + 1)`；
- 每个 item 都写入 `gpu_markers[i] = 1`；
- `sync_wait` 返回后再 `cudaDeviceSynchronize()`，随后 host 检查 managed memory。

这仍然不是“已证明每个物理 GPU 都参与执行”。`cudaGetDeviceCount()` 只能证明机器报告了多少 CUDA device；输出 `multi_gpu_logical_items` 和 `gpu_marked_items` 只能证明 logical bulk 全部完成且在 GPU domain 中写过 marker。要证明每个物理设备实际执行了哪一段，需要围绕同一个二进制采 Nsight Systems 或 CUPTI trace。本课不把 trace 伪装成普通 stdout。

## 编译与标准模式

`V1_nvexec/CMakeLists.txt` 不下载依赖，也不找别的实现替代。它只看本地 pinned stdexec checkout 和本机 `nvc++`。

有 `nvc++` 时，配置阶段先用真实 nvexec 编译选项探测 C++26：

```text
nvc++ -stdpar=gpu -gpu=mem:separate -std=c++26 -x c++ -I<STDEXEC_ROOT>/include probe.cu
```

如果这个模式探测失败，只降低语言模式为 `-std=c++23`。`-stdpar=gpu`、`-gpu=mem:separate`、`-x c++` 不能被删除。`.cu` 文件在 NVHPC 路径按 C++ 语言解析，这与固定上游 nvexec examples 的 CMake 做法一致。语言模式探测失败只说明 C++26 模式不可用；后续 `.cu` 主体编译或运行失败仍然是失败，不能改写成 SKIP。

缺 `nvc++` 时，`V1_nvexec_single_gpu` 和 `V1_nvexec_multi_gpu` 是两个独立 SKIP。这样将来机器补齐 HPC SDK 后，单 GPU 不会被多 GPU 的设备数 SKIP 掩盖，多 GPU 也不会被单 GPU 的通过冒充。

## 完成通道与错误处理

CUDA API 返回错误时，教学程序在 host 边界返回非零退出码；`nvexec` 内部 sender 会把 stream 相关错误映射到 `set_error`。课程检查不允许把 CUDA 错误藏在 assert 或 stdout 后继续 PASS。停止请求也不等于完成：只有 stream 工作产生终结信号，`sync_wait` 才能返回。

读代码时按这个顺序检查：

1. owner 是谁：device vector 或 managed allocation 在哪里创建、在哪里释放；
2. pointer 从哪里借出：sender value channel 传的是地址，不是所有权；
3. 执行位置在哪里改变：`continues_on` / `schedule` 绑定哪个 scheduler；
4. 哪个 adaptor 提交真实 GPU 工作：`nvexec::launch` 或 multi-GPU `bulk`；
5. 结果如何回到 host：`sync_wait` 后回读 marker 和输出；
6. 错误如何失败：CUDA API 失败、异常、结果不匹配都必须非零退出。

本章的验收不是“源码存在”。当前本机缺 `nvc++` 时，正确状态是：代码、命令和检查完整；GPU body 未验证；CTest 中 nvexec 两项按 `NVEXEC_NO_COMPILER` 独立 SKIP。
