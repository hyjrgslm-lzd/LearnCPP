> 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B2

## 目标

把 `FString` 与 `std::string` 的三条核心差异 (编码单元 / 无 SSO / `operator*` vs `GetData()` 语义) 刻成直觉, 掌握 `FStringView` 作为非拥有视图的正确用法与悬空场景, 理解 `FString::Printf` 的 `%s` 必须传 `*FString` 的根因。通过 inspect 阅读 `UnrealString.h.inl`, 看懂 `FString` 内部 `TArray<TCHAR>` 的布局。

## 前置理解

- 已完成 ExA1_HelloModule ~ ExB1_TArrayTMap, 理解 `.Build.cs` 依赖声明和 TArray 重分配语义
- 熟悉 C++ 悬垂引用产生条件 (临时对象析构前其内部 heap 指针被"泄露"出去)
- 熟悉 `std::string::c_str()` / `data()` 的空串行为

## 必做任务

1. 完成 TODO 1: 用含 emoji (surrogate pair) 字符串观察 `Len()` 返 code unit 数而非 codepoint 数
2. 完成 TODO 2: 在注释里写出为何 `return *FString(TEXT("tmp"));` 是悬空 UB
3. 完成 TODO 3: 证明 `FString::GetData()` 返回的地址不落在对象本身 `sizeof(FString)` 范围内, 即无 SSO
4. 完成 TODO 4: 故意漏 `*Name` 直接传 `Name` 给 `%s`, 观察编译器警告或运行时乱码
5. 完成 TODO 5: 取 `FStringView` 子视图后对底层 `FString += ...` 强制重分配, 示范视图悬空

## 进阶任务

- TODO 进阶 1: `TStringBuilder<4096>` vs 多次 `+=` 拼接 1000 个 TCHAR 的耗时对比
- TODO 进阶 2: `TCHAR_TO_UTF8` 宏演示, 把含中文 `FString` 转 UTF-8 `const char*` 传给 `printf` 风格 C 接口
- 读 `Containers/UnrealString.h.inl`, 搜 `operator*` 的实现, 确认 `Data.Num() > 0 ? Data.GetData() : TEXT("")` 的分支
- 读 `#define UE_STRING_CLASS FString` 的宏模板化机制, 理解 `FString` / `FAnsiString` / `FUtf8String` 如何共享同一套 .inl

## 验收点

- [ ] 不查文档能写出 `FString::Printf(TEXT("%s has %d points"), *Name, Score)` 并解释为何 `*Name`
- [ ] 能说出空 `FString` 时 `*Str` 返回 `TEXT("")`, `Str.GetData()` 可能返回 nullptr 的原因
- [ ] 能说出 `FStringView` 悬空的三类触发 (`Append` / `+=` / `Insert`)
- [ ] 读过 `UnrealString.h.inl`, 能说出 `Len()` = `Data.Num() - 1` 的差 1 来自 null terminator
- [ ] 能答出四个固定复盘问题 + 题目专属问题

## 观察点

- `TCHAR` 在 Windows 上是 `wchar_t` (16 bit, UTF-16LE), 在 Linux/Mac 上是 32 bit `wchar_t` (UTF-32). `sizeof(TCHAR)` 不是常量 2
- `FString::Printf` 格式串是 `TCHAR*`, 与 C `printf` (char*) 不是同一套; 混用会在某些编译选项下被 `CheckVAArgs` 捕获, 但并非所有平台都能检测
- `FStringView` 存 pointer+length, 不强制 null terminator; 子视图无法直接传给接受 `const TCHAR*` 的 C 风格 API, 需要先 `ToString()` 到 `FString`
- UE 为何选 UTF-16 内部编码: 历史上与 Windows 宽字符 API 直接兼容。新增的 `FUtf8String` / `UTF8CHAR` 用于需要 UTF-8 的场景 (网络协议 / JSON)

## 常见坑

- `const TCHAR* Ptr = *SomeFuncReturningFString();` — 临时 FString 析构, Ptr 即悬空
- 把 `const TCHAR*` 当 `const char*` 传给 `fopen/puts` 等 C API — 必须 `TCHAR_TO_UTF8` / `TCHAR_TO_ANSI` 转
- `TCHAR_TO_UTF8(*Str)` 返回栈对象, 不要存起来隔帧用
- `%s` 传 `FString` 不是 `*FString` — 对象布局不以字符指针开头, 结果乱码或崩溃

## 提示

- 读 `UnrealString.h.inl` 前先读 `UnrealString.h` 找 `#define UE_STRING_CLASS FString` 宏套路
- AddressSanitizer (`-fsanitize=address`) 可捕获悬空指针读, Debug 构建 + ASan 跑本练习效果最佳
- `TStringBuilder<N>` 的 N 是 inline buffer 大小 (TCHAR 数), 超出时回退堆

## 复盘问题 (四固定 + 专属)

1. **真正开始执行的时刻?** `FString::Printf` 同步立即在调用线程完成构造, 无延迟执行
2. **谁负责生命周期?** `FString` 值语义独占内部 `TArray<TCHAR>` heap 块; `FStringView` 不拥有任何内存, 生命期绑底层 FString
3. **涉及哪些命名线程?** 字符串操作通常在 GameThread; `FString` 本身非线程安全, 多线程共享需外部同步
4. **对 GC 如何可见?** `FString` 非 UObject, GC 不追踪内容; UObject 成员持 FString 时 `UPROPERTY` 仅为序列化, 与 GC 可达性无关
5. **专属**: `std::string_view` (C++17) 与 `FStringView` 是否都保证视图末尾 null terminator? (答: 都不保证. 两者均 pointer+length, 子视图不在末尾插 null, 故不能直接当 `c_str()` 用)

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/Containers/UnrealString.h` (`operator*` 声明)
- `Engine/Source/Runtime/Core/Public/Containers/UnrealString.h.inl` (`Len()`, `GetData()`, `operator*` 实现, `#define UE_STRING_CLASS` 模板化)
- `Engine/Source/Runtime/Core/Public/Containers/StringView.h` (`FStringView`)
- `Engine/Source/Runtime/Core/Public/Containers/StringConv.h` (`TCHAR_TO_UTF8`, `FUTF8ToTCHAR`)
- `Engine/Source/Runtime/Core/Public/Misc/StringBuilder.h` (`TStringBuilder<N>`)
