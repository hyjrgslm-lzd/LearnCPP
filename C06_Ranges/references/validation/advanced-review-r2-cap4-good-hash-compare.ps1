$ErrorActionPreference = 'Stop'

$pairs = Get-ChildItem 'C06_Ranges/exercises/CAPSTONE4_mini_ranges/src/reference/my_ranges' -Filter *.hpp |
    Sort-Object Name |
    ForEach-Object {
        $name = $_.Name
        $ref = $_.FullName
        $good = Join-Path 'C06_Ranges/exercises/CAPSTONE4_mini_ranges/validation/good/my_ranges' $name
        $referenceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ref).Hash.ToLowerInvariant()
        $goodHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $good).Hash.ToLowerInvariant()
        [ordered]@{
            file = $name
            reference_sha256 = $referenceHash
            good_sha256 = $goodHash
            same = ($referenceHash -eq $goodHash)
        }
    }

$pairs | ConvertTo-Json -Depth 3

if (($pairs | Where-Object { $_.same }).Count -ne 0) {
    exit 1
}
