param(
    [string]$Prefix = (Join-Path $PSScriptRoot '..\build\_deps\icu-77.1'),
    [string]$EvidenceDir = (Join-Path $PSScriptRoot '..\..\references\validation'),
    [string]$MSBuild = 'D:\VisualStudio2026\Installed\MSBuild\Current\Bin\MSBuild.exe',
    [int]$TimeoutSeconds = 3600,
    [switch]$SkipBuild,
    [switch]$SkipProbe
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Commit = '457157a92aa053e632cc7fcfd0e12f8a943b2d11'
$RepoUrl = 'https://github.com/unicode-org/icu.git'
$Prefix = [System.IO.Path]::GetFullPath($Prefix)
$EvidenceDir = [System.IO.Path]::GetFullPath($EvidenceDir)
$SourceRoot = Join-Path $Prefix 'source'
$RepoRoot = Join-Path $SourceRoot 'icu-77.1-sparse'
$Stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$RunEvidenceDir = Join-Path $EvidenceDir "icu-prepare-$Stamp"
$Recorder = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..\C02_Objects_Lifetime_Ownership\exercises\tools\record_process.py'))

function New-Directory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Invoke-RecordedCommand {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [string]$WorkingDirectory = (Get-Location).Path,
        [int]$Timeout = $TimeoutSeconds
    )

    $json = Join-Path $RunEvidenceDir "$Name.json"
    $oldLocation = (Get-Location).Path
    Set-Location -LiteralPath $WorkingDirectory
    try {
        $recorderArgs = @('--output', $json, '--timeout', ([string]$Timeout), '--', $FilePath) + @($ArgumentList)
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

function Assert-UnderPrefix([string]$Path) {
    $full = [System.IO.Path]::GetFullPath($Path)
    $root = $Prefix.TrimEnd('\') + '\'
    if (-not ($full.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase) -or $full.Equals($Prefix, [System.StringComparison]::OrdinalIgnoreCase))) {
        throw "Path escaped ICU prefix: $full"
    }
}

New-Directory $Prefix
New-Directory $RunEvidenceDir
Assert-UnderPrefix $SourceRoot

if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'git is required' }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'cmake is required' }
if (-not (Get-Command python -ErrorAction SilentlyContinue)) { throw 'python is required for C02 record_process.py' }
if (-not (Test-Path -LiteralPath $Recorder)) { throw "C02 recorder not found: $Recorder" }
if (-not (Test-Path -LiteralPath $MSBuild)) { throw "MSBuild not found: $MSBuild" }

if (-not (Test-Path -LiteralPath (Join-Path $RepoRoot '.git'))) {
    New-Directory $SourceRoot
    New-Directory $RepoRoot
    Invoke-RecordedCommand -Name 'git-init' -FilePath 'git' -ArgumentList @('init', $RepoRoot) -WorkingDirectory $SourceRoot | Out-Null
    Invoke-RecordedCommand -Name 'git-remote-add' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'remote', 'add', 'origin', $RepoUrl) | Out-Null
}

Invoke-RecordedCommand -Name 'git-config-longpaths' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'config', 'core.longpaths', 'true') | Out-Null
Invoke-RecordedCommand -Name 'git-sparse-init' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'sparse-checkout', 'init', '--cone') | Out-Null
Invoke-RecordedCommand -Name 'git-sparse-set' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'sparse-checkout', 'set', 'icu4c') | Out-Null
Invoke-RecordedCommand -Name 'git-fetch-release-commit' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'fetch', '--depth', '1', 'origin', $Commit) | Out-Null
Invoke-RecordedCommand -Name 'git-checkout-release-commit' -FilePath 'git' -ArgumentList @('-C', $RepoRoot, 'checkout', '--detach', $Commit) | Out-Null
$actualCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($actualCommit -ne $Commit) { throw "ICU commit mismatch: $actualCommit" }

$Icu4cRoot = Join-Path $RepoRoot 'icu4c'
if (-not (Test-Path -LiteralPath $Icu4cRoot)) { $Icu4cRoot = $RepoRoot }
$Solution = Join-Path $Icu4cRoot 'source\allinone\allinone.sln'
if (-not (Test-Path -LiteralPath $Solution)) { throw "ICU solution not found: $Solution" }

if (-not $SkipBuild) {
    foreach ($config in @('Debug', 'Release')) {
        Invoke-RecordedCommand -Name "msbuild-$($config.ToLowerInvariant())" -FilePath 'cmake' -ArgumentList @(
            '-E',
            'env',
            'VSLANG=1033',
            $MSBuild,
            $Solution,
            '/m:1',
            '/t:Build',
            "/p:Configuration=$config",
            '/p:Platform=x64',
            '/p:PlatformToolset=v145',
            '/p:SkipUWP=true'
        ) -WorkingDirectory $Icu4cRoot | Out-Null
    }
}

$includeDir = Join-Path $Icu4cRoot 'include'
$libDir = Join-Path $Icu4cRoot 'lib64'
$binDir = Join-Path $Icu4cRoot 'bin64'
foreach ($required in @($includeDir, $libDir, $binDir)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Expected ICU build output missing: $required" }
}

foreach ($dir in @('include', 'lib64', 'bin64')) {
    New-Directory (Join-Path $Prefix $dir)
}
Get-ChildItem -LiteralPath $includeDir -Force | Copy-Item -Destination (Join-Path $Prefix 'include') -Recurse -Force
Get-ChildItem -LiteralPath $libDir -Force | Copy-Item -Destination (Join-Path $Prefix 'lib64') -Recurse -Force
Get-ChildItem -LiteralPath $binDir -Force | Copy-Item -Destination (Join-Path $Prefix 'bin64') -Recurse -Force

$licenseCandidates = @(
    (Join-Path $Icu4cRoot 'LICENSE'),
    (Join-Path $RepoRoot 'LICENSE')
)
$licensePath = $licenseCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $licensePath) { throw 'ICU license file not found' }

$probeRoot = Join-Path $Prefix 'verify-probe'
$probeBuild = Join-Path $probeRoot 'build'
New-Directory $probeRoot
New-Directory (Join-Path $probeRoot 'src')

$cmakeText = @'
cmake_minimum_required(VERSION 3.28)
project(c05_icu_probe LANGUAGES CXX)
find_package(ICU 77.1 EXACT REQUIRED COMPONENTS uc i18n data)
file(REAL_PATH "${C05_ICU_PREFIX}" C05_ICU_PREFIX_REAL)
function(c05_assert_under_prefix target prop)
    get_target_property(values ${target} ${prop})
    if(NOT values OR values STREQUAL "values-NOTFOUND")
        return()
    endif()
    foreach(value IN LISTS values)
        if(value MATCHES "^\\$<")
            continue()
        endif()
        file(REAL_PATH "${value}" real)
        cmake_path(IS_PREFIX C05_ICU_PREFIX_REAL "${real}" NORMALIZE inside)
        if(NOT inside)
            message(FATAL_ERROR "${target} ${prop} escaped ICU prefix: ${real}")
        endif()
        message(STATUS "${target} ${prop}: ${real}")
    endforeach()
endfunction()
foreach(target ICU::uc ICU::i18n ICU::data)
    foreach(prop INTERFACE_INCLUDE_DIRECTORIES IMPORTED_IMPLIB_DEBUG IMPORTED_IMPLIB_RELEASE IMPORTED_LOCATION_DEBUG IMPORTED_LOCATION_RELEASE)
        c05_assert_under_prefix(${target} ${prop})
    endforeach()
endforeach()
add_executable(c05_icu_probe src/main.cpp)
target_compile_features(c05_icu_probe PRIVATE cxx_std_17)
target_link_libraries(c05_icu_probe PRIVATE ICU::uc ICU::i18n ICU::data)
'@
$mainText = @'
#include <unicode/ubrk.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <unicode/uversion.h>
#include <cstdio>

int main() {
    UVersionInfo icu_version{};
    UVersionInfo unicode_version{};
    u_getVersion(icu_version);
    u_getUnicodeVersion(unicode_version);
    std::printf("ICU %u.%u Unicode %u.%u\n",
        icu_version[0], icu_version[1], unicode_version[0], unicode_version[1]);
    if (icu_version[0] != 77 || icu_version[1] != 1 ||
        unicode_version[0] != 16 || unicode_version[1] != 0) {
        return 4;
    }
    UErrorCode status = U_ZERO_ERROR;
    const UNormalizer2* nfc = unorm2_getNFCInstance(&status);
    if (U_FAILURE(status) || nfc == nullptr) {
        std::printf("normalizer failed %s\n", u_errorName(status));
        return 2;
    }
    UChar sample[] = { 0x0065, 0x0301, 0 };
    UChar out[8]{};
    status = U_ZERO_ERROR;
    const int32_t size = unorm2_normalize(nfc, sample, -1, out, 8, &status);
    if (U_FAILURE(status) || size != 1 || out[0] != 0x00e9) {
        std::printf("normalize failed %s size=%d first=%04x\n", u_errorName(status), size, out[0]);
        return 3;
    }
    return 0;
}
'@
$cmakeText | Set-Content -LiteralPath (Join-Path $probeRoot 'CMakeLists.txt') -Encoding UTF8
$mainText | Set-Content -LiteralPath (Join-Path $probeRoot 'src\main.cpp') -Encoding UTF8

if (-not $SkipProbe) {
    Invoke-RecordedCommand -Name 'cmake-probe-configure' -FilePath 'cmake' -ArgumentList @(
        '-S', $probeRoot,
        '-B', $probeBuild,
        '-G', 'Visual Studio 18 2026',
        '-A', 'x64',
        "-DCMAKE_PREFIX_PATH=$Prefix",
        "-DICU_ROOT=$Prefix",
        "-DC05_ICU_PREFIX=$Prefix"
    ) | Out-Null
    foreach ($config in @('Debug', 'Release')) {
        Invoke-RecordedCommand -Name "cmake-probe-build-$($config.ToLowerInvariant())" -FilePath 'cmake' -ArgumentList @('--build', $probeBuild, '--config', $config, '--parallel', '1') | Out-Null
        $exe = Join-Path $probeBuild "$config\c05_icu_probe.exe"
        Invoke-RecordedCommand -Name "run-probe-$($config.ToLowerInvariant())" -FilePath 'cmake' -ArgumentList @('-E', 'env', "PATH=$(Join-Path $Prefix 'bin64');$([Environment]::GetEnvironmentVariable('PATH', 'Process'))", $exe) | Out-Null
    }
}

$manifest = [ordered]@{
    verdict = 'PASS'
    generated_utc = (Get-Date).ToUniversalTime().ToString('o')
    repo_url = $RepoUrl
    required_commit = $Commit
    actual_commit = $actualCommit
    prefix = $Prefix
    source_root = $RepoRoot
    solution = $Solution
    msbuild = $MSBuild
    cmake = (& cmake --version | Select-Object -First 1)
    git = (& git --version)
    license = $licensePath
    license_sha256 = Get-Sha256OrNull $licensePath
    headers = Get-ChildItem -LiteralPath (Join-Path $Prefix 'include') -Filter '*.h' -Recurse | Measure-Object | Select-Object -ExpandProperty Count
    libraries = Get-ChildItem -LiteralPath (Join-Path $Prefix 'lib64') -Filter '*.lib' -Recurse | Select-Object FullName,Length,@{Name='sha256';Expression={Get-Sha256OrNull $_.FullName}}
    binaries = Get-ChildItem -LiteralPath (Join-Path $Prefix 'bin64') -Filter '*.dll' -Recurse | Select-Object FullName,Length,@{Name='sha256';Expression={Get-Sha256OrNull $_.FullName}}
    evidence_dir = $RunEvidenceDir
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $RunEvidenceDir 'summary.json') -Encoding UTF8
$manifest | ConvertTo-Json -Depth 8
