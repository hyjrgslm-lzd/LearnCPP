$ErrorActionPreference = 'Stop'
$paths = @(
  'Core_Study/exercises/L03_lifetimes/src',
  'Core_Study/exercises/L03_lifetimes/reference',
  'Core_Study/exercises/L03_lifetimes/validation',
  'Core_Study/exercises/L05_special_members/src',
  'Core_Study/exercises/L05_special_members/reference',
  'Core_Study/exercises/L05_special_members/validation'
)
$files = Get-ChildItem $paths -Recurse -File -Include *.hpp,*.cpp
$bare = $files | Select-String -Pattern '#include "(lifetime_model|memory_model)\.hpp"'
if ($bare) {
  $bare | ForEach-Object { Write-Output "$($_.Path):$($_.LineNumber):$($_.Line)" }
  exit 1
}
Write-Output 'implementation trusted-header bare include scan: no matches'
$support = $files | Select-String -Pattern '#include "support/(lifetime_model|memory_model)\.hpp"'
$support | ForEach-Object { Write-Output "$($_.Path):$($_.LineNumber):$($_.Line)" }
Write-Output "support include matches: $($support.Count)"
