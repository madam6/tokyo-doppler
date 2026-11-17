# Shared helpers for the build scripts. Dot-source this file; don't run it directly.

$ErrorActionPreference = 'Stop'

$script:EngineVersion = '5.7'
$script:ProjectRoot   = Split-Path -Parent $PSScriptRoot
$script:ProjectFile   = Join-Path $ProjectRoot 'TokyoDoppler.uproject'

# Finds an Unreal Engine 5.7 install. Checked in this order:
#   1. -EngineDir argument passed to the calling script
#   2. UE_ENGINE_DIR environment variable
#   3. Epic Games Launcher install list
#   4. Registry entry written by the launcher
#   5. Default launcher install path
function Find-UnrealEngine {
    param([string]$Override)

    $candidates = @()
    if ($Override)            { $candidates += $Override }
    if ($env:UE_ENGINE_DIR)   { $candidates += $env:UE_ENGINE_DIR }

    $launcherDat = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $launcherDat) {
        $installs = (Get-Content $launcherDat -Raw | ConvertFrom-Json).InstallationList
        $candidates += $installs | Where-Object { $_.AppName -eq "UE_$EngineVersion" } | ForEach-Object { $_.InstallLocation }
    }

    $regKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$EngineVersion"
    if (Test-Path $regKey) {
        $candidates += (Get-ItemProperty $regKey).InstalledDirectory
    }

    $candidates += "C:\Program Files\Epic Games\UE_$EngineVersion"

    foreach ($dir in $candidates) {
        if ($dir -and (Test-Path (Join-Path $dir 'Engine\Build\BatchFiles\Build.bat'))) {
            return (Resolve-Path $dir).Path
        }
    }

    throw "Unreal Engine $EngineVersion not found. Install it from the Epic Games Launcher, or pass -EngineDir / set UE_ENGINE_DIR to the folder that contains 'Engine'."
}

function Invoke-Checked {
    param([string]$Exe, [string[]]$Arguments)
    Write-Host ">> $Exe $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$([IO.Path]::GetFileName($Exe)) exited with code $LASTEXITCODE" }
}
