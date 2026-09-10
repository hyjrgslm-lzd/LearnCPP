$ErrorActionPreference = 'Stop'

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$paths = @(
    'C06_Ranges/11-模块H-高级实现模式.md',
    'C06_Ranges/exercises/H3_generator_const_iter/README.md',
    'C06_Ranges/references/validation/r3-doc-alias-probe/h3_doc_alias_probe.cpp',
    'C06_Ranges/references/validation/r3-doc-alias-probe/CMakeLists.txt',
    'C06_Ranges/references/validation/advanced-r3-doc-alias-probe.json'
)

$entries = foreach ($relative in $paths) {
    $path = Join-Path $repo $relative
    $item = Get-Item -LiteralPath $path
    [ordered]@{
        path = $relative
        bytes = $item.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.FullName).Hash.ToLowerInvariant()
    }
}

$gitHead = (& git -C $repo rev-parse HEAD) 2>$null
$gitStatus = (& git -C $repo status --short -- C06_Ranges/11-模块H-高级实现模式.md C06_Ranges/exercises/H3_generator_const_iter/README.md C06_Ranges/references/validation/r3-doc-alias-probe C06_Ranges/references/validation/advanced-r3-doc-alias-probe.json) 2>$null

[ordered]@{
    generated_at = (Get-Date).ToUniversalTime().ToString('o')
    repo = $repo
    git_head = $gitHead
    git_status_short = @($gitStatus)
    files = @($entries)
} | ConvertTo-Json -Depth 5
