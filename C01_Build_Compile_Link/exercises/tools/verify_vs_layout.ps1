param(
    [string]$BuildDir = (Join-Path $PSScriptRoot "../build/vs-study")
)
$ErrorActionPreference = "Stop"
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path

function Assert-Layout($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Read-Project([string]$RelativePath) {
    [xml](Get-Content -LiteralPath (Join-Path $BuildDir $RelativePath) -Raw)
}
function Get-Items($Project, [string]$Kind) {
    @($Project.SelectNodes("//*[local-name()='$Kind'][@Include]") |
        ForEach-Object { $_.GetAttribute("Include").Replace('\', '/') })
}

# VS 2026 emits .slnx. Other VS versions can still use the course CMake files.
$solution = Read-Project "C01_Build_Compile_Link.slnx"
$projects = @($solution.SelectNodes("//Project[@Path]"))
$paths = @($projects | ForEach-Object { $_.GetAttribute("Path").Replace('\', '/') })
foreach ($project in $projects) {
    $path = $project.GetAttribute("Path").Replace('\', '/')
    $folder = $project.ParentNode.GetAttribute("Name")
    if ($path -match '^([A-Z][0-9][^/]*)/') {
        Assert-Layout ($folder -like "/$($Matches[1])/*") "Ungrouped lesson target: $path"
    } elseif ($path -like '_deps/*/f2_provider_fixture.vcxproj') {
        Assert-Layout ($folder -eq '/F2_dependencies/Support/') "Provider escaped its lesson"
    } else {
        Assert-Layout ($folder -like '/_CMake/*') "Ungrouped tooling target: $path"
    }
}
$startup = @($projects | Where-Object { $_.GetAttribute("DefaultStartup") -eq "true" })
Assert-Layout ($startup.Count -eq 1 -and $startup[0].GetAttribute("Path") -eq
    'A1_build_debug/A1_build_debug_student.vcxproj') "Incorrect default startup target"
Assert-Layout (-not ($paths -match '(A1_build_debug|B1_preprocessor|C1_odr)_(student|reference)_impl[.]vcxproj$|C2_archive_student_impl[.]vcxproj$')) "Redundant implementation project remains"

$b1 = Read-Project "B1_preprocessor/B1_preprocessor_student.vcxproj"
$compiled = Get-Items $b1 "ClCompile"
Assert-Layout ($compiled.Count -eq 2) "B1 should compile its implementation and checker only"
Assert-Layout (@($compiled -like '*/src/student/student_value.cpp').Count -eq 1) "B1 implementation missing"
Assert-Layout (@($compiled -like '*/checks/student_check.cpp').Count -eq 1) "B1 checker missing"
$headers = Get-Items $b1 "ClInclude"
Assert-Layout (@($headers -like '*/src/student/student_value.hpp').Count -eq 1) "B1 header missing"
Assert-Layout (@($headers -like '*/include/check.hpp').Count -eq 1) "Common checker header missing"
$filters = Read-Project "B1_preprocessor/B1_preprocessor_student.vcxproj.filters"
foreach ($expected in @("Student", "Checks", "Common", "Docs")) {
    Assert-Layout ($null -ne $filters.SelectSingleNode("//*[local-name()='Filter'][@Include='$expected']")) "Missing B1 filter: $expected"
}

$c1 = Read-Project "C1_odr/C1_odr_student.vcxproj"
$c1Compiled = Get-Items $c1 "ClCompile"
Assert-Layout ($c1Compiled.Count -eq 4) "C1 must retain four separate translation units"
Assert-Layout (-not ($c1Compiled -match '/(negative|review_variants)/')) "C1 browsing sources were compiled"
$c1Items = @(Get-Items $c1 "ClInclude") + @(Get-Items $c1 "None")
Assert-Layout (@($c1Items -like '*/negative/duplicate_header_definition/caller_a.cpp').Count -eq 1) "C1 negative example is not browsable"

$e1 = Read-Project "E1_abi/E1_abi_student.vcxproj"
Assert-Layout ($null -ne $e1.DocumentElement) "E1 student target missing"
$loader = Read-Project "D1_shared_library/D1_loader_reference.vcxproj"
Assert-Layout (@((Get-Items $loader "ProjectReference") -like '*/D1_runtime_shared.vcxproj').Count -eq 1) "Loader lacks its DLL build dependency"
$debuggerArguments = @($loader.SelectNodes("//*[local-name()='LocalDebuggerCommandArguments']"))
Assert-Layout ($debuggerArguments.Count -gt 0) "Loader debugger arguments missing"
foreach ($arg in $debuggerArguments) {
    Assert-Layout ($arg.InnerText -match '"[^"]*D1_runtime_shared[.]dll" lesson_runtime_value "[^"]*loader-exit[.]txt"') "Loader arguments lost DLL path, symbol or output path"
}

$cache = Get-Content -LiteralPath (Join-Path $BuildDir "CMakeCache.txt") -Raw
$hasReferences = $cache -match '(?m)^ENGINEERING_STUDY_BUILD_REFERENCE:BOOL=ON\r?$'
foreach ($target in @('A1_build_debug_reference', 'B1_preprocessor_reference', 'C1_odr_reference',
    'E1_abi_reference_static', 'E1_abi_reference_shared', 'F2_dependencies_reference', 'G1_diagnostics_reference')) {
    $found = @($paths -like "*/$target.vcxproj").Count -eq 1
    Assert-Layout ($found -eq $hasReferences) "Reference option disagrees with target: $target"
}
Write-Output "VS layout passed: $($projects.Count) projects; student files, grouping, negative browsing and loader setup verified."
