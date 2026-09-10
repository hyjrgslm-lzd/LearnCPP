# CAPSTONE1 作者验证记录 r2

本轮修复 `references/reviews/sample-review.md` 的 4 个 BLOCK，只修改原作者归属的样章和 CAPSTONE1 文件。

## 修复项

- `count_reparse_demo` 改为正文同形链：`transform(parse_counted) -> filter(optional.has_value) -> transform(unwrap_by_value)`，checker 仍实测 16 次解析。
- Reference/good/bad 的相关收束改为实际调用 `std::ranges::to<std::vector<...>>()`，删除样章里“避开 ranges::to”的说法。
- `summarize(lines, error_limit)` 负数统一抛 `std::invalid_argument`；checker 覆盖 `-1`、`0`、`99`。
- 文本字段说明修正为 `timestamp`、`user_id`、`message` 是 `std::string`，`level` 是 enum。
- `author-validation.md` 改为中文；原始命令、目标名和错误关键字保持英文。

## 命令与结果

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_reference
```

结果：exit 0。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_good
```

结果：exit 0。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_validation_bad
```

结果：exit 0。

```powershell
cmake --build C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author --config Release --target CAPSTONE1_log_pipeline_student
```

结果：exit 0。

```powershell
ctest --test-dir C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author -C Release --output-on-failure
```

结果：exit 0。3/3 通过：

- `CAPSTONE1_log_pipeline_reference`
- `CAPSTONE1_log_pipeline_validation_good`
- `CAPSTONE1_log_pipeline_validation_bad_rejected`

```powershell
C06_Ranges\exercises\CAPSTONE1_log_pipeline\build-sample-author\Release\CAPSTONE1_log_pipeline_student.exe
```

结果：exit 1，输出：

```text
check failed: valid row parses
```

## r2 源文件 SHA256

```text
EC9AF906279E5E64E70F61569A0F425A5D85B028CEBC1A4D485D6F458AD838C0  F:\CPPTrain\LearnCPP\C06_Ranges\chapters\00-log-pipeline-evolution.md
B00391C369C996CBB960CFE2E0325C04834FD201A65CD280EAA3C55DEE7EC7EB  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\README.md
441611281532FE8E4EE26F4E297980ACBB91F77A62B337B4B0F87C88878F1EB7  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\CMakeLists.txt
84E7B5114D4703B24381D21491BBF23F00C3154F349D79A0EE846385000C1F33  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\main.cpp
8FE5CB0FF5C60CB4B0F28A4DBED9C235F3E8F104D258931112ED09DE37B93745  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\include\log_pipeline_data.hpp
4894D231B70C87DC70A3B8CC4BE01D479C6731262FC22AE8E140977F070B1EDA  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\reference\log_pipeline.hpp
A47AF00DB2EF0DC499412F158C2DE54C65A2C573EE95A95EFE8DF4708456DF2C  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\good\log_pipeline.hpp
57DA0A7AA3F764B2B1595F54D37C6C0392C2883E624E21C9BD9D3B70965CA462  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\validation\bad\log_pipeline.hpp
F8D9962E2BFC7A6417A2794C00562620A7ECFAB2B58AFF3D404338D9D42F0DF1  F:\CPPTrain\LearnCPP\C06_Ranges\exercises\CAPSTONE1_log_pipeline\src\student\log_pipeline.hpp
```

## 边界

没有修改 `references/reviews/sample-review.md` 或 review JSON。没有扩展到其他作者 lane。没有声称性能加速。
