param(
    [Parameter(Mandatory)][string]$ProgramPath,
    [Parameter(Mandatory)][string]$Output,
    [string]$QtPrefix = ''
)
$ErrorActionPreference = 'Stop'
$courseRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $courseRoot 'build')) + [IO.Path]::DirectorySeparatorChar
$program = (Resolve-Path -LiteralPath $ProgramPath).Path
$reportPath = [IO.Path]::GetFullPath($Output)
foreach ($candidate in @($program, $reportPath)) {
    if (-not $candidate.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Native UI check accepts only course build executables and outputs.'
    }
}
if (Test-Path -LiteralPath $reportPath) { throw 'Choose a new report path; existing evidence is preserved.' }
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($reportPath)) | Out-Null
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
$process = $null
$report = [ordered]@{ program = $program; status = 'FAIL'; evidence = @(); error = ''; cleanup_error = '' }
try {
    $startInfo = [Diagnostics.ProcessStartInfo]::new($program)
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.WorkingDirectory = [IO.Path]::GetDirectoryName($program)
    $startInfo.Environment['QT_QPA_PLATFORM'] = 'windows'
    $startInfo.Environment['QT_MEDIA_BACKEND'] = 'ffmpeg'
    if ($QtPrefix) { $startInfo.Environment['PATH'] = (Join-Path $QtPrefix 'bin') + ';' + $env:PATH }
    $process = [Diagnostics.Process]::Start($startInfo)
    $condition = [Windows.Automation.PropertyCondition]::new(
        [Windows.Automation.AutomationElement]::ProcessIdProperty, $process.Id)
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $window = $null
    while ($timer.ElapsedMilliseconds -lt 8000 -and -not $process.HasExited) {
        $window = [Windows.Automation.AutomationElement]::RootElement.FindFirst(
            [Windows.Automation.TreeScope]::Children, $condition)
        if ($null -ne $window) { break }
        Start-Sleep -Milliseconds 50
    }
    if ($null -eq $window) { throw 'No native UI Automation window for the launched process.' }
    $report.evidence += @{ window_name = $window.Current.Name; control_type = $window.Current.ControlType.ProgrammaticName }
    if ($window.Current.Name -notlike '*MediaWorkbench*') { throw 'Unexpected native window identity.' }
    $window.SetFocus()
    foreach ($name in @('Media filter', 'Media list', 'Open media files', 'Play or pause')) {
        $element = $window.FindFirst([Windows.Automation.TreeScope]::Descendants,
            [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::NameProperty, $name))
        if ($null -eq $element) { throw "Missing native accessibility element: $name" }
        $report.evidence += @{ name = $name; type = $element.Current.ControlType.ProgrammaticName; focusable = $element.Current.IsKeyboardFocusable }
        if ($name -eq 'Media filter') {
            if (-not $element.Current.IsKeyboardFocusable) { throw 'Filter is not keyboard focusable.' }
            $element.SetFocus()
            $value = $element.GetCurrentPattern([Windows.Automation.ValuePattern]::Pattern)
            $value.SetValue('native-probe')
            if ($value.Current.Value -ne 'native-probe') { throw 'Native edit value did not reach the Qt control.' }
            $value.SetValue('')
        }
    }
    $windowPattern = $window.GetCurrentPattern([Windows.Automation.WindowPattern]::Pattern)
    $windowPattern.Close()
    if (-not $process.WaitForExit(5000)) { throw 'Window close did not settle the application and worker.' }
    if ($process.ExitCode -ne 0) { throw "Native application exited with $($process.ExitCode)." }
    $report.status = 'PASS'
} catch {
    $report.error = $_.Exception.Message
} finally {
    if ($null -ne $process) {
        if (-not $process.HasExited) {
            try {
                # This fixed application launches Qt threads, not external worker processes.
                $process.Kill($true)
                if (-not $process.WaitForExit(3000)) { throw 'Owned process remains alive.' }
            } catch { $report.cleanup_error = $_.Exception.Message; $report.status = 'FAIL' }
        }
        $process.Dispose()
    }
    $report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $reportPath -Encoding utf8
}
Write-Output "Native UI Automation: $($report.status)"
if ($report.status -ne 'PASS') { Write-Output $report.error; exit 1 }
