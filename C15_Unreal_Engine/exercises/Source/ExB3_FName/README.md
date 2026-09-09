> 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B3

## 目标

彻底理解 `FName` 和 `FText` 不是"更好的 FString"而是在不同语义维度上的补充: `FName` 是不可变、大小写不敏感、全局池化的标识符, 适合引用已知字符串集合; `FText` 是可本地化、惰性格式化的显示字符串, 不适合逻辑比较。通过 inspect 阅读 `UnrealNames.cpp` 的分片锁结构和 `Text.h` 的 `FTextHistory` 子类体系, 建立对 `WITH_CASE_PRESERVING_NAME` 在 Editor vs Shipping 差异的直觉。

## 前置理解

- 已完成 ExB2_FString, 理解 FString 堆分配与 `operator*` 边界
- 熟悉字符串驻留 (string interning) 与全局哈希表概念
- 理解惰性求值 (对比 `01-心智模型.md` §RDG = lazy graph)
- 理解 Editor vs Runtime 构建的条件编译宏 (`WITH_EDITORONLY_DATA`)

## 必做任务

1. 完成 TODO 1: 打印 `NameA.GetDisplayIndex()` 与 `NameB.GetDisplayIndex()`, 观察 Editor 下不同、Shipping 下相同
2. 完成 TODO 2: 在注释里写出 `FName ==` vs `ToString() ==` 性能差根因 (整数对比 vs 池查 + 堆分配)
3. 完成 TODO 3: 断言空字符串 `FName(TEXT(""))` 即 `NAME_None`
4. 完成 TODO 4: 在 `FText::Format` 与 `ToString` 之间插入时间戳, 确认实际求值发生在 ToString
5. 完成 TODO 5: 在注释里填写三类字符串用途分类表

## 进阶任务

- TODO 进阶 1: 读 `UnrealNames.cpp` 的 `FNamePoolShardBase` / `FNamePool`, 回答池为何"只写不改" (入池后不删, 保证 ID 永久有效)
- TODO 进阶 2: 读 `Text.h` 第 39-48 行 `ETextFlag`, 列出各 flag 对 FText 行为的影响
- TODO 进阶 3: 切换 culture 为 `de-DE`, 观察 `FText::AsNumber(1234567.89).ToString()` 输出 `"1.234.567,89"`
- 阅读 `NameTypes.h` 第 111 行 `FNameEntryId::ToUnstableInt()` 与 141 行 `FArchive& operator<<`, 回答为何跨进程不能用 ToUnstableInt 比较

## 验收点

- [ ] 一句话解释 FName 的 `operator==` 比较整数 ID 而非逐字符
- [ ] 能说出 `FName::ToString()` 的成本来源 (池查 + FString 构造)
- [ ] 能解释 `WITH_CASE_PRESERVING_NAME` 带来的 Editor vs Shipping 大小写差异与规避方式 (FName 用于逻辑, 显示用 FText)
- [ ] 能说出 `FText::Format` 惰性 + `FTextHistory` 角色
- [ ] 能说出 FText 为何无 `operator==` 的设计决策, 以及何时用 `IdenticalTo` (序列化层) vs `ToString()` (弃用于业务)
- [ ] 能答出四个固定复盘问题 + 专属题

## 观察点

- **FName 双 ID 体系** (Editor): `ComparisonIndex` (全小写, 用于 `operator==`) + `DisplayIndex` (原始大小写, 用于 `ToString()`). 仅在 `WITH_CASE_PRESERVING_NAME` (即 `WITH_EDITORONLY_DATA`) 开
- **池永不回收**: FName 入池后不被移除, 长时间 Editor 进程里 `FNamePool` 持续增长。不要用随机字符串作 FName
- **FText 与反射不兼容**: FText 无 `operator==`, 序列化 dirty 检测用 `IdenticalTo`。蓝图/反射层对 FText 有特殊处理
- **FText::AsNumber 文化敏感**: 英语 `"1,234,567.89"` vs 德语 `"1.234.567,89"`; `FString::Printf("%.2f")` 不跟 culture (C locale)

## 常见坑

- 用 FName 存动态/随机生成字符串 → 池无限增长, OOM
- Shipping 下依赖 `FName::ToString()` 保留大小写 — 只存小写 comparison ID, 返回全小写
- FText 用于日志打印 — `ToString` 涉及 culture 查询 + history 求值, 比 `FString::Printf` 贵。调试用 FString, 玩家可见用 FText
- 手写序列化时用 `TextA.ToString() == TextB.ToString()` 判断 FText 修改 — culture 切换后可能误判
- `TMap<FName, V>` vs `TMap<FString, V>` — 前者 key 大小写不敏感, 后者敏感; 属性名查找时易踩

## 提示

- `FNamePool` 实现在 `.cpp` 文件 (`UnrealNames.cpp`), 头里仅见 `friend` 声明。读实现要去 `Private/UObject/`
- 测量 FName 比较 vs ToString 时, FName 构造 (入池) 排除出计时范围
- `LOCTEXT` 宏必须在带 `#define LOCTEXT_NAMESPACE "..."` 的文件里用, 结尾 `#undef LOCTEXT_NAMESPACE` 关闭
- `INVTEXT("literal")` 产生 `CultureInvariant` FText, 不参与翻译

## 复盘问题 (四固定 + 专属)

1. **真正开始执行的时刻?** FName 构造 (入池) 即同步发生; FText::Format 构造仅存 history, 实际求值在 ToString。两者均在调用线程
2. **谁负责生命周期?** FName 是值类型但指向全局 `FNamePool` 条目 (永不回收); FText 是值类型但内部 `FTextHistory` 可能共享引用计数 (TSharedPtr)
3. **涉及哪些命名线程?** FName 比较 O(1) 任何线程均可; 入池走 shard 锁。FText 构造/求值通常 GameThread; culture 切换涉及全局状态, 需同步
4. **对 GC 如何可见?** FName/FText 皆非 UObject, GC 不追踪。UObject 成员持 FName `UPROPERTY` 是为序列化而非 GC 可达
5. **专属**: `std::string_view` 与 `FStringView` / `FName` 的根本区别? (答: string_view 是纯视图, 不池化; FName 是池化 ID, 比较 O(1) 整数对; FStringView 是视图但生命期绑底层 FString。三者语义轴不同: 视图 / 池化 / 惰性)

## 对应官方参考

- `Engine/Source/Runtime/Core/Public/UObject/NameTypes.h` (`FName`, `FNameEntryId::ToUnstableInt` @111, `FArchive<<FNameEntryId>` @141, `WITH_CASE_PRESERVING_NAME` @32)
- `Engine/Source/Runtime/Core/Private/UObject/UnrealNames.cpp` (`FNamePool`, `FNamePoolShardBase` 分片锁)
- `Engine/Source/Runtime/Core/Public/Internationalization/Text.h` (`FText`, `ETextFlag` @39-48, `FTextHistory_*` 子类 @~920)
- `Engine/Source/Runtime/Core/Public/Internationalization/Internationalization.h` (`FInternationalization::SetCurrentCulture`)
