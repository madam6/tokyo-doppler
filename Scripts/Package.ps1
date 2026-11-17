<#
.SYNOPSIS
    Cooks and packages a standalone Windows build of the demo.

.DESCRIPTION
    Requires the two Fab content packs to be installed in Content/ (see README).
    Output goes to Build\Package\Windows\. Run TokyoDoppler.exe from there.

.EXAMPLE
    .\Scripts\Package.ps1
    .\Scripts\Package.ps1 -Configuration Development
#>
param(
    [string]$EngineDir,
    [ValidateSet('Shipping', 'Development')]
    [string]$Configuration = 'Shipping'
)

. "$PSScriptRoot\Common.ps1"

foreach ($pack in 'TokyoStylizedEnvironment', 'RocketThrusterExhaustFX') {
    if (-not (Test-Path (Join-Path $ProjectRoot "Content\$pack"))) {
        throw "Content\$pack is missing. Add it from Fab first (see 'Third-party content' in the README)."
    }
}

$engine  = Find-UnrealEngine $EngineDir
$archive = Join-Path $ProjectRoot 'Build\Package'

Invoke-Checked (Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat') @(
    'BuildCookRun',
    "-project=$ProjectFile",
    '-target=TokyoDoppler', '-platform=Win64', "-clientconfig=$Configuration",
    '-build', '-cook', '-stage', '-pak', '-iostore', '-compressed', '-prereqs',
    '-archive', "-archivedirectory=$archive",
    '-nop4', '-utf8output', '-unattended', '-nodebuginfo'
)

Write-Host "`nPackaged build: $archive\Windows\TokyoDoppler.exe" -ForegroundColor Green
