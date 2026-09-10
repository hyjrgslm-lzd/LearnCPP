# L04 文件映射

本练习把 file、mapping object 和 view 分开。`c07::map_readonly(path, offset, length)` 返回 move-only owner；owner 析构时解除 view，再释放平台对象。`bytes()` 返回只读 `span<const std::byte>`，它只在 owner 活着时有效。

## Part 1：只读窗口

实现 `plan_readonly_window(file_size, offset, length, granularity)` 和 `map_window(path, offset, length)`。offset 不一定按平台粒度对齐，正确规划必须向下对齐实际映射起点，记录 delta、native mapped length 和 caller-visible length。checker 会消费规划字段，并持有真实 `readonly_mapping` 读取字节。

## Part 2：EOF 和空输入

请求长度超过 EOF 时，返回从 offset 到 EOF 的实际字节数。空文件和 offset 位于 EOF 或之后时返回错误。P1 综合项目会在调用映射前处理空文件成功分支，本题故意让底层映射拒绝空 view。

## Part 3：shared/private

实现 `write_first_byte(path, mode, value)`，在独立临时文件上分别建立 shared 和 private 写映射。shared 写入后通过普通文件读取能看到新字节；private 写入后源文件仍保持原字节。这个 readback 只证明当前系统的可见性路径，不证明断电后持久。

## 解析

映射不是把整个文件复制进内存。view 背后仍有页、保护位、文件偏移和平台对象寿命。关闭 fd/HANDLE 不等于 view 立刻失效，但 owner 设计要让这些事实有单一释放顺序。学生代码若用普通 `ifstream` 截字符串、只返回 bool 或忽略 offset 对齐，checker 会在规划字段、真实 owner 字节和 shared/private 文件状态处拒绝。
