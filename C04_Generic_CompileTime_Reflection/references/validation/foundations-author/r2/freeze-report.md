# C04 foundations author r2 freeze report

Date: 2026-09-09

Source fix basis:

- Non-author review: `references/reviews/foundations-static-review.md`.
- C++23 return operand rule checked against WG21 P2266R3, "Simpler implicit move", published 2022-03-23: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2266r3.html

Blocked items fixed:

- `chapters/02-deduction.md` and `chapters/03-forwarding-ctad.md`: `decltype(auto)` local return text now distinguishes C++20 behavior from C++23 P2266 move-eligible return operands.
- `chapters/03-forwarding-ctad.md`: `std::forward<T>` is no longer described as correct only in forwarding-reference wrappers; the text ties it to a deduced or explicitly preserved cvref model.
- `chapters/01-template-model.md` and `exercises/L01_templates/**`: L01 now has a real multi-TU `extern template` positive observation and an isolated missing-provider link negative.
- `exercises/L01_templates/**`: unit-scale contract now uses centimeter base with distinguishable `centimeters=1`, `meters=100`, and `kilometers=100000`.

Final validation evidence:

- L01 core after fixes:
  - `04-l01-reconfigure-after-path-fix.json`: PASS, exit 0.
  - `05-l01-build-after-path-fix.json`: PASS, exit 0.
  - `06-l01-ctest-after-path-fix.json`: PASS, exit 0. CTest covered reference, validation/good, validation/bad rejection, lazy instantiation observation, extern-template multi-TU positive, and missing-provider link negative.
- L02 core:
  - `07-l02-configure.json`: PASS, exit 0.
  - `08-l02-build-debug.json`: PASS, exit 0.
  - `09-l02-ctest-debug.json`: PASS, exit 0.
- L03 core after checker fix:
  - `10-l03-build-after-checker-template.json`: PASS, exit 0.
  - `11-l03-ctest-after-checker-template.json`: PASS, exit 0.
- Student-only:
  - L01: `12-l01-student-configure.json` PASS, `15-l01-student-build-after-stub-fix.json` PASS, `17-l01-student-ctest-expected-fail-actual-text.json` PASS with expected CTest exit 8 and text `quantity_value_t reads dependent value_type`.
  - L02: `12-l02-student-configure.json` PASS, `13-l02-student-build-debug.json` PASS, `14-l02-student-ctest-expected-fail.json` PASS with expected CTest exit 8.
  - L03: `12-l03-student-configure.json` PASS, `13-l03-student-build-debug.json` PASS, `14-l03-student-ctest-expected-fail.json` PASS with expected CTest exit 8.
- Static checks:
  - `git diff --check -- [foundations author scope]`: PASS, no output.
  - `rg -n "只有在 forwarding reference|括号表达式是左值，会推成|centimeters.*只检查|#include <.*reference|TODO|FIXME|SKIP|C04_" [foundations author scope]`: no stale blocker text; the only `C04_` hit is a local script path to `StudySetup.cmake`.

Retained failure evidence:

- `03-l01-ctest-debug.json`: FAIL before path fix. L01 regular tests all passed except the new missing-provider diagnostic; nested evidence showed MSBuild FileTracker failed during a too-long diagnostic try-compile path before the intended link check.
- `08-l03-build-debug.json` and `09-l03-ctest-debug.json`: FAIL before checker-template fix. MSVC compiled Reference/good but validation/bad hit an invalid `int&` binding inside the checker.
- `13-l01-student-build-debug.json`: FAIL before Student stub fix. Student lacked `value_type` and `unit_type`.
- `16-l01-student-ctest-expected-fail-after-stub-fix.json`: FAIL only because the expected text was stale; it still proved Student executable failed at runtime. `17-l01-student-ctest-expected-fail-actual-text.json` is the corrected evidence.

Source fingerprints, excluding build outputs:

```text
71C10D398C2599D62C67E688B8853AC1195861A877473EA7E35D791F4FD4027E  .\C04_Generic_CompileTime_Reflection\chapters\01-template-model.md
7A690AC4E0EB18FF89665BE7AF5CCD48FA64A1245BCE45D9D335E81F5772E0BA  .\C04_Generic_CompileTime_Reflection\chapters\02-deduction.md
286F00B0C7EF9E9A1329A9A0B93A10EA8F9F4F727AA69A997555AA9EC9D55CF6  .\C04_Generic_CompileTime_Reflection\chapters\03-forwarding-ctad.md
3BF329978149ED7E90E020757E8EFAADA6730D88B60F357F893987C42EAEB14A  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\checks\quantity_checks.cpp
6CADFFCEECC38C267777CB5EAD07995379E3C07DB25AA51038D2760E2D1CF43A  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\CMakeLists.txt
6286FD02EAFC89600CC11767898C5CC5F485C030F5E77580A94F9D9285D1AF4B  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\check_missing_provider.py
E4ECB67511D988EF186E10B1363970591C3B0676EC1B8BB01BEF4AD7FDAC40B0  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\main.cpp
F26A6652CE0A36A2BF21A8EE47D0A31CD35159557AD76564E539F4B468650982  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\missing_provider_main.cpp
0B6B52CF00E6CB46EAB7C31D99C4A21D3355FEFDB13C38FB9818B5CEBE7626BE  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\twice_instantiation.cpp
8192EFA7867F01DBB517E10F6F60D4EE4CD90A5EA47EB0A08DEEB1AC36ADFE69  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\twice.hpp
48F50B568770055D5AB1FA4D20B1C769AB65E820125E891B45A56936CD94EAF6  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\use_a.cpp
05D15B046FE4B02B08C292DB632FD9F36FA38D25B327503F8B7D1FB15AA7B8A3  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\extern_template\use_b.cpp
4080F06B091797489BC37D87A4211A04BA092144B7153CCB89F7242EAA6D1432  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\observations\instantiation.cpp
3AEE0EEA579DD7358D1D9EF9BB43B35E1A4516DF2EB0B6CBD64FB5F8AF93C623  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\README.md
B2D4CD50E913E02445104ADFBF4C326F31CE38BC5E9789F12256254C0709D0D3  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\src\reference\quantity_templates.hpp
1CB45A465A7CCE13948C74E33D9A49E179C36C5D75918CE6CC36D23F7D8B4CCC  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\src\student\quantity_templates.hpp
34658CF4216A5507DE4732818B32C4E80E11CE1E3436223ABE9C1247FCD30844  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\validation\bad\quantity_templates.hpp
C55B81EA121D54F5F9F337AC6B91C1C1EE824BF87AB79AE3DED586F3133AC900  .\C04_Generic_CompileTime_Reflection\exercises\L01_templates\validation\good\quantity_templates.hpp
243BC8F87B4B97E7F5557C89F31FDBDD7F7DD690A773545FFA11E47EC6237CEE  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\checks\deduction_checks.cpp
76F17083CFD67C33DDABAC5117ED03CF1CE7E143B3239809E167792D256FF84D  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\CMakeLists.txt
88FC30A14DE0057C72394B40718AD8E7446ECC1C59E873B31DA9F82AF2AB1318  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\observations\decltype_rules.cpp
1970A263764D3253667688B6687EA7168ACE3CCB4C9438F1324F82BED27B9CE7  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\README.md
10FF4B9A7659B117D1C53A18D65473044F08FF41CB4DAF2C686FDC50E9FEC388  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\src\reference\deduction_probe.hpp
8DE4629A1A57FC8F4061C277107DEE398A759B806943ABDF8F87F702C4E824FF  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\src\student\deduction_probe.hpp
BEC7FD65881A79EB944F5767242C7BCC2F0CB829E9F4EF5122AEDDE6186FAE9C  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\validation\bad\deduction_probe.hpp
772A5ACB85075AAD06135C2FB0BAE3BCEB30E985CD1703FBC11B67B94E50DE89  .\C04_Generic_CompileTime_Reflection\exercises\L02_deduction\validation\good\deduction_probe.hpp
CAC7FC313A5D3FA4AB1968DFDFEFD1E5E4E4024EC37067E22F96F2B871142827  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\checks\forwarding_checks.cpp
CA2B9C7F0D3F6B186A03FB0BBFB7B9FBA83F116E43B94FA0028D1CC1A254A4F2  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\CMakeLists.txt
52905B6543948FDEBC180C7DB0E7204DFF6FCD4794CD764859F9998B705CBDC3  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\observations\counterexamples.cpp
821CB689DFC4FBFDFC0AF3C33EF73A6AA25FD95E77C703E123F2F057E887948D  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\README.md
652C3880909C35438605C6836D01CD3B2DE553AE3F3795CE20193592F7949613  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\src\reference\forwarding_tools.hpp
95E2B04E8BC43BE7A044588B3F2C3EE8DD0B6182F5667D494767D9AE046E2FA8  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\src\student\forwarding_tools.hpp
1ECFB841D8C65827407761EB7939CB38DE12AD469FF5DD0C20C06F070B20980E  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\validation\bad\forwarding_tools.hpp
C19C0ED1F08F62A9FE5AF0A3238E714CE0445C1A57D06CBD91927F07E951B7BD  .\C04_Generic_CompileTime_Reflection\exercises\L03_forwarding\validation\good\forwarding_tools.hpp
```
