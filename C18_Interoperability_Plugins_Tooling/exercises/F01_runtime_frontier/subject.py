"""Pinned CPython observations. Probe absence separately from subject failure."""
from __future__ import annotations

import argparse
import importlib.util
import sys
import sysconfig


def probe(feature: str) -> int:
    if sys.implementation.name != "cpython":
        print("SKIP: this experiment targets CPython")
        return 77
    if feature == "subinterpreters":
        if sys.version_info[:2] not in {(3, 10), (3, 12)} or importlib.util.find_spec("_xxsubinterpreters") is None:
            print("SKIP: pinned CPython 3.10/3.12 _xxsubinterpreters API unavailable")
            return 77
    elif sys.version_info[:2] != (3, 13) or not sysconfig.get_config_var("Py_GIL_DISABLED"):
        print("SKIP: CPython 3.13 free-threaded build unavailable")
        return 77
    print(f"capability available: {feature}")
    return 0


def subinterpreters() -> None:
    import _xxsubinterpreters as interpreters
    identifiers = []
    failures = []
    try:
        first = interpreters.create()
        identifiers.append(first)
        second = interpreters.create()
        identifiers.append(second)
        interpreters.run_string(first, "marker = 41")
        interpreters.run_string(second, "if 'marker' in globals(): raise RuntimeError('leaked global')\nmarker = 99")
        interpreters.run_string(first, "if marker != 41: raise RuntimeError('lost interpreter state')")
        interpreters.run_string(second, "if marker != 99: raise RuntimeError('wrong interpreter state')")
    except BaseException as error:
        failures.append(f"subject: {error}")
    finally:
        for identifier in reversed(identifiers):
            try:
                interpreters.destroy(identifier)
            except BaseException as error:
                failures.append(f"cleanup: {error}")
    if failures:
        raise RuntimeError("; ".join(failures))
    print("subinterpreter namespace isolation and cleanup: PASS")


def free_threaded() -> None:
    import _c18_free
    from concurrent.futures import ThreadPoolExecutor
    from threading import Barrier
    if sys._is_gil_enabled():
        raise RuntimeError("GIL enabled after native extension import")
    inputs = (b"", b"a\x00z\xff", bytes(range(256)))
    table = bytes.maketrans(b"abcdefghijklmnopqrstuvwxyz", b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    barrier = Barrier(4)
    def work(worker: int) -> int:
        barrier.wait(timeout=10)
        for _ in range(200):
            for payload in inputs:
                if _c18_free.upper(payload) != payload.translate(table):
                    raise RuntimeError("native byte result mismatch")
        return worker
    with ThreadPoolExecutor(max_workers=4) as pool:
        completed = list(pool.map(work, range(4)))
    if sorted(completed) != list(range(4)) or sys._is_gil_enabled():
        raise RuntimeError("thread completion or GIL-state mismatch")
    try:
        _c18_free.upper(bytearray(b"abc"))
    except TypeError:
        pass
    else:
        raise RuntimeError("mutable buffer accepted by immutable-input contract")
    print("free-threaded immutable-input native calls: PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("feature", choices=["subinterpreters", "free_threaded"])
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--module-dir")
    args = parser.parse_args()
    if args.probe:
        return probe(args.feature)
    if args.module_dir:
        sys.path.insert(0, args.module_dir)
    try:
        {"subinterpreters": subinterpreters, "free_threaded": free_threaded}[args.feature]()
    except BaseException as error:
        print(f"check failed: {args.feature}: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
