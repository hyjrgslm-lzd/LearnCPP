# 练习 L13：合法类型访问与 `launder` 观察

先阅读 [13 别名、类型访问与指针来源](../../chapters/13-aliasing-and-provenance.md)。

本题只运行合法路径：`std::bit_cast`、`std::memcpy`、字节观察和同一存储中销毁后重建对象并通过 `std::launder` 取新指针。错误类型访问写在正文中，不在默认 target 执行。

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/L13_aliasing -B build/c02-l13 -G "Visual Studio 18 2026" -A x64
cmake --build build/c02-l13 --config Debug
ctest --test-dir build/c02-l13 -C Debug --output-on-failure
```

解析：`bit_cast` 和 `memcpy` 处理的是对象表示；它们不会让源对象拥有另一个动态类型。`launder` 只在新对象生命期已经开始后重新取得指针，不负责开始生命期。
