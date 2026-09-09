$ErrorActionPreference='Stop'
$root=(Resolve-Path '.').Path
$ev=Join-Path $root 'Core_Study/references/validation/reviews/c02-0810-r2-verifier-evidence-r1'
$rec=Join-Path $root 'Core_Study/exercises/tools/record_process.py'
# L10 default smoke with r2 checker containing nested RHS assignment.
$l10='build/c02-0810-r2-verifier-r1/L10-default'
python $rec --output "$ev/l10-configure-default-r2.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S Core_Study/exercises/L10_control_block -B $l10 -G 'Visual Studio 18 2026' -A x64 -DL10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS=ON
foreach($cfg in @('Debug','Release')){
  python $rec --output "$ev/l10-build-default-$($cfg.ToLower())-r2.json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $l10 --config $cfg --clean-first
  python $rec --output "$ev/l10-ctest-default-$($cfg.ToLower())-r2.json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $l10 -C $cfg --output-on-failure
}
# L09 full impacted matrix.
$l09='build/c02-0810-r2-verifier-r1/L09-default'
python $rec --output "$ev/l09-configure-default-r2.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S Core_Study/exercises/L09_shared -B $l09 -G 'Visual Studio 18 2026' -A x64 -DL09_SHARED_BUILD_VALIDATION_VARIANTS=ON
foreach($cfg in @('Debug','Release')){
  python $rec --output "$ev/l09-build-default-$($cfg.ToLower())-r2.json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $l09 --config $cfg --clean-first
  python $rec --output "$ev/l09-ctest-default-$($cfg.ToLower())-r2.json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $l09 -C $cfg --output-on-failure
  foreach($run in @(
    @{target='L09_shared_validation_good'; exit=0; text='L09_shared_validation_contract OK'},
    @{target='L09_shared_validation_bad_hardcoded_noop'; exit=1; text='parent owns child through shared next'},
    @{target='L09_shared_validation_bad_cycle'; exit=1; text='weak backedge does not keep cycle alive'},
    @{target='L09_shared_validation_bad_weak_resurrect'; exit=1; text='lock fails after destruction'}
  )){
    $exe=Join-Path $root (Join-Path $l09 "$cfg/$($run.target).exe")
    python $rec --output "$ev/l09-run-$($run.target)-$($cfg.ToLower())-r2.json" --expect-exit $run.exit --contains $run.text -- $exe
  }
}
$l09s='build/c02-0810-r2-verifier-r1/L09-student-ref-off'
python $rec --output "$ev/l09-configure-student-ref-off-r2.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S Core_Study/exercises/L09_shared -B $l09s -G 'Visual Studio 18 2026' -A x64 -DCORE_STUDY_BUILD_REFERENCE=OFF -DCORE_STUDY_TEST_STUDENTS=ON
foreach($cfg in @('Debug','Release')){
  python $rec --output "$ev/l09-build-student-ref-off-$($cfg.ToLower())-r2.json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $l09s --config $cfg --clean-first --target L09_shared_student
  $exe=Join-Path $root (Join-Path $l09s "$cfg/L09_shared_student.exe")
  python $rec --output "$ev/l09-student-placeholder-ref-off-$($cfg.ToLower())-r2.json" --expect-exit 1 --contains 'make_node returns shared owner' -- $exe
}
python $rec --output "$ev/l09-reference-target-absent-ref-off-r2.json" --expect-exit 1 --contains '项目文件不存在' -- cmake --build $l09s --config Debug --target L09_shared_reference
# L08 smoke after README/interface wording fix.
$l08='build/c02-0810-r2-verifier-r1/L08-smoke'
python $rec --output "$ev/l08-configure-smoke-r2.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S Core_Study/exercises/L08_unique -B $l08 -G 'Visual Studio 18 2026' -A x64 -DL08_UNIQUE_BUILD_VALIDATION_VARIANTS=ON
python $rec --output "$ev/l08-build-smoke-debug-r2.json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $l08 --config Debug --clean-first
python $rec --output "$ev/l08-ctest-smoke-debug-r2.json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $l08 -C Debug --output-on-failure
$good=Join-Path $root (Join-Path $l08 'Debug/L08_unique_validation_good.exe')
python $rec --output "$ev/l08-good-smoke-debug-r2.json" --expect-exit 0 --contains 'L08_unique_validation_contract OK' -- $good
