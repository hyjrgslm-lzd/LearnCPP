# C05 time/path/config author r2 verification 2026-09-09

Fixes covered:

- Config CR handling: only CR before an actual LF is consumed as CRLF; remaining CR bytes are rejected at the CR offset.
- Config UTF-8 and NUL errors now derive 1-based line from byte offset. Global file-size budget remains allowed to report line 0.
- `package_file` device detection uses ASCII folding, rejects Microsoft Win32 reserved COM/LPT superscript digit aliases, and rejects device stems ending in space or dot before extension.
- `format_timestamp` catches only tzdb `runtime_error` while locating a zone; formatting and allocation errors are outside that catch.
- Timestamp formatting uses classic locale, prints offset seconds when nonzero, and rejects local display dates outside years 0001-9999.

Microsoft reference checked for reserved Win32 names:

- https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file

Fresh commands from `C05_Data_Representation_Standard_Facilities/exercises`:

```powershell
foreach ($u in 'L13_configuration','L11_calendar_zones') {
  foreach ($cfg in 'Debug','Release') {
    $b = "build/r2-$u-$cfg"
    cmake -S $u -B $b -G "Visual Studio 18 2026" -A x64
    cmake --build $b --config $cfg --parallel 2
    ctest --test-dir $b -C $cfg --output-on-failure
  }
}
foreach ($u in 'L10_clocks_durations','L12_paths') {
  foreach ($cfg in 'Debug','Release') {
    $b = "build/r2-$u-$cfg"
    cmake -S $u -B $b -G "Visual Studio 18 2026" -A x64
    cmake --build $b --config $cfg --parallel 2
    ctest --test-dir $b -C $cfg --output-on-failure
  }
}
.\build\r2-L11_calendar_zones-Release\Release\L11_calendar_zones_tzdb.exe
```

Observed result:

- L10 Debug: 1/1 passed.
- L10 Release: 1/1 passed.
- L11 Debug: 2/2 passed.
- L11 Release: 2/2 passed.
- L12 Debug: 1/1 passed.
- L12 Release: 1/1 passed.
- L13 Debug: 3/3 passed.
- L13 Release: 3/3 passed.
- tzdb runtime version: 2022g.27.

Limits:

- Author leaf r2 verification only.
- Root aggregate build and independent review remain outside this author slice.
