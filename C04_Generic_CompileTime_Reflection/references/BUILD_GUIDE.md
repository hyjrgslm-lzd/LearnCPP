# C04 构建与验证

在完整LearnCPP checkout内使用。各题可以独立配置，仍可引用同一仓库的C01检查器和C02工具；“独立构建”不表示必须把每题复制成脱离仓库的发行包。

## 环境与入口

核心C++23。CMake脚本最低3.28；Windows预设使用Visual Studio 18 2026，需要CMake4.2以上。VS附带clang-cl可用于局部编译成本观测，不是反射支持证明。

从仓库根目录进入 `C04_Generic_CompileTime_Reflection/exercises`：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core
ctest --preset verify-core
cmake --preset verify-debug
cmake --build --preset verify-debug
ctest --preset verify-debug
```

批量复现建议经外部超时包装；上述命令是被包装的操作。例：在仓库根目录记录一次配置，输出到本地未跟踪目录：

```powershell
New-Item -ItemType Directory -Force build/local-records/C04 | Out-Null
python C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output build/local-records/C04/configure-core.json --timeout 180 -- cmake --preset verify-core -S C04_Generic_CompileTime_Reflection/exercises
```

整课配置上限180秒，构建900秒，普通程序由CTest覆盖运行与退出的30秒上限。编译诊断的配置控制60秒、正例和负例各180秒，外层CTest总上限420秒。外部整批CTest另设有限总时长并记录。超时、无法确认清理、缺DLL或启动失败都是真实FAIL，不计为“预期拒绝”。

单题示例：

```powershell
cmake -S L11_customization -B L11_customization/build/local -G "Visual Studio 18 2026" -A x64
cmake --build L11_customization/build/local --config Release
ctest --test-dir L11_customization/build/local -C Release --output-on-failure
```

## 作业与检查的含义

`GENERIC_STUDY_BUILD_REFERENCE=ON`构建Reference与checker控制；`GENERIC_STUDY_TEST_STUDENTS=OFF`不把未完成作业注册到核心验证。Student目标仍可显式构建。`student`预设关闭Reference并开启Student测试，初始程序应真实拒绝；不要把该有意未完成结果登记为课程实现缺陷，亦不要把它算成学生通过。

每道实现题独立选择 `src/student`、`src/reference`、`validation/good`、`validation/bad`。检查器消费该选择的头文件。good和Reference均需通过；bad只有精确exit1与预期`check failed:`文本吻合才是有效拒绝。编译期占位由概念或`if constexpr`守卫，避免检查器因尚未提供嵌套类型而无法构建；守卫失败仍须发出真实检查失败。

Student隔离审计复用C02 `audit_student.py`：在student-only配置生成CMake File API codemodel，使用当前进程的`CL=/showIncludes`、`VSLANG=1033`重建所有`*_student`目标，保存命令与include trace，再核对没有Reference目标、include路径或链接依赖。变量只作用于本次子进程，不写机器配置。

以下在本课`exercises`目录执行，输出到本地未跟踪目录：

```powershell
$c04AuditTag = Get-Date -Format yyyyMMdd-HHmmss
$c04RecordRoot = "../../build/local-records/C04/$c04AuditTag"
New-Item -ItemType Directory -Force $c04RecordRoot | Out-Null
New-Item -ItemType Directory -Force build/student/.cmake/api/v1/query | Out-Null
New-Item -ItemType File -Force build/student/.cmake/api/v1/query/codemodel-v2 | Out-Null
python ../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output "$c04RecordRoot/include-configure.json" --timeout 180 -- cmake --preset student
if ($LASTEXITCODE -ne 0) { throw 'Student configuration failed' }
$c04StudentTargets = @('L01_templates','L02_deduction','L03_forwarding','L04_lookup',
    'L05_overload','L06_constraints','L07_packs','L08_type_lists','L09_tuple',
    'L10_constexpr','L11_customization','P1_static_record',
    'A01_compiletime_values','A02_field_projection','A03_explicit_object',
    'A04_type_pipelines','A05_expression_templates') | ForEach-Object { "${_}_student" }
$c04PreviousCl = $env:CL
$c04PreviousVsLang = $env:VSLANG
try {
    $env:CL = "$c04PreviousCl /showIncludes"
    $env:VSLANG = '1033'
    python ../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output "$c04RecordRoot/include.json" --timeout 900 -- cmake --build build/student --config Release --clean-first --target $c04StudentTargets
    if ($LASTEXITCODE -ne 0) { throw 'Student include-trace build failed' }
} finally {
    $env:CL = $c04PreviousCl
    $env:VSLANG = $c04PreviousVsLang
}
python ../../C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py --build build/student --config Release --trace "$c04RecordRoot/include.json" --output "$c04RecordRoot/audit.json"
```

`--clean-first --target`这里只重建Student，若随后运行整套Student预设，先普通`cmake --build --preset student`补齐观察/诊断目标。初始Student的CTest非零是预期，仍要逐项确认只有这些Student因其checker失败，不能把任意崩溃/缺exe都当作作业状态。

标签`reference`表示答案契约检查，`observation`表示已运行观察，`validation`与`negative`表示检查器控制，`diagnostic`表示隔离的编译案例，`student`表示学生真实实现，`capability`表示前沿能力。运行程序不能证明预测/解释题完成；解析须另行核对。

## 前沿与ASan

`frontier`预设开启`GENERIC_STUDY_ENABLE_FRONTIER`。每项能力分别记录头文件、宏、真实语法/实例化、链接与运行；OFF不同于能力缺失SKIP。能力已声明而真实程序失败时必须FAIL，禁止统一吞成SKIP。

`asan`预设只给安全、有限的相关生命周期示例加仪器；Windows使用当前编译器旁的匹配ASan DLL。其结果不证明模板规范的所有性质。其他平台配置方式可以提供，未经运行不得声称通过。

ASan测试预设选择L02/L03/L04/L09/L11/P1及A01—A05的reference、validation和observation标签；故意拒绝控制和编译诊断不混入该安全路径统计。Student include输出可能随已安装工具的语言资源而为中文，审计器同时识别`including file:`和`包含文件:`，不能只因英文文本匹配失败就说构建失败。

## 固定元编程库与独立 good 审计

先从本课 `exercises` 目录运行 `./tools/prepare_meta_libraries.ps1`，再使用 `meta-libs` 预设。Mp11/Hana只来自本课隔离目录；准备记录、许可证和固定commit见[依赖说明](revision-dependencies.md)。默认核心配置不会下载或搜索系统Boost；显式打开却缺库、版本不符或源码脏时直接失败，不是能力SKIP。

```powershell
cmake --preset meta-libs
cmake --build --preset meta-libs
ctest --preset meta-libs
cmake --preset meta-student
cmake --build --preset meta-student
ctest --preset meta-student
```

`meta-student`在上面核心Student清单中再加`U01_mp11_student`。做该配置的include审计时，codemodel、build、trace与审计命令中的目录都使用`build/meta-student`。Hana属于观察/迁移实验，不设Student；通过观察不能代替完成其迁移题。

good审计使用同样的实际include-trace办法，但配置为`meta-libs`、目标为全部`*_validation_good`。请求codemodel-v2后，用`--clean-first --target`明确重建这些目标，保存`/showIncludes`记录，再执行：

```powershell
python tools/audit_good.py --build build/meta-libs --config Release --trace <本次good构建记录.json> --output <新的good审计结果.json>
```

审计检查构建输入和实际include没有Reference依赖，不证明作者从未看过答案；独立上下文的解题记录另行保存。clean-first只重建指定目标，之后运行完整CTest前先普通构建补齐其它程序。

负编译案例经`tools/compile_case.py`先编译正控，再验证对应语义错误。Windows子构建放在`exercises/build/_diag/<父构建与用例标识哈希>/<配置>`，避免深路径FileTracker失败，并隔离不同父构建。记录含源文件/辅助脚本SHA与实际仓库内头文件include SHA。`SETUP`只用于可信课程CMake配置，使control与subject使用实际依赖；缺头、链接/工具链错误、ICE、超时都不能作为语义拒绝通过。空平台参数用`--platform=`传递。

## 本地复现记录

复用进程记录器保存命令、exit、stdout/stderr、耗时、timeout及清理状态；它的PASS只针对声明的进程结果。成本测量应另存输入/环境/编译模式、所有独立样本、trace、对象节与符号数据，不能从总耗时直接推出根因。

`build/`是本机构建树，不随课程提交。本地记录、测量样本和二进制都保存在未跟踪目录；需要发布结论时，只把稳定的方法、边界和可复现命令写回课程文档。
