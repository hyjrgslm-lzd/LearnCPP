param(
    [Parameter(Mandatory = $true)][string]$Output,
    [int]$Samples = 5,
    [int]$Warmups = 1,
    [int]$Seed = 42,
    [int]$Parallel = 1,
    [double]$Timeout = 180
)

$ErrorActionPreference = "Stop"
$python = "C:\Users\zhidan.li\AppData\Roaming\uv\python\cpython-3.13.11-windows-x86_64-none\python.exe"
& $python "$PSScriptRoot\measure_build.py" --output $Output --samples $Samples --warmups $Warmups --seed $Seed --parallel $Parallel --timeout $Timeout
exit $LASTEXITCODE
