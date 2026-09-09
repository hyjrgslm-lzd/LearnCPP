# 作者B：MSVC `<functional>` 源码导读输入

日期：2026-09-09。本记录只绑定本机安装的 MSVC STL 头文件入口，用于第 11 章类型擦除源码对照；它不是上游 Git 提交证明，也不是所有标准库实现的布局承诺。

## 本机文件

- 文件：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\functional`
- SHA256：`F30F67EFE61C56535C15E48C7BA3F983A7B2AC4BE8468FF12B9235F5D613DC8C`

## 只读定位

`Select-String` 在本机 `<functional>` 中定位到这些实现入口：

- `_Space_size`：小对象内部存储容量的本机实现常量入口。
- `_Is_large`：按大小、对齐和 nothrow move 条件决定是否走外部分配的本机实现入口。
- `_Func_impl` / `_Func_impl_no_alloc`：保存 callable 并实现 clone/move/delete/call 的具体实现类入口。
- `_Func_class`：`std::function` 主包装类入口。

这些名字以下划线开头，属于实现细节。第 11 章只用它解释“标准库也会用操作表、对象存储和间接调用实现类型擦除”这一思想，不要求学生依赖名称、布局、SBO 阈值或分配策略。

