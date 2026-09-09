# 练习 F2：固定本地来源与安装包消费

先读[06 CMake 与依赖](../../chapters/06-cmake-and-dependencies.md)。本题用仓库内的`F2Provider 1.0.0`作为离线fixture，导出target是`F2Provider::provider`；不是第三方真实发布包，也不依赖后续J1。源码与[许可证](provider_fixture/LICENSE.txt)可直接检查，具体文件版本由课程指纹绑定，版本号本身不能证明字节没变。

以下命令从LearnCPP根目录的x64 Native Tools环境运行。源码状态、依赖消费方式与运行结果分别记录。

## Part 1：通过 FetchContent 取用本地源码

提供了provider源码、公开头、`F2_PROVIDER_VERSION=100`和Reference；你先预测消费者无需手写include/lib路径的原因，再检查实际构建行。

```powershell
cmake -S C01_Build_Compile_Link/exercises/F2_dependencies -B C01_Build_Compile_Link/exercises/build/learner-f2-source -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build C01_Build_Compile_Link/exercises/build/learner-f2-source --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-f2-source --output-on-failure
```

**解析：** 默认`F2_DEPENDENCY_MODE=fetchcontent`，`FetchContent_Declare`直接使用`F2_PROVIDER_FIXTURE_SOURCE_DIR`所指的本地源码。这里没有伪URL或全零hash，也不会访问网络。`_deps/...-build`中出现provider的实际编译，证明不是把provider结果硬编码在consumer。target传播公开头和定义，但链接成功、版本宏符合及API计算正确仍是不同检查。

## Part 2：先安装，再 find_package

这次让consumer读取安装后的config和导出target，不直接添加provider源目录。先设置绝对prefix，避免相对安装路径受工作目录影响。

```powershell
$prefix = Join-Path (Get-Location).Path 'C01_Build_Compile_Link/exercises/build/learner-f2-prefix'
cmake -S C01_Build_Compile_Link/exercises/F2_dependencies/provider_fixture -B C01_Build_Compile_Link/exercises/build/learner-f2-provider -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build C01_Build_Compile_Link/exercises/build/learner-f2-provider
cmake --install C01_Build_Compile_Link/exercises/build/learner-f2-provider --prefix "$prefix"
cmake -S C01_Build_Compile_Link/exercises/F2_dependencies -B C01_Build_Compile_Link/exercises/build/learner-f2-installed -G Ninja -DCMAKE_BUILD_TYPE=Release -DF2_DEPENDENCY_MODE=find_package "-DCMAKE_PREFIX_PATH=$prefix"
cmake --build C01_Build_Compile_Link/exercises/build/learner-f2-installed --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-f2-installed --output-on-failure
```

**解析：** `find_package(F2Provider 1.0 CONFIG REQUIRED)`查找config/version文件，随后使用`F2Provider::provider`的安装接口。它不会因为项目里有同名源码目录就自动编译该目录。对比两次编译行和`CMakeCache.txt`中的包位置；不要用机器上偶然存在的同名包替代指定prefix。

## Part 3：自己的调用与失败对照

只改[src/student/student.cpp](src/student/student.cpp)，把输入交给真实provider并原样保留其结果。初始代码额外减2；不能改provider或checker来补偿它。

```powershell
cmake -S C01_Build_Compile_Link/exercises/F2_dependencies -B C01_Build_Compile_Link/exercises/build/learner-f2-student -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build C01_Build_Compile_Link/exercises/build/learner-f2-student
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-f2-student -L student --output-on-failure
```

**解析：** 学生调用应返回`f2_provider::compute_answer(input)`。本fixture在课内小整数域计算`input*2+2`；真实provider检查覆盖20、-1、0、21，避免恒定42混过单一输入。另有`F2_dependencies_student_delegation`使用spy provider，不链接真实provider；它要求`student.cpp`把当前输入传给`f2_provider::compute_answer`且原样返回spy结果，因此会拒绝“自己复制input*2+2公式”的实现。源码审查仍要确认没有改provider或checker来补偿Student。

在独立副本中把`find_package`请求改为2.0，指向同一个1.0.0 prefix；应该在configure阶段被版本文件拒绝。另用新build目录，把`F2_PROVIDER_FIXTURE_SOURCE_DIR`指向不存在的本地目录，应该在configure阶段得到明确的本地来源错误。找不到编译器、编译错误或timeout不是这两个反例的通过条件。

## 自测与解析

只写“最新版”为什么不够？因为无法复原源码字节、接口和依赖状态。只见`F2_PROVIDER_VERSION=100`为什么不够？宏并不证明源码未修改。真实远端依赖还需固定可核验commit或下载内容hash、许可证和来源；这里的本地fixture只教授消费机制，不能冒充已经验证真实远端下载链。安装后的可重定位交付与动态库搜索在[J1](../J1_package/README.md)继续展开。
