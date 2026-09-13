"""Crash a real SQLite migration at controlled transaction boundaries."""
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
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
    print(json.dumps({"event":"command","case":Path(str(args[1])).parent.name,"mode":args[0],"observed":p.stdout.strip()}),flush=True)
    return p.stdout
def inspect(path,version):
    actual=dict(part.split("=") for part in run("inspect",path).split())
    assert actual=={"version":str(version),"columns":str(2 if version==1 else 3),"rows":"1","legacy":str(int(version==2))},actual
with tempfile.TemporaryDirectory(prefix="c12-sqlite-crash-") as temporary:
    for journal in ("delete","wal"):
        for point in ("after_ddl","after_commit"):
            root=Path(temporary)/(journal+"-"+point);root.mkdir();db=root/"migration.db"
            run("init",db,journal)
            process=subprocess.Popen([exe,"migrate",str(db),point],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE,text=True,**flags)
            try:
                ready=process.stdout.readline().strip()
                assert ready=="READY "+point,ready
                print(json.dumps({"event":"kill","journal":journal,"observed":ready}),flush=True)
                process.kill();out,err=process.communicate(timeout=10)
                assert process.returncode!=0 and not err,(out,err)
            finally:
                if process.poll() is None:process.kill();process.communicate(timeout=5)
            inspect(db,1 if point=="after_ddl" else 2)
            assert run("migrate",db)=="ACK 2\n"
            inspect(db,2)
            assert run("migrate",db)=="ACK 2\n"
            inspect(db,2)
print("SQLite rollback-journal and WAL migration recovery checks passed")
