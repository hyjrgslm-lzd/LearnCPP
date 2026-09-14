# 07. CPython C API：引用和错误状态是同一条控制流

Full C API 的每个 `PyObject*` 都要带一本账。账本有两列：引用所有权和当前线程 error indicator。只看返回值不够；返回值和 error indicator 必须匹配。

三类引用先分清：

```text
new reference
  你拥有一次 DECREF 责任。例如 PyUnicode_FromString、PyTuple_New、Py_NewRef。

borrowed reference
  你只借到一个观察窗口。例如 PyList_GetItem 返回 list 内部元素。
  如果要把它返回给 Python 调用者，先 Py_NewRef。

stolen reference
  callee 接手你已有的 new reference。例如 PyTuple_SetItem 成功后拥有 item。
  成功交出后不要再 DECREF。
```

L06 的 `borrowed_as_new(list, index)` 是最小 borrowed 例子：

```cpp
PyObject* item = PyList_GetItem(list, index); // borrowed
if (!item) return nullptr;                    // error indicator 已由 CPython 设置
return Py_NewRef(item);                       // caller receives new reference
```

如果直接 `return item`，调用者以为拿到 new reference，后续 `DECREF` 会扣掉 list 正在持有的对象。这个错误有时不会马上崩，反而在 list 改写、对象析构或解释器退出时暴露。

`tuple_steals` 是 stolen 例子。账本如下：

```text
tuple = PyTuple_New(2)              new: tuple
a = PyUnicode_FromString(left)      new: tuple, a
b = PyUnicode_FromString(right)     new: tuple, a, b
PyTuple_SetItem(tuple, 0, a)        new: tuple, b      a stolen by tuple
PyTuple_SetItem(tuple, 1, b)        new: tuple         b stolen by tuple
return tuple                        caller owns tuple
```

错误路径也要算账。若创建 `b` 失败，`a` 还没被偷走，就要 `Py_DECREF(a)`；若 tuple 失败，就要释放已经创建的字符串。Reference/good 都按“已经获得什么，就释放什么”回滚。

自定义 owner 类型把 C++ 对象生命周期放进 CPython 类型系统。L06 的 `Owner` 对象内部持有一个 `PyObject* value`。`make_owner(value)` 用 `Py_NewRef(value)` 保存一份强引用。若 value 反过来引用 owner，例如 `list.append(owner)`，单靠引用计数永远不能让计数归零；这个类型必须接入 cyclic GC。

GC 版本多三件事：`tp_traverse` 访问 `value`，`tp_clear` 清掉 `value`，类型 flag 设置 `Py_TPFLAGS_HAVE_GC`。对象用 `PyObject_GC_New` 分配，字段完成后 `PyObject_GC_Track`；析构时先 `PyObject_GC_UnTrack`，再 clear，最后 `PyObject_GC_Del`。`tp_clear` 先把字段置空再 `DECREF`，因为 `DECREF` 可以执行任意 Python 代码，重入时不能再看见旧指针。

checker 的顺序是：创建前计数、创建后加一、删除 Python 名字后回到原值；随后构造 `list -> owner -> list`，`gc.collect()` 后计数也必须回到原值。这个检查证明 owner 类型真的接入了 CPython GC，不是只返回原字符串，也不是只靠 `tp_dealloc` 的 refcount 路径。

模块初始化也有账本。`PyType_Ready(&OwnerType)` 准备类型对象；Full API 3.10 路径可以用 `PyModule_AddObjectRef` 把类型加入模块，失败时 `DECREF` 模块并返回 `nullptr`。若使用 steals reference 的 `PyModule_AddObject`，就要手工 `Py_INCREF` 并处理失败回滚。若模块创建失败，不能继续返回半初始化模块。

错误状态是另一条控制流。CPython C 函数通常用 `NULL` 表示失败，并要求 error indicator 已设置。成功返回非 `NULL` 时，error indicator 必须为空。下面是错误写法：

```cpp
PyErr_SetString(PyExc_ValueError, "leftover error");
return PyUnicode_FromString("ok");
```

Python 层看到的是“你说成功，但异常还挂着”。CPython 会把它升级成 `SystemError`。L06 bad 就是这个错误；checker 捕获 `SystemError` 后输出 `check failed: error indicator leaked through a non-null return`。

修复路径很直接：要失败就设置异常并返回 `nullptr`；要恢复就 `PyErr_Clear()` 后再返回对象。不要把异常状态藏在返回字符串、全局变量或日志里。

本章只覆盖 Full C API。它允许使用 `PyTypeObject` 字段、`PyObject_New` 和大量版本相关能力，因此不能拿它冒充 Stable ABI。第 10 章会把接口面收窄到 Limited API。
