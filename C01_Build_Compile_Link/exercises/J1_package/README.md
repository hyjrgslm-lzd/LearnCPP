# 练习 J1：安装导出、版本文件与独立 consumer

先读 [10：安装、导出与兼容消费](../../chapters/10-packaging-and-compatibility.md)。本题把 E1 的 `lesson_api.h` 和 reference 实现打成 CMake 包，版本固定为 `1.0.0`。

包只导出两个 target：

```cmake
LessonPackage::lesson_static
LessonPackage::lesson_shared
```

没有 `LessonPackage::lesson` alias。这样 consumer 必须明确选择静态或动态消费。

## Part 1：安装到 prefix_A，复制到 prefix_B

运行：

```powershell
ctest --test-dir build --tests-regex J1_package_roundtrip --output-on-failure
```

脚本先执行：

```powershell
cmake --install <J1 build> --config <Debug|Release> --prefix <stage>/prefix_A
```

然后只复制这个专用 stage 到 `prefix_B`。删除和复制都限制在 J1 build 目录下，不碰源码和用户文件。

**解析：** build tree 不是安装包。复制到新 prefix 后再消费，可以检查导出的 include/lib/bin 和 config 文件是否足够，而不是误用原源码或原 build。

## Part 2：静态 consumer

consumer 只设置 `CMAKE_PREFIX_PATH=prefix_B`，禁用 package registry 和 system package registry，再执行：

```cmake
find_package(LessonPackage 1.0.0 CONFIG REQUIRED)
target_link_libraries(package_consumer PRIVATE LessonPackage::lesson_static)
```

**解析：** static target 必须传播 include 目录和 `LESSON_STATIC`。在 Windows 上这会让 `LESSON_API` 为空，不能误变成 `dllimport`。

## Part 3：动态 consumer

动态 consumer 链接：

```cmake
target_link_libraries(package_consumer PRIVATE LessonPackage::lesson_shared)
```

运行时只给子进程 PATH 加入 `prefix_B/bin`。

**解析：** 链接导入库只解决构建阶段。DLL 搜索是运行阶段问题。本题不修改机器全局 PATH。

## Part 4：失败对照

同一个 CTest 还会检查：

- 请求 `LessonPackage 2.0.0` 时 configure 失败。
- 链接不存在的 `LessonPackage::lesson` 时 configure/generate 失败。
- 删除 stage 副本里的 DLL 后，shared consumer 运行失败。

**解析：** 三种失败分别覆盖版本文件、导出 target 集合和运行时 DLL 搜索。任意报错、timeout 或从机器其他位置找到同名包都不能算通过。

## ELF 边界

代码保留 ELF shared library 的安装位置和 `LD_LIBRARY_PATH` 分支。未运行的 ELF 路径不得写成通过。
