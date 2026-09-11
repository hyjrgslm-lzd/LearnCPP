# C07 验证工具

这些小工具只收集和整理证据，不批准课程正文，也不替代独立教学/技术审查。

## snapshot_linux.py

在 WSL2 Linux 发行版内运行。它从 `/mnt/f/CPPTrain/LearnCPP` 复制 C07，以及必要的 C01/C02 公共 include/tool 输入，到全新的 `/root/learncpp-c07/.../snapshots/<runid>`；拒绝非 ext4 目标；跳过 symlink、build 树、cache、deps、二进制和原始 validation 树；写出 `snapshot-manifest.json`，包含源路径与副本 SHA256。

```sh
python3 C07_OS_Memory_System_IO/exercises/tools/snapshot_linux.py \
  --source /mnt/f/CPPTrain/LearnCPP \
  --dest-root /root/learncpp-c07/snapshots \
  --run-id linux-r1
```

## verify_students.py

用于 Reference OFF、Student ON、include trace 构建之后。构建目录必须含 CMake file-api codemodel、父 CMake 生成的 `student-targets-<Config>.txt`，以及 clean rebuild 记录出的真实 include trace。脚本调用现有 C02 `audit_student.py`，从 codemodel artifact 解析每个 Student 可执行文件，再通过 `ctest --show-only=json-v1` 读取同名 Student 测试的真实注册命令，使用 `run_test.py --` 后的完整 subject argv 运行。像 L09 这类需要 DLL/SO fixture 路径的题目，module 参数必须来自 CTest 注册命令；脚本会核对 argv[0] 与 codemodel artifact 一致，缺注册或路径不符直接 FAIL，不猜参数。最终要求受控 checker 拒绝：exit 1 且输出含 `check failed`。

Student 运行通过同目录 `run_test.py` 的 `supervise()` 执行。supervisor 会为每个 child 设置专用临时目录，并在 child 即使用 `exit(1)` 跳过 C++ 清理时由 Python 侧回收；timeout、崩溃、cleanup 错误仍不是合格的 Student 拒绝。

```sh
python3 C07_OS_Memory_System_IO/exercises/tools/verify_students.py \
  --build C07_OS_Memory_System_IO/exercises/build/linux-student \
  --config Debug \
  --trace C07_OS_Memory_System_IO/exercises/build/validation/linux-student-include-trace.json \
  --output C07_OS_Memory_System_IO/exercises/build/validation/linux-student-verification.json
```

## audit_delivery.py

检查本地 Markdown 链接和锚点，确认 `references/coverage.md` 存在并登记其中链接，给可交付 C07 文件生成字节级 SHA256，并汇总 `run_test.py` 产生的 JSON 记录。它按原始字节 hash，不归一化改写原始证据，且从 manifest 中排除自身输出。

`--ctest-records` 只传本次审核要采信的记录目录。不要把历史失败目录和本次最终矩阵混在一起；脚本会保留 `SKIP` 计数，但 `FAIL`、缺失/未知 verdict 和格式错误 JSON 都会阻断交付 PASS。

```sh
python3 C07_OS_Memory_System_IO/exercises/tools/audit_delivery.py \
  --course C07_OS_Memory_System_IO \
  --ctest-records C07_OS_Memory_System_IO/exercises/build/linux-debug/records/Debug \
  --output C07_OS_Memory_System_IO/exercises/build/validation/delivery-audit.json
```

三个脚本都支持 `--self-check`，用于无测试框架的本地 smoke check。
