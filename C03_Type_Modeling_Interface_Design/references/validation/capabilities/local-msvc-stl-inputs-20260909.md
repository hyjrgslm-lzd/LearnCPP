# 本机 MSVC STL 输入记录，2026-09-09

本文件记录 C03 能力探测使用的本机安装输入。它只绑定这台机器上的头文件内容，不是上游 GitHub 提交映射。

记录命令：

```powershell
Get-FileHash D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\{yvals_core.h,optional,variant,expected,functional,memory,vector} -Algorithm SHA256
rg -n "__cpp_lib_(expected|optional|optional_range_support|variant|move_only_function|copyable_function|function_ref|indirect|polymorphic|contracts)|__cpp_contracts|_MSVC_STL_VERSION|_MSVC_STL_UPDATE|_MSVC_STL_HARDENING" D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\yvals_core.h
```

本机环境：

- 工具集：`MSVC 14.51.36231`
- 头文件根目录：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include`
- `_MSVC_STL_VERSION=145`
- `_MSVC_STL_UPDATE=202604L`

头文件 SHA256：

| 头文件 | SHA256 |
|---|---|
| `yvals_core.h` | `10B59D42BDE2105E76CF16735E34DD0F1D45A943CB1B789C4A0406C67C2BD684` |
| `optional` | `3BFC80A64908F77EABA10EC9A1E2C3B0746DD65E993C8C8D636696ADA184F683` |
| `variant` | `138E638AB6A9F8587D4BE74A3C7C6610B566DCD2ABB9970175CF45366C67C4B4` |
| `expected` | `E415EA9EDE09A89CFD1FF1DA61853CE3B7FB2D83E442E443892B90948722CB44` |
| `functional` | `F30F67EFE61C56535C15E48C7BA3F983A7B2AC4BE8468FF12B9235F5D613DC8C` |
| `memory` | `8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C` |
| `vector` | `B254D6FC250164A9F4E71EA66E2C1E9401CB2BF8B0056550FD92135518450B51` |

`yvals_core.h` 中观察到的宏：

| 宏 | 观察值 |
|---|---:|
| `_MSVC_STL_VERSION` | `145` |
| `_MSVC_STL_UPDATE` | `202604L` |
| `__cpp_lib_expected` | `202211L` |
| `__cpp_lib_move_only_function` | `202110L` |
| `__cpp_lib_optional` | `202110L` |
| `__cpp_lib_variant` | `202106L` |

同一次检索未找到：

- `__cpp_lib_optional_range_support`
- `__cpp_lib_copyable_function`
- `__cpp_lib_function_ref`
- `__cpp_lib_indirect`
- `__cpp_lib_polymorphic`
- `__cpp_contracts`
- `__cpp_lib_contracts`
