"""Complete Part 5 reference: a long incomplete B followed by a shorter committed C."""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
if not __debug__:raise RuntimeError("checks require Python without -O")
exe=str(Path(sys.argv[1]).resolve())
flags={"creationflags":subprocess.CREATE_NO_WINDOW} if os.name=="nt" else {}
def run(*args):
    p=subprocess.run([exe,*map(str,args)],capture_output=True,text=True,timeout=15,**flags)
    assert p.returncode==0 and not p.stderr,(args,p.returncode,p.stdout,p.stderr)
    print(json.dumps({"case":Path(str(args[1])).name,"mode":args[0],"observed":p.stdout.strip()}),flush=True)
    return p.stdout
with tempfile.TemporaryDirectory(prefix="c12-extension-") as temporary:
    root=Path(temporary);base=root/"source";long_label="B"*120
    assert run("batch",base,"A")=="ACK 1\n"
    a=(base/"journal.bin").read_bytes()
    assert run("batch",base,long_label)=="ACK 2\n"
    b=(base/"journal.bin").read_bytes()[len(a):]
    for cut in (1,23,24,len(b)//2,len(b)-1,len(b)):
        db=root/str(cut);db.mkdir();(db/"journal.bin").write_bytes(a+b[:cut])
        complete=cut==len(b)
        assert run("get",db,long_label+".1")==("VALUE one\n" if complete else "ABSENT\n")
        assert run("get",db,long_label+".2")==("VALUE two\n" if complete else "ABSENT\n")
        assert run("batch",db,"C")==("ACK 3\n" if complete else "ACK 2\n")
        assert run("get",db,"A.1")=="VALUE one\n"
        assert run("get",db,"C.2")=="VALUE two\n"
print("long-tail then shorter append reference passed")
