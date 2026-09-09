$ErrorActionPreference = 'Stop'
$root = (Resolve-Path '.').Path
$ev = Join-Path $root 'Core_Study/references/validation/reviews/c02-foundation-verifier-evidence-r1'
$rec = Join-Path $root 'Core_Study/exercises/tools/record_process.py'
$obsLessons = @(
  @{ id='L01'; src='Core_Study/exercises/L01_initialization' },
  @{ id='L02'; src='Core_Study/exercises/L02_value_categories' },
  @{ id='L06'; src='Core_Study/exercises/L06_move_return' }
)
foreach($l in $obsLessons){
  $prefix = Join-Path $ev ($l.id.ToLower())
  $build = "build/c02-foundation-verifier-r1/$($l.id)-default"
  python $rec --output "$prefix-configure-default.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S $l.src -B $build -G 'Visual Studio 18 2026' -A x64
  foreach($cfg in @('Debug','Release')){
    python $rec --output "$prefix-build-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $build --config $cfg --clean-first
    python $rec --output "$prefix-ctest-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $build -C $cfg --output-on-failure
  }
}
$implLessons = @(
  @{ id='L03'; src='Core_Study/exercises/L03_lifetimes'; student='L03_lifetimes_student'; ref='L03_lifetimes_reference'; good='L03_lifetimes_validation_good'; bad=@(@{name='L03_lifetimes_validation_bad_dangling_view'; text='borrow must become invalid when owner lifetime ends'}) },
  @{ id='L05'; src='Core_Study/exercises/L05_special_members'; student='L05_special_members_student'; ref='L05_special_members_reference'; good='L05_special_members_validation_good'; bad=@(@{name='L05_special_members_validation_bad_shallow_copy'; text='copy must have independent storage'}, @{name='L05_special_members_validation_bad_move_leak'; text='move assignment should release old target storage'}) }
)
foreach($l in $implLessons){
  $prefix = Join-Path $ev ($l.id.ToLower())
  $build = "build/c02-foundation-verifier-r1/$($l.id)-default"
  python $rec --output "$prefix-configure-default.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S $l.src -B $build -G 'Visual Studio 18 2026' -A x64
  foreach($cfg in @('Debug','Release')){
    python $rec --output "$prefix-build-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $build --config $cfg --clean-first
    python $rec --output "$prefix-ctest-default-$($cfg.ToLower()).json" --expect-exit 0 --contains '100% tests passed' -- ctest --test-dir $build -C $cfg --output-on-failure
    $goodExe = Join-Path $root (Join-Path $build "$cfg/$($l.good).exe")
    python $rec --output "$prefix-good-$($cfg.ToLower()).json" --expect-exit 0 -- $goodExe
    foreach($b in $l.bad){
      $badExe = Join-Path $root (Join-Path $build "$cfg/$($b.name).exe")
      python $rec --output "$prefix-bad-$($b.name)-$($cfg.ToLower()).json" --expect-exit 1 --contains $b.text -- $badExe
    }
  }
  $studentBuild = "build/c02-foundation-verifier-r1/$($l.id)-student-ref-off"
  python $rec --output "$prefix-configure-student-ref-off.json" --expect-exit 0 --contains 'Configuring done' -- cmake -S $l.src -B $studentBuild -G 'Visual Studio 18 2026' -A x64 -DCORE_STUDY_BUILD_REFERENCE=OFF -DCORE_STUDY_TEST_STUDENTS=ON
  foreach($cfg in @('Debug','Release')){
    python $rec --output "$prefix-build-student-ref-off-$($cfg.ToLower()).json" --expect-exit 0 --contains '.vcxproj' -- cmake --build $studentBuild --config $cfg --clean-first --target $l.student
    $studentExe = Join-Path $root (Join-Path $studentBuild "$cfg/$($l.student).exe")
    python $rec --output "$prefix-student-placeholder-ref-off-$($cfg.ToLower()).json" --expect-exit 1 --contains 'check failed' -- $studentExe
  }
  python $rec --output "$prefix-reference-target-absent-ref-off.json" --expect-exit 1 --contains '项目文件不存在' -- cmake --build $studentBuild --config Debug --target $l.ref
}
