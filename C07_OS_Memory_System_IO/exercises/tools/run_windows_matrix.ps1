param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$RunId,
    [Parameter(Mandatory=$true)][ValidateSet('core','debug','asan','student')][string]$Profile
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..\..')).Path
$exerciseRoot = Join-Path $repoRoot 'C07_OS_Memory_System_IO\exercises'
$buildRoot = Join-Path $exerciseRoot ('build\' + $RunId)
$evidenceRoot = Join-Path $repoRoot ('C07_OS_Memory_System_IO\references\validation\' + $RunId)
if ((Test-Path -LiteralPath $buildRoot) -or (Test-Path -LiteralPath $evidenceRoot)) {
    throw 'Use a new run id: build/evidence directories are not overwritten.'
}
$pythonPath = (Get-Command python).Source
$recordTool = Join-Path $repoRoot 'C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py'
$configuration = 'Release'
$options = @('-DC07_STUDY_BUILD_REFERENCE=ON', '-DC07_STUDY_ENABLE_IO_URING=OFF')
if ($Profile -eq 'debug') { $configuration = 'Debug' }
if ($Profile -eq 'asan') {
    $configuration = 'RelWithDebInfo'
    $options += '-DC07_STUDY_ENABLE_ASAN=ON'
}
if ($Profile -eq 'student') {
    $options += @('-DC07_STUDY_BUILD_REFERENCE=OFF', '-DC07_STUDY_TEST_STUDENTS=ON', '-DC07_STUDY_TRACE_INCLUDES=ON')
}
New-Item -ItemType Directory -Path $evidenceRoot | Out-Null
New-Item -ItemType Directory -Path (Join-Path $buildRoot '.cmake\api\v1\query') -Force | Out-Null
New-Item -ItemType File -Path (Join-Path $buildRoot '.cmake\api\v1\query\codemodel-v2') | Out-Null
function Invoke-C07Record([string]$RecordName, [int]$Limit, [string[]]$CommandParts) {
    & $pythonPath $recordTool --output (Join-Path $evidenceRoot ($RecordName + '.json')) --timeout $Limit -- @CommandParts
    if ($LASTEXITCODE -ne 0) { throw "C07 $RecordName failed; raw evidence retained." }
}
Push-Location $repoRoot
try {
    Invoke-C07Record 'configure' 180 (@('cmake', '-S', $exerciseRoot, '-B', $buildRoot,
        '-G', 'Visual Studio 18 2026', '-A', 'x64') + $options)
    $buildArguments = @('cmake', '--build', $buildRoot, '--config', $configuration, '--parallel', '4')
    if ($Profile -eq 'student') {
        $studentTargets = @(Get-Content -LiteralPath (Join-Path $buildRoot "student-targets-$configuration.txt") | Where-Object { $_.Trim() })
        $buildArguments += @('--clean-first', '--target') + $studentTargets
    }
    Invoke-C07Record 'build' 1200 $buildArguments
    if ($Profile -eq 'student') {
        & $pythonPath (Join-Path $exerciseRoot 'tools\verify_students.py') --build $buildRoot --config $configuration `
            --trace (Join-Path $evidenceRoot 'build.json') --output (Join-Path $evidenceRoot 'student-verification.json')
        if ($LASTEXITCODE -ne 0) { throw 'C07 Student audit failed.' }
    } else {
        Invoke-C07Record 'ctest' 900 @('ctest', '--test-dir', $buildRoot, '-C', $configuration,
            '--output-on-failure', '--output-junit', (Join-Path $evidenceRoot 'ctest.xml'))
    }
    & $pythonPath (Join-Path $exerciseRoot 'tools\record_environment.py') --build $buildRoot --output (Join-Path $evidenceRoot 'environment.json')
    if ($LASTEXITCODE -ne 0) { throw 'C07 environment record failed.' }
} finally {
    $records = Join-Path $buildRoot 'records'
    if (Test-Path -LiteralPath $records) { Copy-Item -LiteralPath $records -Destination (Join-Path $evidenceRoot 'records') -Recurse }
    Pop-Location
}
