# 05. 从关系模型进入 SQLite 客户端

本章不假设读者已经学过 SQL。先修是 C03 的错误与异常安全、C02 的资源生命周期，以及 C05 的文本和整数边界。[L04](../exercises/L04_sqlite_client/README.md)直接编译固定 SQLite 3.53.4，课堂包装器只承担连接、语句和事务资源责任，SQL 和引擎行为仍由真实 SQLite 执行。

## 表达关系，而不是保存一个 C++ 对象指针

表是一组有名字和约束的列，行通过主键获得稳定身份。下面的表把身份、文本、可缺失说明和二进制数据分别建模：

```sql
CREATE TABLE items(
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  note TEXT,
  payload BLOB NOT NULL
) STRICT;
```

PRIMARY KEY 用于唯一识别行，NOT NULL 拒绝缺失值，CHECK 约束进一步限制有效域。外键表达两张表间的引用关系；SQLite 的 foreign_keys 是连接设置，应明确开启并核对。STRICT 收紧存储类约束，但不替代路径、UTF-8、资源大小或应用不变量的验证。

SELECT 指定需要的列，WHERE 过滤行，ORDER BY 明确顺序。没有 ORDER BY 的结果不能当作稳定排序。JOIN 根据键连接关系，GROUP BY 把行分组后计算聚合；建立索引不应改变这些查询的结果契约。练习里的 count(*) 只是观察方法，不能用总数替代逐行值和约束检查。

## NULL 不是空字符串，也不是零

NULL 表示值缺失。SQL 的比较会出现 unknown，因此用 IS NULL 检查缺失，而不是 note=NULL。空字符串仍然是一个存在的 TEXT 值；零长度 BLOB 也是存在的 BLOB。业务若需要区分它们，客户端必须保留这种区别。

SQLite 的绑定接口还有一个具体边界：空缓冲若传入空指针，可能被当成 SQL NULL。课堂 bind_blob 为零长度 BLOB 提供非空哨兵地址；bind_text 同样使用非空的空字符串地址。L04 同时插入 NULL、空文本和空 BLOB，再用类型与内容检查辨别，防止“字节数都是零”掩盖语义损失。

## 准备、绑定、执行和读行

可信 SQL 模板与外部数据分离：

```cpp
auto insert = db.prepare("INSERT INTO items VALUES(?1,?2,?3,?4)");
insert.bind(1, std::int64_t{1});
insert.bind(2, external_name);
insert.bind_null(3);
insert.bind_blob(4, payload);
insert.step();
```

?1 等占位符代表值，不能拿来绑定表名、列名或 SQL 语法。需要动态选择标识符时，应从程序定义的允许集合选择，而不是拼接外部字符串。绑定解决 SQL 与数据的分离，编码正确性和业务约束仍由外层承担。

课堂使用 SQLITE_TRANSIENT，让 SQLite 在绑定调用返回前取得值的副本。不能把临时 string 的 data 以 SQLITE_STATIC 交给未来才执行的语句。这里的数据边界与网络异步缓冲的存活责任相通，但具体协议不同。

step 返回 true 表示当前有一行，false 表示 SQLITE_DONE；其他结果抛出保留原生结果码的 db_error。读取列必须处于有效行，并检查下标和存储类。课堂 integer 不把 TEXT 或 REAL 悄悄转换成整数，text/blob 返回拥有型副本，避免下一次 step/reset/finalize 使借用值失效。

## 拒绝多条语句不是 SQL 沙箱

prepare 只接受程序作者提供的可信模板。尾部检查拒绝第二条语句，是对调用错误的检测，不是接受任意用户 SQL 的安全边界。某些 PRAGMA 在准备阶段就会生效，等检查 tail 时可能已经改变连接设置。

L04 实际准备 `PRAGMA foreign_keys=OFF; SELECT 1`，观察到包装器拒绝多语句，但 foreign_keys 已经变成 0，随后恢复为 ON。这个受控反例说明：不能将“最终抛了异常”推断成“此前没有任何效果”。[SQLite PRAGMA 文档](https://www.sqlite.org/pragma.html)明确区分准备时和执行时发生的行为。

exec 允许可信的多语句设置脚本；多个 SQL 出现在同一字符串里不等于这些操作具有共同事务。下一章会把完整业务动作放到明确的事务边界中。

## 连接与语句是谁的资源

database 独占一个逻辑使用入口；statement 持有同一连接状态的共享所有权，保证语句比外层 database 变量活得更久时，底层连接仍存在。语句先 finalize，最后一个持有者再关闭连接。L04 将语句移出创建连接的作用域，实际执行 SELECT 42，验证这个生命周期。

所有操作仍须由同一线程使用或由调用者外部串行化。SQLite FULLMUTEX 不会自动把跨多次 API 调用的业务事务变成一个不可插入的动作。

移动后的语句不能继续产生计数或读行。status 检查对象状态和受支持的计数器编号；误用不能以“返回 0”伪装成性能观测。返回的 raw handle 只用于同步短借用，不应保存后绕过包装器的失效状态。

## 析构清理也会失败

事务析构不能抛异常，但这不代表 ROLLBACK 必然成功。L04 的独立进程替换 SQLite malloc，并在析构前制造真实分配失败；原生连接可能仍处于事务中。

共享 connection_state 因此保存 cleanup_error。回滚失败时，database 和已经存在的 statement 都拒绝继续执行、读取或报告计数，并保留原始错误，要求释放相关持有者后重新打开。显式 rollback 可以立即报告失败；析构路径则通过冻结状态留下可检查结果。不能让下一次业务请求意外接在失败事务后面。

fault 测试同时验证“原生事务仍未结束”和“所有受支持的包装器调用已冻结”。诊断中短借用的 raw handle 仅用来证明故障，不能变成应用绕过冻结的捷径。

## 备份是一个需要验收的操作

直接复制正在使用的数据库主文件，可能遗漏 WAL 中的提交。L04 使用 SQLite online backup API，分批复制页面，检查 step 和 finish 的结果，再打开目标并执行 integrity_check。

接口要求调用者独占一个新的目标路径。已有目标会被拒绝；这不是跨进程抢占条件下的原子创建承诺。失败时保留未完成目标供诊断，不把它发布为成功备份。练习在源写事务中触发 BUSY，观察目标文件残留；同路径重试得到 CANTOPEN，调用者需要处理自己拥有的失败产物或选择新路径。

## 自测解析

- **INSERT 返回 SQLITE_DONE 就已经持久提交了吗？** 显式事务中的语句完成不等于 COMMIT；持久性还取决于日志和同步配置。
- **复制列值后能否销毁语句？** 拥有型副本可以；直接使用 sqlite3_column_text 返回的地址则受语句状态约束。
- **回滚失败后只返回一个错误够不够？** 不够。其他持有者也可能继续操作同一连接，必须在共享责任点冻结。
- **备份文件存在是否表示备份成功？** 不表示。必须完成 API 协议及恢复检查；失败残留与可用备份分别处理。
