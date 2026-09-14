#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

namespace {
long live_count = 0;

struct Box {
  PyObject_HEAD
  PyObject* held;
};

PyTypeObject BoxType = {PyVarObject_HEAD_INIT(nullptr, 0)};

int visit_box(Box* self, visitproc visit, void* arg) {
  Py_VISIT(self->held);
  return 0;
}

int clear_box(Box* self) {
  PyObject* old = self->held;
  self->held = nullptr;
  Py_XDECREF(old);
  return 0;
}

void destroy_box(Box* self) {
  PyObject_GC_UnTrack(self);
  clear_box(self);
  --live_count;
  PyObject_GC_Del(self);
}

PyObject* value(Box* self, PyObject*) {
  if (self->held == nullptr) {
    Py_RETURN_NONE;
  }
  return Py_NewRef(self->held);
}

PyMethodDef box_methods[] = {{"value", reinterpret_cast<PyCFunction>(value), METH_NOARGS, nullptr},
                             {nullptr, nullptr, 0, nullptr}};

PyObject* make_owner(PyObject*, PyObject* value) {
  auto* box = PyObject_GC_New(Box, &BoxType);
  if (!box) return nullptr;
  box->held = Py_NewRef(value);
  ++live_count;
  PyObject_GC_Track(box);
  return reinterpret_cast<PyObject*>(box);
}

PyObject* live_owners(PyObject*, PyObject*) { return PyLong_FromLong(live_count); }

PyObject* borrowed_as_new(PyObject*, PyObject* args) {
  PyObject* list{};
  Py_ssize_t index{};
  if (!PyArg_ParseTuple(args, "On", &list, &index)) return nullptr;
  PyObject* item = PyList_GetItem(list, index);
  return item ? Py_NewRef(item) : nullptr;
}

PyObject* tuple_steals(PyObject*, PyObject* args) {
  const char *left{}, *right{};
  if (!PyArg_ParseTuple(args, "ss", &left, &right)) return nullptr;
  PyObject* tuple = PyTuple_New(2);
  if (!tuple) return nullptr;
  PyObject* first = PyUnicode_FromString(left);
  if (!first) {
    Py_DECREF(tuple);
    return nullptr;
  }
  PyObject* second = PyUnicode_FromString(right);
  if (!second) {
    Py_DECREF(first);
    Py_DECREF(tuple);
    return nullptr;
  }
  PyTuple_SET_ITEM(tuple, 0, first);
  PyTuple_SET_ITEM(tuple, 1, second);
  return tuple;
}

PyObject* clear_error_then_return(PyObject*, PyObject*) {
  PyErr_SetString(PyExc_RuntimeError, "temporary");
  PyErr_Clear();
  return PyUnicode_FromString("ok");
}

PyMethodDef methods[] = {
    {"make_owner", make_owner, METH_O, nullptr},
    {"live_owners", live_owners, METH_NOARGS, nullptr},
    {"borrowed_as_new", borrowed_as_new, METH_VARARGS, nullptr},
    {"tuple_steals", tuple_steals, METH_VARARGS, nullptr},
    {"clear_error_then_return", clear_error_then_return, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};
PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};
}  // namespace

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  BoxType.tp_name = C18_STR(C18_MODULE_NAME) ".Box";
  BoxType.tp_basicsize = sizeof(Box);
  BoxType.tp_dealloc = reinterpret_cast<destructor>(destroy_box);
  BoxType.tp_traverse = reinterpret_cast<traverseproc>(visit_box);
  BoxType.tp_clear = reinterpret_cast<inquiry>(clear_box);
  BoxType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
  BoxType.tp_methods = box_methods;
  if (PyType_Ready(&BoxType) < 0) return nullptr;
  PyObject* m = PyModule_Create(&module);
  if (!m) return nullptr;
  if (PyModule_AddObjectRef(m, "Owner", reinterpret_cast<PyObject*>(&BoxType)) < 0) {
    Py_DECREF(m);
    return nullptr;
  }
  return m;
}
