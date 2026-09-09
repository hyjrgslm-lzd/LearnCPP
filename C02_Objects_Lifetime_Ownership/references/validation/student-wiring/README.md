# Student 接线检查的正反验证

这是验证工具自己的 fixture，不是 C02 学生作业。`good` 编译独立源码，`bad` 则实际包含 `reference/answer.hpp`；两者都能正常编译运行，因此仅看退出零无法识别答案依赖。

从 LearnCPP 根目录运行，RunName 必须全新：

```powershell
./Core_Study/references/validation/student-wiring/check_wiring.ps1 -RunName independent-001
```

脚本请求 CMake codemodel-v2，fresh 配置/重建具体 Student target，再把实际预处理 include 输出与目标依赖和本地源码接线合并检查。当前自检使用已验证的 VS18/MSVC；原始 `good-audit-r1.json` 因未识别本地中文 include 前缀而失败，修订后的 r2 已匹配真实中文/英文输出。旧失败保留，不能作为最新通过证据。

`audit_student.py` 要求 trace 对应相同 build/config，显式 `--clean-first --target` 重建全部被审 Student，并且确实出现 include 输出；空日志或 no-op build 不通过。检查器定位本课程声明的 Reference target、目录和 include，配合人工源代码审查，不声称能自动识别任意拷贝粘贴的答案算法。

接口字段依据 [CMake file-api codemodel v2](https://cmake.org/cmake/help/v4.2/manual/cmake-file-api.7.html)。实际执行为本机 CMake4.2.3；在线4.2手册可能显示后续补丁版，本工具只使用已在本机验证的 v2 target/source/include/dependency 字段。
