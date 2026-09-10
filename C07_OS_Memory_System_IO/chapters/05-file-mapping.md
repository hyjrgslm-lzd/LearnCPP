# 05 文件映射

文件映射把文件内容接到进程地址空间，但它不是普通读写的同义词。至少有三层对象：文件本身、平台 mapping object、映射 view。view 的地址可读，不表示文件 owner 可以随便提前销毁；owner 必须定义释放顺序。

偏移是最容易错的地方。Windows `MapViewOfFile` 的文件偏移要按 allocation granularity 对齐，Linux `mmap` 的 offset 要按 page size 对齐。用户想读 offset 3 时，实际 view 常从 offset 0 开始，返回给用户的 span 要跳过 delta。请求长度超过 EOF 时，只能返回实际存在的尾部字节；空文件没有可映射页面，底层 `map_readonly` 拒绝它。

P1 需要的最小只读入口：

```cpp
auto mapped = c07::map_readonly(path, offset, length).value();
std::span<const std::byte> bytes = mapped.bytes();
```

shared/private 是另一条语义轴。shared 写入可以通过普通文件 readback 看到；private 写入是 copy-on-write，不应改源文件。`FlushViewOfFile` 或 `msync(MS_SYNC)` 只能作为当前系统写回请求和 readback 观察，不能证明断电后数据、metadata 或设备 cache 已持久化。

本章练习是 [L04_mapping](../exercises/L04_mapping/README.md)。它用非对齐 offset、EOF、空文件、shared/private 写映射和只读源不变性检查 owner/view 设计。

练习接口分两层：窗口规划函数暴露 `aligned_offset/delta/mapped_length/visible_length`，真实 `map_window` 返回 move-only owner。checker 同时消费这两层，避免普通文件读取或固定字符串冒充映射实现。
