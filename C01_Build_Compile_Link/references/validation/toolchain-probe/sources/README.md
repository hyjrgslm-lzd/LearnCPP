# Toolchain probe source archive

This directory contains the minimal source inputs used by the C01 toolchain probe.

Included:

- `cmake-plain/`: plain CMake executable used to prove compiler detection/build.
- `msvc-named-modules/`: named module interface and importer.
- `msvc-import-std/`: CMake `import std` target with the CMake 4.2.3 experimental gate.
- `msvc-module-install/`: module library install/export and independent consumer source.
- `clang-tools/`: ASan, ASan detection, libFuzzer, and `-ftime-trace` probes.

Excluded:

- build trees under `out/`
- CMake cache files
- compiler generated BMI/IFC/object/PDB/exe/lib files

Hashes are recorded in `SHA256SUMS.txt`.

Rebuild the full normal CMake module chain from the repository root:

```cmd
cmd.exe /d /c Engineering_Study\references\validation\toolchain-probe\400-normal-cmake-chain.command.cmd
```

That command uses the local VS x64 environment, CMake 4.2.3, and VS Ninja 1.13.2. It writes raw stdout/stderr beside the command file.
