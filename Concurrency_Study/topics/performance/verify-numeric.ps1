param([switch]$AddressSanitizer, [switch]$FastMath, [switch]$Diagnostics, [switch]$NoParallel, [switch]$RunnerSelfCheck, [string]$XsimdInclude = '')
$ErrorActionPreference = 'Stop'
if ($FastMath -and $Diagnostics) { throw 'FastMath and Diagnostics are separate experiments' }
$course = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$exercise = Join-Path $course 'exercises'
function Invoke-NumericRun([string[]]$Command, [int]$Expected = 0, [double]$Timeout = 30) {
    # Thin adapter only: process creation, external timeout, and tree cleanup
    # belong to the existing shared runner. Preserve its original result fields.
    $adapter = 'import json,sys; sys.path.insert(0,sys.argv[1]); from run_benchmarks import run_process; print(json.dumps(run_process(json.loads(sys.argv[2]),float(sys.argv[3]))))'
    $raw = & python -c $adapter (Join-Path $exercise 'tools') (ConvertTo-Json -InputObject $Command -Compress) $Timeout
    if ($LASTEXITCODE -ne 0) { throw 'shared run_process adapter failed' }
    $result = $raw | ConvertFrom-Json
    Write-Host $result.stdout
    Write-Host $result.stderr
    Write-Host "status=$($result.status); exit=$($result.exit_code); timeout=$($result.timeout)"
    if ($result.error -or $result.cleanup_error) { throw "runner/cleanup error: $($result.error) $($result.cleanup_error)" }
    if ($result.timeout) { throw "TIMEOUT: $($Command[0]); failed even if exit $Expected was expected" }
    if ($result.exit_code -eq 77) { Write-Host 'SKIP (77), not a passed check'; return }
    if ($result.exit_code -ne $Expected) { throw "check failed: exit $($result.exit_code), expected $Expected" }
    if ($Expected -ne 0) { Write-Host "EXPECTED INCOMPLETE STARTER: original exit $Expected retained, not Reference PASS" }
}
if ($RunnerSelfCheck) {
    $python = (Get-Command python).Source
    Invoke-NumericRun -Command @($python,'-c','raise SystemExit(0)')
    Invoke-NumericRun -Command @($python,'-c','raise SystemExit(77)')
    $failed = $false
    try { Invoke-NumericRun -Command @($python,'-c','raise SystemExit(7)') }
    catch { $failed = $_.Exception.Message -like '*exit 7*' }
    if (-not $failed) { throw 'nonzero child exit was not reported faithfully' }
    foreach ($code in @(
        'import threading; print("CONTROLLED HANG",flush=True); threading.Event().wait()',
        'import atexit,threading; atexit.register(lambda: threading.Event().wait()); print("CONTROLLED EXIT HANG",flush=True)')) {
        $timedOut = $false
        try { Invoke-NumericRun -Command @($python,'-c',$code) -Expected 1 -Timeout 1 }
        catch { $timedOut = $_.Exception.Message -like '*TIMEOUT*' }
        if (-not $timedOut) { throw 'controlled execution/exit hang was not rejected as timeout' }
    }
    Write-Host 'Runner self-check OK: 0/77/7 preserved; timeout cannot masquerade as expected Starter exit 1'
    return
}
$mode = if ($Diagnostics) { 'diagnostics' } elseif ($FastMath) { 'fast-isolated' } else { 'strict' }
if ($AddressSanitizer) { $mode += '-asan' }
if ($NoParallel) { $mode += '-no-parallel' }
if ($XsimdInclude) { $mode += '-xsimd' }
$build = Join-Path $exercise "build/numeric-review-p2-$mode"
New-Item -ItemType Directory -Force -Path $build | Out-Null
# Import this installation's developer environment only into this process.
$environment = & cmd.exe /d /s /c '"G:\Visual Studio 2026 Community\VC\Auxiliary\Build\vcvars64.bat" >nul && set'
if ($LASTEXITCODE -ne 0) { throw 'vcvars64 failed' }
foreach ($line in $environment) {
    if ($line -cmatch '^Path=') { continue } # The inherited duplicate would overwrite vcvars PATH.
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1],$matches[2],'Process') }
}
$flags = @('/nologo','/std:c++23preview','/EHsc','/utf-8','/W4','/O2','/GL-','/Zc:__cplusplus','/Zc:preprocessor',
    "/I$exercise/include",("/DCS_HAS_PARALLEL_ALGORITHMS=" + [int](-not $NoParallel)),'/DCS_HAS_STD_SIMD=0')
if ($XsimdInclude) { $flags += @('/DCS_HAS_XSIMD=1',"/I$XsimdInclude") }
else { $flags += '/DCS_HAS_XSIMD=0' }
if ($AddressSanitizer) { $flags += @('/fsanitize=address','/Zi') }
if ($Diagnostics) { $flags += @('/fp:precise','/Qvec-report:2','/FAs','/Favectorization_probe.asm') }
elseif (-not $FastMath) { $flags += '/fp:strict' }
$sources = @('runtime_tests/numeric_test.cpp')
if (-not $FastMath) {
    $ids = 'J1_false_sharing','J2_interference_size','K1_simd_basics','K2_simd_dotproduct','K3_simd_where_select',
           'L1_par_algorithms','L2_parallel_reduce_scan','L3_par_vs_seq_bench','Capstone3_parallel_compute'
    foreach ($id in $ids) {
        $sources += "$id/solution.cpp"
        $sources += "$id/main.cpp"
    }
}
$sources += @('benchmarks/simd_bench.cpp','benchmarks/layout_bench.cpp','L3_par_vs_seq_bench/benchmark.cpp','Capstone3_parallel_compute/benchmark.cpp')
if ($Diagnostics) { $sources = @('../topics/simd/vectorization_probe.cpp') }
$compiler = 'G:/Visual Studio 2026 Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe'
Start-Transcript -Path (Join-Path $build 'verification.log') -Append | Out-Null
Push-Location $build
try {
    if ($FastMath) {
        # No production inline definitions enter the strict translation unit.
        Write-Output "STRICT checker: $($flags -join ' ') /c /fp:strict /FAs"
        & $compiler @flags /c /fp:strict /FAs /Fafast_math_check.asm (Join-Path $course 'topics/simd/fast_math_check.cpp') /Fofast_math_check.obj /Fdfast_math_check.pdb
        if ($LASTEXITCODE -ne 0) { throw 'strict fast-math checker compile failed' }
        Write-Output "FAST kernel: $($flags -join ' ') /c /fp:fast /FAs"
        & $compiler @flags /c /fp:fast /FAs /Fafast_math_kernel.asm (Join-Path $course 'topics/simd/fast_math_kernel.cpp') /Fofast_math_kernel.obj /Fdfast_math_kernel.pdb
        if ($LASTEXITCODE -ne 0) { throw 'fast kernel compile failed' }
        Write-Output 'LINK: ordinary machine-code objects; /LTCG:OFF /OPT:NOICF'
        & $compiler @flags fast_math_check.obj fast_math_kernel.obj /Fefast_math_check.exe /link /LTCG:OFF /OPT:NOICF
        if ($LASTEXITCODE -ne 0) { throw 'isolated fast-math link failed' }
        Invoke-NumericRun -Command @((Join-Path $build 'fast_math_check.exe'))
        return
    }
    Write-Output "Compiler: $compiler; flags: $($flags -join ' ')"
    foreach ($source in $sources) {
        $stem = $source.Replace('../','').Replace('/','_').Replace('.cpp','')
        $exe = Join-Path $build "$stem.exe"
        & $compiler @flags (Join-Path $exercise $source) "/Fe:$exe" "/Fo:$stem.obj" "/Fd:$stem.pdb"
        if ($LASTEXITCODE -ne 0) { throw "compile failed: $source" }
        $expected = if ($source -in @('L3_par_vs_seq_bench/main.cpp','Capstone3_parallel_compute/main.cpp')) { 1 } else { 0 }
        Invoke-NumericRun -Command @($exe) -Expected $expected
    }
} finally { Pop-Location; Stop-Transcript | Out-Null }
