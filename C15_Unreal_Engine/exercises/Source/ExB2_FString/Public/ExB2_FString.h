// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B2
// 小节: FString / FStringView 三件套, operator* 与 GetData() 边界, Printf 格式
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FB2FStringModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// 各演示函数 — .cpp 里逐个实现, TODO 留白
	void DemoTCharEncoding();
	void DemoOperatorStarVsGetData();
	void DemoNoSSO();
	void DemoPrintfFormat();
	void DemoFStringView();
};
