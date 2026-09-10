# L06 process IPC

目标：实现 `c07_l06::run_process_ipc()`，让父进程启动同一个可执行文件的 child 模式，通过共享字节区、平台同步对象和 pipe frame 完成一次可验证交换。

## Part

1. 正常路径：创建 child，传入共享区、同步对象和 pipe 写端，等待 child 写回未知 payload 的变换结果。
2. 坏路径：child 可执行文件不存在时返回受控错误。
3. 短帧：child 写不完整 frame 时拒绝成功。
4. peer close：child 关闭 pipe 但不写 frame 时拒绝成功。
5. 异常退出和 timeout：父进程必须回收 child；timeout 只终止本次启动的 child。

## 接口

```cpp
c07_l06::ProcessResult run_process_ipc(
    const std::filesystem::path& executable,
    std::string_view payload,
    const std::filesystem::path& witness_path,
    c07_l06::ChildMode mode,
    std::chrono::milliseconds timeout);
```

checker 持有真实 child fixture。实现必须消费 `executable`、`payload`、`witness_path`、`mode` 和 `timeout`，不能只返回自报成功。checker 会在 supervisor 私有临时目录传入 witness path；只有真实 child fixture 知道如何写出带 child pid 与运行时 payload 结果的 witness。checker 独立验证 witness、pipe frame、共享字节、exit code 和回收状态，不信 `ok` 自报。

## 本地命令

```sh
cmake --build C07_OS_Memory_System_IO/exercises/build/process-author-debug --config Debug --target L06_process_ipc_reference
ctest --test-dir C07_OS_Memory_System_IO/exercises/build/process-author-debug -C Debug -R L06_process_ipc
```

## 观察：文件字节锁

`L06_file_locking` 是观察型入口，不是 Student 作业。它使用自有临时文件验证平台字节锁：

- Windows：同一进程内两个独立文件句柄，`LockFileEx(LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY)` 验证重叠范围被拒绝、非重叠范围可用、`UnlockFileEx` 后重试成功。
- Linux：POSIX `fcntl(F_SETLK)` record lock 是进程关联；同进程两个 fd 不能伪造冲突，所以用真实 `fork` child 尝试重叠锁并通过 pipe 回传 `errno`，parent `waitpid` 回收后再验证解锁重试成功。Linux 同一观察里还演示 `pread`/`pwrite` 不推进共享 file offset。

```sh
cmake --build C07_OS_Memory_System_IO/exercises/build/process-author-debug --config Debug --target L06_file_locking
ctest --test-dir C07_OS_Memory_System_IO/exercises/build/process-author-debug -C Debug -R L06_file_locking
```


