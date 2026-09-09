$ErrorActionPreference = 'Stop'
$root = (Resolve-Path '.').Path
$ev = Join-Path $root 'Core_Study/references/validation/reviews/c02-0810-verifier-evidence-r1'
$rec = Join-Path $root 'Core_Study/exercises/tools/record_process.py'
$lessons = @(
  @{ id='L08'; src='Core_Study/exercises/L08_unique'; build='build/c02-0810-verifier-r1/L08-default'; opt='-DL08_UNIQUE_BUILD_VALIDATION_VARIANTS=ON'; good='L08_unique_validation_good'; bad=@(@{name='L08_unique_validation_bad_noop'; text='check failed'}, @{name='L08_unique_validation_bad_release_keeps_owner'; text='release leaves unique_ptr empty'}); ref='L08_unique_reference'; obs='L08_unique_observation'; stu='L08_unique_student' },
  @{ id='L09'; src='Core_Study/exercises/L09_shared'; build='build/c02-0810-verifier-r1/L09-default'; opt='-DL09_SHARED_BUILD_VALIDATION_VARIANTS=ON'; good='L09_shared_validation_good'; bad=@(@{name='L09_shared_validation_bad_cycle'; text='weak backedge does not keep cycle alive'}, @{name='L09_shared_validation_bad_weak_resurrect'; text='lock fails after destruction'}); ref='L09_shared_reference'; obs='L09_shared_observation'; stu='L09_shared_student' },
  @{ id='L10'; src='Core_Study/exercises/L10_control_block'; build='build/c02-0810-verifier-r1/L10-default'; opt='-DL10_CONTROL_BLOCK_BUILD_VALIDATION_VARIANTS=ON'; good='L10_control_block_validation_good'; bad=@(@{name='L10_control_block_validation_bad_leak_control_block'; text='control block still alive'}, @{name='L10_control_block_validation_bad_weak_resurrect'; text='object destroyed after last strong'}); ref='L10_control_block_reference'; obs='L10_control_block_observation'; stu='L10_control_block_student' }
)
foreach ($l in $lessons) {
  $prefix = Join-Path $ev ($l.id.ToLower())
  python $rec --output "$prefix-configure-default.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S $l.src -B $l.build -G 'Visual Studio 18 2026' -A x64 $l.opt
  foreach ($cfg in @('Debug','Release')) {
    python $rec --output "$prefix-build-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $l.build --config $cfg --clean-first
    python $rec --output "$prefix-ctest-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $l.build -C $cfg --output-on-failure
    $goodPath = Join-Path $root (Join-Path $l.build "$cfg/$($l.good).exe")
    python $rec --output "$prefix-good-$($cfg.ToLower()).json" --expect-exit 0 --contains 'validation_contract OK' -- $goodPath
    foreach ($b in $l.bad) {
      $badPath = Join-Path $root (Join-Path $l.build "$cfg/$($b.name).exe")
      python $rec --output "$prefix-bad-$($b.name)-$($cfg.ToLower()).json" --expect-exit 1 --contains $b.text -- $badPath
    }
  }
  $studentBuild = "build/c02-0810-verifier-r1/$($l.id)-student-ref-off"
  python $rec --output "$prefix-configure-student-ref-off.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S $l.src -B $studentBuild -G 'Visual Studio 18 2026' -A x64 -DCORE_STUDY_BUILD_REFERENCE=OFF -DCORE_STUDY_TEST_STUDENTS=ON
  python $rec --output "$prefix-build-student-ref-off.json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $studentBuild --config Debug --clean-first --target $l.stu
  $studentExe = Join-Path $root (Join-Path $studentBuild "Debug/$($l.stu).exe")
  python $rec --output "$prefix-student-placeholder-ref-off.json" --expect-exit 1 --contains 'check failed' -- $studentExe
  python $rec --output "$prefix-reference-target-absent-ref-off.json" --expect-exit 1 --contains 'does not exist' -- cmake --build $studentBuild --config Debug --target $l.ref
}
