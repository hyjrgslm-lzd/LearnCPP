from __future__ import annotations

import importlib.util
import gc
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


def main() -> int:
    if len(sys.argv) != 2:
        return fail("usage: checks.py module")
    module = load(sys.argv[1])
    try:
        result = module.clear_error_then_return()
    except SystemError:
        return fail("error indicator leaked through a non-null return")
    if result != "ok":
        return fail("cleared error path returned wrong value")
    before = module.live_owners()
    owner = module.make_owner("alpha")
    if owner.value() != "alpha":
        return fail("custom owner did not retain its value")
    if module.live_owners() != before + 1:
        return fail("custom owner lifetime counter did not increment")
    del owner
    if module.live_owners() != before:
        return fail("custom owner dealloc did not release")
    cycle_before = module.live_owners()
    payload = []
    owner = module.make_owner(payload)
    payload.append(owner)
    if module.live_owners() != cycle_before + 1:
        return fail("custom owner cycle counter did not increment")
    del payload, owner
    gc.collect()
    if module.live_owners() != cycle_before:
        return fail("custom owner cycle was not collected")
    data = ["borrowed"]
    item = module.borrowed_as_new(data, 0)
    data[0] = "changed"
    if item != "borrowed":
        return fail("borrowed list item was returned without owning a new reference")
    if module.tuple_steals("left", "right") != ("left", "right"):
        return fail("stolen tuple references were not committed")
    print("L06 CPython refs/errors PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
