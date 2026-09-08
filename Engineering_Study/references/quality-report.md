# C01 质量与验证报告

本批交付11章连续正文、13个练习单元及构建、诊断、模块和包消费实验。范围遵循[实施规格](implementation-spec.md)；[覆盖表](coverage.md)连接正文与作业，[文件清单](delivery-manifest.md)列出持久产物。本报告区分批次验证与最终整合门，不能仅凭目录或CTest绿灯判定学习完成。

## 本轮状态

| 批次 | 当前状态 | 证据/下一步 |
|---|---|---|
| 规划 | 非作者 architect、critic APPROVE | implementation-spec保存范围和验收，不能代替实施审查 |
| C1 ODR样章 | 非作者 APPROVE，样章门通过 | [独立复验](validation/odr-independent-review.md)：7个非student检查、有效学生完成体、4类坏变体；[被审源码指纹](validation/odr/review-fix3c-source-fingerprints.sha256) |
| F-I工具能力准备 | 直接MSVC与正常CMake/Ninja的模块/import std/模块包消费均取得通过证据；Clang诊断可运行 | [探测报告](validation/toolchain-probe/summary.md)与400整链记录；这些是能力准备，不等于正式课单元验收 |
| A/B/C2 | 非作者APPROVE；Debug/Release、预处理与符号、学生正反变体已复验 | [基础独立报告](validation/foundations-independent-review.md) |
| D/E/J | 非作者APPROVE；Debug/Release分别D5/5、E5/5、J1/1，真实C、opaque handle和重定位包消费 | [二进制/包独立报告](validation/binary-package-independent-review.md) |
| F-I | 代码/正文/检查链非作者APPROVE；最终README与补强检查另审 | [工具链独立报告](validation/toolchain-independent-review.md)、[原始命令汇总](validation/toolchain/summary.md) |
| 核心整合 | fresh VS Release34/34、Debug34/34 PASS | [第一轮记录](validation/integration/final-matrix-r1/report.json)，该轮模块失败仍保持FAIL |
| Modules/import std整合 | fresh Ninja分别39/39、40/40 PASS | [第二轮记录](validation/integration/final-matrix-r2/report.json) |
| C2重复运行 | 已存在负例对象的Ninja树中，C1/C2符号检查连续两次通过 | [JUnit](validation/integration/c2-repeat-final.xml) |
| 最终完整矩阵 | fresh四配置共147项PASS：核心Release34、Debug34、Modules39、importstd40；零失败/零禁用 | [r3完整命令与结果](validation/integration/final-matrix-r3/report.json)及同目录四份JUnit；随后唯一G1检查头变更另有下面定点复验 |
| G1/F2最终完成门 | 非作者APPROVE：F2有效调用通过/复制公式被spy拒绝；G1 Debug/Release各good、constant42、special42、wrong_exception全部符合预期，坏例为正常exit1诊断 | [最终教学报告](validation/final-teaching-review.md)、[581公开结果](validation/toolchain/581-verify-g1-contract-controlled-failure.stdout.txt)；G1最后检查头另经[Debug](validation/integration/g1-final-Debug.xml)/[Release](validation/integration/g1-final-Release.xml)重编译与Reference验证 |
| G2正式测量 | 72/72轮PASS，12预热+60正式；12组各5样本，全部原始值保留，测量前后fixture指纹一致 | [结果与解释](validation/measurements/README.md)、[原始JSON](validation/measurements/g2-formal-r1/report.json)；前期1+1仅为driver预检 |
| 最终快照与非作者终审 | 本表记录实测事实；最终批准、发现和关闭依据由两份非作者报告给出 | [教学审查](validation/final-teaching-review.md)、[技术与数据审查](validation/final-technical-review.md)、[最终源码指纹](validation/snapshots/delivery/source.sha256)、[最终证据指纹](validation/snapshots/delivery/evidence.sha256) |

## 证据边界

Windows是本次实测环境；其他平台完整代码/命令与讲解保留，未运行不写PASS。本轮工具版本只作为环境快照，执行时重新读取实际程序身份与选项。共享preset不含本机安装路径。

实测Windows11 x64，CMake4.2.3、VS2026/MSVC14.51.36231（编译器19.51）、VS Ninja1.13.2、Clang22.1.3、Python3.13.11。调试另用本机MinGW GCC13.2/GDB14.2，实际断点、调用栈、局部变量有运行记录。未安装新工具、修改全局环境或添加CI。

受限自动化进程中的Ninja ABI探测曾超时；获准的正常本机进程成功完成默认可执行程序ABI探测，最终矩阵没有强制COMPILER_WORKS或以STATIC_LIBRARY绕过。旧失败记录保留，未断言底层OS根因。公共check在Debug/Release均为stderr诊断+正常exit1，正反自检均通过，避免未捕获异常触发CRT对话框。

诊断证据包含ASan安全正例和独立heap-buffer-overflow故障、真实parser libFuzzer及坏parser的专属G1_ORACLE_MISMATCH判定、静态分析与编译trace。fuzzer和ASan分开验证；组合配置曾遇MSVC STL annotation链接问题，不以单独成功冒充组合成功。LLDB未修复或安装，调试要求通过已有GDB完成。

J1错误target对照使用不存在的LessonPackage::lesson，说明消费方请求了错误名称，不是删除合法已安装target；实际导出lesson_static/lesson_shared。Windows包复制到prefix_B后独立消费，不依赖原源码/build目录。没有运行跨CRT释放等真实UB来证明ABI兼容。

## 审查修复与快照

分批审查保持作者/非作者分离。历史报告及指纹绑定当时快照；后续公共helper、检查器与导航改动进入最终整合审查，不能沿用旧指纹宣称新源码获批。

已关闭的主要发现：ODR逐object证据缺失、重复/缺失符号混淆；B1不同宏外部inline违反ODR和数字散落匹配可误过；A1恒真配置检查；Debug失败弹框；G1未完成Student越界、fuzzer未连接真实parser、任意异常退出被误判成功；G2共享构建产物不公平与计时/子集汇总问题；H1未导出类型可达性表述；聚合Ninja下符号脚本的build根目录错误。

最终教学审查另发现G1仅特判42、F2复制provider公式仍能通过Student。现已统一G1完整parser契约并增加F2独立spy provider；原special42坏变体进一步暴露valid输入抛异常未受控，已在同一契约检查头中将非预期异常转为check诊断。非作者581复验Debug/Release的有效实现及三类坏变体，G1/F2阻断全部关闭。最终技术审查同时核验C2范围修复、完整矩阵、正式数据和冻结指纹，结论以报告末次签署为准。

最终快照由公开freeze_snapshot.py生成，排除build产物、用户原有guide/P2、审查报告自身和旧指纹，避免循环自哈希。分批旧指纹是历史审查输入，最终交付以delivery目录为准；其后仅独立报告签署，不改课程源码。跨课导航检查40份文档零缺失链接，待交付可见文件未混入编译产物；用户guide与P2 SHA256保持会话输入值。

构建成本结果只适用于当前五TU、本机和计时协议，不宣称PCH/LTO普遍加速或运行期收益；不修改系统缓存策略或后台服务。性能采样期间冻结被测源码，保留全部样本和无收益结果。

本批未改变Concurrency算法或重采其性能数据；C08/C09定点补修有独立报告，不外推为所有平台、ASan或C++29新正文完成。Windows之外保留可执行材料但标未验证；import std依赖固定CMake4.2.3官方gate，更换工具组合须重核，不承诺BMI跨编译器稳定。
