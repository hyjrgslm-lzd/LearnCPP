from __future__ import annotations

import ctypes
from pathlib import Path

C18_STATUS_OK = 0
C18_STATUS_CLOSING = 4
_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p)


class Host(ctypes.Structure):
    _fields_ = [
        ("version", ctypes.c_uint32),
        ("struct_size", ctypes.c_uint32),
        ("userdata", ctypes.c_void_p),
        ("on_event", _CALLBACK),
    ]


class Api(ctypes.Structure):
    pass


Api._fields_ = [
    ("version", ctypes.c_uint32),
    ("struct_size", ctypes.c_uint32),
    ("create", ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.POINTER(Host), ctypes.POINTER(ctypes.c_void_p))),
    (
        "process",
        ctypes.CFUNCTYPE(
            ctypes.c_uint32,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ),
    ),
    ("request_stop", ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)),
    ("destroy", ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)),
]
_GET = ctypes.CFUNCTYPE(ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(Api))


class CtypesSession:
    def __init__(self, library: str):
        self._callback = None
        self._callback_error = ""
        self._self_object = ctypes.py_object(self)
        self._self_ptr = ctypes.cast(ctypes.pointer(self._self_object), ctypes.c_void_p)
        self._event = _CALLBACK(self._on_event)
        self._dll = ctypes.CDLL(str(Path(library).resolve()))
        self._api = Api()
        status = _GET(("c18_get_api", self._dll))(1, ctypes.sizeof(self._api), ctypes.byref(self._api))
        if status:
            raise RuntimeError(status)
        self._ctx = ctypes.c_void_p()
        host = Host(1, ctypes.sizeof(Host), self._self_ptr, self._event)
        status = self._api.create(ctypes.byref(host), ctypes.byref(self._ctx))
        if status:
            raise RuntimeError(status)

    def set_callback(self, callback):
        self._callback = callback
        self._callback_error = ""

    def _on_event(self, userdata, name):
        session = ctypes.cast(userdata, ctypes.POINTER(ctypes.py_object)).contents.value
        if session._callback is None:
            return
        try:
            session._callback((name or b"").decode("ascii"))
        except BaseException as exc:
            session._callback_error = str(exc)

    def process_into(self, payload: bytes, output: bytearray):
        if not self._ctx:
            return C18_STATUS_CLOSING, len(payload)
        src = (ctypes.c_uint8 * len(payload)).from_buffer_copy(payload) if payload else None
        dst = (ctypes.c_uint8 * len(output)).from_buffer(output) if output else None
        written = ctypes.c_size_t()
        status = self._api.process(self._ctx, src, len(payload), dst, len(output), ctypes.byref(written))
        return status, written.value

    def process(self, payload: bytes) -> bytes:
        result = bytearray(len(payload))
        status, written = self.process_into(payload, result)
        if status:
            raise RuntimeError(status)
        return bytes(result[:written])

    def callback_error(self):
        return self._callback_error

    def close(self):
        if not self._ctx:
            return C18_STATUS_OK
        stopped = self._api.request_stop(self._ctx)
        if stopped:
            return stopped
        destroyed = self._api.destroy(self._ctx)
        if destroyed == C18_STATUS_OK:
            self._ctx = None
            self._callback = None
            self._event = None
            self._self_object = None
            self._self_ptr = None
        return destroyed
