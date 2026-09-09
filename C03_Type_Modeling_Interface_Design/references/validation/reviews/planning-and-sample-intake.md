# 计划与样章形成性审查

日期：2026-09-09。记录来自本轮非作者原生代理审查，供后续复验定位；原始私人对话不作为课程材料保存。

## 已批准计划

独立设计审查：APPROVE，要求锁定查询快照、值比较、事务边界及完整课程完成条件。独立 critic 在整合这些要求，并补齐C26 optional引用/范围及variant成员visit后返回APPROVE，无阻断。实施规格保存于 `../../implementation-spec.md`，规格与公共构建另经非作者只读审查，无设计阻断。上述批准不是课程最终验收。

## 公共构建初检

非作者核对Student/ref选项、helper实际路径、ASan DLL与当前编译器匹配、负例精确退出1及checker消息等接线。初次L06的独立helper smoke配置/构建成功，Reference、good、bad拒绝共3项通过，原记录为 `../sample-helper-configure-r1.json`、`../sample-helper-build-r1.json`、`../sample-helper-ctest-r1.json`。这是当时版本的有限运行证据，不能替代以下教学阻断修复后的复验。

## 形成性教学预审：ITERATE

审查期间作者仍在编辑，因此不是冻结终审。最后读取的06正文SHA为 `FCB975ACB1FEC036A3BD09082886DA036C04B6F8CE353BADF4BB114265902627`，checker SHA为 `211EA284284F36BE992A262B8D2F7ED37CBC7D2526751595CD6E216E624AB91D`，Student SHA为 `6C36E8ECF5DB7BA38AA4E6476E4964E8A9FC0B141FA8B9578C29A1A4857C5FCE`。这些旧指纹只标识反馈对象，不证明现在的文件已经修复。

发现与必须复验的项目：

1. 逐k失败检查编辑中出现未定义变量，部分函数仍固定k=2；要求fresh构建，并逐项证实完整prepare失败点。
2. Student的失败注入fixture是no-op，和“只实现Table”的作业边界不符；要求提供独立、完整、共享fixture，检查实际学生操作。
3. strong_prefix的提交阶段仍可能复制抛异常；要求整个提交使用已证明不抛出的操作，并用同输入失败点复验。
4. optional/variant/expected正文不足以支持承诺能力，且C++23引用optional限制被写成无版本的普遍断言；要求补足规则、正反例与C26边界。
5. good转引Reference不能作为独立正确完成体；要求独立实现和实际checker通过。

样章门保持关闭。原作者修复后，由非作者针对冻结文件及新证据复验；不能仅凭作者回复把本记录改写为通过。
