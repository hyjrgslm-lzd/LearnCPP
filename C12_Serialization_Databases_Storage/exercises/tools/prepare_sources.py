"""Explicit dependency preparation. CMake never downloads. Evidence stays under --root."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import stat
import urllib.request
import zipfile

def digest(path,algorithm):
    with path.open("rb") as source:
        return hashlib.file_digest(source,algorithm).hexdigest()

def prepare(name,pin,root):
    if "directory" not in pin:
        raise ValueError(name+": reuse the declared local source checkout; no implicit clone")
    archive=root/pin["url"].rsplit("/",1)[1]
    if not archive.exists():
        partial=archive.with_suffix(archive.suffix+".part")
        with urllib.request.urlopen(pin["url"],timeout=45) as response,partial.open("xb") as output:
            while chunk:=response.read(1024*1024):
                output.write(chunk)
        if partial.stat().st_size!=pin["size"]:
            raise RuntimeError("download length differs; preserve partial and use a fresh root")
        if pin.get("sha3_256") and digest(partial,"sha3_256")!=pin["sha3_256"]:
            raise RuntimeError("publisher SHA3 mismatch; preserve download")
        partial.replace(archive)
    if archive.stat().st_size!=pin["size"]:
        raise RuntimeError("archive length differs")
    if pin.get("sha3_256") and digest(archive,"sha3_256")!=pin["sha3_256"]:
        raise RuntimeError("publisher SHA3 mismatch")
    local_sha=digest(archive,"sha256")
    if pin.get("sha256") and local_sha!=pin["sha256"]:
        raise RuntimeError("archive differs from the pinned artifact fingerprint")
    destination=root/pin["directory"]
    marker=destination/".c12-origin.json"
    if destination.exists():
        if not marker.exists() or json.loads(marker.read_text())["sha256"]!=local_sha:
            raise RuntimeError("existing source lacks matching origin; use a fresh root")
    else:
        with zipfile.ZipFile(archive) as source:
            for entry in source.infolist():
                target=(root/entry.filename).resolve()
                if not target.is_relative_to(root) or stat.S_ISLNK(entry.external_attr>>16):
                    raise RuntimeError("unsafe archive member")
            source.extractall(root)
        marker.write_text(json.dumps({"url":pin["url"],"version":pin["version"],"sha256":local_sha,
                                     "publisher_checksum_verified":bool(pin.get("sha3_256"))},indent=2))
    print(json.dumps({"name":name,"source":str(destination),"sha256":local_sha,
                      "publisher_checksum_verified":bool(pin.get("sha3_256"))}),flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root",type=Path,required=True)
    parser.add_argument("names",nargs="+")
    args=parser.parse_args()
    pins=json.loads((Path(__file__).resolve().parents[2]/"references/dependencies.json").read_text())
    if any(name not in ("sqlite","sqlite_source","postgres") for name in args.names):
        parser.error("download names are sqlite, sqlite_source, postgres")
    root=args.root.resolve();root.mkdir(parents=True,exist_ok=True)
    for name in args.names:prepare(name,pins[name],root)
