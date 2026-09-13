"""Build a supplied OpenSSL checkout into a private prefix, without system changes."""
import argparse
import os
from pathlib import Path
import subprocess
import shutil

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "build", "prefix", "perl"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--config", choices=["Debug", "Release"], default="Release")
    args = parser.parse_args()
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    installation = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True, env=dict(os.environ)).strip()
    vcvars = Path(installation) / "VC/Auxiliary/Build/vcvars64.bat"
    raw = subprocess.check_output(f'cmd.exe /d /s /c ""{vcvars}" >nul && set"', env=dict(os.environ), text=True)
    env = dict(os.environ)
    for line in raw.splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            env[key.upper()] = value
    env["PATH"] = str(args.perl.resolve().parent) + os.pathsep + env["PATH"]
    env.update(LC_ALL="C", LC_CTYPE="C", LANG="C", VSLANG="1033")
    nmake = shutil.which("nmake.exe", path=env["PATH"])
    if not nmake: raise RuntimeError("vcvars64 did not provide nmake.exe")
    args.build.mkdir(parents=True, exist_ok=True)
    prefix = args.prefix.resolve()
    configure = [str(args.perl.resolve()), str(args.source.resolve() / "Configure"), "VC-WIN64A",
                 "no-shared", "no-tests", "no-asm", "no-makedepend", "--libdir=lib",
                 f"--prefix={prefix}", f"--openssldir={prefix / 'ssl'}"]
    # ponytail: no-asm avoids another bootstrap tool; enable ASM before measuring crypto throughput.
    if args.config == "Debug": configure.append("--debug")
    subprocess.run(configure, cwd=args.build, env=env, check=True, timeout=120)
    subprocess.run([nmake, "/nologo", "install_sw"], cwd=args.build, env=env, check=True, timeout=1800)
