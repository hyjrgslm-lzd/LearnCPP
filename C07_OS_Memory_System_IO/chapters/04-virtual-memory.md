# 04 虚拟内存

程序拿到的指针首先是虚拟地址。CPU 执行 load/store 时，MMU 按页表把虚拟页翻译成物理页或触发缺页；TLB 缓存最近翻译结果。操作系统可以让一段地址暂时没有物理页、只读、不可访问，或在 fork / copy-on-write 场景中延迟复制。C++ 对象模型只管“这段 storage 里是否有活跃对象”；它不负责让页存在。

Windows 把匿名虚拟内存拆成 reserve 和 commit。reserve 只占地址空间；commit 建立可访问页面并让系统承诺后备存储。`VirtualFree(..., MEM_DECOMMIT)` 撤销提交，`MEM_RELEASE` 释放整段 reservation。Linux 常用 `mmap(..., MAP_PRIVATE | MAP_ANONYMOUS)` 建立区域，再用 `mprotect` 控制访问；课程封装把二者暴露为同一组教学动作，但不声称平台语义完全相同。

最小代码入口：

```cpp
const auto page = c07::query_page_info().page_size;
auto region = c07::virtual_region::reserve(page * 2).value();
region.commit(0, page, c07::page_access::read_write).value();
auto bytes = region.bytes(0, page);
bytes[0] = std::byte{0x41};
region.protect(0, page, c07::page_access::read_only).value();
region.decommit(0, page).value();
```

这个例子证明的是页面状态变化，不证明 `bytes.data()` 处有某个 `T` 对象。若要放 `T`，仍需要 C02 的 `std::construct_at`。Linux 版本的 `decommit` 只是 `mprotect(PROT_NONE)`，用于撤销访问；它不声称丢弃页内容或拥有 Windows `MEM_DECOMMIT` 的提交状态。若要观察越权访问，必须在隔离子进程里做；主线 checker 只检查系统调用结果、页状态查询和安全读回。

本章练习是 [L03_virtual_memory](../exercises/L03_virtual_memory/README.md)。它要求学生返回真实 `virtual_region` owner，由 checker 持有并覆盖 page size、reserve、commit、protect、decommit 和无效范围拒绝。
