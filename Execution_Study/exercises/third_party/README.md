# 如何将 stdexec 离线放置到此目录

如果你的网络环境无法在 CMake 配置时自动拉取 GitHub 仓库，可以手动操作：

```bash
cd exercises/third_party
git clone https://github.com/NVIDIA/stdexec.git
```

然后在 `exercises/CMakeLists.txt` 中：
1. 注释掉 "方案 A"（FetchContent 部分）
2. 取消注释 "方案 B"（add_subdirectory 部分）

重新运行 cmake 配置即可。
