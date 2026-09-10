# C05 time/path/config r2 minimal repro 2026-09-09

New checks were added before fixing production/reference code.

Command from `C05_Data_Representation_Standard_Facilities/exercises`:

```powershell
foreach ($u in 'L13_configuration','L11_calendar_zones') {
  $cfg='Release'
  $b = "build/repro-$u-$cfg"
  cmake -S $u -B $b -G "Visual Studio 18 2026" -A x64
  cmake --build $b --config $cfg --parallel 2
  ctest --test-dir $b -C $cfg --output-on-failure
}
```

Observed failures before fix:

```text
L13_configuration_reference: check failed: nul reports line
L13_configuration_validation_good: check failed: nul reports line
L11_calendar_zones_tzdb: check failed: prints non-minute historical offsets with seconds
```

The added checks also cover:

- line-internal CR and bare trailing CR rejection at the CR byte.
- invalid UTF-8 and NUL line number derivation.
- Win32 device basenames with superscript COM digit and device stem trailing space before extension.
- local display overflow outside the manifest calendar range.

This file records the failing reproduction only. r2 fix verification is in the sibling r2 verification record.
