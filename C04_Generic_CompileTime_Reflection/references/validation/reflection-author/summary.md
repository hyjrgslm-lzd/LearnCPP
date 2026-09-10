# C04 reflection author summary

作者范围：`chapters/13-reflection-model.md`、`chapters/14-splicing-generation.md`、`chapters/15-annotations-frontier.md`、`exercises/F01_frontier/**`。

## 已交付

- 第13章：讲清`std::meta::info`、实体/对象区别、查询前提、字段/枚举、`access_context`。
- 第14章：讲splicing、`template for`、`std::define_static_*`、`data_member_spec`/`define_aggregate`、格式化与序列化边界。
- 第15章：讲annotations与attributes区别，分开C++26、C++29、DR和未采纳提案。
- F01：12个独立probe源文件，每项独立SKIP/PASS/FAIL语义；默认OFF不注册test。r2修复后，无官方宏的主体由最小正例开启，主体失败不回退SKIP。

## 未执行项

按leader指令，当前处于成本作者采样窗口，未启动CMake configure/build/CTest。F01目录尚未接入共享`exercises/CMakeLists.txt`，等待leader集成。

## 规范来源

- P2996R13: https://wg21.link/p2996r13
- P3394R4: https://wg21.link/p3394r4
- P1306R5: https://wg21.link/p1306r5
- P3491R3: https://wg21.link/p3491r3
- P2662R3: https://wg21.link/p2662r3
- P2963R3: https://wg21.link/p2963r3
- P3068R6: https://wg21.link/p3068r6
- P3670R4: https://wg21.link/p3670r4
- P4101R1: https://wg21.link/p4101r1
- P3385R8: https://wg21.link/p3385r8
