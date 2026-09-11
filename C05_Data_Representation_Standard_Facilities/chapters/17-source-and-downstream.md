# 17：沿真实实现回查规则，再把数据边界交给下游

源码阅读的目标不是记下私有函数名，而是验证一个具体问题：输入从哪里进入，状态在哪里改变，哪些出口表示失败，什么条件允许交付结果。本章固定MSVC STL提交`4edbc1d63a1ec156bed6dbf1727f413fa682abba`。安装版头文件另有指纹；不能把固定上游源码等同本机二进制，也不能因两个文件相似就推断所有实现一致。

本章的基础规则已在对应主讲章节展开，这里串联阅读任务和下游迁移，不让一个GitHub链接代替必要讲解。私有名字只用于这个固定版本定位，不作为用户应调用的接口。

## 1. charconv：发生溢出后为什么仍继续扫描

入口是[charconv](https://github.com/microsoft/STL/blob/4edbc1d63a1ec156bed6dbf1727f413fa682abba/stl/inc/charconv#L269)的整数from_chars实现，再沿公开重载的转发进入内部循环。先标出三个独立变量：下一字符位置、累积无符号值、是否发生溢出。符号与基数决定可接受数值的边界，而不是先在signed类型中乘加到溢出再检测。

实现用商和余数把“下一步乘以base再加digit是否超界”转换成执行前比较。对于signed负数，它允许绝对值到达最小值对应的边界；不能简单用正最大值作所有符号的限制。数字超界后设置状态，但继续推进输入位置，因为返回指针仍要指向匹配的数字序列之后。

沿出口检查：没有数字时返回invalid_argument，溢出返回result_out_of_range，只有成功出口才把累积结果写回调用者目标。这个顺序说明两个重要事实：失败不是“返回零”，而且失败时目标保持原值；返回ptr和返回ec各有独立信息。

回到L08，用`123tail`观察“成功解析数字前缀”和“协议要求完整消费”的差别；用一个超出uint32范围且末尾带字母的串同时观察ec和ptr。不要因为库已经检查溢出，就省略业务范围或完整消费检查。

**解析题：为什么超界后不马上return？** 因为接口还承诺描述匹配区间的终点。停止更新有效数值和停止识别数字不是同一步。**为什么不能自己用signed乘加模拟这个循环？** 中间表达式若先发生signed overflow，后面的检测已经失去合法推理基础；第三章的先检查再运算正是这里的先修。

## 2. format：检查格式文本不等于拥有全部参数

从[format](https://github.com/microsoft/STL/blob/4edbc1d63a1ec156bed6dbf1727f413fa682abba/stl/inc/format#L2934)的basic_format_string开始。这个版本的构造在常量求值中建立参数类型信息并检查格式语法；保存的格式文本是视图。随后沿make_format_args进入参数存储，再看format_to/vformat_to以及返回string的vformat。

要分清两个方向。checked format string约束的是格式说明与参数类型如何匹配；参数存储决定的是随后格式化时从哪里取得值、引用或句柄。编译期格式检查没有给任意业务对象延长生命周期。把格式参数存储留到对象销毁后再使用，不能因格式字符串合法而变成安全操作。

本版本make_format_args接收lvalue引用，能挡住一类直接传临时对象的错误，但“某个调用表达式被拒绝”和“所有延迟使用都安全”仍然不同。阅读失败出口时区分编译诊断与运行时format_error：固定字面量错格式可以在构建阶段失败，来自运行时的格式文本经vformat则需处理运行期错误。

返回string的路径构造输出容器并通过输出迭代器格式化；它不是一个无分配保证。format_to交给调用者输出位置，也不自动为任意裸缓冲提供容量检查。format_to_n限制写入数量时还应理解其返回计数含义，不能把缓冲里已有的字节误认为完整显示文本。

**解析题：把std::format换成vformat就能缓存临时参数吗？** 不能。它改变格式文本的检查路径，不改变对象存活责任。**C++26 runtime_format提供了什么？** 它提供显式的运行时格式入口；是否可用、与这个固定版本的区别在18章独立核对，本版本不存在的入口不能凭名字补出实现。

## 3. chrono：同一个本地时间为什么有多个出口

进入[chrono的time_zone::to_sys](https://github.com/microsoft/STL/blob/4edbc1d63a1ec156bed6dbf1727f413fa682abba/stl/inc/chrono#L1829)，先看没有choose参数的重载。它取得local_info，分别处理nonexistent和ambiguous，再在唯一映射时减去offset得到sys_time。由此可见：减offset只是最后一步；本地日历时间未必先天对应唯一instant。

再看带choose的重载。歧义时它在两个offset中选择；不存在的本地时间则映射到转换边界，两种choose都不凭空制造一段从未发生的本地时刻。不能把“不带choose时抛异常”的描述推广到所有重载，也不能把earliest/latest简单理解成对不存在时刻也必然给两个结果。

从公开locate_zone继续向下，会进入实际tzdb查询，而不是仅靠字符串运算完成转换。数据库取得失败、zone名称不存在、当地时间歧义和格式化错误属于不同失败原因。L11先在独立能力入口确认数据库可用，再执行固定2024年转换案例；如果把所有异常catch后return77，就会把真正逻辑失败也伪装成环境缺失。

**解析题：把local_time中的duration直接塞进sys_time是否可行？** 类型都能承载duration不意味着纪元意义一样；这种做法跳过了时区映射。**为何包只保存UTC毫秒？** 它固定instant表示，将显示策略和动态时区规则留在外层；这不等于UTC clock与Unix system time在闰秒处理上完全相同，第11章仍需分开说明。

## 4. filesystem：同样的字节序列，入口类型会改变转换

从[filesystem的输入转换](https://github.com/microsoft/STL/blob/4edbc1d63a1ec156bed6dbf1727f413fa682abba/stl/inc/filesystem#L193)对照string_view与basic_string_view<char8_t>分支。普通窄输入采用实现选择的原生窄代码页路径；char8_t输入走UTF8转换。出口处的[u8string](https://github.com/microsoft/STL/blob/4edbc1d63a1ec156bed6dbf1727f413fa682abba/stl/inc/filesystem#L940)把本版本Windows原生宽表示转换为UTF8输出。

这些分支解释了为什么P1显式构造u8string再交path，而不把所有std::string都默认当UTF8。源码内部通过char读取char8_t对象表示，是允许的字节观察方向；不能反过来推导“把任意char缓冲强转char8_t指针读取也总合法”。第四章的类型与别名规则仍然适用。

接下来分别看generic表示和路径元素迭代。分隔符调整不等于访问磁盘后的canonical身份，字符串相等不等于文件相同，大小写折叠也不是所有平台统一的身份策略。固定MSVC源码描述的是这个Windows实现；Linux的原生路径字节和不同文件系统的约束需要实际环境分别验证。

**解析题：u8string往返成功证明文件存在吗？** 只证明所运行的表示转换，没有执行存在性或身份检查。**为什么不读path formatter实现？** 这个固定提交尚无对应标准formatter，18章保留独立能力探测；不存在的入口不是“源码导读已覆盖”。

## 5. 回到已有课程和未来应用

本章原本固定 MSVC STL 作为标准库源码入口。2026-09-10 的跨课增量另加两类生产库源码阅读：[第19章 fmt](19-fmt-library.md)使用 fmt 12.1.0 commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`，看格式串检查、编译期格式表示和 type-erased 参数；[第20章 spdlog](20-spdlog-frontend.md)使用 spdlog v1.17.0 commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`，看同步 logger 前端、宏裁剪、运行时过滤、pattern formatter 和 error_handler。它们是 C04 编译期机制在 C05 输出边界上的应用样本，不把第三方接口说成标准接口，也不把日志异步系统提前纳入本课完成声明。

| 下游入口 | 本课必须交付的先修 | 下游仍承担的责任 |
|---|---|---|
| C03 Document模型 | 从外部文本/字节到合法整数、编码、错误位置 | 类型不变量、批更新与异常安全；C05不改原Document实现 |
| C09 RPC项目 | 完整消费、长度头、上限、payload与业务含义分层 | 实际连接、分帧、取消、调度、异步缓冲存活 |
| C11网络主课 | 有界解码、端序、版本错误和资源预算 | TCP字节流、部分到达、背压及安全关闭，不能把本课同步span解码当完整网络运行时 |
| C12存储主课 | wire与对象布局分离、schema演进、unknown字段信息损失 | 格式成本、持久化、事务、索引、WAL及恢复 |
| C15 FString/FArchive | 编码/字符串身份、字段表示、版本演进边界 | UE版本专有TCHAR/对象/反射/资产协议；通用Unicode知识不证明某个UE平台布局 |

现有C09 RPC正文给出十进制长度头、请求字段、from_chars与完整消费检查。读者应能解释“参数x不是整数”“合法数字前缀后还有垃圾”“超过消息上限”分别在哪层拒绝；本课提供这些语言和数据前提，但不改变旧RPC代码。

C15中的FString、FName、FText各有身份、显示和本地化责任，不能因为都能输出文本便互换。本课也不宣称标准静态反射自动替代FArchive/UHT。阅读任何UE具体编码或布局描述时仍须回到其固定版本实现，不把本课的Windows观察套到所有UE平台。

## 6. 结课反向检查

遮住Reference，解释P1每次跨边界用了什么已讲规则：config_text借用多久，为什么返回错误拥有field，UTF校验在哪层，name的相等策略是什么，长度为何先比较再分配，Unix毫秒为何不等于本地时间，旧读者为什么可以丢note，输出既有文件为什么不能覆盖。

如果必须猜某条规则，返回其主讲章节补齐推导和正反例；不要用“去看上游源码”替代课程应交付的解释。源码版本、运行输入、实际flags、验证与审查记录共同界定本课结论，文件数量与单次PASS不能代替这条链。
