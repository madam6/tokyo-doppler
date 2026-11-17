<#
.SYNOPSIS
    Runs the DSP unit tests headless (no window, no GPU) and fails if any test fails.

.DESCRIPTION
    Uses Unreal's Automation framework. The same tests are visible in the editor under
    Tools > Test Automation, filtered by "DspAp".
    -NoSound is deliberately not passed: in UE 5.7 it crashes the editor on its first tick.

.EXAMPLE
    .\Scripts\RunTests.ps1
#>
param(
    [string]$EngineDir,
    [string]$Filter = 'DspAp'
)

. "$PSScriptRoot\Common.ps1"

$engine    = Find-UnrealEngine $EngineDir
$editorCmd = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$reportDir = Join-Path $ProjectRoot 'Saved\Automation\Report'

if (Test-Path $reportDir) { Remove-Item $reportDir -Recurse -Force }

& $editorCmd $ProjectFile `
    "-ExecCmds=Automation RunTests $Filter; Quit" `
    '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$reportDir" `
    -Unattended -NoPause -NullRHI -NoSplash -Log=AutomationTests.log | Out-Null

$index = Join-Path $reportDir 'index.json'
if (-not (Test-Path $index)) {
    throw "No test report produced. Check Saved\Logs\AutomationTests.log (did the module build? run Scripts\Build.ps1 first)."
}

# The report is UTF-8 with a BOM, which ConvertFrom-Json in Windows PowerShell 5.1 rejects
$report = [IO.File]::ReadAllText($index).TrimStart([char]0xFEFF) | ConvertFrom-Json

foreach ($t in $report.tests | Sort-Object fullTestPath) {
    $ok = $t.state -eq 'Success'
    $mark = if ($ok) { 'PASS' } else { 'FAIL' }
    $color = if ($ok) { 'Green' } else { 'Red' }
    Write-Host ("  [{0}] {1}" -f $mark, $t.fullTestPath) -ForegroundColor $color
    if (-not $ok) {
        $t.entries | Where-Object { $_.event.type -eq 'Error' } | ForEach-Object { Write-Host "         $($_.event.message)" -ForegroundColor Red }
    }
}

Write-Host ("`n{0} passed, {1} failed" -f $report.succeeded, $report.failed)
if ($report.failed -gt 0 -or $report.succeeded -eq 0) { exit 1 }
