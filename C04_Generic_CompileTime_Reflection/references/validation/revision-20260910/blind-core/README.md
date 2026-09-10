# C04 L07-L10 blind good core

本目录保存 2026-09-10 独立盲写的 L07-L10 正确完成体。盲写阶段未读取任何现有 `validation/good`、`validation/bad`、`src/reference` 或旧审查记录。

## 初始读取范围

- `C04_Generic_CompileTime_Reflection/exercises/L07_packs/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/L07_packs/src/student/pack_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/L07_packs/checks/pack_tools_checks.cpp`
- `C04_Generic_CompileTime_Reflection/chapters/07-packs-nttp.md`
- `C04_Generic_CompileTime_Reflection/exercises/L08_type_lists/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/L08_type_lists/src/student/type_list_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/L08_type_lists/checks/type_list_checks.cpp`
- `C04_Generic_CompileTime_Reflection/chapters/08-type-lists.md`
- `C04_Generic_CompileTime_Reflection/exercises/L09_tuple/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/L09_tuple/src/student/tuple_for_each.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/L09_tuple/checks/tuple_for_each_checks.cpp`
- `C04_Generic_CompileTime_Reflection/chapters/09-tuple-traversal.md`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/README.md`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/src/student/constexpr_tools.hpp`
- `C04_Generic_CompileTime_Reflection/exercises/L10_constexpr/checks/constexpr_tools_checks.cpp`
- `C04_Generic_CompileTime_Reflection/chapters/10-constant-evaluation.md`

## 输出文件

- `L07/pack_tools.hpp`
- `L08/type_list_tools.hpp`
- `L09/tuple_for_each.hpp`
- `L10/constexpr_tools.hpp`

SHA256 会在初版冻结后追加记录。

## 初版 SHA256

- `L07/pack_tools.hpp`: `44CF0CBAB053448749C79F3578125D59DF6725AE09476F9178B8371543CFD13C`
- `L08/type_list_tools.hpp`: `E335503F716B166E91B711602B13896CD2E505C8953FBED876E21C2EFD28BF27`
- `L09/tuple_for_each.hpp`: `82290B453433C965F00EC7D8B731E69ACCC58C653A5629F78E7C91846631DD1B`
- `L10/constexpr_tools.hpp`: `18726CC10B93750A17988C012357BF9F3F35A823A0EA67B35A34668E17A295A5`

## 冻结后修复记录

- `L08/type_list_tools.v2.hpp`: 初版在不可调用 `value_sig` 进入 checker concept 时，因为 `transform_completion_signatures` 无约束展开 `transform_one` 导致硬错误。v2 给列表偏特化增加 `requires (transformable_signature<F, Sigs> && ...)`，不可调用时别名不可用，由 concept 得到 `false`。
- `L10/constexpr_tools.v2.hpp`: 初版在 `consteval` 函数体内用局部变量参与 `static_assert`，MSVC 判定不是常量表达式。v2 改成模板递归解析，字符、累加值和溢出检查都在模板实例化层面完成。
- `L09/tuple_for_each.v2.hpp`: Reference 复核发现初版直接使用 `std::remove_reference_t`，但未显式包含 `<type_traits>`。v2 只补头文件自包含所需 include，行为不变。

## 修正版 SHA256

- `L08/type_list_tools.v2.hpp`: `993D3684653610CF69909B997D905DCC0783DAA08FD852BFB8B41E1F9B1514D2`
- `L09/tuple_for_each.v2.hpp`: `90DEC28AE086FE74A8C41687B178B9ACED42D96E411653B8E358BA951E938BA6`
- `L10/constexpr_tools.v2.hpp`: `C72742879475CC8943CE71296AAE59CC334986F9F942A9BCBD203A207085DB77`
