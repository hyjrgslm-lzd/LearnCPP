// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B3
// 小节: FName 全局池 + FText 惰性求值
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FB3FNameModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void DemoFNamePooling();
	void DemoFNameToStringCost();
	void DemoNameNone();
	void DemoFTextLazy();
	void DemoFTextNoCompare();
};
