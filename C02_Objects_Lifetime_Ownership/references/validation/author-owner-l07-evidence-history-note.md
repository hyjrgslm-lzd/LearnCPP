# L07 作者验证证据历史说明

状态：仅补证据历史说明。L07 r2 代码保持冻结，等待非作者复验。本说明不重跑命令、不伪造已删除 JSON 的 stdout/stderr，也不把后续重跑记录冒充旧记录。

## 已删除的 r2 中间 JSON

以下文件曾在 r2 返修过程中生成，后来被作者用 `Get-ChildItem -Filter author-owner-l07-r2*.json | Where-Object { $_.Name -notlike 'author-owner-l07-r2-final*.json' } | Remove-Item` 清理。该清理违反了规格中“原始失败/旧记录保留”的证据要求。本次不再清理任何验证证据。

- `author-owner-l07-r2-configure.json`：中间 configure 记录。删除前为 PASS。
- `author-owner-l07-r2-build-debug.json`：中间 Debug build 记录。删除前为 PASS。
- `author-owner-l07-r2-ctest-debug.json`：中间 Debug CTest 记录。删除前为 PASS。
- `author-owner-l07-r2-build-release.json`：中间 Release build 记录。删除前为 PASS。
- `author-owner-l07-r2-ctest-release.json`：中间 Release CTest 记录。删除前为 PASS。
- `author-owner-l07-r2-student-configure.json`：中间 Student-only configure 记录。删除前为 PASS。
- `author-owner-l07-r2-student-build.json`：中间 Student-only build 记录。删除前为 PASS。
- `author-owner-l07-r2-student-ctest-placeholder-fails.json`：曾先生成一份 FAIL 记录，真实原因是作者把 expected exit 写成 `1`，而 `ctest` 对失败测试返回 `8`；诊断文本已包含 `check failed: normal owner must own first`。随后作者删除该 FAIL 记录，并按 expected exit `8` 重新生成同名 PASS 中间记录；该 PASS 中间记录后来又被上述非 final 清理命令删除。原始 FAIL JSON 已缺失，无法无损恢复。
- `author-owner-l07-r2-reference-off-target-absent.json`：中间 Reference-off target 缺席记录。删除前为 PASS。
- `author-owner-l07-r2-validation-configure.json`：中间 validation configure 记录。删除前为 PASS。
- `author-owner-l07-r2-validation-build.json`：中间 validation build 记录。删除前为 PASS。
- `author-owner-l07-r2-validation-good-run.json`：中间 validation good 运行记录。删除前为 PASS。
- `author-owner-l07-r2-validation-bad-noop-run.json`：中间 validation bad_noop 运行记录。删除前为 PASS。
- `author-owner-l07-r2-validation-bad-fake-completed-run.json`：中间 validation bad_fake_completed 运行记录。删除前为 PASS。

## 已删除的 r1/初版非 final JSON

初版也发生过同类清理。以下非 final 记录已缺失，不能恢复原始 stdout/stderr：

- `author-owner-l07-configure-debug-release.json`
- `author-owner-l07-build-debug.json`
- `author-owner-l07-ctest-debug.json`
- `author-owner-l07-build-release.json`
- `author-owner-l07-ctest-release.json`
- `author-owner-l07-student-configure.json`
- `author-owner-l07-student-build.json`
- `author-owner-l07-student-build-default.json`
- `author-owner-l07-student-ctest-placeholder-fails.json`
- `author-owner-l07-reference-off-target-absent.json`
- `author-owner-l07-validation-configure.json`
- `author-owner-l07-validation-build.json`
- `author-owner-l07-validation-good-run.json`
- `author-owner-l07-validation-bad-noop-run.json`

其中 `author-owner-l07-student-ctest-placeholder-fails.json` 也曾先出现一份 FAIL 记录，真实原因同样是 expected exit 写成 `1`，而 `ctest` 实际返回 `8`；随后被删除并按 expected exit `8` 重新记录过。该原始 FAIL JSON 已缺失。

## 当前仍保留的可用证据

- r1 final JSON 仍在 `Core_Study/references/validation/author-owner-l07-final-*.json`。
- r2 final JSON 仍在 `Core_Study/references/validation/author-owner-l07-r2-final-*.json`。
- r2 返修报告仍在 `Core_Study/references/validation/author-owner-l07-r2-report.md`。

这些 final 记录是后续真实运行生成的证据，只证明它们对应的运行，不恢复被删除的中间记录。

## 证据缺口

已删除的中间 JSON 没有从文件系统恢复路径。本文只按当前会话中的操作事实做历史说明，不包含被删除文件的完整原始 stdout/stderr、时间戳、duration、cleanup 状态或 command 数组。后续审查应把这些中间记录视为历史缺失，而不是把 final 记录当作旧记录替代。
