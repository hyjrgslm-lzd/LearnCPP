# C01 实施规格与批次

批准日期：2026-09-08。依据根目录的全局计划与重构指引；用户已确认实施本会话经独立 architect、critic 批准的计划。本文保存教学、接口和验收约定，不是完成证明。

## 目标与边界

面向会写现代 C++、但尚未系统掌握工程机制的读者。正文先讲背景、概念与因果，再通过真实代码、实验和解析验证。C01 独立构建；已有 Coroutine、Concurrency 保留主线，仅补确证缺口和跨课衔接。Windows 实测；其他平台完整保留正文、代码、命令，并标未验证。没有系统安装、全局环境修改、新依赖、CI、提交或推送任务。用户 P2 generator 学习代码及通用指引已有修改必须保留。

## 知识与交付链

| 组 | 能力与实验 | 作者范围 |
|---|---|---|
| A | 从源码到配置、构建、运行、断点、局部变量和调用栈；Debug/Release | 作者 A |
| B | 翻译单元、预处理、宏、包含与声明定义；预处理产物 | 作者 A |
| C | linkage、ODR、inline、模板工程边界、对象/符号/静态归档；重复与缺失符号 | 作者 A |
| D | 静态/动态库、导入库、导出/可见性、装载、初始化和 CRT | 作者 B |
| E | 布局、调用约定、版本/选项兼容；opaque handle、同模块创建销毁、status 和异常边界 | 作者 B |
| F | target 使用要求、presets、配置、toolchain file、固定依赖、find_package/FetchContent | 作者 C |
| G | CTest、静态分析、ASan、fuzz 与 profiling；clean/no-op/实现/公共头变更，PCH/LTO | 作者 C |
| H | module interface/implementation/partition、fragment、可达性、BMI、扫描与构建 DAG | 作者 C |
| I | import std 的标准/实现/生成器/metadata 门槛与独立实测 | 作者 C |
| J | 安装导出/版本文件、静态和动态包、独立 consumer、prefix 重定位、失败对照 | 作者 B |

每组有连续正文、关键代码、实际实验、独立学生编辑位置/Reference、检查与完整解析；不以目录存在、检查器绿灯或外链代替教学。基础概念不能用未经说明的高级实现作为先修。C04 尚未建设时，C01 必须自己讲足实例化与 ODR 所需的模板规则。

## 样章门与演进

ODR 样章：合法单消费者头文件定义 → 第二个 TU 的重复符号复现 → 预处理和符号证据 → include guard/ODR/inline 因果 → 分离单定义和 inline 两种修复 → 原输入复验。标准不保证诊断每种 ODR 违规，inline 不等于强制内联。正文、Starter/Reference、解析、检查正反例经非作者审查和返修复验后，才批量扩展 C01。旧课补修和工具能力调查可独立并行。

优化先定位后改动，保留正确基线及不可比较场景的区分。正式成本实验采用一轮预热、五次独立进程采样；固定输入/版本顺序种子，保留全部样本和无收益结果，不设固定加速门槛。

## 公共构建接口

- 根课程 `exercises/CMakeLists.txt`、`CMakePresets.json`、`cmake/StudySetup.cmake`、`include/check.hpp`、导航/覆盖/全局报告由主负责人维护。作者只修改自己的正文、练习与叶级构建；共享改动先交主负责人集成。
- C++23、核心 CMake 3.28；本机实际选项独立记录。每个练习可被顶层 `add_subdirectory` 或作为独立 CMake 工程配置。
- `engineering_configure_target(target)` 设置本课 C++23、编译诊断、公共检查头目录。普通目标不扫描 Modules，专项显式启用。
- `engineering_add_test(name COMMAND ... LABELS ... TIMEOUT n)` 注册检查；COMMAND 默认同名目标，默认 TIMEOUT 30。编译/安装/消费子工程检查用 180。此函数不把 77 默认设为成功；真正能力跳过由具体测试显式声明。
- `ENGINEERING_STUDY_BUILD_REFERENCE` 默认 ON；`ENGINEERING_STUDY_TEST_STUDENTS` 默认 OFF；`ENGINEERING_STUDY_ENABLE_MODULES`、`ENGINEERING_STUDY_ENABLE_IMPORT_STD`、`ENGINEERING_STUDY_ENABLE_UNSAFE_DEMOS` 默认 OFF。负例不能破坏普通构建。
- 目标采用 `<单元>_student` / `<单元>_reference`；标签分 student、observation、reference、negative、capability。未完成 Student 明确失败，不能用 SKIP/77、常量答案或完成标记冒充实际实现。
- `check(condition, message)` 是 Release 下仍生效的最小检查；失败写清诊断并正常退出1，不借未捕获异常或Debug运行库对话框作为失败信号。它是测试进程判定，不是被测API的异常通道；真实异常语义由被测API及其单独检查负责。检查器必须拒绝代表性错误，而不是只运行不判定。编译/链接负例要同时匹配失败阶段和目标符号/诊断，超时或无关错误不算通过。
- preset：student、verify-core、verify-debug、modules-msvc-ninja、import-std-msvc-ninja；独立 build 目录。测试命令和代码实际接受的参数保持一致。

## 依赖、ABI 与 Modules

依赖示例默认使用离线的小型 provider/consumer。固定第三方仅复用仓库已有版本，不增加包管理器或静默下载。包消费只从复制后的 prefix 查找，不依赖原源码/build 目录；检查静态/动态、Debug/Release和不兼容版本请求。

ABI 示例采用 opaque handle、同模块 create/destroy、status，检查空参数和请求版本，内部异常转换；不暴露 STL 布局，不运行跨 CRT 释放等真实 UB 来证明兼容。Modules 安装导出消费单独验证，不宣称 BMI 跨编译器稳定。

本轮 import std 固定 CMake 4.2.3 + MSVC + Ninja，在 project 前设置该版本官方 gate `d0edc3af-4c50-42ea-a356-e2862fe7a444`，检查 modules.json、CMAKE_CXX_COMPILER_IMPORT_STD 和 CXX_MODULE_STD，随后实际编译链接运行。换版本必须核对其固定源码，不猜 gate。依据：[CMake 4.2.3 实验说明](https://raw.githubusercontent.com/Kitware/CMake/v4.2.3/Help/dev/experimental.rst)。

## 非作者审查与完成门

计划评审：architecture_review APPROVE；strict_plan_review APPROVE。两者均未创作本实施代码；计划通过不等于代码通过。

实施分批接受教学、技术、实验审查：遮答案沿先修试做；检查标准/ABI/生命周期/失败路径；复跑原失败、检查正反例、超时和原始测量。审查绑定文件指纹，作者修复后非作者复验。最终另做跨课导航、先修、构建组合与报告审查。

最终 fresh configure/build/CTest 覆盖 C01 Debug/Release、Reference、调试、DLL/ABI、Modules/import std/包消费、诊断；Coroutine 核心与受影响学生/runtime/已有可用依赖路径；Concurrency 核心、材料和工具检查。Windows 必交专项若仍不可证则明确该单元环境受限，不把 SKIP 升为整课完成。所有已知阻断关闭且最终快照获非作者批准，才回填“审查通过”。

## 证据和状态

当前工具版本只是待实施刷新证据：CMake4.2.3、Ninja1.12、MSVC14.51、Clang22.1/ASan、Python3.13.11；MinGW13.2/GDB14.2已验证可断点/单步/读取局部变量。共享 preset 不包含机器路径。

原始命令、输入、环境、实际编译链接参数、stdout/stderr/退出与超时、代码/文档/数据指纹及审查修复记录保存到 references/validation。已有样本不覆盖，私人会话不提交。质量报告和覆盖表是证据入口，其存在不等于课程完成。全局只回填本次 C01 与 G0/G2 切片，不把 G1/G2 或全局18课标为完成。
