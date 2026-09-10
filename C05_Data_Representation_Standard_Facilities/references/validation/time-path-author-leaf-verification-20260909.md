# C05 time/path/config author leaf verification 2026-09-09

Scope owned by this author:

- chapters/10-clocks-and-durations.md
- chapters/11-calendars-and-time-zones.md
- chapters/12-filesystem-paths.md
- chapters/13-configuration.md
- exercises/L10_clocks_durations
- exercises/L11_calendar_zones
- exercises/L12_paths
- exercises/L13_configuration
- exercises/include/c05/paths.hpp
- exercises/include/c05/time.hpp
- exercises/include/c05/config.hpp

Environment observed:

- Generator: Visual Studio 18 2026, x64
- Compiler: MSVC 19.51.36256.0 from v145 toolset path shown by CMake
- MSBuild: 18.9.1+a81b43525
- tzdb runtime version: 2022g.27

Commands run from `C05_Data_Representation_Standard_Facilities/exercises`:

```powershell
$units = 'L10_clocks_durations','L11_calendar_zones','L12_paths','L13_configuration'
foreach ($u in $units) {
  foreach ($cfg in 'Debug','Release') {
    $b = "build/leaf-$u-$cfg"
    cmake -S $u -B $b -G "Visual Studio 18 2026" -A x64
    cmake --build $b --config $cfg --parallel 2
    ctest --test-dir $b -C $cfg --output-on-failure
  }
}
```

Result:

- L10 Debug: 1/1 passed.
- L10 Release: 1/1 passed.
- L11 Debug: 2/2 passed; UTC observation and tzdb capability both ran.
- L11 Release: 2/2 passed; UTC observation and tzdb capability both ran.
- L12 Debug: 1/1 passed.
- L12 Release: 1/1 passed.
- L13 Debug: 3/3 passed; reference, validation_good, and validation_bad_rejected.
- L13 Release: 3/3 passed; reference, validation_good, and validation_bad_rejected.
- After replacing L13 validation_good and validation_bad with independent implementations, L13 Debug and Release were rerun; both stayed 3/3 passed. validation_bad rejects through expected diagnostic `duplicate package_file`.

Additional command:

```powershell
.\build\leaf-L11_calendar_zones-Release\Release\L11_calendar_zones_tzdb.exe
```

Output:

```text
tzdb version: 2022g.27
```

Limits:

- This is author leaf verification only, not independent review.
- L11 tzdb capability passed on this machine; other machines without tzdb should report capability skip via return code 77.
- No root aggregate C05 build was changed or verified by this author.
