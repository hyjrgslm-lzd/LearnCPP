# F01 frontier standard facilities

目标：把 C++26/C++29 数据表示相关设施做成可运行能力探针。默认 `DATA_STUDY_ENABLE_FRONTIER=OFF` 时，本题只在 configure 阶段报告 `F01 disabled; no capability tests registered`，不注册任何 F01 测试；显式打开后，每个设施单独编译、链接、运行。

每个探针只做一件事：

- 头文件或 feature-test macro 缺失：返回 `77`，CTest 记为 SKIP。
- macro 宣称支持：编译真实调用，并运行最小语义断言；失败返回非零，CTest 记为 FAIL。
- 没有可靠 macro 的状态：只探测已入稿/源码可核对的旧入口，不把旧 macro 直接当完整失败。

叶级验证命令：

```powershell
cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off
cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off --config Release
ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-off -C Release --output-on-failure

cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on -DDATA_STUDY_ENABLE_FRONTIER=ON
cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on --config Release
ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/f01-frontier-on -C Release --output-on-failure
```

本机 MSVC 14.51 / STL145 update202604 预期：ON 时多数项目 SKIP。OFF 是 DISABLED，没有能力结论；SKIP 只说明显式探测时此工具链没有声明该标准设施，或没有到达对应 feature-test macro 阈值。
