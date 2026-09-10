# L03 虚拟内存

本练习验证“地址空间”和“对象”不是一回事。`virtual_region::reserve` 只取得一段地址范围；`commit` 后页面才可访问；`protect` 改访问权限；`decommit` 撤销已提交页面，但 owner 仍负责最终 release。

## Part 1：页面信息

实现 `make_region(page_count, pattern)`，先调用 `c07::query_page_info()`，确认 page size 非零。Windows 还会给 allocation granularity；本题只用 page size，因为匿名页操作以页面为粒度。`page_count == 0` 必须返回错误。

## Part 2：reserve 与 commit

保留两页地址空间。不要在 commit 前读写这段地址。commit 第一页后把首尾两个字节写成 `pattern`，再把 `virtual_region` move 返回给 checker。checker 会持有真实 owner，先用 `VirtualQuery` 或 `/proc/self/maps` 确认可读，再读真实字节。

## Part 3：protect、decommit 与错误路径

checker 把第一页改成只读并确认仍可读；不要故意写只读页。随后 decommit 该页。Windows 这里撤销 commit；Linux 实现用 `mprotect(PROT_NONE)` 撤销访问，不声称丢弃物理页或与 Windows commit 状态等价。越界范围和零长度范围必须得到错误。

## 解析

危险反例不能放进主 checker：写未提交页或只读页在真实系统上可能让进程收到访问违例/SIGSEGV，有限测试不能把崩溃形状当成标准结论。本题检查真实 owner、平台页状态和已提交写入的可见性。对象生命期仍由 C02 的 `construct_at` / `destroy_at` 规则决定，不能因为地址可写就把任意 `T*` 当作活跃对象。
