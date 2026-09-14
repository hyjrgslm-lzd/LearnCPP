from __future__ import annotations

import ctypes
from pathlib import Path

C18_STATUS_OK = 0
C18_STATUS_BUFFER_TOO_SMALL = 3
C18_STATUS_CLOSING = 4

_EVENT = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p)


class HostApi(ctypes.Structure):
    _fields_ = [
        ("version", ctypes.c_uint32),
        ("struct_size", ctypes.c_uint32),
        ("userdata", ctypes.c_void_p),
        ("on_event", _EVENT),
    ]


class Api(ctypes.Structure):
    pass


_CREATE = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(HostApi), ctypes.POINTER(ctypes.c_void_p))
_PROCESS = ctypes.CFUNCTYPE(
    ctypes.c_uint32,
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_uint8),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
)
_REQUEST_STOP = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)
_DESTROY = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)
Api._fields_ = [
    ("version", ctypes.c_uint32),
    ("struct_size", ctypes.c_uint32),
    ("create", _CREATE),
    ("process", _PROCESS),
    ("request_stop", _REQUEST_STOP),
    ("destroy", _DESTROY),
]
_GET_API = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(Api))


class CtypesSession:
    def __init__(self, library: str):
        self._callback = None
        self._callback_error = ""
        self._userdata = ctypes.py_object(self)
        self._userdata_ptr = ctypes.cast(ctypes.pointer(self._userdata), ctypes.c_void_p)
        self._c_callback = _EVENT(self._host_event)
        self._dll = ctypes.CDLL(str(Path(library).resolve()))
        self._api = Api()
        status = _GET_API(("c18_get_api", self._dll))(1, ctypes.sizeof(self._api), ctypes.byref(self._api))
        if status != C18_STATUS_OK:
            raise RuntimeError(f"c18_get_api failed: {status}")
        host = HostApi(1, ctypes.sizeof(HostApi), self._userdata_ptr, self._c_callback)
        self._ctx = ctypes.c_void_p()
        status = self._api.create(ctypes.byref(host), ctypes.byref(self._ctx))
        if status != C18_STATUS_OK:
            raise RuntimeError(f"create failed: {status}")

    def set_callback(self, callback):
        self._callback = callback
        self._callback_error = ""

    def _host_event(self, userdata, name):
        owner = ctypes.cast(userdata, ctypes.POINTER(ctypes.py_object)).contents.value
        callback = owner._callback
        if callback is None:
            return
        try:
            callback((name or b"").decode("ascii"))
        except BaseException as exc:
            owner._callback_error = f"{type(exc).__name__}: {exc}"

    def process_into(self, payload: bytes, output: bytearray):
        if not self._ctx:
            return C18_STATUS_CLOSING, len(payload)
        src = (ctypes.c_uint8 * len(payload)).from_buffer_copy(payload) if payload else None
        dst = (ctypes.c_uint8 * len(output)).from_buffer(output) if output else None
        written = ctypes.c_size_t()
        status = self._api.process(self._ctx, src, len(payload), dst, len(output), ctypes.byref(written))
        return status, written.value

    def process(self, payload: bytes) -> bytes:
        output = bytearray(len(payload))
        status, written = self.process_into(payload, output)
        if status != C18_STATUS_OK:
            raise RuntimeError(f"process failed: {status}")
        return bytes(output[:written])

    def callback_error(self) -> str:
        return self._callback_error

    def close(self):
        if not self._ctx:
            return C18_STATUS_OK
        stop_status = self._api.request_stop(self._ctx)
        if stop_status != C18_STATUS_OK:
            return stop_status
        destroy_status = self._api.destroy(self._ctx)
        if destroy_status == C18_STATUS_OK:
            self._ctx = None
            self._callback = None
            self._userdata = None
            self._userdata_ptr = None
            self._c_callback = None
        return destroy_status
