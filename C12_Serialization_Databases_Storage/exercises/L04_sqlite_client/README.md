# L04：真实 SQLite 客户端与资源责任

正文：[05](../../chapters/05-sql-and-sqlite-clients.md)。本单元为完整观察和故障实验。配置时显式提供 C12_ENABLE_SQLITE=ON 与固定 C12_SQLITE_ROOT，单题和整课均可构建。

| Part | 已提供的实验与学生任务 | Reference 和判定 |
|---|---|---|
| 1 绑定 | 预测带 SQL 语法的字符串、NULL、空文本和空 BLOB 如何存储 | main.cpp 查询实际类型与值；SQL模板可信，外部值只绑定 |
| 2 生命周期 | 将语句移出 database 变量作用域，解释 shared owner；复制列值后继续 step | 实际 SELECT42、列副本及 moved-from 拒绝检查 |
| 3 事务清理 | 解释应用异常与数据库错误；再观察析构/显式回滚的真实 OOM | rollback_fault.cpp，cleanup_error保留且所有持有者冻结 |
| 4 准备边界 | 预测被尾部检查拒绝的 PRAGMA 是否可能已生效 | main.cpp 实测 foreign_keys，再恢复设置 |
| 5 备份 | 比较成功备份与源写事务中的 BUSY，解释失败目标残留 | 新路径要求、恢复计数、integrity_check及同路径重试拒绝 |

Reference 为 main.cpp、rollback_fault.cpp 和公共 sqlite.hpp。故障分配器只在独立测试进程生效，结束时恢复；不修改机器分配器配置。程序通过仅说明已执行的检查成立，逐 Part 的预测与解释仍需完成。

```powershell
ctest --test-dir build/c12/sqlite -C Release -V -R "^C12_L04"
```

解析：SQL NULL 需独立表示，SQLITE_TRANSIENT解除输入字符串的后续存活责任；拥有型列副本解除语句后续状态的影响。连接互斥不替代事务级串行化。ROLLBACK失败必须广播到共享状态，不能只丢弃析构返回码。备份失败保留的自有目标不等于成功备份，只有完整协议和恢复检查通过才能发布。
