# P1：资源清单包的实际数据流

先读[16章](../../chapters/16-resource-manifest.md)，回查13—15章。本项目从类型化fixture构造资源清单，真实外部输入为配置和二进制解码，不新增JSON/TSV清单语法，不读写资源路径指向的文件。

实现位置：`src/student/pipeline.hpp`中`c05_ex::run_pipeline`。输入为Manifest、UTF8配置、调用者独占且已存在的临时目录；返回拥有型PipelineResult或DataError。`provided/contract.hpp`固定接口。`provided/operations.hpp`提供有界文件I/O、UTF8文件名转换和报告格式化；之前章节的config/manifest/time公共操作可以复用，学生要完成的是它们的真实组合和错误传播，不是重新实现之前所有算法。

| Part | 学生任务 | 完整解析与检查 |
|---|---|---|
| 1：准备阶段 | 解析配置、验证/编码Manifest，确认时间可展示 | 失败必须在创建输出前返回。默认zone为UTC，不依赖tzdb；配置错误/重复ID检查输出不存在 |
| 2：实际文件写入 | 以配置basename在给定目录下创建新包，禁止覆盖已有文件 | 使用`write_new`，它通过C++23 `ios::noreplace`拒绝既有路径，不是先exists再truncate；keeper fixture必须保持`keep` |
| 3：真实回读 | 调用有界read，再把真实文件字节交decoder | 不能直接返回输入对象或内存中刚encode的副本冒充磁盘回读；检查实际文件与独立v2黄金字节相等 |
| 4：报告与错误 | 用解码结果生成报告，逐层传播错误 | 拷贝输入、恒定报告、丢失空note都会被不同输入/独立结果检查拒绝；DataError不得借用临时配置字符串 |
| 5：责任边界 | 说明校验失败、I/O失败和目录清理责任 | 校验/展示错误不创建文件；写失败可能留下一份新部分文件，由独占scratch拥有者清理。本题不承诺文件事务或断电恢复 |

Reference为`src/reference/pipeline.hpp`，独立good在`validation/good/pipeline.hpp`，只共享已交付的低层操作，不调用Reference的pipeline。bad故意只给报告而不做磁盘I/O，通过`creates the package file`被拒绝。修改完成标记或引入另一份pipeline答案不构成作业实现。

checker先收集失败，清理自己新建的临时目录，然后才调用会exit的check。临时父目录先canonical并处理末尾空路径元素，避免Windows尾分隔符造成假不一致；清理失败优先于任何预期bad诊断，不能被包装器吞成通过。这一点对Student与bad也成立。它验证数据流及若干错误条件，不是防抄袭证明，也不能穷尽真实设备的故障方式。

```powershell
cmake -S P1_resource_manifest -B build/leaf-P1 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-P1 --config Release --parallel 2
ctest --test-dir build/leaf-P1 -C Release --output-on-failure
```

Debug同样运行。Student通过学生预设独立构建，初态必须失败；最终记录源码和命令对应版本。Windows之外的文件系统行为需在相应平台另验，不把本机成功当作发布级存储保证。
