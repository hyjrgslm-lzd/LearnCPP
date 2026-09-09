# 项目 P1：`object_buffer<T>`

先阅读 [15 object_buffer](../../chapters/15-object-buffer.md)，并回看 11-14 章。

你只编辑 `src/student/object_buffer.hpp`。实现 `p1::object_buffer<T>`，公开接口固定为：

```cpp
object_buffer();
object_buffer(object_buffer&&) noexcept;
object_buffer& operator=(object_buffer&&) noexcept;
std::size_t size() const noexcept;
std::size_t capacity() const noexcept;
void reserve(std::size_t);
void push_back(T value);
void pop_back();
void clear() noexcept;
std::span<const T> view() const noexcept;
```

禁止复制容器，不新增 mutable view、iterator、自定义 allocator 或完整 vector API。

## Part 1：建立存储不变量

`std::allocator<T>` 只分配存储。只有 `[0, size())` 是活跃对象，`[size(), capacity())` 不能放进 `view()`。`clear()` 销毁对象但保留容量；析构销毁活跃对象并释放存储。

解析：这是第 12 章存储和对象生命期的直接应用。capacity 是槽数，不是对象数。

## Part 2：实现 `reserve`

无增长时无操作。增长时先分配新存储，再迁移旧元素；若 `T` nothrow move constructible，使用 move，否则 copy。全部成功后才提交新指针和容量。

失败解析：copy 第 k 个旧元素失败时，只销毁新存储中已成功构造的前缀，然后释放新存储。旧元素、旧 capacity 和旧借用都保持原样。不能先提交 `capacity_`。

## Part 3：实现 `push_back(T value)`

按值参数已在函数体前构造，它的外部副作用不在回滚范围。无扩容时先构造尾元素，成功后才增加 size。

扩容时先分配新存储，再先构造新尾。新尾构造失败时，旧元素还没有被 move/copy，所以只释放新存储。新尾成功后再迁移旧元素；迁移失败时销毁已成功的新前缀和新尾。

解析：这个顺序专门防止“先 reserve 提交 capacity，再构造新尾失败导致状态改变”的错误。

## Part 4：借用与移动

`view()` 不保活容器。未扩容追加保留旧元素地址，但旧 span 长度不变。扩容、clear、pop 对应地结束对象生命期。

move 构造把 allocation 转交给新 owner，源变空，旧借用随新 owner 存活。move 赋值先销毁目标旧数据，再接管源；self move 无损。

## Part 5：验证

```powershell
cmake -S C02_Objects_Lifetime_Ownership/exercises/P1_object_buffer -B build/c02-p1 -G "Visual Studio 18 2026" -A x64 -DCORE_STUDY_TEST_STUDENTS=ON
cmake --build build/c02-p1 --config Debug
ctest --test-dir build/c02-p1 -C Debug --output-on-failure
```

checker 覆盖：空/边界/growth、自身 const view 元素追加、无增长 reserve、copy 第 k 次失败、新尾失败、迁移失败、over-aligned、nontrivial、nothrow move-only、move 转交借用、move assignment 目标旧借用失效、self move、throwing-move-only 编译拒绝。

公开验证变体：

- `validation/good` 应通过同一 checker。
- `validation/bad_noop` 应被拒绝，证明 checker 不只看程序退出。
- `validation/bad_early_commit` 应在新尾失败时被拒绝，证明 checker 能抓早提交 capacity。
