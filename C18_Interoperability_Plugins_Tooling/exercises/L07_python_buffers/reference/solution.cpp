#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define C18_CAT2(a, b) a##b
#define C18_CAT(a, b) C18_CAT2(a, b)
#define C18_STR2(x) #x
#define C18_STR(x) C18_STR2(x)

namespace {
struct ExportBox {
  PyObject_HEAD
  PyObject* bytes;
  int exports;
};

PyTypeObject ExportBoxType = {PyVarObject_HEAD_INIT(nullptr, 0)};

void box_dealloc(ExportBox* self) {
  Py_XDECREF(self->bytes);
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

int box_init(ExportBox* self, PyObject* args, PyObject*) {
  PyObject* input{};
  if (!PyArg_ParseTuple(args, "O", &input)) return -1;
  if (!PyBytes_CheckExact(input)) {
    PyErr_SetString(PyExc_TypeError, "ExportBox requires exact bytes");
    return -1;
  }
  if (self->exports != 0) {
    PyErr_SetString(PyExc_BufferError, "cannot reinitialize while buffers are exported");
    return -1;
  }
  PyObject* next = Py_NewRef(input);
  PyObject* old = self->bytes;
  self->bytes = next;
  Py_XDECREF(old);
  return 0;
}

PyObject* box_exports(ExportBox* self, PyObject*) { return PyLong_FromLong(self->exports); }

PyMethodDef box_methods[] = {{"exports", reinterpret_cast<PyCFunction>(box_exports), METH_NOARGS, nullptr},
                             {nullptr, nullptr, 0, nullptr}};

int box_getbuffer(PyObject* exporter, Py_buffer* view, int flags) {
  auto* self = reinterpret_cast<ExportBox*>(exporter);
  if (self->bytes == nullptr) {
    PyErr_SetString(PyExc_BufferError, "ExportBox is not initialized");
    return -1;
  }
  char* data{};
  Py_ssize_t size{};
  if (PyBytes_AsStringAndSize(self->bytes, &data, &size) < 0) return -1;
  if (PyBuffer_FillInfo(view, exporter, data, size, 1, flags) < 0) return -1;
  ++self->exports;
  return 0;
}

void box_releasebuffer(PyObject* exporter, Py_buffer*) {
  auto* self = reinterpret_cast<ExportBox*>(exporter);
  --self->exports;
}

PyBufferProcs box_buffer{box_getbuffer, box_releasebuffer};

PyObject* transform_view(PyObject*, PyObject* obj) {
  Py_buffer view{};
  if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) < 0) return nullptr;
  PyObject* result = PyBytes_FromStringAndSize(nullptr, view.len);
  if (result == nullptr) {
    PyBuffer_Release(&view);
    return nullptr;
  }
  auto* input = static_cast<unsigned char*>(view.buf);
  auto* output = reinterpret_cast<unsigned char*>(PyBytes_AS_STRING(result));
  for (Py_ssize_t i = 0; i < view.len; ++i) {
    const unsigned char byte = input[i];
    output[i] = static_cast<unsigned char>((byte >= 'a' && byte <= 'z') ? byte - 32 : byte);
  }
  PyBuffer_Release(&view);
  return result;
}

PyObject* request_writable(PyObject*, PyObject* obj) {
  Py_buffer view{};
  if (PyObject_GetBuffer(obj, &view, PyBUF_WRITABLE) < 0) {
    PyErr_Clear();
    Py_RETURN_TRUE;
  }
  PyBuffer_Release(&view);
  Py_RETURN_FALSE;
}

PyMethodDef methods[] = {{"transform_view", transform_view, METH_O, nullptr},
                         {"request_writable", request_writable, METH_O, nullptr},
                         {nullptr, nullptr, 0, nullptr}};
PyModuleDef module = {PyModuleDef_HEAD_INIT, C18_STR(C18_MODULE_NAME), nullptr, -1, methods};
}  // namespace

PyMODINIT_FUNC C18_CAT(PyInit_, C18_MODULE_NAME)() {
  ExportBoxType.tp_name = C18_STR(C18_MODULE_NAME) ".ExportBox";
  ExportBoxType.tp_basicsize = sizeof(ExportBox);
  ExportBoxType.tp_dealloc = reinterpret_cast<destructor>(box_dealloc);
  ExportBoxType.tp_flags = Py_TPFLAGS_DEFAULT;
  ExportBoxType.tp_new = PyType_GenericNew;
  ExportBoxType.tp_init = reinterpret_cast<initproc>(box_init);
  ExportBoxType.tp_methods = box_methods;
  ExportBoxType.tp_as_buffer = &box_buffer;
  if (PyType_Ready(&ExportBoxType) < 0) return nullptr;

  PyObject* m = PyModule_Create(&module);
  if (!m) return nullptr;
  if (PyModule_AddObjectRef(m, "ExportBox", reinterpret_cast<PyObject*>(&ExportBoxType)) < 0) {
    Py_DECREF(m);
    return nullptr;
  }
  return m;
}
