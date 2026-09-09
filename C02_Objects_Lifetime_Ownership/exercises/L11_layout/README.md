# 练习 L11：布局、对齐与对象表示观察

先阅读 [11 布局、对齐与对象表示](../../chapters/11-layout-and-representation.md)。

本题是观察型练习，没有 Student。运行 observation 后，记录本机 `sizeof`、`alignof`、数组步长、standard-layout、trivially-copyable 和 EBO 结果。检查器只断言标准保证：数组步长等于 `sizeof(T)`、尾后指针不被解引用、trivially copyable 对象可用 `memcpy` 恢复值。本机尺寸只作为输出，不作为跨平台结论。

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L11_layout -B build/c02-l11 -G "Visual Studio 18 2026" -A x64
cmake --build build/c02-l11 --config Debug
ctest --test-dir build/c02-l11 -C Debug --output-on-failure
```

解析：结构体填充来自成员对齐和数组连续存放要求。`standard_layout` 不表示没有填充；`trivially_copyable` 不表示可以用任意类型指针访问。
