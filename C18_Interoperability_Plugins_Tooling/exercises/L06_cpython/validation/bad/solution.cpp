#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

static PyObject* ok_owner(PyObject*, PyObject* value) { return Py_NewRef(value); }
static PyObject* zero(PyObject*, PyObject*) { return PyLong_FromLong(0); }
static PyObject* borrowed_as_new(PyObject*, PyObject* args) {
  PyObject* list{};
  Py_ssize_t index{};
  if (!PyArg_ParseTuple(args, "On", &list, &index)) return nullptr;
  return Py_NewRef(PyList_GetItem(list, index));
}
static PyObject* tuple_steals(PyObject*, PyObject* args) {
  const char *left{}, *right{};
  PyArg_ParseTuple(args, "ss", &left, &right);
  return Py_BuildValue("(ss)", left, right);
}
static PyObject* clear_error_then_return(PyObject*, PyObject*) {
  PyErr_SetString(PyExc_ValueError, "leftover error");
  return PyUnicode_FromString("ok");
}
static PyMethodDef methods[] = {
    {"make_owner", ok_owner, METH_O, nullptr},
    {"live_owners", zero, METH_NOARGS, nullptr},
    {"borrowed_as_new", borrowed_as_new, METH_VARARGS, nullptr},
    {"tuple_steals", tuple_steals, METH_VARARGS, nullptr},
    {"clear_error_then_return", clear_error_then_return, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  return PyModule_Create(&module);
}
