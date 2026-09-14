from __future__ import annotations

import argparse
import gc
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


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def check_module(module) -> None:
    module.reset_counts()

    holder_owner = module.Owner(b"xy")
    alias = module.make_owner_alias(holder_owner)
    del holder_owner
    gc.collect()
    require(module.owner_destructed() == 0, "holder did not keep the shared object alive")
    require(alias.view_bytes() == b"xy", "holder object did not retain usable owner state")
    del alias
    gc.collect()
    require(module.owner_destructed() == 1, "holder did not release the shared object")

    module.reset_counts()
    owner = module.Owner(b"az")
    view = owner.view()
    del owner
    gc.collect()
    require(module.owner_destructed() == 0, "owner died while exported view was still alive")
    require(bytes(view) == b"az", "reference_internal view returned wrong bytes")
    del view
    gc.collect()
    require(module.owner_destructed() == 1, "owner was not released after view lifetime ended")

    module.reset_counts()
    registry = module.Registry()
    token = module.Token("callback")
    registry.remember(token)
    require(registry.last == "callback", "registry did not consume the token before release")
    del token
    gc.collect()
    require(module.token_destructed() == 0, "keep_alive did not keep token alive")
    del registry
    gc.collect()
    require(module.token_destructed() == 1, "keep_alive kept token past registry lifetime")

    try:
        module.raise_cpp()
    except RuntimeError as exc:
        require("mapped to Python exception" in str(exc), "C++ exception message was lost")
    else:
        raise AssertionError("C++ exception was not translated to RuntimeError")

    class Upper(module.Transformer):
        def transform(self, payload: str) -> str:
            return "override:" + payload.upper()

    result = module.call_transformer_from_worker(Upper(), b"az")
    require(result == b"override:AZ", "Python override was not called from the worker")
    trace = module.trace()
    require("worker_gil" in trace, "worker did not acquire the GIL before Python callback")
    require("override_returned:override:AZ" in trace, "trace did not record override result")

    require(module.transform(b"azA\x00\xffm") == b"AZA\x00\xffM", "pybind transform broke byte semantics")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("module")
    args = parser.parse_args()
    module = load(args.module)
    try:
        check_module(module)
    except AssertionError as exc:
        return fail(str(exc))
    print("L10 pybind11 PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
