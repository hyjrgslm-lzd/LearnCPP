# P1：可复制文档的类型与事务接口

先读[第16章](../../chapters/16-document-model-and-transactions.md)，并完成不变量、状态/错误与[异常事务样章](../L06_transactions/README.md)。题目不涉及渲染、文件格式或线程。给定的`checks/document_types.hpp`提供真实公共类型与输入校验；只编辑`src/student/document.hpp`。独立答案在`src/reference`，另一种正确完成体在`validation/good`。

## 固定接口与契约

`ElementId`非零，`PositiveLength`为正，非法原始数值由工厂返回`ValueError`。`Element`有ID、`variant<Rectangle,Circle>`和可选文本label；label缺失是合法业务状态。尺寸只是数据，不计算可能溢出的面积。文档空状态合法，元素ID唯一，顺序为插入顺序。

```cpp
Document();
Document(const Document&);
Document& operator=(const Document&);
Document(Document&&) noexcept;
Document& operator=(Document&&) noexcept;
std::size_t size() const noexcept;
std::optional<Element> find(ElementId) const;
std::vector<Element> snapshot() const;
std::expected<void, EditError> apply(const Edit&);
std::expected<void, EditError> apply_batch(std::span<const Edit>);
void swap(Document&) noexcept;
bool operator==(const Document&) const;
```

find与snapshot返回独立值，可能在复制时抛异常，不返回保活承诺不清的内部借用。文档相等按有序元素值比较。复制后改其中一份不影响另一份；移动后源为空且仍可复用，自复制/自移动保留原值。

Add重复ID拒绝，Replace/Erase缺失ID拒绝，错误携带相应ID。Replace保留位置，Erase保留剩余顺序。批内操作顺序可见，但整个批只提交一次：后一项失败必须撤销前面暂存的所有效果。空批成功。业务拒绝返回expected错误；分配或复制异常传播，同时保持调用前文档。不承诺回滚调用前的实参求值和调用者其他副作用。

## Part与解析

| Part | 学生实现与检查 | 完整解析 |
|---|---|---|
| 1 值与查询 | 建立空文档，实际Add，再查询、取snapshot；修改返回值不改变文档 | optional拥有查到的Element副本；vector快照拥有自己的元素。文档不可公开可写vector，否则唯一ID不变量绕过编辑接口。 |
| 2 单条命令 | 验证重复/缺失及保序，返回准确错误 | 新增查重；替换定位后保留原位置；删除仅移除该ID。错误类型是可分类值，不把分配异常压成missing_id。 |
| 3 批事务 | 批内先Replace/Add、再失败Erase，要求原文档完全不变；测试批内Add后Replace/Erase | 候选状态必须承接前一条临时结果；循环一半直接改live文档只有basic保证，不满足本题strong。把单条编辑内部函数限制在候选状态上。 |
| 4 值的转交 | 复制、非空目标赋值、自赋值、移动后源复用、相等关系 | copy assignment先构造完整副本再swap；move转交存储并明确源状态。地址不同不是复制正确性的充分条件，实际修改副本再检查原值。 |
| 5 失败路径 | 普通分配点测量后逐点注入，检查原值与失败后复用；copy assignment失败保持目标 | 先建立正确成功基线，按当前实现实测分配次数运行实验，不把某个次数当接口契约。所有准备工作均在候选对象内，异常展开析构候选；最终swap不分配。 |

Reference用`visit`＋候选Document，good用显式`get_if`分支＋候选vector。checker通过公开操作检验值与错误，不要求采用其中某一种写法。bad只保留“提前提交批内操作”这一错误，必须命中`failed batch preserves the entire original document`。Student初始虽然返回success，但没有插入元素，checker会以`Add inserts an actual element`拒绝，不能靠返回成功或完成标记通过。

## 本机命令

从本目录执行：

```powershell
cmake -S . -B build/local -G "Visual Studio 18 2026" -A x64
cmake --build build/local --config Debug
ctest --test-dir build/local -C Debug --output-on-failure
cmake --build build/local --config Release
ctest --test-dir build/local -C Release --output-on-failure
```

学生独立构建另用`-DTYPE_STUDY_BUILD_REFERENCE=OFF -DTYPE_STUDY_TEST_STUDENTS=ON`，不会生成allocation答案目标。未完成Student应明确失败，观察成功、Reference通过与学生完成含义不同。

## 分配实验与Debug边界

普通Reference/good/Student目标保留默认MSVC Debug迭代器检查，未替换全局new。`P1_document_allocation_reference`与`P1_document_allocation_good`是独立实验：仅它们替换普通new/delete，并将MSVC迭代器调试设为0。这样故障注入针对可恢复的文档分配路径，不误伤标准库noexcept移动中的Debug代理分配。两者使用同一份对应实现源码，不是删掉失败测试。

这一区分来自 Debug allocator 实验的边界：注入只覆盖该输入实际经过的普通分配，未证明 aligned allocation、线程并发、真实系统 OOM 或所有库内部失败路径。失败时禁用注入后才检查 snapshot，避免检查器自己的分配混入被测窗口。ASan 无报告也不构成一般正确性证明。
