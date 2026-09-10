# C08 WSL Setup Record - 2026-09-10

Scope: dedicated WSL2 guest for C08 Linux validation. No Windows global settings, default distro command, other distro changes, repo commit, or push were performed.

## Host Registration

- Distro name: `LearnCPP-C08-Ubuntu-24.04`
- Registration location: `H:\wsl\LearnCPP-C08-Ubuntu-24.04`
- Download cache: `H:\wsl\downloads`
- Image URL: `https://releases.ubuntu.com/noble/ubuntu-24.04.4-wsl-amd64.wsl`
- SHA256 source: `https://releases.ubuntu.com/noble/SHA256SUMS`
- Image size: `391541571` bytes
- Expected SHA256: `9b2f7730dc68227dd04a9f3e5eab86ad85caf556b8606ad94f1f29ff5c4fd3f5`
- Actual SHA256: `9b2f7730dc68227dd04a9f3e5eab86ad85caf556b8606ad94f1f29ff5c4fd3f5`
- Match: `True`

Install command:

```powershell
wsl --install --from-file H:\wsl\downloads\ubuntu-24.04.4-wsl-amd64.wsl --name LearnCPP-C08-Ubuntu-24.04 --location H:\wsl\LearnCPP-C08-Ubuntu-24.04 --no-launch
```

`wsl --list --verbose` after setup:

```text
NAME                         STATE    VERSION
LearnCPP-C08-Ubuntu-24.04    Running  2
```

Because this machine had no registered distros before this setup, WSL marks this only distro with `*`. No explicit `wsl --set-default` command was run.

## Guest Environment

Kernel:

```text
Linux lizhidan-p02 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
```

OS:

```text
Ubuntu 24.04.4 LTS (Noble Numbat)
VERSION_ID=24.04
VERSION_CODENAME=noble
```

Compiler and tool versions:

```text
g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Ubuntu clang version 18.1.3 (1ubuntu1)
cmake version 3.28.3
ninja-build 1.11.1
Python 3.12.3
git version 2.43.0
```

Installed package versions:

```text
g++ 4:13.2.0-7ubuntu1
clang-18 1:18.1.3-1ubuntu1
cmake 3.28.3-1build7
ninja-build 1.11.1-2
python3 3.12.3-0ubuntu2.1
git 1:2.43.0-1ubuntu7.3
libclang-rt-18-dev 1:18.1.3-1ubuntu1
```

Guest working directories created for later validation:

```text
/root/learncpp-c08-src
/root/learncpp-c08-builds
```

## Probe Results

Probe command:

```powershell
wsl -d LearnCPP-C08-Ubuntu-24.04 --user root -- bash /mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/wsl-setup/run-probes.sh
```

Output:

```text
sum=4
sum=4
tsan_clean_rc=0
tsan_race_rc=66
tsan_race_warning=detected
```

Verdict:

- `g++ -std=c++23 -pthread` builds and runs the C++23 thread/barrier probe.
- `clang++-18 -std=c++23 -pthread` builds and runs the same C++23 probe.
- `clang++-18 -fsanitize=thread` accepts the clean atomic/thread probe.
- `clang++-18 -fsanitize=thread` reports the intentional data race.

This validates the guest as usable for later C08 Linux builds and TSan calibration. It does not validate the full C08 course tree.
