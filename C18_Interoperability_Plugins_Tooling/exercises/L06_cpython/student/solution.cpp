#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

static PyObject* not_done(PyObject*, PyObject*) {
  PyErr_SetString(PyExc_NotImplementedError, "finish the CPython reference exercise");
  return nullptr;
}

static PyMethodDef methods[] = {
    {"make_owner", not_done, METH_O, nullptr},
    {"live_owners", not_done, METH_NOARGS, nullptr},
    {"borrowed_as_new", not_done, METH_VARARGS, nullptr},
    {"tuple_steals", not_done, METH_VARARGS, nullptr},
    {"clear_error_then_return", not_done, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  return PyModule_Create(&module);
}
