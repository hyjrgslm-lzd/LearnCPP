"""Own one temporary PostgreSQL cluster and every process it starts. Never register a service."""
from pathlib import Path
import argparse
import json
import os
import secrets
import socket
import subprocess
import tempfile
import time
import sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[3]/"C01_Build_Compile_Link/exercises/tools"))
from process_runner import run_process

FLAGS={"creationflags":subprocess.CREATE_NO_WINDOW} if os.name=="nt" else {}

class Cluster:
    def __init__(self,distribution):
        self.distribution=Path(distribution).resolve()
        self.temporary=None;self.server=None;self.log=None
    def command(self,args,timeout=30):
        result=run_process(list(map(str,args)),timeout,env=self.env)
        if result["status"]!="PASS":
            raise RuntimeError(f"command {Path(str(args[0])).name} failed: {result}")
        return result["stdout"]
    def __enter__(self):
        self.temporary=tempfile.TemporaryDirectory(prefix="c12-postgres-")
        self.directory=Path(self.temporary.name);self.data=self.directory/"data"
        self.env={key:value for key,value in os.environ.items() if not key.upper().startswith("PG")}
        self.env.update(PATH=str(self.distribution/"bin")+os.pathsep+self.env.get("PATH",""),
                        PGUSER="c12",PGDATABASE="postgres",PGHOST="127.0.0.1",PGSSLMODE="disable",
                        PGCLIENTENCODING="UTF8",PGCONNECT_TIMEOUT="3",PGPASSWORD=secrets.token_urlsafe(24),
                        LC_ALL="C")
        password=self.directory/"init-password"
        password.write_text(self.env["PGPASSWORD"]+"\n",encoding="utf-8")
        try:
            version=self.command([self.distribution/"bin/initdb","--version"])
            if "18.6" not in version:raise RuntimeError("expected PostgreSQL 18.6")
            self.command([self.distribution/"bin/initdb","-D",self.data,"-U","c12","--auth=scram-sha-256",
                          "--pwfile",password,"--encoding=UTF8","--locale=C"],timeout=60)
            password.unlink()
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1",0));port=reservation.getsockname()[1]
            self.env["PGPORT"]=str(port)
            self.log=(self.directory/"server.log").open("w",encoding="utf-8")
            self.server=subprocess.Popen([str(self.distribution/"bin/postgres"),"-D",str(self.data),
                                          "-h","127.0.0.1","-p",str(port),"-c","fsync=on","-c","synchronous_commit=on"],
                                         env=self.env,stdout=self.log,stderr=subprocess.STDOUT,**FLAGS)
            deadline=time.monotonic()+20
            while time.monotonic()<deadline:
                if self.server.poll() is not None:raise RuntimeError("private PostgreSQL exited during startup")
                ready=subprocess.run([str(self.distribution/"bin/pg_isready"),"-q"],env=self.env,
                                     capture_output=True,timeout=4,**FLAGS)
                if ready.returncode==0:break
                time.sleep(.05)
            else:raise TimeoutError("PostgreSQL readiness timeout")
            self.command([self.distribution/"bin/psql","-X","-A","-t","-c","SELECT 1"])
            print(json.dumps({"event":"private_postgres_ready","version":version.strip(),"host":"127.0.0.1","port":port}),flush=True)
            return self
        except BaseException:
            self.close()
            raise
    def close(self):
        problem=None
        try:
            if self.server is not None and self.server.poll() is None:
                try:
                    self.command([self.distribution/"bin/pg_ctl","stop","-D",self.data,"-m","fast","-w","-t","10"],timeout=15)
                    self.server.wait(timeout=10)
                except BaseException as error:
                    problem=error
                    if self.server.poll() is None:
                        if os.name=="nt":
                            stopped=subprocess.run(["taskkill","/PID",str(self.server.pid),"/T","/F"],
                                                   capture_output=True,timeout=8,**FLAGS)
                            if stopped.returncode:problem=RuntimeError("private process tree cleanup unconfirmed")
                        else:
                            self.server.kill()
                        self.server.wait(timeout=5)
            if self.server is not None and self.server.returncode not in (None,0):
                problem=problem or RuntimeError(f"private PostgreSQL exit {self.server.returncode}")
        finally:
            if self.log is not None:
                self.log.flush()
                if problem:
                    print(json.dumps({"event":"postgres_failure_log","text":(self.directory/"server.log").read_text(errors="replace")}),flush=True)
                self.log.close();self.log=None
            if self.temporary is not None:
                self.temporary.cleanup();self.temporary=None
        if problem:raise problem
    def __exit__(self,*exc):
        self.close()
        print(json.dumps({"event":"private_postgres_stopped","cleanup_confirmed":True}),flush=True)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root",type=Path,required=True)
    parser.add_argument("--client",type=Path)
    parser.add_argument("--case",default="all")
    args=parser.parse_args()
    with Cluster(args.root) as cluster:
        if args.client:
            print(cluster.command([args.client.resolve(),args.case],timeout=90),end="")
        else:
            print(cluster.command([args.root.resolve()/"bin/psql","-X","-A","-t","-c",
                                   "SELECT current_user, current_setting('server_version'), current_setting('fsync')"]),end="")
