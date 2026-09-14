# L08：NUMA 拓扑观察

默认程序只调用 C08 的 `cs::numa::discover()`，打印当前进程可见的 CPU、core、socket、node 和 memory node。它不跑远端比较，不改变公共配置。

```powershell
cmake -S C13_Performance_Numerics_Data_Layout/exercises/L08_numa -B build/c13-l08 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/c13-l08
ctest --test-dir build/c13-l08 --output-on-failure
.\build\c13-l08\C13_L08_numa.exe
```

显式传 `--placement 1 --pages 4` 才运行小型 `firsttouch` 页面观察。单节点、权限不足或查询不可用返回 77/SKIP；这不是跨节点性能结论。
