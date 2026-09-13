# L05：两行更新的事务实现

正文：[06](../../chapters/06-transactions-and-schema-changes.md)。已提供真实 SQLite 连接、accounts 表及检查器；学生编辑 student/solution.hpp 中的 move_units。

- Part 1：拒绝不合法ID、同一源目标、非正数量和越界数量；在一个事务里读取或条件更新。
- Part 2：源减少与目标增加必须整体提交；目标缺失或容量不足时保留失败前的两个行。
- Part 3：数据库触发器主动拒绝目标更新，保留原生错误并回滚先前修改，连接不能残留事务。
- Part 4：比较 Reference 的条件更新与 good 的事务内读取；解释为什么只检查退出码或行数不够。

Reference、good 与 bad 的源文件彼此独立。bad 的真实缺陷是先自动提交源修改，后发现目标不存在；检查器必须拒绝它。初始 Student 返回 unfinished，并明确失败。

```powershell
cmake -S C12_Serialization_Databases_Storage/exercises/L05_transactions -B build/c12-l05-student -DC12_ENABLE_SQLITE=ON "-DC12_SQLITE_ROOT=P:/C++Code/LearnCPP/build/c12/deps/sqlite-amalgamation-3530400" -DC12_BUILD_REFERENCE=OFF -DC12_TEST_STUDENTS=ON
cmake --build build/c12-l05-student --config Release
ctest --test-dir build/c12-l05-student -C Release --output-on-failure
```

完整解析：先建立事务，再进行影响决策的读取；只有两侧更新各命中合法行才提交。提前返回会触发守卫回滚，异常同样离开事务作用域。BEGIN IMMEDIATE 是本 SQLite 场景的写者接纳策略，不能直接套成 PostgreSQL 的隔离级别。回滚自身失败时公共连接冻结，应用必须重新建立可用连接，而不是继续下一次移动。
