param([Parameter(Mandatory = $true)][string]$RunName)
$ErrorActionPreference = 'Stop'
if ($RunName -notmatch '^[a-zA-Z0-9_-]+$') { throw 'RunName must be a simple new identifier' }
$studyRepo = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$studyEvidence = Join-Path $PSScriptRoot $RunName
$studyWork = Join-Path $studyRepo ('build/c02-wiring-' + $RunName)
if ((Test-Path -LiteralPath $studyEvidence) -or (Test-Path -LiteralPath $studyWork)) {
    throw 'Use a new RunName; existing sources or evidence are never removed'
}
New-Item -ItemType Directory -Path $studyEvidence -Force | Out-Null
Push-Location $studyRepo
try {
    foreach ($studyCase in @('good', 'bad')) {
        $studyBuild = Join-Path $studyWork $studyCase
        $studyQuery = Join-Path $studyBuild '.cmake/api/v1/query/client-c02'
        New-Item -ItemType Directory -Path $studyQuery -Force | Out-Null
        New-Item -ItemType File -Path (Join-Path $studyQuery 'codemodel-v2') | Out-Null
        $studyBad = if ($studyCase -eq 'bad') { 'ON' } else { 'OFF' }
        python Core_Study/exercises/tools/record_process.py --output (Join-Path $studyEvidence ($studyCase + '-configure.json')) -- cmake -S $PSScriptRoot -B $studyBuild -G 'Visual Studio 18 2026' -A x64 ('-DC02_WIRING_BAD=' + $studyBad)
        if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
        $studyTrace = Join-Path $studyEvidence ($studyCase + '-build.json')
        python Core_Study/exercises/tools/record_process.py --output $studyTrace -- cmake --build $studyBuild --config Release --clean-first --target wiring_student
        if ($LASTEXITCODE -ne 0) { throw 'build failed' }
        $studyAudit = Join-Path $studyEvidence ($studyCase + '-audit.json')
        python Core_Study/exercises/tools/audit_student.py --build $studyBuild --config Release --trace $studyTrace --expect-target wiring_student --output $studyAudit
        $studyExpected = if ($studyCase -eq 'bad') { 1 } else { 0 }
        if ($LASTEXITCODE -ne $studyExpected) { throw 'unexpected audit exit code' }
        $studyResult = Get-Content -LiteralPath $studyAudit -Raw | ConvertFrom-Json
        if ($studyCase -eq 'bad' -and -not ($studyResult.failures -match 'Reference in actual include trace')) {
            throw 'bad reference include was not detected in the actual compiler trace'
        }
    }
    Write-Output 'PASS: real good/bad CMake builds and three-layer Student audit'
} finally {
    Pop-Location
}
