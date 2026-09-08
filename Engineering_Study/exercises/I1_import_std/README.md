# 练习 I1：证明程序确实经过标准库模块路径

先读[09 import std](../../chapters/09-import-std.md)。本题是观察/工具集成实验：提供完整Reference，不需要为运行而填TODO。你要说明每层证据能证明什么，而不是只看到输出10。下面从LearnCPP根目录的x64 Native Tools环境执行。

## Part 1：CMake 集成路径

本次固定CMake4.2.3。实验gate必须在首次`project()`启用CXX前设置；随后确认`CMAKE_CXX_COMPILER_IMPORT_STD`包含23，并启用target的`CXX_MODULE_STD`。本题独立配置也实现了这些步骤。

```powershell
cmake -S Engineering_Study/exercises/I1_import_std -B Engineering_Study/exercises/build/learner-i1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Engineering_Study/exercises/build/learner-i1 --verbose
ctest --test-dir Engineering_Study/exercises/build/learner-i1 --output-on-failure
```

**解析：** gate是当前构建工具的实验开关，不是C++语言关键字。只设置语言标准不能替代标准库模块接口、metadata和生成器支持；CMake working compiler check在ABI成功后显示skipped，是复用已获得的同轮证据，不是跳过所有验证。更换CMake版本时核对它的固定版本说明，不能猜或沿用旧UUID。

## Part 2：源码、扫描、metadata 与运行

检查`reference/main.cpp`中的真实`import std`，再检查生成的`.ddi`中`requires`是否包含`logical-name: std`，以及`CXXModules.json`中的标准库IFC映射。最后对照编译`std.ixx`/`std.compat.ixx`的实际命令和程序结果。

**解析：** 源码显示意图；扫描结果证明编译器识别了导入依赖；metadata连接到具体BMI；编译/链接/运行闭合实际路径。只有metadata里出现`std.ifc`或只有输出10，都不足以排除错误/未使用的配置。不能把main改成include版本后沿用原模块证据。

## Part 3：直接编译器路径

这条路径独立于CMake。先读取本机`VCToolsInstallDir`中的`modules/std.ixx`，用新的build目录保存产物，保持原源码不变。

```powershell
$source = (Resolve-Path Engineering_Study/exercises/I1_import_std/reference/main.cpp).Path
$stdSource = Join-Path $env:VCToolsInstallDir 'modules/std.ixx'
$out = 'Engineering_Study/exercises/build/learner-i1-direct'
if (Test-Path -LiteralPath $out) { throw '请选择新的输出目录' }
New-Item -ItemType Directory -Path $out | Out-Null
Push-Location $out
try {
    cl /nologo /std:c++latest /EHsc /c /interface "$stdSource" /ifcOutput std.ifc /Fostd.obj
    if ($LASTEXITCODE -ne 0) { throw '标准库模块编译失败' }
    cl /nologo /std:c++latest /EHsc /c "$source" /reference std=std.ifc /Fomain.obj
    if ($LASTEXITCODE -ne 0) { throw 'importer编译失败' }
    link /nologo main.obj std.obj /out:import_std_direct.exe
    if ($LASTEXITCODE -ne 0) { throw '链接失败' }
    .\import_std_direct.exe
    if ($LASTEXITCODE -ne 0) { throw '运行失败' }
} finally { Pop-Location }
```

`direct-cl/direct_import_std.cmd`保留了作者本机固定工具路径的快捷记录；上述过程才是按当前开发环境选择路径的复验入口。

**解析：** 手工路径证明编译器与标准库模块能配合，不证明CMake扫描/导出功能。反过来，CMake成功不要求你手工分发它生成的BMI。两条记录应分开保存。

## Part 4：用自己的副本验证失败边界

按[H1独立工作区](../H1_modules/README.md#独立工作区)方法把题目名换成I1。只改副本：在首次语言启用后才设置gate，或去掉对标准库模块的必要配置，用新build目录观察失败阶段；保留正确配置作对照。不要修改已验证Reference，也不要使用旧cache冒充缺条件实验。

**解析：** 能力发现发生在语言启用过程中，后设置gate不能当作首次正确启用。不同工具版本的诊断文字可能不同，必须核对缺的是gate、metadata、标准级别、生成器还是链接输入，不能将任意失败统一写成“不支持modules”。本轮标准库模块与C++20 named modules分别验收，未运行的平台不能继承Windows结果。
