"""Parent-owned real crashes and a separate persisted-byte model; never a power-cut claim."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import zlib
import struct
import json
if not __debug__:raise RuntimeError("checks require Python without -O")

EXE = str(Path(sys.argv[1]).resolve())
FLAGS = {"creationflags": subprocess.CREATE_NO_WINDOW} if os.name == "nt" else {}

def run(*args, expected=0):
    result = subprocess.run([EXE, *map(str,args)], capture_output=True, text=True, timeout=15, **FLAGS)
    assert result.returncode == expected, (args,result.returncode,result.stdout,result.stderr)
    assert not result.stderr, result.stderr
    print(json.dumps({"event":"command","case":Path(str(args[1])).name,"mode":args[0],"exit":result.returncode,"observed":result.stdout.strip()}),flush=True)
    return result.stdout

def crash(directory, mode, label, point):
    child = subprocess.Popen([EXE,mode,str(directory),label,point], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,**FLAGS)
    try:
        # The outer C07 supervisor bounds this read and every descendant.
        ready=child.stdout.readline().strip()
        assert ready == "READY "+point
        print(json.dumps({"event":"fault_ready","case":directory.name,"observed":ready}),flush=True)
        child.kill()
        out,err = child.communicate(timeout=10)
        assert child.returncode != 0 and not err, (out,err)
    finally:
        if child.poll() is None:
            child.kill()
            child.communicate(timeout=5)

def inspect(directory, committed, expected_lsn, snapshot=0, rejected=0, repaired=0):
    output = run("inspect",directory)
    fields=output.splitlines()[0].split()
    metadata=dict(zip(fields[::2],map(int,fields[1::2])))
    assert metadata=={"LSN":expected_lsn,"SNAP":snapshot,"REPAIRED":repaired,"BAD":rejected},metadata
    actual = dict(line.split("=",1) for line in output.splitlines()[1:])
    expected = {label+"."+str(n): ("one" if n==1 else "two") if label in committed else "<absent>"
                for label in "ABC" for n in (1,2)}
    assert actual == expected, (actual,expected)
    return output

with tempfile.TemporaryDirectory(prefix="c12-recovery-") as temporary:
    root=Path(temporary)
    base=root/"base"
    assert run("batch",base,"A") == "ACK 1\n"
    raw_a=(base/"journal.bin").read_bytes()
    assert run("batch",base,"B") == "ACK 2\n"
    raw_ab=(base/"journal.bin").read_bytes()
    raw_b=raw_ab[len(raw_a):]
    # Independent Python header/checksum verification, not encode->decode alone.
    for data,sequence in ((raw_a,1),(raw_b,2)):
        magic,version,lsn,length,header_crc=struct.unpack(">IIQII",data[:24])
        assert (magic,version,lsn)==(0x4331324c,1,sequence)
        assert zlib.crc32(data[:20])==header_crc
        assert len(data)==length+32 and zlib.crc32(data[24:-8])==struct.unpack(">I",data[-8:-4])[0]
        assert data[-4:]==b"CMIT"
    for point in ("half_record","before_flush","after_flush"):
        db=root/point
        assert run("batch",db,"A")=="ACK 1\n"
        crash(db,"batch","B",point)
        # On process termination, completed synchronous writes remain in the OS cache.
        b_present=point!="half_record"
        inspect(db,{"A","B"} if b_present else {"A"},2 if b_present else 1,repaired=0 if b_present else len(raw_b)//2)
        expected=3 if b_present else 2
        assert run("batch",db,"C")==f"ACK {expected}\n"
        inspect(db,{"A","B","C"} if b_present else {"A","C"},expected)
    for point in ("half_snapshot","snapshot_flushed"):
        db=root/point
        run("batch",db,"A");run("checkpoint",db);run("batch",db,"B")
        crash(db,"checkpoint","unused",point)
        expected_snapshot=1 if point=="half_snapshot" else 2
        bad=int(point=="half_snapshot")
        inspect(db,{"A","B"},2,snapshot=expected_snapshot,rejected=bad)
        run("batch",db,"C");inspect(db,{"A","B","C"},3,snapshot=expected_snapshot,rejected=bad)
    # Persisted-byte model: A is already stable. B may persist as any prefix.
    # This models a declared crash/storage contract, not actual machine power failure.
    for cut in range(len(raw_b)+1):
        db=root/f"prefix-{cut}";db.mkdir()
        (db/"journal.bin").write_bytes(raw_a+raw_b[:cut])
        complete=cut==len(raw_b)
        inspect(db,{"A","B"} if complete else {"A"},2 if complete else 1,repaired=0 if complete else cut)
        run("batch",db,"C")
        inspect(db,{"A","B","C"} if complete else {"A","C"},3 if complete else 2)
    for offset in (0,19,24,len(raw_a)-5,len(raw_a)+24):
        db=root/f"corrupt-{offset}";db.mkdir()
        broken=bytearray(raw_ab);broken[offset]^=1
        (db/"journal.bin").write_bytes(broken)
        assert run("inspect",db,expected=3).startswith("ERROR 3 ")
        assert (db/"journal.bin").read_bytes()==broken,"corruption must not trigger tail repair"
    db=root/"bad-snapshot"
    run("batch",db,"A");run("checkpoint",db);run("batch",db,"B")
    damaged=bytearray((db/"snapshot0.bin").read_bytes());damaged[-5]^=1
    (db/"snapshot0.bin").write_bytes(damaged)
    inspect(db,{"A","B"},2,rejected=1)
print("real process crashes, every modeled tail prefix, corruption, and resume checks passed")
