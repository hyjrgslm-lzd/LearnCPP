# 离线依赖放置说明

阶段一、二的全部题目**零外部依赖**，纯标准库即可构建，本目录与它们无关。

只有以下阶段三题目需要外部库（默认走 FetchContent 自动联网拉取）：

| 模块 | 题目 | 依赖 | 用途 |
|------|------|------|------|
| K | K1/K2/K3 | [xsimd](https://github.com/xtensor-stack/xsimd) | `std::simd` 的 MSVC 友好回退（header-only） |
| M | M2_execution_bridge | [stdexec](https://github.com/NVIDIA/stdexec) | P2300 `std::execution` 参考实现，仅桥接演示 |

## 离线方式

无法联网时，手工克隆到本目录，再按对应题目 `CMakeLists.txt` 顶部的注释切换为本地路径：

```bash
cd exercises/third_party
git clone --depth 1 --branch 13.2.0 https://github.com/xtensor-stack/xsimd.git
git clone --depth 1 https://github.com/NVIDIA/stdexec.git
```

## 锁定版本

`cmake/XsimdSetup.cmake` 与 `cmake/StdexecSetup.cmake` 中的 `GIT_TAG` 可改为具体
commit/tag 以锁定版本。xsimd 当前锁定 `13.2.0`，stdexec 跟随 `main`。
