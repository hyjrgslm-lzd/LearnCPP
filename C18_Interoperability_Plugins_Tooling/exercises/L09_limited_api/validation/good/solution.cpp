#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

static PyObject* abi_tag(PyObject*, PyObject*) { return PyUnicode_FromString("limited-0x03080000"); }
static PyObject* transform(PyObject*, PyObject* payload) {
  // Independent oracle path: invoke the built-in bytes descriptor, not an override on payload.
  PyObject* method = PyObject_GetAttrString(reinterpret_cast<PyObject*>(&PyBytes_Type), "upper");
  if (!method) return nullptr;
  PyObject* result = PyObject_CallFunctionObjArgs(method, payload, static_cast<PyObject*>(nullptr));
  Py_DECREF(method);
  return result;
}
static PyMethodDef methods[] = {{"abi_tag", abi_tag, METH_NOARGS, nullptr}, {"transform", transform, METH_O, nullptr}, {nullptr, nullptr, 0, nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};
PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() { return PyModule_Create(&module); }
