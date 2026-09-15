# C11 构建、依赖与运行

核心需要 C++23、CMake3.28+、Python3.10+，默认离线。本文 Windows 示例采用 Visual Studio18 2026 x64（该生成器需 CMake4.2+）；本次使用 MSVC19.51、SDK26100、CMake4.2.3。按最新约定只要求 Windows 编译和关键表现；Linux 与 sanitizer 是独立可选学习入口，不是本次验证要求。

## 1. 默认核心

在仓库根执行，所有构建产物放 build：

```powershell
cmake -S C11_Networking_Service_Design/exercises -B build/c11-windows -G 'Visual Studio 18 2026' -A x64
cmake --build build/c11-windows --config Release --parallel 2
ctest --test-dir build/c11-windows -C Release --output-on-failure
```

默认包含 L01/L02/L03/L04/L05/L10。Reference/good 和故意错误控制属于不同结果类型；bad 必须以指定退出码和诊断被拒绝。检查使用 Release 下仍生效的 check.hpp，不靠会消失的 assert。网络使用回环临时端口，不要求外部服务。

生成的 VS solution 按 L/P/B 单元分组。实现题的启动项目是 `C11_Lxx_student`；观察题和专项题以真实可执行目标为启动项目。Reference、good/bad、额外 bad 控制和观察子目标保留为独立项目，收在同题分组下，便于单独构建和调试。项目文件清单包含 README、CMakeLists、实际源码、学生/参考/控制头；P2 gRPC 还显示 `tasks.proto` 与生成的 `.pb.h/.grpc.pb.h`。

若宿主同时传入 PATH 和 Path，MSBuild 可能报告环境字典重复键。仅对当前命令规范化，不修改机器配置：

```powershell
python -c 'import os,subprocess,sys; sys.exit(subprocess.call(sys.argv[1:],env=dict(os.environ)))' cmake -S C11_Networking_Service_Design/exercises -B build/c11-normalized -G 'Visual Studio 18 2026' -A x64
```

也可用 tools/run_command.py 同时记录输出和外部超时：

```powershell
python C11_Networking_Service_Design/exercises/tools/run_command.py --output build/c11-command.json --timeout 180 -- cmake --build build/c11-windows --config Release --parallel 2
```

output 必须是新文件；超时、清理失败或原命令错误都保持失败。受限执行环境若拒绝 MSBuild FileTracker，应在能管理自身构建进程的授权 Windows 环境执行；不要修改检查器忽略失败。

## 2. 单题与 Student

每个 L/P/B 单元可独立 cmake -S 指向该目录，共享相对路径下 C01/C07 公共设施。实现题为 L01/L02/L03/L05/L10，其余是完整观察驱动。

```powershell
cmake -S C11_Networking_Service_Design/exercises/L01_framing -B build/c11-student -DC11_BUILD_REFERENCE=OFF -DC11_TEST_STUDENTS=ON
cmake --build build/c11-student --config Release --target C11_L01_student
ctest --test-dir build/c11-student -C Release --output-on-failure
```

初始 Student 输出 UNFINISHED 并失败。C11_TEST_STUDENTS 默认 OFF，使未完成学生题不混进参考成功矩阵；Student target 仍可显式构建。修改学生解法后只跑相应题即可，不要求每次重建所有第三方库。

## 3. 显式准备固定源码

完整 pin 见 [dependencies.json](../references/dependencies.json)。配置不联网；可自行提供已核对来源的目录，或明确运行以下工具下载到 build。下载工具单独需要 **Python3.12+ 和 Git**，使用 archive SHA256、固定提交及上游 gitlink 子模块；不是系统安装器。

```powershell
python C11_Networking_Service_Design/exercises/tools/prepare_sources.py --root build/c11/deps boost openssl nghttp2
# 按所需路线另选，不必一次下载全部
python C11_Networking_Service_Design/exercises/tools/prepare_sources.py --root build/c11/deps asio stdexec
```

已有 archive 先核对 SHA256；未带来源标记的预存解压目录拒绝认证，应使用新 root。标记表示首次由核验 archive 解压，不是后续本地文件从未修改的证明；复现实验须记录本地修改或用新目录解压。Git 目录检查 HEAD 与 tracked modifications，子模块版本由父提交锁定，不静默切换滚动分支。

下面将已准备目录转为正斜线绝对路径。PowerShell 中完整 `-D名字=路径` 参数必须引用，尤其包含盘符时：

```powershell
$c11Deps = (Resolve-Path build/c11/deps).Path.Replace('\','/')
$c11Prefix = (Join-Path (Get-Location) 'build/c11/prefix').Replace('\','/')
```

## 4. Asio、HTTP/WebSocket 与 TLS/HTTP2

只需 Boost 即可先编译 L06/P1：

```powershell
cmake -S C11_Networking_Service_Design/exercises -B build/c11-asio -G 'Visual Studio 18 2026' -A x64 -DC11_ENABLE_BOOST=ON "-DC11_BOOST_ROOT=$c11Deps/boost_1_92_0"
cmake --build build/c11-asio --config Release --target C11_L06_asio C11_P1_service --parallel 2
ctest --test-dir build/c11-asio -C Release -R 'C11_L06_asio|C11_P1_service' --output-on-failure
```

TLS 需要私有 OpenSSL3.5.8 prefix，包含 include 与 lib。已有匹配库可直接提供；需要自行构建时使用已准备的**原生 Windows Perl**和 VS BuildTools。工具不会安装 Perl/NASM，也不改变 PATH 或系统信任。示例中 perl 路径按自己的本地来源调整：

```powershell
python C11_Networking_Service_Design/exercises/tools/build_openssl_windows.py --source build/c11/deps/openssl --build build/c11/openssl-build --prefix build/c11/prefix/openssl --perl build/c11/tools/perl/perl/bin/perl.exe
cmake -S C11_Networking_Service_Design/exercises -B build/c11-asio -DC11_ENABLE_TLS=ON "-DC11_OPENSSL_ROOT=$c11Prefix/openssl" -DC11_ENABLE_HTTP2=ON "-DC11_NGHTTP2_SOURCE=$c11Deps/nghttp2"
cmake --build build/c11-asio --config Release --parallel 2
ctest --test-dir build/c11-asio -C Release -R 'C11_L06_|C11_L07_|C11_L08_|C11_P1_' --output-on-failure
```

SSL 教学构建使用 no-shared/no-tests/no-asm，并将 install_sw 限定私有 prefix。no-asm 避免引入汇编工具，不适合作为生产密码吞吐对照。需要 Debug 时另建匹配 Debug 库，不把 Release 依赖冒称全配置验证。Boost JSON 编译为本地静态目标；MSVC 的 /bigobj 用于较大的 Beast 模板翻译单元。

## 5. QUIC（独立构建目录）

```powershell
python C11_Networking_Service_Design/exercises/tools/prepare_sources.py --root build/c11/deps msquic
python C11_Networking_Service_Design/exercises/tools/build_openssl_windows.py --source build/c11/deps/msquic/submodules/openssl --build build/c11/quic-tls-build --prefix build/c11/prefix/quic-tls --perl build/c11/tools/perl/perl/bin/perl.exe
cmake -S C11_Networking_Service_Design/exercises/L09_quic -B build/c11-quic -G 'Visual Studio 18 2026' -A x64 -DC11_ENABLE_QUIC=ON "-DC11_MSQUIC_SOURCE=$c11Deps/msquic" "-DC11_QUIC_TLS_ROOT=$c11Prefix/quic-tls"
cmake --build build/c11-quic --config Release --target C11_L09_quic --parallel 2
ctest --test-dir build/c11-quic -C Release --output-on-failure
```

MsQuic 使用其固定 OpenSSL 子模块，不能拿普通 OpenSSL prefix 顶替。上游 Windows backend 编译引用 xdp-for-windows 头，所以准备工具同步该子模块；不构建/安装 XDP 驱动。DLL 随目标复制到可执行目录，运行时检查版本2.6.1。

MSVC19.51 对上游 crypto_tls.c 的两位 timestamp transport parameter 产生 C28020 范围误报；课程只对这一源文件关闭该诊断，其掩码/移位值域为0—3，未改上游源码或全局关闭分析。不同宿主身份可能使上游内部 git hash 查询失败；外部 pin 核验仍执行，不能因此声称 DLL 嵌入了正确提交号。

## 6. gRPC（独立 BoringSSL 实例）

```powershell
python C11_Networking_Service_Design/exercises/tools/prepare_sources.py --root build/c11/deps grpc
cmake -S C11_Networking_Service_Design/exercises/P2_grpc -B build/c11-grpc -G 'Visual Studio 18 2026' -A x64 -DC11_ENABLE_BOOST=ON "-DC11_BOOST_ROOT=$c11Deps/boost_1_92_0" -DC11_ENABLE_GRPC=ON "-DC11_GRPC_SOURCE=$c11Deps/grpc"
cmake --build build/c11-grpc --config Release --target C11_P2_grpc --parallel 2
ctest --test-dir build/c11-grpc -C Release --output-on-failure
```

gRPC 固定子模块提供匹配 Protobuf35.1.0/protoc/grpc_cpp_plugin 与 BoringSSL。首次编译量大，后续只需目标增量构建。不要与 C11_ENABLE_TLS/QUIC 混到这个实例；加密后端保持分离。上游 zlib 配置可能将 zconf.h 改名为 zconf.h.included，这是构建引入的来源树变化，复核时要记录，不能抹去后当作全新干净 checkout。

## 7. 跨课桥接与性能

```powershell
cmake -S C11_Networking_Service_Design/exercises/L11_bridges -B build/c11-bridges -G 'Visual Studio 18 2026' -A x64 "-DC11_ASIO_SOURCE=$c11Deps/asio" -DC11_ENABLE_BOOST=ON "-DC11_BOOST_ROOT=$c11Deps/boost_1_92_0" -DC11_ENABLE_STDEXEC=ON "-DC11_STDEXEC_SOURCE=$c11Deps/stdexec"
cmake --build build/c11-bridges --config Release --parallel 2
ctest --test-dir build/c11-bridges -C Release --output-on-failure
```

stdexec 目标启用 /Zc:preprocessor，采用固定第三方接口。legacy 只用 standalone Asio，sender 只用 Boost.Asio。性能入口见 [B01](B01_pool_cost/README.md)，不进入默认测试耗时。

## 8. Linux 与工具边界

本次无需安装或运行 Linux/WSL。Linux 专属 epoll 与可选 io_uring 源保留在 L04；若以后自行在 Linux 原生文件系统复现，使用支持 C++23 expected 的编译器/标准库，配置 CMAKE_BUILD_TYPE=Release。io_uring 额外开启 C11_ENABLE_URING 并提供 liburing2.15 头/库，受内核与策略能力限制；源码存在和 Windows 通过都不等于它已通过。

ASan 可单独配置 C11_ASAN=ON；UBSan 只在支持的非 MSVC 路径开启 C11_UBSAN。不同工具检测能力不互相替代，本次不要求扩展这些矩阵。CTest 使用 C07 包装器和 C01 进程外监督；日志、审查、测量留在忽略的 build，课程只保留可复用源码和方法。
