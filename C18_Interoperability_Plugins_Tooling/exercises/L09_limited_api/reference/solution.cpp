#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

static PyObject* abi_tag(PyObject*, PyObject*) {
  return PyUnicode_FromString("limited-0x03080000");
}

static PyObject* transform(PyObject*, PyObject* obj) {
  char* data{};
  Py_ssize_t size{};
  if (PyBytes_AsStringAndSize(obj, &data, &size) < 0) return nullptr;
  PyObject* out = PyBytes_FromStringAndSize(nullptr, size);
  if (!out) return nullptr;
  char* dst{};
  Py_ssize_t out_size{};
  if (PyBytes_AsStringAndSize(out, &dst, &out_size) < 0) {
    Py_DECREF(out);
    return nullptr;
  }
  for (Py_ssize_t i = 0; i < size; ++i) {
    unsigned char b = static_cast<unsigned char>(data[i]);
    dst[i] = static_cast<char>((b >= 'a' && b <= 'z') ? b - 32 : b);
  }
  return out;
}

static PyMethodDef methods[] = {
    {"abi_tag", abi_tag, METH_NOARGS, nullptr},
    {"transform", transform, METH_O, nullptr},
    {nullptr, nullptr, 0, nullptr}};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  return PyModule_Create(&module);
}
