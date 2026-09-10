# C05 text/time review validation 2026-09-10

Reviewer scope: `include/c05/{text,time,paths,config,utf}.hpp`, exercises `L07`-`L13`, `U01_icu_unicode`, `F01_frontier`, and chapters `07`-`13`/`18`.

Repository HEAD during review: `f261bea559d6722c31135fb2d3589be52fe958ed`.

Tool gap: `lsp_diagnostics` and `ast_grep_search` were searched but not available in this Codex surface. Replacement checks used fresh MSVC/CMake/CTest builds, direct executable runs, and `rg` pattern scans for broad catch/fallback, global locale/TZ mutation, filesystem identity probes, and hardcoded secrets.

## Fresh commands

All commands were run from `F:\CPPTrain\LearnCPP` unless noted.

```powershell
foreach ($u in 'L07_text_semantics','L08_parsing','L09_formatting','L10_clocks_durations','L11_calendar_zones','L12_paths','L13_configuration') {
  cmake -S C05_Data_Representation_Standard_Facilities/exercises/$u -B C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/$u -G "Visual Studio 18 2026" -A x64
  cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/$u --config Release --parallel 2
  ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/$u -C Release --output-on-failure
}

cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/F01-off -G "Visual Studio 18 2026" -A x64
ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/F01-off -N

cmake -S C05_Data_Representation_Standard_Facilities/exercises/F01_frontier -B C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/F01-on -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_ENABLE_FRONTIER=ON
cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/F01-on --config Release --parallel 2
ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/F01-on -C Release --output-on-failure

foreach ($cfg in 'Release','Debug') {
  cmake -S C05_Data_Representation_Standard_Facilities/exercises/U01_icu_unicode -B C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/U01-$($cfg.ToLower()) -G "Visual Studio 18 2026" -A x64 -DDATA_STUDY_ENABLE_ICU=ON
  cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/U01-$($cfg.ToLower()) --config $cfg --parallel 2
  ctest --test-dir C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/U01-$($cfg.ToLower()) -C $cfg --output-on-failure
}
```

Extra contract probe:

```powershell
cmake -S C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/contract-probe-r2 -B C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/contract-probe-r2/build -G "Visual Studio 18 2026" -A x64
cmake --build C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/contract-probe-r2/build --config Release --parallel 2
.\C05_Data_Representation_Standard_Facilities\exercises\build\text-time-review\contract-probe-r2\build\Release\contract_probe.exe
```

## Results

- L07 Release: 2/2 passed. Observation plus `bad_identity` negative control.
- L08 Release: 3/3 passed. Reference, independent good, and bad trailing-consume control.
- L09 Release: 2/2 passed. Observation plus `format_to_n` truncation negative control.
- L10 Release: 1/1 passed.
- L11 Release: 2/2 passed. UTC test and tzdb test both ran, no SKIP.
- L12 Release: 1/1 passed.
- L13 Release: 3/3 passed. Reference, independent good, and duplicate-key bad control.
- F01 OFF: configure succeeded and `ctest -N` reported `Total Tests: 0`.
- F01 ON: configure/build succeeded; 9 capability tests registered, each returned 77 with a precise unsupported-capability reason.
- U01 Release direct run: exit 0, `ICU 77.1 Unicode 16.0`, `NormalizationTest records 19965`, `GraphemeBreakTest records 1093`.
- U01 Debug direct run: exit 0, same ICU/Unicode versions and record counts.
- L11 tzdb direct run: exit 0, `tzdb version: 2022g.27`.
- Extra contract probe: exit 0, `contract probe passed`.

Raw captured outputs:

- `C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/text-time-review-run-20260909-r2.json`
- `C05_Data_Representation_Standard_Facilities/exercises/build/text-time-review/text-time-review-direct-20260909-r1.json`

## Source hashes

```text
C05_Data_Representation_Standard_Facilities/exercises/include/c05/text.hpp d8dd1fb751eaca98f3de2d7da6087cbfd304bff9be8e19e6a73b5f1858c8d50c
C05_Data_Representation_Standard_Facilities/exercises/include/c05/time.hpp aed042d3b6399ce3879019dec85ab6ea8187aa04994b439a35c8ccb92fd9f750
C05_Data_Representation_Standard_Facilities/exercises/include/c05/paths.hpp 6bfca0c4a15f896ecc5a8497e5e3ee4a15d9760cab96f67c4a6446e52517c1c0
C05_Data_Representation_Standard_Facilities/exercises/include/c05/config.hpp 773adf8948fc43e8b8079507f8e42d33e226de3680ab9c41a41f5dff17a3627b
C05_Data_Representation_Standard_Facilities/exercises/include/c05/utf.hpp 36f6b4f3b99d0eba3e62669e18e7026fa8171ea913b768b8c7469835c2f1b9f6
C05_Data_Representation_Standard_Facilities/exercises/U01_icu_unicode/checks/icu_unicode_checks.cpp aeaf29fe75195c1bd2701007b53d796dd6057018e40cd30de52a6121d6aefebe
C05_Data_Representation_Standard_Facilities/exercises/F01_frontier/CMakeLists.txt 37f87bac294b4d4e68a37763bb64690362f9cb91edfaa59bdac9e968a4c7e365
C05_Data_Representation_Standard_Facilities/chapters/07-unicode-text-semantics.md 134477c45b8468f3b648941e8c15d2cda8380c56ccacefc6f43a078eefc07bdf
C05_Data_Representation_Standard_Facilities/chapters/08-parsing-and-charconv.md 070cdfe521e38ccb602a37e0246d7e3e788af90b15840f3961db345895354e4c
C05_Data_Representation_Standard_Facilities/chapters/09-formatting-and-locales.md 4e0c9ff984de329eecd0b331ebaba963ca161f77a822fef99117d1cf550558a6
C05_Data_Representation_Standard_Facilities/chapters/10-clocks-and-durations.md cf8f9997634edcc88cb38eee67388a291107ce9238968f90a24285d2a5a0dd4d
C05_Data_Representation_Standard_Facilities/chapters/11-calendars-and-time-zones.md a8dbd9243313d8083c8b2d90386c10c3df891c7f8bf9442458fd405163a222d5
C05_Data_Representation_Standard_Facilities/chapters/12-filesystem-paths.md e283f92b6435dbbfd1ac94101bb8a8062ee94b88dc0f5c3ab8c447279c89799a
C05_Data_Representation_Standard_Facilities/chapters/13-configuration.md b1f47713ee89a4310e9afa47a3dbacdc275447405e3a8e579f8f83b2aecf544e
C05_Data_Representation_Standard_Facilities/chapters/18-standard-frontier.md dd947d1d5b14dd0d61414a43d072a3841f9f62107adcdf446c42d50380797198
```

## Notes

Generated directories found under `exercises/build` and several leaf `build` folders are ignored by Git (`git status --short --ignored` reports them as `!!`). They are not normal add targets.
