#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

static PyObject* abi_tag(PyObject*, PyObject*) { return PyUnicode_FromString("full-3.8"); }
static PyObject* transform(PyObject*, PyObject* payload) { return PyObject_CallMethod(payload, "upper", nullptr); }
static PyMethodDef methods[] = {{"abi_tag", abi_tag, METH_NOARGS, nullptr}, {"transform", transform, METH_O, nullptr}, {nullptr, nullptr, 0, nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};
PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() { return PyModule_Create(&module); }
