// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B2
// C++ 标准要求: C++20
// 本题目标: 刻出 FString vs std::string 三差异 (编码单元 / 无 SSO / operator* vs GetData() 语义), 掌握 FStringView 悬空场景
//
// 骨架阶段预期行为: 仅 StartupModule 打印 [B2] StartupModule called, 其他函数打印提示 TODO
// 完成后预期行为 (Editor 启动日志):
//   LogExB2: [B2] sizeof(TCHAR) = 2   FString("Hello, 世界").Len() = 9
//   LogExB2: [B2] *Empty == nullptr ? 0    Empty.GetData() == nullptr ? 1
//   LogExB2: [B2] sizeof(FString) = 16   heap ptr = 0x...
//   LogExB2: [B2] Printf: Name=Player, Score=9999
//   LogExB2: [B2] FStringView Left(5) = "Hello"   底层 Append 后悬空示范

#include "ExB2_FString.h"
#include "Modules/ModuleManager.h"
#include "Containers/StringView.h"
#include "Containers/StringConv.h"  // TCHAR_TO_UTF8

DEFINE_LOG_CATEGORY_STATIC(LogExB2, Log, All);

IMPLEMENT_MODULE(FB2FStringModule, ExB2_FString);

// ══════════════════════════════════════════════════════
// DemoTCharEncoding: 验证 sizeof(TCHAR) 与 FString::Len() 返回 code unit 数
// ══════════════════════════════════════════════════════
void FB2FStringModule::DemoTCharEncoding()
{
	UE_LOG(LogExB2, Log, TEXT("[B2] sizeof(TCHAR) = %d"), (int32)sizeof(TCHAR));

	FString Str = TEXT("Hello, 世界");
	UE_LOG(LogExB2, Log, TEXT("[B2] Str = '%s'   Len()=%d"), *Str, Str.Len());

	// ══════════════════════════════════════════════════════
	// TODO [必做] 1: 打印 FString("Hi\xD83D\xDE00").Len() (含 U+1F600 emoji, surrogate pair, 占 2 code unit)
	//   观察 Len() 是 4 (Hi + 2 surrogate), 而不是 Unicode codepoint 数 3
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoOperatorStarVsGetData: 空串场景两者语义差
// UnrealString.h.inl: operator* 保证返回非 null; GetData() 未分配时可能 nullptr
// ══════════════════════════════════════════════════════
void FB2FStringModule::DemoOperatorStarVsGetData()
{
	FString Empty;
	const TCHAR* PtrStar = *Empty;        // 保证 TEXT("")
	const TCHAR* PtrData = Empty.GetCharArray().GetData(); // 可能 nullptr (空串未分配)

	UE_LOG(LogExB2, Log, TEXT("[B2] *Empty == nullptr ? %d"), PtrStar == nullptr ? 1 : 0);
	UE_LOG(LogExB2, Log, TEXT("[B2] Empty.GetData() == nullptr ? %d"), PtrData == nullptr ? 1 : 0);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 2: 注释写出为何 `return *FString(TEXT("tmp"));` 悬空
	//   (临时 FString 析构, 其内部 TArray<TCHAR> heap 块释放, 返回的指针悬空)
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoNoSSO: 验证 FString 无 SSO, 非空一律 heap 分配
// ══════════════════════════════════════════════════════
void FB2FStringModule::DemoNoSSO()
{
	UE_LOG(LogExB2, Log, TEXT("[B2] sizeof(FString) = %d"), (int32)sizeof(FString));

	FString S = TEXT("hi");
	const void* ObjAddr = &S;
	const void* DataAddr = S.GetCharArray().GetData();

	UE_LOG(LogExB2, Log, TEXT("[B2] FString obj @ %p   internal data @ %p   same buffer? %d"),
		ObjAddr, DataAddr,
		((const uint8*)DataAddr >= (const uint8*)ObjAddr &&
		 (const uint8*)DataAddr <  (const uint8*)ObjAddr + sizeof(FString)) ? 1 : 0);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 3: 断言 DataAddr 不在 ObjAddr 范围内, 证明无 SSO
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoPrintfFormat: %s 必须传 const TCHAR*, 即 *FString
// ══════════════════════════════════════════════════════
void FB2FStringModule::DemoPrintfFormat()
{
	FString Name = TEXT("Player");
	int32 Score = 9999;
	FString Msg = FString::Printf(TEXT("Name=%s, Score=%d"), *Name, Score);
	UE_LOG(LogExB2, Log, TEXT("[B2] Printf: %s"), *Msg);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 4: 故意漏掉 *Name, 直接传 Name
	//   FString Bug = FString::Printf(TEXT("Name=%s"), Name);
	//   触发 CheckVAArgs, Shipping 构建下可能静默乱码
	// ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// DemoFStringView: 零拷贝视图, 底层 Append 后悬空
// ══════════════════════════════════════════════════════
void FB2FStringModule::DemoFStringView()
{
	FString Long = TEXT("Hello World");
	FStringView View = Long;

	UE_LOG(LogExB2, Log, TEXT("[B2] View Len=%d   Left(5) starts with 'Hello'? %d"),
		View.Len(), View.StartsWith(TEXT("Hello")) ? 1 : 0);

	// ══════════════════════════════════════════════════════
	// TODO [必做] 5: 演示悬空视图
	//   FStringView SubView = View.Left(5);           // 指向 Long 内部 TCHAR 数组
	//   Long += TEXT(" and more text to force realloc"); // 可能触发重分配
	//   // 此时 SubView 悬空, 不要再解引用
	//   // 可以用 AddressSanitizer 验证
	// ══════════════════════════════════════════════════════

	// ══════════════════════════════════════════════════════
	// TODO [进阶] 1: TStringBuilder vs 多次 += 性能对比
	//   TStringBuilder<4096> Builder;
	//   for (int32 i = 0; i < 1000; ++i) Builder << TEXT("x");
	//   FString Result(Builder);
	//   用 FPlatformTime::Cycles64() 测量
	// ══════════════════════════════════════════════════════

	// ══════════════════════════════════════════════════════
	// TODO [进阶] 2: TCHAR_TO_UTF8 转换宏
	//   FString Cn = TEXT("你好");
	//   const ANSICHAR* Utf8 = TCHAR_TO_UTF8(*Cn);
	//   注意: 栈宏, 返回值只在当前作用域有效, 不要持久持有
	// ══════════════════════════════════════════════════════
}

void FB2FStringModule::StartupModule()
{
	UE_LOG(LogExB2, Log, TEXT("[B2] StartupModule called"));

	DemoTCharEncoding();
	DemoOperatorStarVsGetData();
	DemoNoSSO();
	DemoPrintfFormat();
	DemoFStringView();
}

void FB2FStringModule::ShutdownModule()
{
	UE_LOG(LogExB2, Log, TEXT("[B2] ShutdownModule called"));
}


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(sizeof(TCHAR) == 2, TEXT("Windows 上 TCHAR 应为 2 字节 UTF-16 code unit"));
// ensureMsgf(FString().GetData() == nullptr, TEXT("空 FString 内部 TArray 未分配, GetData 应返回 nullptr"));
// ensureMsgf(*FString() != nullptr, TEXT("operator* 保证返回 non-null, 空串返回 TEXT(\"\")"));
