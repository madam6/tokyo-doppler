<#
.SYNOPSIS
    Compiles the C++ module for the editor (UnrealEditor-DspAp.dll).

.EXAMPLE
    .\Scripts\Build.ps1
    .\Scripts\Build.ps1 -EngineDir "D:\Epic\UE_5.7"
#>
param(
    [string]$EngineDir,
    [ValidateSet('Development', 'DebugGame')]
    [string]$Configuration = 'Development'
)

. "$PSScriptRoot\Common.ps1"

$engine = Find-UnrealEngine $EngineDir
Write-Host "Engine : $engine"
Write-Host "Project: $ProjectFile"

Invoke-Checked (Join-Path $engine 'Engine\Build\BatchFiles\Build.bat') @(
    'TokyoDopplerEditor', 'Win64', $Configuration,
    "-Project=$ProjectFile", '-WaitMutex', '-NoHotReloadFromIDE'
)

Write-Host "`nBuild succeeded. Open TokyoDoppler.uproject to launch the editor." -ForegroundColor Green
