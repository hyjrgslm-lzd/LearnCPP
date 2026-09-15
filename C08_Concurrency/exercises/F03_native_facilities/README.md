# F03：标准原生并发设施能力主体

本题对应 [原生设施正文](../../topics/frontier/03-native-facilities.md)。

F03 是原生能力主体集合，不是带 `main.cpp` 的普通练习。目录只注册 `F03_std_senders_reference`、`F03_std_hazard_pointer_reference`、`F03_std_rcu_reference` 三个独立 CTest；这些主体只在 `CONCURRENCY_STUDY_BUILD_REFERENCE=ON` 时生成，各自缺能力返回 77，各自可用后失败就是 FAIL。

## 运行

```powershell
cmake -S C08_Concurrency/exercises -B C08_Concurrency/exercises/build/c08-frontier-author/f03 -G "Visual Studio 18 2026" -A x64 -DCONCURRENCY_STUDY_ENABLE_CXX26=ON -DCONCURRENCY_STUDY_BUILD_REFERENCE=ON
cmake --build C08_Concurrency/exercises/build/c08-frontier-author/f03 --config Release --target F03_std_senders_reference F03_std_hazard_pointer_reference F03_std_rcu_reference
ctest --test-dir C08_Concurrency/exercises/build/c08-frontier-author/f03 -C Release -R "F03_std_" --output-on-failure
```

## 答案要点

固定 stdexec、教学 HP/RCU 和平台线程 API 都不是标准原生 PASS。PASS 只来自标准头、特性宏、实例化、链接和运行。SKIP 表示当前工具链缺接口，不表示课程删掉该知识点。

## IDE 与工程入口

Visual Studio 方案中，本目录没有普通 `main.cpp` 主入口；三个原生主体在 `F03_native_facilities/Reference`：`F03_std_senders_reference`、`F03_std_hazard_pointer_reference`、`F03_std_rcu_reference`。源码文件 `std_senders.cpp`、`std_hazard_pointer.cpp`、`std_rcu.cpp` 是各自真实编译入口；`README.md` 只作为文档加入工程。

单题构建沿上面的命令，从 `C08_Concurrency/exercises` 配置整课并构建三个 `F03_*_reference` 目标。缺标准库能力时测试返回 77；这不是待填学生作业，也不代表删掉该主题。
