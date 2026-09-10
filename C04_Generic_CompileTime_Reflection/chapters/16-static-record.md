# 16. 将泛型机制用于字段访问、格式化和序列化

先修：[推导与转发](03-forwarding-ctad.md)、[约束](06-constraints.md)、[tuple遍历](09-tuple-traversal.md)、[CPO样章](11-customization-points.md)，以及C03的expected与异常安全。真实反射分支再读[反射模型](13-reflection-model.md)与[splicing](14-splicing-generation.md)。本章的可运行核心不要求编译器已经支持反射。

场景是一组小型内存记录。工具需要按定义顺序访问字段、提供可读显示，并在“字段名称与字符串值”的拥有型列表之间往返。先把契约限制清楚：没有网络、文件协议和数据库；只处理已登记的公开普通成员聚合，编解码字段是int、bool、string。库式扩展必须遵守元信息完整性要求，不把简单教学实现包装成任意C++对象的通用序列化器。

## 已知类型的正确入口

对于`Person { int id; bool active; string name; }`，可以直接写：

```cpp
encoded_fields encode_known_person(const Person& p) {
    return {{"id", std::to_string(p.id)},
            {"active", p.active ? "true" : "false"},
            {"name", p.name}};
}
```

这里每个FieldValue都拥有名称和值。结果不是string_view集合，因此不依赖Person继续存活。对已知类型和列出的字段，这就是正确基线。[练习](../exercises/P1_static_record/README.md)中的known_baseline实际核对条目与顺序；第一次构建先验证它，再验证泛型候选。

增加第二种记录时，可以继续写另一个函数，也可以把“字段描述”和“遍历字段后的操作”分开。本课选择后者，是因为它能把模板参数推导、异构类型、调用表达式与反射连接起来；这是支持域扩展，不是已有测量证明的性能优化。

## 成员指针保留了什么信息

`int Person::*`不是`int*`。前者说明如何在某个Person对象中指定一个int成员，后者指向已经存在的具体int对象。成员指针本身不拥有Person，也不是标准承诺的字节偏移。对象通过`object.*pointer`产生真实成员表达式，不能先强转成整数偏移再拼地址。

描述器`field<Owner, Member>`保存一个名称和`Member Owner::*`。三个字段类型不同，描述器也不同，因此使用tuple而非把成员指针强转进同一vector。`schema<Person>::fields`只提供数据，没有遍历或解析算法。学生拥有自己的operations实现，不能因为元信息已提供就把调用Reference当成作业。

手工schema的注册是显式扩展点：注册者保证成员完整、名称正确、对象属于约定结构。实现能检查已列条目的名称唯一性、成员所属类型和codec类型，但C++23没有标准能力从这个tuple发现一个根本没登记的成员。[manual_schema_gap观察](../exercises/P1_static_record/observations/manual_schema_gap.cpp)故意违反完整性前提：shown保留，omitted丢失。它证明的是元信息前提的边界，不是说明编译通过可以免除语义责任。

## 从对象表达式推导遍历契约

关键调用不是“得到一个字段类型”，而是：

```cpp
std::invoke(function, descriptor.name,
            std::forward<T>(object).*descriptor.member);
```

传入Person左值时，T推导为Person&，forward后仍是左值，int成员表达式为int&。传入const Person左值时，成员为const引用；传入Person右值时，普通成员是相应右值表达式。这里不复制整个Person。回调可以选择复制或移动某个字段，那是回调自己的行为。

F虽然也以转发引用接收，循环内部的function却作为具名左值反复调用。若每次forward<F>，同一个右值函数对象可能被反复按`operator()&&`消费。检查器提供只允许`operator()&`的函数对象，证明本实现没有做这种重复消费。

遍历合法性必须对每个实际成员表达式成立。只接受int&的回调不能因为第一个字段是int就通过整个Person约束。`std::invocable<F&, string_view, field_result<T,I>>`对每个I成立才允许调用；空tuple的全称条件自然为真，不需要回调真的可调用。

返回void的访问器并不表示异常规格可以随便写。所有底层调用均nothrow时，遍历才nothrow。`std::get`、成员访问和本例中名称读取没有额外用户代码；真正的可抛点来自回调。Reference据同一组表达式推导约束和noexcept，避免“探测的是A，执行的是B”。

Reference使用std::apply展开描述器，独立good完成体使用index_sequence。每次调用先转void，再作逗号折叠，固定从左到右的访问顺序，并排除回调返回对象重载逗号的干扰。

## 异常与借用不因泛型化消失

访问器不存储object或function。调用期间的同步借用由调用者保证；把临时Person的字段地址保存到外面，仍会在完整表达式结束后悬垂。保留cv/ref只能保留语义，不能延长生命期。

假设第二个字段回调抛出，第三个不会被访问；第一个回调已经写出的日志或修改不回滚。检查器直接记录调用次数并受控抛出，不依赖真实资源耗尽。codec解码的保证不同：它构造自己的局部T对象，成功后才把值交给调用者，因此校验错误不会使调用者原有对象部分改变。两种保证分别属于不同操作，不能只写一句“强异常安全”覆盖整个工具。

## 编码与解码各负责什么

编码遍历const对象，将每个名称复制为string，将字段转换为拥有型string值。int使用to_chars，bool输出true/false，string保持原始字节。没有指向原始Person的悬垂借用，分配失败会正常抛出并销毁局部vector。

解码按schema顺序找到输入中的对应名称，因而输入列表可以乱序。每个名称恰好出现一次；缺失、重复和未知名称拒绝。整数必须是from_chars能够完整消费且不越界的十进制串，`7x`、空串、前导空白、前导加号均不接受。bool只接收true或false。string没有自行解释转义，NUL和换行都是字段值里的字节。

返回类型为`expected<T, field_error>`。枚举错误不会借用临时输入的名称；分配异常仍经异常通道传播。expected不是“整个函数绝不抛”的承诺。输入含多个错误时不承诺错误优先级，测试应验证拒绝与声明的单故障分类，而非依赖未承诺的扫描细节。

这个解码器对每个字段扫描输入，复杂度O(字段数×输入条目数)。小固定schema下不增加索引；若未来扩大到大量字段，先测量，再讨论索引的分配和维护成本。本课不提供未测量的加速数字。

格式化产生`record(id=7, active=true, name="Ada")`。std::quoted处理字符串引号和反斜杠，但不是完整JSON转义。可读输出和序列化字段列表是不同接口，不能用打印看起来像JSON来宣称兼容某个协议。

## 真实反射分支的替换边界

`src/reflection/record_ops.hpp`与手工描述器使用同一codec、同一检查器。替换的是字段发现、名称获取及成员访问；它们来自meta查询、annotations和splicing。schema是否存在只用来限定相同教学域，不从手工tuple取字段或制造假反射。

反射枚举到一个成员后，先检查查询前提，再获取名称和类型。Renamed的成员拼写是code，外部名称是id；手工路径在描述器中写id，反射路径读取external_name annotation。重复外部名称仍不允许。annotations的格式化排除实验放在F01，核心codec不靠默认值推断被排除成员，从而保持完整往返契约。

编译器必须同时提供meta、反射、annotations以及展开等所用能力。缺少基础能力时，capability驱动返回77，并明确主体没有编译和运行；手工路径通过不会加到反射通过数。若编译器宣称所需能力而真正源码失败，则是FAIL，不能继续吞为SKIP。当前本机验证边界与实际结果见最终质量报告。

## 自测与解析

1. 为什么不能把不同成员指针转为void*统一保存？成员指针不是对象指针，表示与布局不由这种转换保证；tuple保留类型信息才能让编译器检查真实成员表达式。
2. 为什么对象要forward而回调不反复forward？对象值类别决定成员表达式，回调是同一同步操作中重复使用的函数对象；两者承担不同责任。
3. 编译通过能否证明schema没有漏字段？不能。受控遗漏实验说明手工元信息需要语义前提；真实反射有不同信息来源，但本机未跑通前不能宣称实测修复。
4. 为什么解码失败不修改旧Person，而回调失败可能有副作用？前者只写局部候选，后者执行调用者的操作；所有权和提交边界不同。
5. 为什么改成员名或外部字段名可能破坏数据兼容？字段名字是输入识别契约；反射自动发现当前名字不等于自动维护旧版本映射。C05/C12继续主讲协议和schema演进。

完成[逐Part练习与检查](../exercises/P1_static_record/README.md)，再到[源码与下游回访](17-source-and-bridges.md)检查这些机制是否可迁移。
