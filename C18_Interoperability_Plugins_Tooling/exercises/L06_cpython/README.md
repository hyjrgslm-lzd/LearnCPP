# L06 CPython refs and errors

本题写一个 CPython Full C API 扩展模块。目标是让引用账本和 error indicator 对齐，而不是只让模块能 import。

Student 编辑文件：`student/solution.cpp`。骨架导出同名函数，但全部抛 `NotImplementedError`。

## Part 1：自定义 owner 类型

实现 `make_owner(value)` 返回扩展类型实例。对象内部保存 `PyObject* value`：

- 创建时用 `Py_NewRef(value)` 取得强引用。
- 类型必须参与 CPython GC：设置 `Py_TPFLAGS_HAVE_GC`、`tp_traverse`、`tp_clear`，用 `PyObject_GC_New/Track/UnTrack/Del`。
- `tp_clear` 先把字段置空再 `DECREF`，避免释放过程重入时再次看见旧指针。
- `live_owners()` 返回当前活跃 owner 数。

checker 会创建 owner、调用 `owner.value()`、删除 owner，并要求计数回到创建前。随后再构造 `list -> owner -> list` 循环，`gc.collect()` 后计数也必须回到创建前。只靠 refcount 的 `tp_dealloc` 处理不了这个循环。

## Part 2：borrowed -> new

实现 `borrowed_as_new(list, index)`：

```cpp
PyObject* item = PyList_GetItem(list, index); // borrowed
if (!item) return nullptr;
return Py_NewRef(item);
```

返回给 Python 的必须是 new reference。否则 list 被修改或调用者释放结果时，引用账本会错。

## Part 3：stolen reference

实现 `tuple_steals(left, right)`。`PyUnicode_FromString` 返回 new reference，`PyTuple_SetItem(tuple, i, item)` 成功后 steals 这个 reference。交出去后不要再 `DECREF` item。失败路径按已经获得的对象逐个释放。

## Part 4：error indicator

实现 `clear_error_then_return()`。它先设置一个临时异常，再 `PyErr_Clear()`，最后返回 `"ok"`。这证明你理解：非 `NULL` 返回前 error indicator 必须为空。

bad 版本故意设置异常后返回对象。CPython 把它变成 `SystemError`；checker 输出 `check failed: error indicator leaked through a non-null return`。

## Reference / good / bad

- `reference/solution.cpp`：完整答案。
- `validation/good/solution.cpp`：独立好实现，同样接入 GC。
- `validation/bad/solution.cpp`：error indicator 泄漏。
- `checks.py`：动态导入 `.pyd`，检查 owner 计数、cycle GC、borrowed 升级、stolen 交接和错误状态。

## 构建与运行

Windows Release：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L06_cpython -B C18_Interoperability_Plugins_Tooling/build/l06 -G "Visual Studio 18 2026" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release -DC18_ENABLE_PYTHON=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/l06 --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l06 -C Release --output-on-failure
```

期望：reference/good 通过；bad_rejected 通过，表示 bad 被精确诊断拒绝。

Student 验证：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises -B C18_Interoperability_Plugins_Tooling/build/python-student -DC18_BUILD_REFERENCE=OFF -DC18_ENABLE_PYTHON=ON -DC18_TEST_STUDENTS=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/python-student --config Release --target c18_l06_student
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/python-student -C Release -R "c18_l06_student" --output-on-failure
```

期望：Student 当前失败，说明 starter 没有实现引用账本。
