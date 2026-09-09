# 练习 F1：沿 target 图追踪使用要求

先读[06 CMake 与依赖](../../chapters/06-cmake-and-dependencies.md)。本题包含构建观察和一个独立实现任务。以下命令从 **LearnCPP 根目录的 x64 Native Tools 环境**运行；Ninja使用Release，换Visual Studio生成器时构建/CTest要加`--config Release`/`-C Release`。

## Part 1：谁接收到哪些要求

已提供`F1_math_public`、公开头、Reference和`F1_warning_policy`。后者虽然名字含warning，本题实际承载的是`F1_REQUIRE_EXPLICIT_RESULT`定义；警告选项由课程公共helper设置。先预测三件事：消费者是否能包含`f1_math/math.hpp`，是否得到`F1_REQUIRE_EXPLICIT_RESULT`，是否得到实现专用的`F1_BUILDING_MATH`。

```powershell
cmake -S C01_Build_Compile_Link/exercises/F1_cmake_targets -B C01_Build_Compile_Link/exercises/build/learner-f1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build C01_Build_Compile_Link/exercises/build/learner-f1 --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-f1 --output-on-failure
```

读本题[CMakeLists](CMakeLists.txt)和实际编译行，而不是只看退出码。检查器验证公开头可用、传递定义到达及三组运算；编译行补充证明实现宏没有泄漏。

**解析：** `PUBLIC`要求用于当前target并传给消费者；`PRIVATE`只用于当前target的编译；`INTERFACE`要求只供消费者使用。`INTERFACE library`是另一层概念：它不编译自身源码，但普通库也可以有`INTERFACE`要求。这里`app -> math -> policy`使policy定义传到app；`F1_BUILDING_MATH`只在math的编译行出现。

## Part 2：完成自己的操作

学生只编辑[src/student/student.cpp](src/student/student.cpp)，不改Reference或检查器。契约是对课内不会溢出的整数先相加，再乘2；检查包含`(20,1)->42`、`(0,0)->0`、`(-2,3)->2`。初始实现只相加，因此会明确失败。

```powershell
cmake -S C01_Build_Compile_Link/exercises/F1_cmake_targets -B C01_Build_Compile_Link/exercises/build/learner-f1-student -G Ninja -DCMAKE_BUILD_TYPE=Release -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build C01_Build_Compile_Link/exercises/build/learner-f1-student --verbose
ctest --test-dir C01_Build_Compile_Link/exercises/build/learner-f1-student -L student --output-on-failure
```

**解析：** 正确计算是`(left + right) * 2`，不是恒定返回42。Student target已经有自己的include目录和policy依赖；改学生函数不会改变[Reference](reference/math.cpp)。成功说明这几组输入和要求传播通过，不证明任意整数算术不会溢出。

## Part 3：预测一个传播错误

在你自己的练习副本中，把`F1_math_public`到policy的连接从`PUBLIC`改为`PRIVATE`，保留原题作为Reference。预测发生在包含、链接还是运行判定阶段，再查看编译行和CTest。复制副本的方法见[H1的独立工作区](../H1_modules/README.md#独立工作区)，将目录名换成F1即可。

**解析：** 公开include目录仍能传播；policy的非链接使用要求不再传给消费者。当前检查器在缺少宏时编译出失败分支，所以这个对照可以编译后在运行检查中失败，不能错误地承诺一定是编译失败。恢复`PUBLIC`后复跑原对照；不要通过改checker或全局加同名宏掩盖target边界错误。
