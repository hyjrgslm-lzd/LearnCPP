param(
    [string]$DepsRoot = (Join-Path $PSScriptRoot '..\build\_deps'),
    [string]$EvidenceDir = (Join-Path $PSScriptRoot '..\..\references\validation\revision-20260910'),
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$DepsRoot = [System.IO.Path]::GetFullPath($DepsRoot)
$EvidenceDir = [System.IO.Path]::GetFullPath($EvidenceDir)
$Stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$RunEvidenceDir = Join-Path $EvidenceDir "dependencies-format-$Stamp"
$Recorder = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py'))

function New-Directory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Assert-UnderRoot([string]$Path, [string]$Root) {
    $full = [System.IO.Path]::GetFullPath($Path)
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (-not ($full.StartsWith($rootFull, [System.StringComparison]::OrdinalIgnoreCase) -or $full.Equals($Root, [System.StringComparison]::OrdinalIgnoreCase))) {
        throw "Path escaped dependency root: $full"
    }
}

function Invoke-RecordedCommand {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [string]$WorkingDirectory = (Get-Location).Path
    )

    $json = Join-Path $RunEvidenceDir "$Name.json"
    $oldLocation = (Get-Location).Path
    Set-Location -LiteralPath $WorkingDirectory
    try {
        $recorderArgs = @('--output', $json, '--timeout', ([string]$TimeoutSeconds), '--', $FilePath) + @($ArgumentList)
        & python $Recorder @recorderArgs
        if ($LASTEXITCODE -ne 0) { throw "Recorded command failed: $Name" }
        return Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
    }
    finally {
        Set-Location -LiteralPath $oldLocation
    }
}

function Get-Sha256OrNull([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    return $null
}

function Write-DependencyMarker {
    param(
        [Parameter(Mandatory)][string]$Prefix,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$RepoUrl,
        [Parameter(Mandatory)][string]$Commit,
        [Parameter(Mandatory)][string]$SourceDir,
        [Parameter(Mandatory)][string]$LicensePath,
        [Parameter(Mandatory)][string[]]$Headers
    )

    $markerJson = [ordered]@{
        name = $Name
        repo_url = $RepoUrl
        commit = $Commit
        source_dir = $SourceDir
        license = $LicensePath
        license_sha256 = Get-Sha256OrNull $LicensePath
        headers = $Headers
    }
    $markerJson | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Prefix '.learncpp-dependency.json') -Encoding UTF8

    $cmake = @(
        "set(LEARNCPP_DEP_NAME `"$Name`")"
        "set(LEARNCPP_DEP_REPO_URL `"$RepoUrl`")"
        "set(LEARNCPP_DEP_COMMIT `"$Commit`")"
        "set(LEARNCPP_DEP_SOURCE_DIR `"$($SourceDir.Replace('\', '/'))`")"
        "set(LEARNCPP_DEP_LICENSE `"$($LicensePath.Replace('\', '/'))`")"
        "set(LEARNCPP_DEP_LICENSE_SHA256 `"$((Get-Sha256OrNull $LicensePath))`")"
    )
    $cmake | Set-Content -LiteralPath (Join-Path $Prefix '.learncpp-dependency.cmake') -Encoding UTF8
}

function Initialize-PinnedRepo {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$PrefixName,
        [Parameter(Mandatory)][string]$RepoUrl,
        [Parameter(Mandatory)][string]$Commit,
        [Parameter(Mandatory)][string[]]$Headers,
        [Parameter(Mandatory)][string[]]$LicenseCandidates
    )

    $prefix = Join-Path $DepsRoot $PrefixName
    $source = Join-Path $prefix 'source'
    Assert-UnderRoot $prefix $DepsRoot
    Assert-UnderRoot $source $DepsRoot

    if (Test-Path -LiteralPath $prefix) {
        $sourceGit = Join-Path $source '.git'
        if (-not (Test-Path -LiteralPath $sourceGit)) {
            $items = @(Get-ChildItem -LiteralPath $prefix -Force)
            if ($items.Count -gt 0) { throw "$Name prefix exists but is not a prepared git checkout: $prefix" }
        }
    }

    if (-not (Test-Path -LiteralPath (Join-Path $source '.git'))) {
        New-Directory $prefix
        New-Directory $source
        Invoke-RecordedCommand -Name "$Name-git-init" -FilePath 'git' -ArgumentList @('init', $source) | Out-Null
        Invoke-RecordedCommand -Name "$Name-git-remote-add" -FilePath 'git' -ArgumentList @('-C', $source, 'remote', 'add', 'origin', $RepoUrl) | Out-Null
        Invoke-RecordedCommand -Name "$Name-git-fetch" -FilePath 'git' -ArgumentList @('-C', $source, 'fetch', '--depth', '1', 'origin', $Commit) | Out-Null
        Invoke-RecordedCommand -Name "$Name-git-checkout" -FilePath 'git' -ArgumentList @('-C', $source, 'checkout', '--detach', $Commit) | Out-Null
    }

    $remote = (& git -C $source remote get-url origin).Trim()
    if ($remote -ne $RepoUrl) { throw "$Name remote mismatch: $remote" }
    $actualCommit = (& git -C $source rev-parse HEAD).Trim()
    if ($actualCommit -ne $Commit) { throw "$Name commit mismatch: $actualCommit" }
    $status = (& git -C $source status --porcelain=v1)
    if ($status) { throw "$Name checkout is dirty; refusing to overwrite $source" }

    foreach ($header in $Headers) {
        $headerPath = Join-Path $source $header
        if (-not (Test-Path -LiteralPath $headerPath)) { throw "$Name header marker missing: $headerPath" }
    }

    $licensePath = $null
    foreach ($candidate in $LicenseCandidates) {
        $path = Join-Path $source $candidate
        if (Test-Path -LiteralPath $path) {
            $licensePath = [System.IO.Path]::GetFullPath($path)
            break
        }
    }
    if (-not $licensePath) { throw "$Name license file not found" }

    Write-DependencyMarker -Prefix $prefix -Name $Name -RepoUrl $RepoUrl -Commit $Commit -SourceDir ([System.IO.Path]::GetFullPath($source)) -LicensePath $licensePath -Headers $Headers

    return [ordered]@{
        name = $Name
        verdict = 'PASS'
        repo_url = $RepoUrl
        required_commit = $Commit
        actual_commit = $actualCommit
        prefix = [System.IO.Path]::GetFullPath($prefix)
        source_dir = [System.IO.Path]::GetFullPath($source)
        license = $licensePath
        license_sha256 = Get-Sha256OrNull $licensePath
        headers = $Headers
    }
}

New-Directory $DepsRoot
New-Directory $RunEvidenceDir
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'git is required' }
if (-not (Get-Command python -ErrorAction SilentlyContinue)) { throw 'python is required for C02 record_process.py' }
if (-not (Test-Path -LiteralPath $Recorder)) { throw "C02 recorder not found: $Recorder" }

$dependencies = @(
    (Initialize-PinnedRepo -Name 'fmt' -PrefixName 'fmt-12.1.0' -RepoUrl 'https://github.com/fmtlib/fmt.git' -Commit '407c905e45ad75fc29bf0f9bb7c5c2fd3475976f' -Headers @('include\fmt\format.h', 'CMakeLists.txt') -LicenseCandidates @('LICENSE.rst', 'LICENSE')),
    (Initialize-PinnedRepo -Name 'spdlog' -PrefixName 'spdlog-1.17.0' -RepoUrl 'https://github.com/gabime/spdlog.git' -Commit '79524ddd08a4ec981b7fea76afd08ee05f83755d' -Headers @('include\spdlog\spdlog.h', 'CMakeLists.txt') -LicenseCandidates @('LICENSE'))
)

$summary = [ordered]@{
    verdict = 'PASS'
    generated_utc = (Get-Date).ToUniversalTime().ToString('o')
    deps_root = $DepsRoot
    evidence_dir = $RunEvidenceDir
    dependencies = $dependencies
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $RunEvidenceDir 'summary.json') -Encoding UTF8
$summary | ConvertTo-Json -Depth 8
