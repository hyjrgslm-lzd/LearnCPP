// 占位主模块实现: 整个 Target 内 IMPLEMENT_PRIMARY_GAME_MODULE 只出现一次, 必须在主模块
// 其他 ExXN_* 模块使用 IMPLEMENT_MODULE(FDefaultModuleImpl, ...)

#include "C15_Unreal_Engine.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, C15_Unreal_Engine, "C15_Unreal_Engine");
