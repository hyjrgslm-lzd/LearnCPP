"""Explicitly obtain pinned course dependencies; configure never calls this tool."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import urllib.request

def sha256(path: Path) -> str:
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()

def command(args: list[str]) -> str:
    return subprocess.check_output(args, env=dict(os.environ), text=True, timeout=600).strip()

def prepare(name: str, pin: dict, root: Path) -> None:
    destination = root / pin.get("directory", name)
    if "url" in pin:
        archive = root / pin["url"].rsplit("/", 1)[1]
        if not archive.exists():
            with urllib.request.urlopen(pin["url"], timeout=60) as response, archive.open("xb") as output:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
        if sha256(archive) != pin["sha256"]:
            raise RuntimeError(f"{name}: archive hash mismatch; preserve the failed download and use a fresh root")
        marker = destination / ".c11-archive-sha256"
        if destination.exists() and not marker.exists():
            raise RuntimeError("existing unmarked source directory: use a fresh root")
        if not destination.exists():
            with tarfile.open(archive) as source:
                # data filter rejects traversal, external links and special devices.
                source.extractall(root, filter="data")
        if marker.exists() and marker.read_text().strip() != pin["sha256"]:
            raise RuntimeError("existing archive marker differs")
        marker.write_text(pin["sha256"] + "\n")
    else:
        if not destination.exists():
            command(["git", "clone", "--depth", "1", "--branch", pin["tag"], pin["repository"], str(destination)])
        actual = command(["git", "-C", str(destination), "rev-parse", "HEAD"])
        if actual != pin["commit"]:
            raise RuntimeError(f"{name}: expected {pin['commit']}, found {actual}")
        if pin.get("submodules"):
            command(["git", "-C", str(destination), "submodule", "update", "--init", "--recursive", "--depth", "1", "--", *pin["submodules"]])
        if command(["git", "-C", str(destination), "status", "--porcelain", "--untracked-files=no"]):
            raise RuntimeError(f"{name}: tracked source modifications present")
    print(f"{name}: {destination} ({pin['commit']})", flush=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("names", nargs="+")
    args = parser.parse_args()
    pins = json.loads((Path(__file__).resolve().parents[2] / "references/dependencies.json").read_text())
    if any(name not in pins for name in args.names):
        parser.error("unknown dependency")
    args.root.mkdir(parents=True, exist_ok=True)
    for name in args.names:
        prepare(name, pins[name], args.root.resolve())
