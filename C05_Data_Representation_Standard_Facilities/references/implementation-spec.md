# C05 实施规格

2026-09-10 已批准与C04联合增加fmt/spdlog前端教学，范围及门槛见[增量实施记录](../../C04_Generic_CompileTime_Reflection/references/revision-plan-20260910.md)。本页原规格记录前次基线，本次新增范围另行验证。

用户于本会话确认完整计划并要求 Implement the plan。基准 HEAD 为 f261bea559d6722c31135fb2d3589be52fe958ed；实施开始时 C04 与多课 README、根计划和指南存在他人改动，全部保留。当前指南允许合理内部耦合；不以解耦为独立目标。

## 范围与组织

新增本课 chapters/exercises/references 与入口，额外仅在根 README、LEARNCPP_GLOBAL_PLAN 7.2 及 C03/C09/C15 README 追加导航。指南、旧正文、旧算法、学习文件和共享工具不改。无需全局安装、机器配置、提交/推送、生产系统或凭据操作。ICU 官方源码在本课 ignored build/_deps 隔离准备；这是用户已批准的唯一新增第三方库。

完整计划已通过非作者 code-reviewer 修订复审及 critic 审查；原生 Plan 阶段没有文件写入，不伪造正式 OMX durable Ralplan gate。执行使用可用原生 executor 与非作者 reviewer/verifier，不创建 Codex goal 或 tmux team。

00路线；01字节/对象表示；02位操作/端序；03整数转换与数值边界；04字符串/借用；05Unicode编码；06严格转码；07文本语义/ICU；08解析；09格式化；10时钟/精度；11日历/时区；12路径；13配置；14字段；15版本；16资源清单包；17源码/下游；18前沿。15个专题练习，P1综合，U01 ICU，F01前沿。所有Part有完整解析，按观察或实现语义分别登记。样章先交01—06必要机制及L14有界UTF8字段：正确基线、独立bad复现、游标与长度推导、UTF校验、checker正反例、非作者审查修复复验；样章通过才批量展开后续正文。

## 核心契约

C++23，namespace c05。视图只在调用期间借用，结果拥有数据；expected<T,DataError>返回输入错误，分配异常可传播。不发布部分对象。DataError含类别、位置、Byte/Utf16CodeUnit单位、拥有型field及配置line，不能返回悬垂错误上下文。

Manifest记录：非零唯一u32 id、非空UTF8 name、非空UTF8 generic相对path、u64 byte_size、i64 UTC Unix毫秒modified_at；v2可选note。所有文本严格UTF8、拒绝NUL；note可空。path禁止根/盘符/反斜杠/空段/./..，只作元数据，不打开其资源目标；标识按字节比较，不自动规范化或casefold。最多1024记录、单字符串4096字节、包1MiB。timestamp为公历0001—9999的Unix毫秒范围[-62135596800000,253402300799999]；时区仅影响显示。

CHAR_BIT==8，显式大端：header="C05M"|major:u16|minor:u16|record_count:u32；record=body_length:u32|{tag:u16|length:u32|payload}*。tags1—5=id/name/path/size/mtime，tag6=note。writer只写(1,1)/(1,2)，v1有note拒绝。reader接受major1、minor>=1；未知major/minor0拒绝。minor仅增加可忽略字段，必需字段或语义变化须major++。未知字段有界跳过、重编码不保留；重复字段（包括未知）、缺必需字段、坏长度和尾随字节拒绝。tag递增写，record保留输入序。独立v1 reader仅理解1—5，v2理解1—6；测试双向兼容、future minor与有意信息丢失，不能只自往返。

配置：UTF8 key=value子集，LF/CRLF、首BOM、空行与整行#，首=分割、ASCII边缘trim；固定package_file默认manifest.c05m与display_zone默认UTC。package_file仅basename，不得根/盘符/分隔符/./..，其他原生非法文件名在IO边界拒绝；展示zone不改系统时区。未知/重复/空值/坏行给byte offset与line。总64KiB、行4KiB。通用UTF转换保留NUL，配置与manifest另行拒绝。P1由typed C++ fixture构造manifest，真实外部输入为配置与binary decoder；不新增manifest文本语法。仅自有临时目录读写元数据包。

## 构建与扩展

本机MSVC14.51/STL145 update202604、VS2026 v145/CMake4.2.3。脚本最低CMake3.28，VS2026预设要求4.2。复用C01 check/process_runner、C02 record_process/audit_student，参考C03轻量StudySetup。DATA_STUDY_BUILD_REFERENCE=ON，TEST_STUDENTS/ENABLE_ASAN/ENABLE_FRONTIER/ENABLE_ICU=OFF。Release/Debug/student(ref-off)/ASan/frontier/ICU Debug+Release，以及所有叶级独立配置构建。

ICU4C77.1固定SHA457157a92aa053e632cc7fcfd0e12f8a943b2d11、Unicode16.0；只在exercises/build/_deps/icu-77.1。原生FindICU EXACT REQUIRED uc/i18n/data，检查所有头/库resolve路径属于此前缀；DLL位于本课配置对应输出目录。使用已装MSBuild v145，Debug/Release x64，SkipUWP=true。打印核验u_getVersion/u_getUnicodeVersion。显式ON缺失/错版本/越界/运行失败均FAIL；默认OFF只是未启用。本轮U01须真实Debug/Release验证才完成。规范化四形式、casefold、root grapheme、UTF16与UTF8坐标，固定官方Unicode16测试数据/许可/指纹。非法输入先严格验证，不以ICU默认replacement冒充成功。

规范固定N4950/N5050/N5054和对应编辑报告。C26 text_encoding是识别非转码；format/path/runtime/to_string与charconv增量逐项记录。C29位移/置换与format相关增量按已入稿证据；P2728R14不冒充标准实现。头/宏/真实实例化/链接/运行分别记录。

## 验证、审查、停止

整数极值/转换/长度溢出、UTF非法类别/截断/NUL/BOM/代理项、charconv全消费、格式化边界；calendar范围/精度/时区歧义与不存在时刻，filesystem词法与身份，配置正反例；wire黄金字节、每截断点、记录/长度上限、未知重复缺失字段、v1/v2/future minor。checker good通过、bad精确拒绝；Student初态失败单独登记，Release检查有效，ref-off include+codemodel审计。普通CTest外部30秒，构建/记录有界；超时、启动或清理失败均FAIL。

默认不排名性能；若实际优化，先取得归因证据，再1次预热+5次独立进程并保存全部结果。最终fresh完整矩阵、叶级、核心脱离ICU，链接/覆盖/遮答案教学/技术与实验非作者审查、修复复验，绑定命令、环境、源码/输入指纹。允许真正缺失的tzdb/前沿能力单列SKIP；不得依赖其结果推进。已知阻断清零、全部正文和约定可用分支通过、ICU实测、独立终审完成才更新C05状态；其他平台未测不作PASS。
