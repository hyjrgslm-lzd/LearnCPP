# U03 spdlog 同步前端观察

对应正文：[第20章 spdlog同步前端](../../chapters/20-spdlog-frontend.md)。

本单元只验证同步 logger 前端，不进入 async queue、overflow、flush/shutdown 或服务观测。运行同一源码的两个目标：

- `U03_spdlog_DEBUG`：`SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG`，debug宏存在；运行时 level 过滤仍会求值调用实参，但不会写入 sink。
- `U03_spdlog_INFO`：debug宏在预处理层变成 `(void)0`，实参不求值。

观察程序还覆盖局部 logger、可观测 sink、自定义 formatter 穿透、pattern runtime 配置、后端格式化异常与 logger error_handler 的区别。`DATA_STUDY_SPDLOG_BACKEND=fmt|std` 两种预设都应运行；默认 `fmt` 路径使用外部静态 fmt，不能回退到 spdlog bundled fmt。

迁移题：为业务类型 `AuditEvent { std::string user; int code; }` 加 formatter，让 `logger.info("{}", event)` 输出 `user#code`。要求用局部 logger 和注入 sink 做检查；不要使用全局默认 logger，也不要把运行时过滤误写成“不会求值”。
## IDE 项目

生成 Visual Studio 工程后，启动项目是 `U03_spdlog_DEBUG`。

本题是观察题，没有本题内 `src/student/`、`src/reference/` 或 `validation/` 变体。
- 源码入口：`observation.cpp`。
- 诊断或辅助入口：`diagnostics/bad_compile_format.cpp`、`diagnostics/good_compile_format.cpp`。

单题独立构建：从本目录运行 cmake -S . -B build/leaf -G "Visual Studio 18 2026" -A x64，然后 cmake --build build/leaf --config Debug --target U03_spdlog_DEBUG。
修改后先重建 `U03_spdlog_DEBUG`，再按课程 `BUILD_GUIDE.md` 运行对应检查。
