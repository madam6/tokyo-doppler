<#
.SYNOPSIS
    Compiles the C++ module for the editor (UnrealEditor-DspAp.dll).

.EXAMPLE
    .\Scripts\Build.ps1
    .\Scripts\Build.ps1 -EngineDir "D:\Epic\UE_5.7"
    .\Scripts\Build.ps1 -MaxParallelActions 4    # low-memory machines (compiler error C3859 / C1076)
#>
param(
    [string]$EngineDir,
    [ValidateSet('Development', 'DebugGame')]
    [string]$Configuration = 'Development',
    [int]$MaxParallelActions = 0
)

. "$PSScriptRoot\Common.ps1"

$engine = Find-UnrealEngine $EngineDir
Write-Host "Engine : $engine"
Write-Host "Project: $ProjectFile"

$ubtArgs = @(
    'TokyoDopplerEditor', 'Win64', $Configuration,
    "-Project=$ProjectFile", '-WaitMutex', '-NoHotReloadFromIDE'
)
if ($MaxParallelActions -gt 0) { $ubtArgs += "-MaxParallelActions=$MaxParallelActions" }

Invoke-Checked (Join-Path $engine 'Engine\Build\BatchFiles\Build.bat') $ubtArgs

Write-Host "`nBuild succeeded. Open TokyoDoppler.uproject to launch the editor." -ForegroundColor Green
