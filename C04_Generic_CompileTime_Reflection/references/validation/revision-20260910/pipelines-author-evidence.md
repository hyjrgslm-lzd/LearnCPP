# A04/A05 类型管线与表达式模板验证证据

日期：2026-09-10。原始逐命令 JSON 保存在 [`pipelines-author-records`](pipelines-author-records/)。构建目录保留在各练习的 `build/revision-pipelines-*` 下，未删除 raw/build。

## 最终有效 JSON

源码与独立性：

- [`00-source-head.json`](pipelines-author-records/00-source-head.json)：记录当前 Git `HEAD`。
- [`01b-good-hashes.json`](pipelines-author-records/01b-good-hashes.json)：记录 A04/A05 good 实现 SHA256。
- [`21-final-source-hashes.json`](pipelines-author-records/21-final-source-hashes.json)：记录正文、Student、Reference、good 最终 SHA256。
- [`22-good-no-reference-include.json`](pipelines-author-records/22-good-no-reference-include.json)：`rg` 反向检查，预期 exit 1，表示 good 未引用 Reference/答案路径。

A04：

- [`02-a04-configure-debug.json`](pipelines-author-records/02-a04-configure-debug.json)：Reference/good/bad 全量配置。
- [`03-a04-build-debug.json`](pipelines-author-records/03-a04-build-debug.json)：Debug 构建。
- [`04-a04-ctest-debug.json`](pipelines-author-records/04-a04-ctest-debug.json)：Debug CTest，7/7 passed。
- [`05-a04-build-release.json`](pipelines-author-records/05-a04-build-release.json)：Release 构建。
- [`06-a04-ctest-release.json`](pipelines-author-records/06-a04-ctest-release.json)：Release CTest，7/7 passed。
- [`07-a04-student-configure.json`](pipelines-author-records/07-a04-student-configure.json)：student-only 配置。
- [`08b-a04-student-build.json`](pipelines-author-records/08b-a04-student-build.json)：Student Debug 构建。
- [`09b-a04-student-ctest-expected-fail.json`](pipelines-author-records/09b-a04-student-ctest-expected-fail.json)：Student 初态 CTest，预期 exit 8 且包含 `check failed`。

A05：

- [`10-a05-configure-debug.json`](pipelines-author-records/10-a05-configure-debug.json)：Reference/good/bad 全量配置。
- [`11-a05-build-debug.json`](pipelines-author-records/11-a05-build-debug.json)：Debug 构建。
- [`12-a05-ctest-debug.json`](pipelines-author-records/12-a05-ctest-debug.json)：Debug CTest，6/6 passed。
- [`13-a05-build-release.json`](pipelines-author-records/13-a05-build-release.json)：Release 构建。
- [`14-a05-ctest-release.json`](pipelines-author-records/14-a05-ctest-release.json)：Release CTest，6/6 passed。
- [`15-a05-student-configure.json`](pipelines-author-records/15-a05-student-configure.json)：student-only 配置。
- [`16-a05-student-build.json`](pipelines-author-records/16-a05-student-build.json)：Student Debug 构建。
- [`17-a05-student-ctest-expected-fail.json`](pipelines-author-records/17-a05-student-ctest-expected-fail.json)：Student 初态 CTest，预期 exit 8 且包含 `check failed`。
- [`18-a05-asan-configure.json`](pipelines-author-records/18-a05-asan-configure.json)：ASan Debug 配置。
- [`19-a05-asan-build-debug.json`](pipelines-author-records/19-a05-asan-build-debug.json)：ASan Debug 构建。
- [`20-a05-asan-ctest-debug.json`](pipelines-author-records/20-a05-asan-ctest-debug.json)：ASan Debug CTest，6/6 passed。

## 摘要结果

A04 Debug：

```text
100% tests passed, 0 tests failed out of 7
Label Time Summary:
diagnostic     =   8.91 sec*proc (3 tests)
negative       =   0.45 sec*proc (1 test)
observation    =   0.02 sec*proc (1 test)
reference      =   0.43 sec*proc (1 test)
validation     =   0.45 sec*proc (1 test)
```

A05 Debug：

```text
100% tests passed, 0 tests failed out of 6
Label Time Summary:
diagnostic     =   2.89 sec*proc (1 test)
negative       =   0.07 sec*proc (1 test)
observation    =   0.03 sec*proc (2 tests)
reference      =   0.02 sec*proc (1 test)
validation     =   0.01 sec*proc (1 test)
```

A05 ASan Debug：`20-a05-asan-ctest-debug.json` 记录 6/6 passed，覆盖嵌套临时生命周期与别名写回路径。

## 失败记录与替代

以下 JSON 保留为原始过程记录，不作为最终通过证据：

- [`01-good-hashes.json`](pipelines-author-records/01-good-hashes.json)：recorder 子进程内 `Get-FileHash` 不可用；已用 Python `hashlib` 的 [`01b-good-hashes.json`](pipelines-author-records/01b-good-hashes.json) 替代。
- [`08-a04-student-build.json`](pipelines-author-records/08-a04-student-build.json)：修正前 Student 头文件缺少可实例化占位实现，不能构建；已改为可构建初态，并由 [`08b-a04-student-build.json`](pipelines-author-records/08b-a04-student-build.json)、[`09b-a04-student-ctest-expected-fail.json`](pipelines-author-records/09b-a04-student-ctest-expected-fail.json) 替代。

## 关键修正记录

- A04 `zip` 对不等长输入需要在偏特化入口约束，否则 MSVC 会在 concept 查询中实例化到未定义尾部并产生硬错误。
- A04 `IntOnly` 不是不可调用反例，因为 `double` 可隐式转 `int`；已替换为 `PointerOnly`。
- A04 `transformable` 改为单步 concept 折叠，避免总别名查询误判。
- A04 Student 改为可构建 starter，并在运行时检查中明确失败。
- A05 bad 改为完整表达式实现，仅保留直接写回别名缺陷，确保 negative checker 拒绝的是目标缺陷。
- A05 诊断 pattern 改为稳定 ASCII 错误码，避免本机中文诊断文本影响匹配。
