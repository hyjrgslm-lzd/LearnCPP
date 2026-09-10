$ErrorActionPreference = 'Stop'

$RunId = 'final-20260910-173010-d5356576'
$RepoRoot = (Resolve-Path '.').Path
$Distro = 'LearnCPP-C08-Ubuntu-24.04'
$GuestSnapshotRoot = "\\wsl.localhost\$Distro\root\learncpp-c08-src\$RunId"
$EvidenceRoot = Join-Path $RepoRoot "C08_Concurrency\references\validation\c08-revision\linux-final\$RunId"

New-Item -ItemType Directory -Force -Path $GuestSnapshotRoot | Out-Null

$SnapshotStart = Get-Date
$SnapshotStart.ToString('o') | Set-Content -Path (Join-Path $EvidenceRoot 'snapshot-start.txt') -Encoding utf8

$ExcludeDirs = @(
    '.git', '.omx', '.codex', '.agents', '.omc', '.claude', '.vs',
    '_deps', 'CMakeFiles', '.cache', '.pytest_cache', '__pycache__'
)
$ExcludeExts = @(
    '.exe', '.dll', '.pdb', '.ilk', '.obj', '.o', '.lib', '.a', '.so', '.dylib',
    '.zip', '.7z', '.tar', '.gz', '.xz', '.bz2', '.bin', '.tmp', '.log'
)
$ExcludeNames = @('CMakeCache.txt', 'compile_commands.json')

function Test-SkipDirName([string]$Name) {
    if ($ExcludeDirs -contains $Name) { return $true }
    if ($Name -like 'build*') { return $true }
    return $false
}

function Test-SkipPath([string]$FullName) {
    $rel = [System.IO.Path]::GetRelativePath($RepoRoot, $FullName)
    foreach ($part in ($rel -split '[\\/]')) {
        if (Test-SkipDirName $part) { return $true }
        if ($part.StartsWith('.')) { return $true }
    }
    return $false
}

function Test-CopyableFile([System.IO.FileInfo]$File) {
    if (($File.Attributes -band [System.IO.FileAttributes]::Hidden) -ne 0) { return $false }
    if ($ExcludeNames -contains $File.Name) { return $false }
    if ($ExcludeExts -contains $File.Extension.ToLowerInvariant()) { return $false }
    if (Test-SkipPath $File.FullName) { return $false }
    return $true
}

function Copy-RepoFile([System.IO.FileInfo]$File) {
    $rel = [System.IO.Path]::GetRelativePath($RepoRoot, $File.FullName)
    $dest = Join-Path $GuestSnapshotRoot $rel
    New-Item -ItemType Directory -Force -Path ([System.IO.Path]::GetDirectoryName($dest)) | Out-Null
    Copy-Item -LiteralPath $File.FullName -Destination $dest -Force
}

$copied = New-Object System.Collections.Generic.List[string]

Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'C08_Concurrency') -Recurse -File -Force |
    Where-Object { Test-CopyableFile $_ } |
    ForEach-Object {
        Copy-RepoFile $_
        $copied.Add([System.IO.Path]::GetRelativePath($RepoRoot, $_.FullName).Replace('\','/'))
    }

Get-ChildItem -LiteralPath $RepoRoot -Recurse -File -Force -Filter '*.md' |
    Where-Object {
        $_.FullName -notlike (Join-Path $RepoRoot 'C08_Concurrency') + '*'
    } |
    Where-Object { Test-CopyableFile $_ } |
    ForEach-Object {
        Copy-RepoFile $_
        $copied.Add([System.IO.Path]::GetRelativePath($RepoRoot, $_.FullName).Replace('\','/'))
    }

$copied | Sort-Object | Set-Content -Path (Join-Path $EvidenceRoot 'copied-files.txt') -Encoding utf8

$SnapshotEnd = Get-Date
$SnapshotEnd.ToString('o') | Set-Content -Path (Join-Path $EvidenceRoot 'snapshot-end.txt') -Encoding utf8

$cppChanged = Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'C08_Concurrency\exercises') -Recurse -File -Force -Include '*.cpp','*.hpp','*.h','*.cmake','CMakeLists.txt','CMakePresets.json' |
    Where-Object { $_.LastWriteTime -gt $SnapshotStart } |
    ForEach-Object {
        [PSCustomObject]@{
            path = [System.IO.Path]::GetRelativePath($RepoRoot, $_.FullName).Replace('\','/')
            lastWriteTime = $_.LastWriteTime.ToString('o')
        }
    }

$cppChanged | ConvertTo-Json -Depth 3 | Set-Content -Path (Join-Path $EvidenceRoot 'cpp-files-modified-during-snapshot.json') -Encoding utf8

@{
    runId = $RunId
    repoRoot = $RepoRoot
    distro = $Distro
    guestSnapshotRoot = $GuestSnapshotRoot
    copiedFileCount = $copied.Count
    snapshotStart = $SnapshotStart.ToString('o')
    snapshotEnd = $SnapshotEnd.ToString('o')
    cppFilesModifiedDuringSnapshot = @($cppChanged).Count
} | ConvertTo-Json -Depth 4 | Set-Content -Path (Join-Path $EvidenceRoot 'snapshot-copy-summary.json') -Encoding utf8

Write-Output "snapshot=$GuestSnapshotRoot"
Write-Output "files=$($copied.Count)"
