# C1_7 完成通道中的错误恢复

本题实际使用固定stdexec的`then`、`upon_error`、`let_error`，通过`sync_wait`观察最终通道。编辑src/student/solution.hpp；main.cpp为所有实现共用检查器，src/reference为答案，validation/good采用独立stream解析和不同的sender构造，bad演示错误恢复策略。

## 输入与结果

输入为`非空name,int,finite-double`，必须恰好三个字段，数值完整消费，不接受残留字符、前置加号/空白或非有限浮点值。年龄在int可表示范围内；本题不附加年龄业务规则。结果只保留名字/成功状态和教学错误码，数值字段用于产生有意义的解析错误。这是局部通道实验，不冒充P1的记录格式。

## Part与解析

1. 正常路径：`just(owned_text) | then(parse)`。just构造不会解析；connect/start之后then调用parse。返回parse_result进入set_value；解析抛异常由then转换为set_error(exception_ptr)。
2. 值恢复：接`upon_error`，回调返回`{false,"BadRequest",-1}`这个普通值。它只拦截到达该节点的error。若回调自身抛异常，新error继续下传，不反复递归调用自身。
3. sender恢复：接`let_error`，回调返回`just({false,"RecoveredBySender",-2})`。算法须连接并启动新sender，并维持其operation state到完成；返回sender不是把sender对象当业务结果。
4. 恢复之后再加一个抛`after recovery`的then，验证下游新错误不会倒流给上游恢复节点。sync_wait重新抛出该异常，是阻塞消费者的映射；图内部仍是error通道。
5. 用多组名字/数值、空数据、缺字段、数值尾随垃圾、越界整数和nan/inf验证完整消费。不能只识别alice测试样本；新增合法名字必须同样处理。

完整机制讲解见[完成通道](../../chapters/04-channels.md)。实现这些使用型组合不需要提前重写库里的adaptor；G1之后再深入包装receiver和子operation state。

## 构建与判定

按[构建指南](../BUILD_GUIDE.md)设置STDEXEC_ROOT后，单独配置本目录或root选择C1_7_error_channel。构建reference/validation_good/validation_bad并运行CTest；Student原名目标构建其_student实现，初态必须UNFINISHED/exit2。Reference OFF+STUDENT_ROOT注入good仍须通过。bad必须在`let_error returns fallback sender`处返回1，不能拿编译失败或崩溃当作正确拒绝。
