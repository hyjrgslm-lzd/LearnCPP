// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B3
// C++ 标准要求: C++20
// 本题目标: 理解 FName 全局池 + 大小写不敏感 + ComparisonIndex/DisplayIndex 双 ID; 理解 FText 惰性求值与无 operator== 的设计决策
//
// 骨架阶段预期行为: StartupModule 打印各 Demo 的基础输出, TODO 留白
// 完成后预期行为 (Editor 启动日志):
//   LogExB3: [B3] NameA==NameB (IgnoreCase)=1  IsEqual(CaseSensitive)=0
//   LogExB3: [B3] NameA.ComparisonIndex == NameC.ComparisonIndex = 1
//   LogExB3: [B3] FName ==   cycles: N    ToString ==   cycles: M   (M 比 N 大 1-2 量级)
//   LogExB3: [B3] None1.IsNone()=1   NAME_None ToString='None'
//   LogExB3: [B3] FText lazy Format 构造发生; ToString 触发求值 -> "Player: Alice, Score: 100"

#include "ExB3_FName.h"
#include "Modules/ModuleManager.h"
#include "UObject/NameTypes.h"
#include "Internationalization/Text.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "ExB3_FName"

DEFINE_LOG_CATEGORY_STATIC(LogExB3, Log, All);

IMPLEMENT_MODULE(FB3FNameModule, ExB3_FName);

// ══════════════════════════════════════════════════════
// DemoFNamePooling: 大小写不敏感 operator==, ComparisonIndex 对齐
// 源码: Runtime/Core/Public/UObject/NameTypes.h, Runtime/Core/Private/UObject/UnrealNames.cpp
// ══════════════════════════════════════════════════════
void FB3FNameModule::DemoFNamePooling()
{
	FName NameA(TEXT("MyActor"));
	FName NameB(TEXT("MYACTOR")); // 不同大小写, comparison 相等
	FName NameC(TEXT("MyActor"));

	UE_LOG(LogExB3, Log, TEXT("[B3] NameA == NameB (IgnoreCase): %d"), NameA == NameB ? 1 : 0);
	UE_LOG(LogExB3, Log, TEXT("[B3] NameA.IsEqual(NameB, CaseSensitive): %d"),
		NameA.IsEqual(NameB, ENameCase::CaseSensitive) ? 1 : 0);
	UE_LOG(LogExB3, Log, TEXT("[B3] NameA == NameC: %d"), NameA == NameC ? 1 : 0);

	UE_LOG(LogExB3, Log, TEXT("[B3] NameA.Comparison == NameC.Comparison: %d"),
		NameA.GetComparisonIndex() == NameC.GetComparisonIndex() ? 1 : 0);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 1: 打印 NameA.GetDisplayIndex() vs NameB.GetDisplayIndex()
	//   Editor (WITH_CASE_PRESERVING_NAME) 下两者不同, Shipping 下相同
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoFNameToStringCost: 直接 FName == 与 ToString() 比较的耗时差
// ══════════════════════════════════════════════════════
void FB3FNameModule::DemoFNameToStringCost()
{
	FName Target(TEXT("TestName"));
	FName Probe(TEXT("TestName"));

	constexpr int32 N = 1'000'000;

	// 方式一: FName 直接比较 (FNameEntryId 整数对)
	uint64 T0 = FPlatformTime::Cycles64();
	volatile int32 HitsA = 0;
	for (int32 i = 0; i < N; ++i) { if (Target == Probe) ++HitsA; }
	uint64 T1 = FPlatformTime::Cycles64();

	// 方式二: ToString 后 FString 比较
	volatile int32 HitsB = 0;
	for (int32 i = 0; i < N; ++i) { if (Target.ToString() == TEXT("TestName")) ++HitsB; }
	uint64 T2 = FPlatformTime::Cycles64();

	UE_LOG(LogExB3, Log, TEXT("[B3] FName==    cycles: %llu"), (unsigned long long)(T1 - T0));
	UE_LOG(LogExB3, Log, TEXT("[B3] ToString== cycles: %llu"), (unsigned long long)(T2 - T1));

	// ══════════════════════════════════════════════════════
	// TODO [必做] 2: 注释写出原因
	//   FName == 内部: GetComparisonIndex() == Rhs.GetComparisonIndex() -> uint32 比较, O(1) 无分配
	//   ToString 内部: FNamePool 查表 + 复制构造 FString, 触发堆分配
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoNameNone: NAME_None 与空字符串等价, IsNone() 判定
// ══════════════════════════════════════════════════════
void FB3FNameModule::DemoNameNone()
{
	FName None1;
	FName None2(NAME_None);
	FName None3(TEXT("None"));
	FName Empty(TEXT(""));

	UE_LOG(LogExB3, Log, TEXT("[B3] None1.IsNone()=%d  None2=%d  None3=%d  Empty=%d"),
		None1.IsNone() ? 1 : 0, None2.IsNone() ? 1 : 0,
		None3.IsNone() ? 1 : 0, Empty.IsNone() ? 1 : 0);
	UE_LOG(LogExB3, Log, TEXT("[B3] NAME_None ToString: '%s'"), *None1.ToString());

	// ══════════════════════════════════════════════════════
	// TODO [必做] 3: 断言 None1 == Empty (空 FName 即 NAME_None)
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoFTextLazy: FText::Format 惰性, ToString 触发实际求值
// ══════════════════════════════════════════════════════
void FB3FNameModule::DemoFTextLazy()
{
	FText PlayerText = FText::FromString(TEXT("Alice"));
	FText ScoreText  = FText::AsNumber(100);
	FText FormatPat  = LOCTEXT("ScoreFmt", "Player: {0}, Score: {1}");

	UE_LOG(LogExB3, Log, TEXT("[B3] 将构造 LazyResult, 内部持 FTextHistory_OrderedFormat, 不求值"));
	FText LazyResult = FText::Format(FormatPat, PlayerText, ScoreText);

	UE_LOG(LogExB3, Log, TEXT("[B3] 将调 ToString 触发 BuildDisplayString"));
	FString DisplayStr = LazyResult.ToString();

	UE_LOG(LogExB3, Log, TEXT("[B3] LazyResult -> '%s'"), *DisplayStr);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 4: 在 FText::Format 与 ToString 之间各插入 UE_LOG 时间戳
	//   观察 ToString 触发时才发生字符串分配的时序
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoFTextNoCompare: FText 无 operator==, 本地化文字不适合做逻辑标识
// ══════════════════════════════════════════════════════
void FB3FNameModule::DemoFTextNoCompare()
{
	FText TextA = FText::FromString(TEXT("Hello"));
	FText TextB = FText::FromString(TEXT("Hello"));

	// if (TextA == TextB) { ... } // 编译错误, FText 无 operator==
	bool bSameStr = TextA.ToString() == TextB.ToString();
	bool bIdentical = TextA.IdenticalTo(TextB);

	UE_LOG(LogExB3, Log, TEXT("[B3] ToString 字面相等=%d   IdenticalTo=%d"),
		bSameStr ? 1 : 0, bIdentical ? 1 : 0);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 5: 写下三类字符串的用途分类表 (注释里)
	//   | 场景 | 用哪个 |
	//   |---|---|
	//   | 日志/调试                       | FString::Printf / UE_LOG |
	//   | TMap key / 属性名 / 骨骼名      | FName |
	//   | 玩家可见本地化 UI 文字          | FText |
	//   | 网络传输字符串                  | FString (序列化后) |
	// ══════════════════════════════════════════════════════

	// ══════════════════════════════════════════════════════
	// TODO [进阶] 1: 阅读 Runtime/Core/Private/UObject/UnrealNames.cpp
	//   搜 FNamePoolShardBase 与 FNamePool, 回答为何池"只写不改"
	// ══════════════════════════════════════════════════════

	// ══════════════════════════════════════════════════════
	// TODO [进阶] 2: 阅读 Internationalization/Text.h 第 39-48 行 ETextFlag
	//   列出各 flag 及其对 FText 行为的影响
	// ══════════════════════════════════════════════════════

	// ══════════════════════════════════════════════════════
	// TODO [进阶] 3: FText::AsNumber 文化敏感性
	//   FInternationalization::Get().SetCurrentCulture(TEXT("de-DE"));
	//   FText::AsNumber(1234567.89).ToString() -> "1.234.567,89"
	//   记得恢复 culture
	// ══════════════════════════════════════════════════════
}

void FB3FNameModule::StartupModule()
{
	UE_LOG(LogExB3, Log, TEXT("[B3] StartupModule called"));

	DemoFNamePooling();
	DemoFNameToStringCost();
	DemoNameNone();
	DemoFTextLazy();
	DemoFTextNoCompare();
}

void FB3FNameModule::ShutdownModule()
{
	UE_LOG(LogExB3, Log, TEXT("[B3] ShutdownModule called"));
}

#undef LOCTEXT_NAMESPACE


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(NameA == NameB, TEXT("FName 默认大小写不敏感 operator=="));
// ensureMsgf(NameA.GetComparisonIndex() == NameC.GetComparisonIndex(), TEXT("同字符串 ComparisonIndex 一致"));
// ensureMsgf(FName(TEXT("")).IsNone(), TEXT("空字符串 FName 即 NAME_None"));
// ensureMsgf((T2 - T1) > (T1 - T0) * 10, TEXT("ToString 比直接 == 慢至少一量级"));
