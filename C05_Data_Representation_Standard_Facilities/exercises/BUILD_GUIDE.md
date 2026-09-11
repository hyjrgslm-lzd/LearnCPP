# C05 构建与验证

命令在本目录执行，要求完整LearnCPP checkout。主线C++23；脚本最低CMake3.28，Windows的Visual Studio 18 2026预设要求CMake4.2以上。本机使用MSVC14.51、x64。跨课相对路径只读复用现有工具，不要求单独复制common目录。

## 核心路径

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --parallel 2
ctest --preset verify-core
cmake --preset verify-debug
cmake --build --preset verify-debug --parallel 2
ctest --preset verify-debug
```

默认Reference与检查器正反控制体纳入测试；Student编译目标存在但不在默认ALL中。未开启ICU/frontier仅表示未启用，不能从核心通过推断扩展已验证。普通测试有CTest进程外30秒超时，negative包装另有20秒进程超时；启动失败、缺DLL、超时都不能冒充“预期失败”。

## 学生与答案

每题README列出对应正文、Part目标、编辑位置和检查。实现型题只修改src/student中的实现；检查程序直接包含选中的实现目录。src/reference提供独立答案，validation/good是可以通过检查的独立完成体，validation/bad用于证明检查器能拒绝指定错误。good通过不是当前student完成。

```powershell
cmake --preset student
cmake --build --preset student --parallel 2
ctest --preset student
```

初始Student必须明确失败，因为未完成实现；记录其精确诊断，不把它伪装成PASS或SKIP。观察型程序可以完整运行，它的通过只覆盖程序内观察检查；预测和解释Part还需对照解析。Release中的check不依赖assert或NDEBUG。

## 单题

例如样章可在完整checkout中独立配置：

```powershell
cmake -S L14_binary_fields -B build/leaf-L14 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L14 --config Release --parallel 2
ctest --test-dir build/leaf-L14 -C Release --output-on-failure
```

其他题将-S和-B中的单元名替换为实际目录。每个叶级自己include公共StudySetup并注册测试；最终交付须实际验证所有叶级，不能以顶层add_subdirectory成功代替。

## ASan 与前沿

```powershell
cmake --preset asan
cmake --build --preset asan --parallel 2
ctest --preset asan
cmake --preset frontier
cmake --build --preset frontier --parallel 2
ctest --preset frontier
```

Windows ASan从实际选中编译器目录复制匹配runtime，缺匹配DLL直接配置失败，不从其他VS版本拷贝替代。该检查不证明所有生命周期或所有平台都安全。前沿逐设施探测头/宏并真实实例化、链接、运行；只有明确缺能力才返回77作为SKIP，运行失败不得降级为SKIP。

## ICU 扩展

先显式运行tools/prepare_icu.ps1准备固定版本，详细过程与限制见[ICU构建记录](../references/icu-build.md)。准备只影响本课ignored build/_deps，不修改机器配置。ICU只从该目录解析头/库，并以Debug/Release匹配DLL运行；第三方源码和二进制不随课程提交。

准备好隔离依赖后运行：

```powershell
cmake --preset icu-release
cmake --build --preset icu-release --parallel 2
ctest --preset icu-release
cmake --preset icu-debug
cmake --build --preset icu-debug --parallel 2
ctest --preset icu-debug
```

DATA_STUDY_ENABLE_ICU默认OFF；ON却缺依赖、错版本或越界是FAIL。扩展必须实际运行；ICU与frontier同时开启时仍分别报告能力SKIP。实际矩阵命令使用新的本地记录目录，避免重用早期作者缓存冒充fresh验收。

## 记录证据与隔离审计

使用现有记录器，输出到本地未跟踪目录：

```powershell
New-Item -ItemType Directory -Force ../../build/local-records/C05 | Out-Null
python ../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output ../../build/local-records/C05/my-core-configure.json --timeout 180 -- cmake --preset verify-core
```

记录器包含命令、cwd、stdout/stderr、退出码、超时/清理和预期检查结果。构建命令选择足够且有界的超时；目录名中build/内容不作为可发布证据。

Student隔离审计要求：先在build目录请求.cmake/api/v1/query/codemodel-v2；以Reference OFF配置并启用MSVC /showIncludes；通过record_process记录所有Student目标的--clean-first显式重建；再运行C02工具audit_student.py，逐目标给--expect-target。需要真实预处理输出和codemodel，单纯搜索文本或检查目录名不够；它也不是防抄袭证明。

这里是操作方法，不是执行通过声明。最终对外文档只保留稳定命令、范围和边界，不提交本机过程记录。


## fmt 与 spdlog 固定扩展

在本课 `exercises` 目录运行 `./tools/prepare_format_libraries.ps1`，再启用扩展；版本、隔离目录和错误边界见[依赖说明](../references/revision-dependencies.md)。默认核心配置完全离线，不需要这两个库。显式开启却缺依赖、源码版本不符或使用污染的同名target时必须失败，不能记为前沿能力SKIP。

```powershell
cmake --preset format-libs
cmake --build --preset format-libs
ctest --preset format-libs
cmake --build build/format-libs --config Debug
ctest --test-dir build/format-libs -C Debug --output-on-failure

cmake --preset format-libs-std
cmake --build --preset format-libs-std
ctest --preset format-libs-std
cmake --build build/format-libs-std --config Debug
ctest --test-dir build/format-libs-std -C Debug --output-on-failure
```

`format-libs`使用external fmt 12.1.0和spdlog1.17.0的静态目标；`format-libs-std`在另一个构建目录使用spdlog的标准格式化backend。两个预设都包含U02的fmt教学，但只有前者把fmt链接给spdlog。不能在一个构建目录混用两种spdlog backend。上面每条实际验证命令仍需用本指南既有进程记录器设置外部超时并保存新的JSON。

U02/U03为观察、源码阅读与迁移实验，没有假Student。格式文本检查、运行期格式化、日志宏裁剪、运行期级别过滤和日志错误处理分别验证；输出正确不是完成全部预测与迁移题的证明。实验仅使用进程内局部logger和有限输入，不配置机器级日志服务。异步队列、关闭协议和服务观测由C08/C11承接。

## 其他平台的实验入口（本轮未验证）

在已有支持本课C++23语言与库设施的GCC/Clang环境，可采用通用CMake入口：

```sh
cmake -S . -B build/unix -DCMAKE_BUILD_TYPE=Release -DDATA_STUDY_ENABLE_ICU=OFF
cmake --build build/unix --parallel 2
ctest --test-dir build/unix --output-on-failure
```

这只是复现实验规格，不是本轮Linux/macOS通过证据。`format/print/expected/chrono`等实际库支持必须随所选版本检查。ICU准备脚本及本轮已验证的Debug/Release前缀布局针对Windows；其他平台需按ICU77.1官方构建方式准备同版本隔离依赖、确认实际库位置与Unicode16.0数据后再验证U01。不要把Windows DLL或其测试结果当成其他平台实现。
