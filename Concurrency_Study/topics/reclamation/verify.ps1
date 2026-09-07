#requires -Version 7.4
param(
    [switch]$Asan,
    [ValidateSet('Reference', 'Starter', 'Student')]
    [string]$Mode = 'Reference'
)
$ErrorActionPreference = 'Stop'
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run from an x64 MSVC Developer PowerShell (C++23 library required).'
}
$study = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$build = Join-Path $PSScriptRoot $(if ($Asan) { '.build-asan' } else { '.build' })
New-Item -ItemType Directory -Force -Path $build | Out-Null
$sources = @()
if ($Mode -eq 'Reference') {
    $sources += 'exercises/runtime_tests/reclamation_test.cpp', 'topics/reclamation/lifetime_reference.cpp'
}
foreach ($exercise in @('I2_hazard_pointer', 'I3_rcu', 'R1_epoch_reclamation', 'R2_qsbr')) {
    $entry = if ($Mode -eq 'Reference') { 'solution.cpp' } else { 'main.cpp' }
    $sources += "exercises/$exercise/$entry"
}
foreach ($relative in $sources) {
    $name = $relative.Replace('/', '_').Replace('.cpp', '')
    $exe = Join-Path $build "$name.exe"
    $obj = Join-Path $build "$name.obj"
    $options = @('/nologo', '/std:c++23preview', '/EHsc', '/W4', '/WX', '/utf-8', '/O2', '/DNDEBUG', "/I$study/exercises/include", "/Fo$obj", "/Fe$exe")
    if ($Asan) { $options += '/fsanitize=address', '/Z7' }
    & cl.exe @options (Join-Path $study $relative)
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $relative" }
    # 每个进程外部超时，避免协议回归把整个验收无限挂住。
    $runtimePath = (Split-Path (Get-Command cl.exe).Source) + ';' + $env:PATH
    $process = Start-Process -FilePath $exe -PassThru -WindowStyle Hidden -Environment @{ PATH = $runtimePath } -RedirectStandardOutput "$exe.stdout" -RedirectStandardError "$exe.stderr"
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        throw "TIMEOUT (30 s): $relative"
    }
    $process.WaitForExit()
    $stdout = Get-Content -Raw -LiteralPath "$exe.stdout"
    $stderr = Get-Content -Raw -LiteralPath "$exe.stderr"
    Write-Output $stdout
    Write-Output $stderr
    $expected = if ($Mode -eq 'Starter') { 1 } else { 0 }
    if ($process.ExitCode -ne $expected) {
        throw "Run failed: $relative (expected $expected, got $($process.ExitCode))"
    }
    if ($Mode -eq 'Starter' -and ($stdout -match 'starting student workers' -or $stderr -notmatch 'TODO')) {
        throw "Expected an incomplete starter rejected before worker launch: $relative"
    }
    if ($Mode -eq 'Student' -and $stdout -notmatch 'student PASS:') {
        throw "Student checks did not complete: $relative"
    }
    if ($stderr -match 'AddressSanitizer') { throw "ASan report: $relative" }
}
Write-Output "$Mode verification PASS ($($sources.Count) executables; expected exits checked)"
