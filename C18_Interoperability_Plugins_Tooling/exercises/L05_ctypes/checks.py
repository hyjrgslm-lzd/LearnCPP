from __future__ import annotations

import ctypes
import gc
import importlib.util
import sys
import weakref
from pathlib import Path

C18_STATUS_OK = 0
C18_STATUS_BUFFER_TOO_SMALL = 3
C18_STATUS_CLOSING = 4
C18_STATUS_BUSY = 5


class Counts(ctypes.Structure):
    _fields_ = [
        ("get_api_calls", ctypes.c_uint32),
        ("create_calls", ctypes.c_uint32),
        ("process_calls", ctypes.c_uint32),
        ("request_stop_calls", ctypes.c_uint32),
        ("destroy_calls", ctypes.c_uint32),
    ]


class Observer:
    def __init__(self, library: str):
        self._dll = ctypes.CDLL(str(Path(library).resolve()))
        self._dll.c18_l05_reset_counts.argtypes = []
        self._dll.c18_l05_reset_counts.restype = None
        self._dll.c18_l05_set_destroy_busy_once.argtypes = [ctypes.c_uint32]
        self._dll.c18_l05_set_destroy_busy_once.restype = None
        self._dll.c18_l05_query_counts.argtypes = [ctypes.POINTER(Counts)]
        self._dll.c18_l05_query_counts.restype = ctypes.c_uint32

    def reset(self):
        self._dll.c18_l05_reset_counts()

    def busy_destroy_once(self):
        self._dll.c18_l05_set_destroy_busy_once(1)

    def counts(self) -> Counts:
        counts = Counts()
        status = self._dll.c18_l05_query_counts(ctypes.byref(counts))
        if status != C18_STATUS_OK:
            raise RuntimeError(f"query counts failed: {status}")
        return counts


class Callback:
    def __init__(self):
        self.events: list[str] = []

    def __call__(self, name: str):
        self.events.append(name)


def fail(message: str) -> int:
    print(f"check failed: {message}")
    return 1


def load_solution(path: str):
    spec = importlib.util.spec_from_file_location("c18_l05_solution", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def expect_counts(counts: Counts, *, create: int, process: int, request_stop: int, destroy: int) -> int:
    if counts.get_api_calls < 1:
        return fail("solution did not query c18_get_api")
    if counts.create_calls != create:
        return fail(f"native create count expected {create} got {counts.create_calls}")
    if counts.process_calls != process:
        return fail(f"native process count expected {process} got {counts.process_calls}")
    if counts.request_stop_calls != request_stop:
        return fail(f"native request_stop count expected {request_stop} got {counts.request_stop_calls}")
    if counts.destroy_calls != destroy:
        return fail(f"native destroy count expected {destroy} got {counts.destroy_calls}")
    return 0


def main() -> int:
    if len(sys.argv) != 3:
        return fail("usage: checks.py solution.py plugin")
    solution = load_solution(sys.argv[1])
    observer = Observer(sys.argv[2])
    observer.reset()
    session = None
    try:
        session = solution.CtypesSession(sys.argv[2])
        if expect_counts(observer.counts(), create=1, process=0, request_stop=0, destroy=0):
            return 1

        callback = Callback()
        callback_ref = weakref.ref(callback)
        session.set_callback(callback)
        del callback
        gc.collect()

        payload = b"azA\x00\xff!m"
        if session.process(payload) != b"AZA\x00\xff!M":
            return fail("ctypes call did not preserve the byte contract")
        if expect_counts(observer.counts(), create=1, process=1, request_stop=0, destroy=0):
            return 1
        kept_callback = callback_ref()
        if kept_callback is None:
            return fail("callback target was not kept alive")
        if kept_callback.events != ["process:bang"]:
            return fail("callback did not cross the ABI with userdata intact")

        small = bytearray(b"\x7f" * 3)
        status, required = session.process_into(payload, small)
        if status != C18_STATUS_BUFFER_TOO_SMALL or required != len(payload) or small != bytearray(b"\x7f" * 3):
            return fail("small output must report required capacity and keep caller buffer unchanged")
        if expect_counts(observer.counts(), create=1, process=2, request_stop=0, destroy=0):
            return 1

        raising_seen = []

        def raising_callback(name: str):
            raising_seen.append(name)
            raise RuntimeError("callback boom")

        active_callback_ref = weakref.ref(raising_callback)
        session.set_callback(raising_callback)
        del raising_callback
        gc.collect()
        if session.process(b"!") != b"!":
            return fail("callback error path changed process bytes")
        if raising_seen != ["process:bang"]:
            return fail("raising callback was not invoked through native callback")
        error = getattr(session, "callback_error", lambda: "")()
        if "callback boom" not in error:
            return fail("callback error was not recorded outside the C boundary")

        observer.busy_destroy_once()
        if session.close() != C18_STATUS_BUSY:
            return fail("busy destroy must keep the session open for retry")
        if expect_counts(observer.counts(), create=1, process=3, request_stop=1, destroy=1):
            return 1
        if active_callback_ref() is None:
            return fail("callback owner was released after failed close")

        if session.close() != C18_STATUS_OK:
            return fail("retry close failed")
        if expect_counts(observer.counts(), create=1, process=3, request_stop=2, destroy=2):
            return 1
        gc.collect()
        if active_callback_ref() is not None:
            return fail("callback owner was not released after close")
        status, _ = session.process_into(b"a", bytearray(1))
        if status != C18_STATUS_CLOSING:
            return fail("process after close must be rejected")
        if session.close() != C18_STATUS_OK:
            return fail("repeated close must be idempotent")
        print("L05 ctypes PASS")
        return 0
    finally:
        if session is not None:
            try:
                session.close()
            except Exception as exc:
                print(f"check cleanup warning: {exc}", file=sys.stderr)


if __name__ == "__main__":
    raise SystemExit(main())
