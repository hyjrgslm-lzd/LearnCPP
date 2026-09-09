# 作者 C：MSVC STL `<functional>` 源码入口，2026-09-09

本记录只绑定本机安装头文件，不能等同于某个 GitHub 提交。公开实现状态仍以 Microsoft STL 仓库、changelog 和标准索引中的一手链接为准。

本机输入：

- 文件：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\functional`
- SHA256：`F30F67EFE61C56535C15E48C7BA3F983A7B2AC4BE8468FF12B9235F5D613DC8C`
- 只读检索命令：`rg -n "bad_function_call|_Func_base|_Func_impl_no_alloc|_Copy\(|_Delete_this|_Function_storage_mode|empty move_only_function call|class move_only_function|_Small_object_num_ptrs" D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\functional`

`std::function` 相关入口：

- `bad_function_call`：817 行附近。
- `_Xbad_function_call`：828 行；`_Func_class` 空调用路径 1054 行进入该函数。
- `_Func_base`：847 行起；`_Copy` 在 849 行，`_Delete_this` 在 853 行。
- `_Func_impl_no_alloc`：972 行起；`_Copy` 在 984 行，局部构造与堆分配分支在 988—991 行附近，`_Delete_this` 在 1026 行。
- `_Func_class` 复制路径：1081 行使用 `_Getimpl()->_Copy(&_Mystorage)`。
- `_Func_class` 析构/释放路径：1138 行使用 `_Getimpl()->_Delete_this(!_Local())`。
- SBO 相关存储：`_Space_size = (_Small_object_num_ptrs - 1) * sizeof(void*)` 在 870 行，`_Ptrs[_Small_object_num_ptrs]` 在 1178 行。

`std::move_only_function` 相关入口：

- `_Function_storage_mode { _Small, _Large }`：1443 行。
- 小/大对象存储数组：1450 行。
- 空调用报告文本：1491 行，文本为 `empty move_only_function call (N5008 [func.wrap.move.inv]/2)`；1497 行后备进入 `_Xbad_function_call`。
- 小对象销毁与大对象销毁路径：1538、1554、1560、1566 行附近。
- 小对象复制进入 move-only 存储的实现细节：1695、1719、1733 行附近。
- `move_only_function` 类模板：2143 行起；生成的 cv/ref/noexcept 调用特化在 1926—2123 行附近。

教学结论：这些行只说明本机 MSVC STL 14.51.36231 的实现路径。SBO 大小、分派表组织和错误报告文本是实现细节，课程正文不能硬断其他标准库实现相同。
