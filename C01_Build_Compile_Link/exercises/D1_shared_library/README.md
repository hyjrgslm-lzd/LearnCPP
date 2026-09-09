# 练习 D1：静态库、动态库、导入库与显式装载

先读 [04：动态库、装载与运行时](../../chapters/04-dynamic-libraries-and-runtime.md)。本题只用一个简单 C 形状函数：

```cpp
extern "C" int lesson_runtime_value();
```

这里还不引入 opaque handle。目标是先分清静态库、动态库、Windows 导入库、导出符号、装载失败和运行时初始化退出。

## Part 1：静态库链接

运行：

```powershell
ctest --test-dir build --tests-regex D1_static_reference --output-on-failure
```

**解析：** 静态库在链接阶段把所需对象代码放进 exe。运行时不需要再找到 `.lib`。这个测试检查 `lesson_runtime_value()` 返回 `7`，并确认库内全局初始化已经发生。

## Part 2：动态库隐式装载

运行：

```powershell
ctest --test-dir build --tests-regex D1_shared_reference --output-on-failure
```

**解析：** Windows shared 目标会产出 DLL 和导入库。exe 链接导入库，但运行时还要找到 DLL。链接成功不等于启动成功。

## Part 3：显式装载和退出观察

运行：

```powershell
ctest --test-dir build --tests-regex D1_loader_reference --output-on-failure
```

loader 用绝对路径打开动态库，查找 `lesson_runtime_value`，再记录卸载时的退出文件。

**解析：** `LoadLibrary`/`GetProcAddress` 或 `dlopen`/`dlsym` 把“库不存在”和“导出不存在”变成可检查步骤。退出文件证明模块卸载会触发库内清理阶段。

## Part 4：失败对照

运行：

```powershell
ctest --test-dir build --tests-regex D1_negative --output-on-failure
```

- `D1_negative_missing_dll` 只把 exe 复制到没有 DLL 的专用目录运行，期望它在进入程序逻辑前失败。
- `D1_negative_missing_export` 用真实 DLL 查找不存在的导出名，期望 loader 报 `missing export`。

**解析：** 缺 DLL 是运行时装载失败；缺导出是显式符号查找失败。configure 失败、编译失败、timeout 或跑到正常输出都不能算负例通过。
