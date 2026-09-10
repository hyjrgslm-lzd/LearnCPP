# P1：静态字段工具

先读[综合正文](../../chapters/16-static-record.md)，具备tuple、约束、调用表达式和C03 expected基础。本题核心C++23，反射是单独的C++26能力分支。

输入与元信息由`checks/record_schema.hpp`提供；学生只编辑`src/student/record_ops.hpp`中的`implementation`。完整答案为`src/reference/record_ops.hpp`及其`codec.hpp`；`validation/good`是独立保存的正确完成体，遍历使用index sequence；`validation/bad`实际漏写最后一字段。检查器是`checks/record_checks.hpp`，消费当前选中的实现，不调用另一个答案实现代替学生。

## Part 1：从已知类型开始

运行`P1_static_record_known_baseline`前预测Person的三个字段及顺序。它只处理Person，不声称能发现任意类型成员。

解析：字段列表是三个拥有名称和值的条目，`id=7`、`active=true`、`name=Ada`。改变Person输入后结果应随之改变。后续泛型化改变支持类型集合，不能据此声称它比固定类型函数更快。

## Part 2：保留真实字段表达式

实现`implementation::visit_fields(T&&, F&&)`，每次把`string_view`名称和真实成员表达式传给回调。已提供不同成员指针组成的tuple；需要自己展开、调用并推导约束与noexcept。

解析：`std::forward<T>(object).*descriptor.member`保留对象的cv/ref。回调参数本身使用具名左值，不能每个字段都`std::forward<F>`再消费一次同一函数对象。以void转换加逗号折叠规定顺序；空tuple不调用。异常终止后续字段访问，但不回滚外部回调已经发生的改变。正文说明回调不存储、返回借用仍受对象生命周期约束。

检查：成员地址、修改回写、const和右值字段、只允许左值调用的callback、不能接受所有字段的callback、空记录与第二次回调抛出。不要只打印字段名冒充访问正确。

## Part 3：字段编解码

实现`encode_fields(const T&)`和`decode_fields<T>(span<const FieldValue>)`。编码输出vector中每个名称和值都拥有内存。解码返回`expected<T, field_error>`，允许字段输入乱序；拒绝重复、未知、缺失和非法标量。用局部对象构造完整结果；异常传播时不留下对调用者已有对象的部分更新。

解析：int用`to_chars/from_chars`，检查范围与完整消费；bool只有true/false；string保持原始字节，包括NUL与换行。`from_chars`不跳空白，也不接受整数前导加号。本题不定义一个文本传输协议，名称/值是已经拆开的字符串条目，不能把std::quoted的显示输出当通用JSON。多个同时存在的输入错误不承诺优先级。

检查包含INT_MIN/INT_MAX、空串、尾随字符、越界、非法bool、名字映射、乱序、错误列表及空schema。错误是枚举，不借用已失效的输入字符串。分配异常仍可能抛出，expected只承载列出的校验错误。

## Part 4：显示与注册边界

实现`format_record`得到`record(id=7, active=true, name="Ada")`，字段顺序与访问顺序相同。字符串用`std::quoted`处理引号和反斜杠；这不是完整控制字符转义协议。

支持域为已登记的公开普通成员聚合，codec字段为int/bool/string。未知schema、unsupported指针字段、重复外部名称被约束拒绝。schema注册者须保证完整；C++23遍历器不能从手工tuple发现被遗漏的成员。`P1_static_record_schema_gap`安全观察这一边界，不能解读成支持域内Reference故障。

## Part 5：真实反射对照

开启frontier后，`P1_static_record_reflection`用真实meta查询、annotations和splicing运行同一`record_checks.hpp`。`src/reflection/record_ops.hpp`只把schema存在性用于限定教学域，实际字段发现、名称和访问均来自反射；codec复用Reference中与元信息获取无关的逻辑。

本机缺少meta或对应语言能力时，这一分支返回77，表示主体未编译/运行；不把C++23手工描述器的通过算到反射名下。annotations的字段排除另在F01格式化实验讨论，本题codec所有字段完整往返。

## 本机命令

在仓库根目录：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/P1_static_record -B C04_Generic_CompileTime_Reflection/exercises/P1_static_record/build/local -G "Visual Studio 18 2026" -A x64
cmake --build C04_Generic_CompileTime_Reflection/exercises/P1_static_record/build/local --config Debug
ctest --test-dir C04_Generic_CompileTime_Reflection/exercises/P1_static_record/build/local -C Debug --output-on-failure
```

自动验证使用[有界进程与证据方式](../../references/BUILD_GUIDE.md)。学生配置另加`-DGENERIC_STUDY_BUILD_REFERENCE=OFF -DGENERIC_STUDY_TEST_STUDENTS=ON`；初始Student有意失败，Reference/good应通过，bad被精确诊断拒绝。只有真实完成Student操作后，其结果才代表作业完成。
