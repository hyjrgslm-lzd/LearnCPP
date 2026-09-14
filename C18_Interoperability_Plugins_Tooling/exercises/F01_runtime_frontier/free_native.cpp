#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "c18/bytes.hpp"
#if PY_VERSION_HEX < 0x030D0000 || PY_VERSION_HEX >= 0x030E0000 || !defined(Py_GIL_DISABLED)
#error "This native subject targets the CPython 3.13 free-threaded Full C API"
#endif

static PyObject* upper(PyObject*, PyObject* value) {
    // Immutable exact bytes plus a live argument reference avoid borrowed mutable-container races.
    if (!PyBytes_CheckExact(value)) {
        PyErr_SetString(PyExc_TypeError, "exact immutable bytes required");
        return nullptr;
    }
    char* input = nullptr;
    Py_ssize_t length = 0;
    if (PyBytes_AsStringAndSize(value, &input, &length) < 0) return nullptr;
    if (length < 0 || length > 1024 * 1024) {
        PyErr_SetString(PyExc_ValueError, "input exceeds the 1 MiB budget");
        return nullptr;
    }
    PyObject* result = PyBytes_FromStringAndSize(nullptr, length);
    if (!result) return nullptr;
    size_t written = 0;
    auto status = c18::transform_bytes(reinterpret_cast<const uint8_t*>(input), static_cast<size_t>(length),
                                     reinterpret_cast<uint8_t*>(PyBytes_AsString(result)), static_cast<size_t>(length), &written);
    if (status != C18_STATUS_OK || written != static_cast<size_t>(length)) {
        Py_DECREF(result);
        PyErr_SetString(PyExc_RuntimeError, "native transform contract failed");
        return nullptr;
    }
    return result;
}

static PyMethodDef methods[] = {{"upper", upper, METH_O, "ASCII conversion of immutable bytes."}, {nullptr, nullptr, 0, nullptr}};
static PyModuleDef_Slot slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, nullptr}
};
static PyModuleDef module = {PyModuleDef_HEAD_INIT, "_c18_free", nullptr, 0, methods, slots, nullptr, nullptr, nullptr};
PyMODINIT_FUNC PyInit__c18_free() { return PyModuleDef_Init(&module); }
