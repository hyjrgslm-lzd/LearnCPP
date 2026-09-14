#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

namespace {
long live_owners = 0;

struct Owner {
  PyObject_HEAD
  PyObject* value;
};

PyTypeObject OwnerType = {PyVarObject_HEAD_INIT(nullptr, 0)};

int owner_traverse(Owner* self, visitproc visit, void* arg) {
  Py_VISIT(self->value);
  return 0;
}

int owner_clear(Owner* self) {
  Py_CLEAR(self->value);
  return 0;
}

void owner_dealloc(Owner* self) {
  PyObject_GC_UnTrack(self);
  owner_clear(self);
  --live_owners;
  PyObject_GC_Del(self);
}

PyObject* owner_value(Owner* self, PyObject*) {
  if (self->value == nullptr) {
    Py_RETURN_NONE;
  }
  return Py_NewRef(self->value);
}

PyMethodDef owner_methods[] = {{"value", reinterpret_cast<PyCFunction>(owner_value), METH_NOARGS, nullptr},
                               {nullptr, nullptr, 0, nullptr}};

PyObject* make_owner(PyObject*, PyObject* arg) {
  auto* owner = PyObject_GC_New(Owner, &OwnerType);
  if (owner == nullptr) {
    return nullptr;
  }
  owner->value = Py_NewRef(arg);
  ++live_owners;
  PyObject_GC_Track(owner);
  return reinterpret_cast<PyObject*>(owner);
}

PyObject* live(PyObject*, PyObject*) {
  return PyLong_FromLong(live_owners);
}

PyObject* borrowed_as_new(PyObject*, PyObject* args) {
  PyObject* list = nullptr;
  Py_ssize_t index = 0;
  if (!PyArg_ParseTuple(args, "On", &list, &index)) return nullptr;
  PyObject* item = PyList_GetItem(list, index);
  if (!item) return nullptr;
  return Py_NewRef(item);
}

PyObject* tuple_steals(PyObject*, PyObject* args) {
  const char* left = nullptr;
  const char* right = nullptr;
  if (!PyArg_ParseTuple(args, "ss", &left, &right)) return nullptr;
  PyObject* tuple = PyTuple_New(2);
  if (!tuple) return nullptr;
  PyObject* a = PyUnicode_FromString(left);
  if (!a) {
    Py_DECREF(tuple);
    return nullptr;
  }
  PyObject* b = PyUnicode_FromString(right);
  if (!b) {
    Py_DECREF(a);
    Py_DECREF(tuple);
    return nullptr;
  }
  PyTuple_SET_ITEM(tuple, 0, a);
  PyTuple_SET_ITEM(tuple, 1, b);
  return tuple;
}

PyObject* clear_error_then_return(PyObject*, PyObject*) {
  PyErr_SetString(PyExc_ValueError, "transient parse failure");
  PyErr_Clear();
  return PyUnicode_FromString("ok");
}

PyMethodDef methods[] = {
    {"make_owner", make_owner, METH_O, nullptr},
    {"live_owners", live, METH_NOARGS, nullptr},
    {"borrowed_as_new", borrowed_as_new, METH_VARARGS, nullptr},
    {"tuple_steals", tuple_steals, METH_VARARGS, nullptr},
    {"clear_error_then_return", clear_error_then_return, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};

PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};
}  // namespace

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  OwnerType.tp_name = C18_STR(C18_MODULE_NAME) ".Owner";
  OwnerType.tp_basicsize = sizeof(Owner);
  OwnerType.tp_dealloc = reinterpret_cast<destructor>(owner_dealloc);
  OwnerType.tp_traverse = reinterpret_cast<traverseproc>(owner_traverse);
  OwnerType.tp_clear = reinterpret_cast<inquiry>(owner_clear);
  OwnerType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
  OwnerType.tp_methods = owner_methods;
  if (PyType_Ready(&OwnerType) < 0) return nullptr;

  PyObject* created = PyModule_Create(&module);
  if (!created) return nullptr;
  if (PyModule_AddObjectRef(created, "Owner", reinterpret_cast<PyObject*>(&OwnerType)) < 0) {
    Py_DECREF(created);
    return nullptr;
  }
  return created;
}
