$ErrorActionPreference = 'Stop'

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$targets = @(
    'C06_Ranges/11-模块H-高级实现模式.md',
    'C06_Ranges/exercises/H1_non_propagating_cache',
    'C06_Ranges/exercises/H2_common_iter_proxy',
    'C06_Ranges/exercises/H3_generator_const_iter',
    'C06_Ranges/exercises/CAPSTONE3_impl_source_reading',
    'C06_Ranges/exercises/CAPSTONE4_mini_ranges'
)

$files = foreach ($target in $targets) {
    $path = Join-Path $repo $target
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Get-Item -LiteralPath $path
    } else {
        Get-ChildItem -LiteralPath $path -Recurse -File |
            Where-Object {
                $_.FullName -notmatch '\\(build|out|\.vs)\\' -and
                $_.Extension -notin @('.obj', '.pdb', '.ilk', '.exe', '.dll', '.lib')
            }
    }
}

$entries = $files |
    Sort-Object FullName |
    ForEach-Object {
        $relative = $_.FullName.Substring($repo.Length + 1).Replace('\', '/')
        [ordered]@{
            path = $relative
            bytes = $_.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        }
    }

$gitHead = (& git -C $repo rev-parse HEAD) 2>$null
$gitStatus = (& git -C $repo status --short -- C06_Ranges) 2>$null

[ordered]@{
    generated_at = (Get-Date).ToUniversalTime().ToString('o')
    repo = $repo
    git_head = $gitHead
    git_status_short_C06_Ranges = @($gitStatus)
    file_count = @($entries).Count
    files = @($entries)
} | ConvertTo-Json -Depth 6
