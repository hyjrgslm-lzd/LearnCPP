# 源码导读 03：oneTBB 哈希表如何把查找结果变成受保护的访问

队列交出一个值之后，消费者通常拥有自己的副本；哈希表却经常返回仍在容器内的元素。若查找返回后立即释放桶锁，另一个线程 erase 该节点，先前返回的引用由谁保护？本篇用 oneTBB `concurrent_hash_map` 回答这个问题，再追扩容为什么不必一次搬完整张表。

只读核查日期为 2026-09-08。固定 oneTBB `v2022.2.0`，commit [`06ce6212da6710f4bb2d20a1904b018aa44069bf`](https://github.com/uxlfoundation/oneTBB/tree/06ce6212da6710f4bb2d20a1904b018aa44069bf)，主文件为 [include/oneapi/tbb/concurrent_hash_map.h](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h)。仓库由原 oneapi-src 地址重定向到 uxlfoundation；以完整 SHA 和文件内容为准。本篇未构建、运行该库。

范围限定：默认 `spin_rw_mutex` 分支、普通整数 key/value、并发 find/insert/erase。用户 hash/equality、分配器、preview 自定义 mutex 与并发遍历会扩大契约，不能把这里的一条路径当作全部模板参数的证明。教学对照是 [共享状态与锁](../../chapters/04-shared-state-and-locks.md)、[RAII 与对象](../../chapters/00-execution-and-objects.md)和 [回收先修](../reclamation/01-lifetime-and-ownership.md)。课程没有手写同等功能的生产哈希表。

## 1. 两种锁保护不同东西

先读字段，而不是立即钻模板重载。`hash_map_node_base` 有 `next` 和节点 mutex；bucket 有自己的 mutex 与原子的 `node_list`；表层有分段指针数组 `my_table`、掩码 `my_mask` 和大小 `my_size`。bucket 锁保护链表结构及查找时的节点可达性，node 锁保护一次持续的元素访问及 erase 与访问者的交接。[字段入口](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L50)

`const_accessor` 私有继承节点 scoped lock，并保存 `my_node`、`my_hash`；`accessor` 在此基础上提供可修改 value 的引用。release 会释放节点锁并清空 my_node，析构由 scoped lock 清理。accessor 不是一个任意长期保存的裸指针，也不是 shared_ptr：它的存在保留了这个库协议中的访问资格。[accessor 实现](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L775)

因此，操作形状应该理解为“取得 accessor → 在其保护期间访问 value → release/析构”，不能把 `&accessor->second` 保存到外部，在 accessor 结束以后仍假定 erase 会等待自己。数据结构线程安全也不表示同一个 accessor 对象能由多个线程无同步并发操纵。

## 2. find 的成功路径与重试出口

从 `find(accessor&, key)` 到 `lookup<false>`：公开入口先释放 result 原有保护，lookup 计算 hash，读取当前 mask，取得对应 bucket_accessor；bucket 确认完成必要重哈希之后，`search_bucket` 才沿节点链查找。[公开 find](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L1114)

找到 n 后，代码在桶仍受保护时尝试取得 n 的节点锁，再退出桶锁作用域，把 n 和 hash 填入 result。这个先后关系防止“找到裸节点、桶锁已经释放、还没开始保活节点”的空隙。

但是它也不无限占着桶锁等某个长期 accessor：`try_acquire` 加有限 backoff 后仍未成功，就释放 bucket，yield、重读 mask 并从 restart 开始。旧 n 不再作为一个已经安全的结果继续使用。这样是在缩小锁之间的等待耦合，不是宣称 find 无阻塞或 wait-free。[lookup 关键路径](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L1277)

未找到节点时，还不能立刻返回 false。若扩容改变了 mask，该 key 对应的节点可能已经迁到新桶；`check_mask_race` 和 `check_rehashing_collision` 判断是否需要按新掩码重试。这和队列读一个原子 head 后再访问 next 一样：一个局部观察需要和结构变化协议配套，不能仅因为该字段是 atomic 就忽略拓扑变化。

## 3. insert 先准备节点，只有真正入链才交出所有权

`insert` 也进入 lookup，但模板参数是 true。未找到 key 时，它可能分配并构造一个暂存节点 `tmp_n`，然后把桶锁从读升级为写。升级可能释放并重新取得锁，期间另一个插入者已经完成同 key 插入，因此代码必须再 search。这个分支正常返回“已存在”，并在出口 `delete_node(tmp_n)` 清理未采用的候选。

真正新增节点经 `insert_new_node` 和 `add_to_bucket` 入链，随后 tmp_n 设为空，表示容器接管节点。`create_node` 用临时 RAII guard 清理构造未完成的节点存储；`delete_node` 依次销毁 value、node 并调用分配器释放。分配、value 构造和 hash/equality 的异常能力必须另查，不能把“有一个 guard”理解为所有操作都强异常安全。[节点构造与删除](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L658)

特别留意后置扩容：insert_new_node 先增加 size 并入链，再通过 segment 指针上的 CAS 认领扩容责任，lookup 的后段才调用 enable_segment。于是如果在这条具体路径中，后续段分配抛出，不能未经核查就断言“异常说明插入完全没有发生”。阅读任务要把“候选构造失败”“竞争者抢先插入”“节点入链后的段分配失败”分成三条出口。[入链与扩容认领](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L287)

## 4. 分段扩容与按桶迁移

段数组把桶存储拆成按规模增长的段。`enable_segment` 分配并初始化一个新段，release 发布段指针，随后 release 更新 my_mask。未完成段分配时不会先发布一个要求访问不存在桶的新 mask；认领中的段使用特殊标记与真实指针区别。段分配异常路径把认领位置恢复为 nullptr，使以后的操作仍有机会扩容。[enable_segment](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L180)

新段桶初始带 `rehash_req_flag`，所以扩大 mask 不代表元素立即全数迁移。某次 bucket_accessor 遇到这个标记，会取得相应写权限并调用 `rehash_bucket`：根据父 mask 找旧桶，沿旧链重新计算节点的归属，将属于新桶的节点从旧链摘下，链接到新桶。节点本体没有被复制成另一个地址；改变的是桶链归属。[按桶迁移](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L709)

如果为迁移升级旧桶锁时失去了连续持锁资格，代码回到 restart 重新走链，因为并发 erase 可能已经令旧 curr 无效。这解释了为何源码里反复出现重查，而不能把它们删成“我刚刚已经找过”的优化。

此设计把扩容和搬迁工作分摊到后续桶访问，可能降低一次全表迁移的集中代价，同时让普通 find 也承担慢路径和锁竞争。这里只提出源码支持的成本来源；没有测量不能许诺某种尾延迟或吞吐优势。

## 5. erase 如何从“不可再找到”走到“可以释放”

沿 `erase(key)` 到 `internal_erase`：它取得 bucket，搜索节点，必要时升级为桶写锁并重新搜索；未找到且没有 mask race 时返回 false。找到后先从桶链摘下节点并减少 my_size，然后结束桶锁作用域。

这时新查找无法再沿桶链取得该节点，但以前返回的 accessor 可能仍持有节点锁。erase 接着取得该节点的写锁，等已有访问者离开；这个短作用域结束后，才 `delete_node`。这是“摘除”和“回收”两个边界，即使没有 hazard pointer，生命周期义务也没有消失。[internal_erase](https://github.com/uxlfoundation/oneTBB/blob/06ce6212da6710f4bb2d20a1904b018aa44069bf/include/oneapi/tbb/concurrent_hash_map.h#L1429)

为什么不需要在这里给节点加 HP？在所追的路径中，查找持桶保护取得节点锁，删除持桶写锁摘除，再等待节点访问锁；可达性与访问资格的锁协议封闭了那个空隙。代价是长期 accessor 会延迟 erase，甚至由用户锁顺序造成死锁。`erase(accessor&)` 则是另一条 exclude 路径，需要处理现有 accessor 的锁升级与 release，不能把 erase(key) 的步骤逐字替换过去。

最后追容器析构：`~concurrent_hash_map` 调 clear，遍历剩余节点调用 delete_node，并删除段。这里的普通清场遍历不是与仍运行的 find/erase 并发的回收协议。使用者必须先停止访问并释放 accessor，才能销毁整个表；不要把正常并发 erase 的安全性推广到并发析构。

## 6. 和教学版本的契约对照

| 问题 | 教学中已经建立的原则 | 本次 oneTBB 路径的落实 |
|---|---|---|
| 返回引用的寿命 | 借用必须覆盖最后访问 | accessor 持节点访问锁；release 后外部裸引用不再受保护 |
| 结构修改与数据访问 | 保护整个复合操作 | 桶锁管链，节点锁管持续访问；两层有明确交接顺序 |
| 扩容 | 不能先发布未初始化存储 | 段发布先于新 mask；新桶按需迁移并检查 mask race |
| 回收 | 摘除不等于可以 delete | internal_erase 先摘链，再等节点写锁，最后删除 |
| 失败 | 区分返回失败、抛异常与部分改变 | key 不存在/已存在、候选构造失败、后置扩容失败分别追踪 |
| 进展 | 包含用户代码和分配 | 默认读写自旋锁、重试、allocator 与 hash 均需计入完整操作 |

它是并发哈希表方案族中的“分段桶表＋桶/节点锁”分支，不是开地址无锁哈希表，也不是课程中新增了一份生产实现。若将来换成 `concurrent_unordered_map`，还要重新检查 erase 的并发许可；不能仅凭库名相同就套用本篇。

## 7. 可复验阅读任务与完整答案

用已有固定检出或网页全文搜索。PowerShell 中 `$src` 指向该 oneTBB 仓库：

```powershell
git -C $src rev-parse HEAD
rg -n 'class const_accessor|bool find|bool lookup|insert_new_node|enable_segment|rehash_bucket|internal_erase|delete_node' "$src/include/oneapi/tbb/concurrent_hash_map.h"
rg -n 'check_mask_race|upgrade_to_writer|goto restart|tmp_n = nullptr' "$src/include/oneapi/tbb/concurrent_hash_map.h"
```

**任务 A：找出 find 成功时桶锁与节点锁的重叠区间。** 答案：lookup 的 bucket_accessor 作用域内，先 search，再 result->try_acquire(n->mutex, write)；成功取得节点锁后才离开桶锁作用域。之后填 my_node/my_hash，返回 accessor。删掉重叠区间会留下 erase 能释放 n 的窗口。

**任务 B：两个线程插入同一个 key，候选节点为何不会都进入表？** 答案：桶锁升级失去连续持锁时必须重查；胜者在写锁内入链，败者走 exists，并在正常出口删除自己没有采用的 tmp_n。不能用“第一次 search 都不存在”作为插入授权。

**任务 C：扩大 mask 后为什么旧桶没找到还要再查？** 答案：该 hash 可能因新增高位映射到新桶，节点可能已迁移；check_mask_race/check_rehashing_collision 检查这个条件并 restart。新段分配与节点搬迁不是同一动作。

**任务 D：R 持 accessor，W erase 同 key，什么先发生？** 答案：W 可以把节点从桶链摘下，使新的 find 不再获得它；W 最终释放存储前还需取得节点写锁，因此 R 的既有访问尚未结束时不能完成删除。R 自己也应及时 release，避免长期阻塞 W。

**任务 E：哪条异常路径不能草率写成“表未改变”？** 答案：沿 insert_new_node → tmp_n=nullptr → check_growth → enable_segment，节点入链早于后置段分配。这个路径与 create_node 的未发布候选构造失败不同。复验时标出相应行，明确只说明这个源码顺序，不替所有重载编造统一异常保证。

阅读交付应包含三张小图：节点访问资格、段发布与桶迁移、erase 到最终释放。没有运行库的记录就只标源码核查和推导，不附虚构 benchmark 或生产稳定性结论。
