# F01 原生前沿：能力与主体分开

先读 [源码与前沿](../../chapters/11-sources-and-frontier.md) 和 [标准索引](../../references/standards-and-implementations.md)。本单元不使用第三方替代标准头；六个源码分别对应 SIMD、linalg、submdspan、padded layout、aligned accessor、mdspan copy/fill。

从 `exercises` 开发终端执行：

```powershell
cmake -S F01_native -B ../build/native -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DC13_ENABLE_FRONTIER=ON
cmake --build ../build/native
ctest --test-dir ../build/native --output-on-failure
```

MSVC 使用 `/std:c++latest`，其他编译器使用 `-std=c++2c`；这些编译选项本身不是完整 C++26/C++29 支持声明。每项最小 probe 编译真实表达式，成功才生成对应 `c13_native_*` 主体目标。主体失败是真实失败。关闭开关是主动禁用，不能写成探针失败；缺能力时不会生成一个只打印 PASS 的替身。

本机配置报告在构建目录 `capabilities.txt`，正文不绑定某次机器状态。要换编译器重新探测，应使用新的构建目录，避免沿用旧缓存。

## 观察任务与解析

- SIMD 五元素输出应是 `[2,3,4,5,6]`：第五项要求正确的 partial 操作，不允许越界加载凑足一组。
- linalg 的矩阵乘以单位阵后保持原值：先填错误哨兵，确保输出真正写入。
- submdspan 的第二列为 `[1,4]`，修改第二项会修改原矩阵；它不是拥有者。
- 2×3 padded 矩阵的行距为 4，写 `(1,2)` 对应底层偏移 6，填充不能被当作逻辑元素改写。
- aligned accessor 的存储实际按 64 字节对齐；声明访问器不能修复错误地址。
- copy/fill 主体在不同布局之间复制相同逻辑元素，再填充全部逻辑位置；支持普通 iterator copy 不代表支持这个重载。
