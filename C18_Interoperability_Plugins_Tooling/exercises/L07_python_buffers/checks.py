from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


def fail(message: str) -> int:
    print(f"check failed: {message}")
    return 1


def load(path: str):
    spec = importlib.util.spec_from_file_location(Path(path).stem, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def expect_raises(kind: type[BaseException] | tuple[type[BaseException], ...], action, message: str) -> int:
    try:
        action()
    except kind:
        return 0
    except Exception as error:
        return fail(f"{message}: wrong exception {type(error).__name__}")
    return fail(message)


def main() -> int:
    if len(sys.argv) != 2:
        return fail("usage: checks.py module")
    module = load(sys.argv[1])
    box = module.ExportBox(b"azA\x00\xffm")
    view = memoryview(box)
    if box.exports() != 1:
        return fail("exporter did not count active views")
    view.release()
    if box.exports() != 0:
        return fail("released memoryview did not notify exporter")
    if module.transform_view(box) != b"AZA\x00\xffM":
        return fail("Py_buffer transform broke byte semantics")
    if box.exports() != 0:
        return fail("Py_buffer was not released")
    box = module.ExportBox(b"old")
    view = memoryview(box)
    if box.exports() != 1:
        return fail("active memoryview was not counted")
    rc = expect_raises(BufferError, lambda: box.__init__(b"new"), "active view reinit was not rejected")
    if rc:
        view.release()
        return rc
    if box.exports() != 1 or view.tobytes() != b"old":
        view.release()
        return fail("failed reinit changed active view or export count")
    view.release()
    if box.exports() != 0:
        return fail("release after failed reinit did not restore exports")
    box.__init__(b"new")
    if module.transform_view(box) != b"NEW":
        return fail("reinit after release did not replace bytes")
    raw = module.ExportBox.__new__(module.ExportBox)
    rc = expect_raises(BufferError, lambda: memoryview(raw), "uninitialized exporter buffer was not rejected")
    if rc:
        return rc
    rc = expect_raises(BufferError, lambda: module.transform_view(raw), "uninitialized exporter consumer was not rejected")
    if rc:
        return rc
    readonly = module.ExportBox(b"abc")
    if not module.request_writable(readonly) or readonly.exports() != 0:
        return fail("readonly writable request changed exports")
    strided = memoryview(bytearray(b"abcdef"))[::2]
    rc = expect_raises((BufferError, TypeError, ValueError), lambda: module.transform_view(strided), "non-contiguous view was not rejected")
    if rc:
        return rc
    print("L07 Python buffers PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
