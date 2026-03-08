param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$SkipBuild,
    [switch]$SkipUiRestore
)

$ErrorActionPreference = "Stop"

if(-not $SkipBuild) {
    & "$PSScriptRoot\build_backend.ps1" -Config $Config
    & "$PSScriptRoot\build_ui.ps1" -Config $Config -SkipRestore:$SkipUiRestore
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$uiExe = Join-Path $repoRoot ("ui\winui3\bin\" + $Config + "\DualSenseLinkUI.exe")

if(-not (Test-Path $uiExe)) {
    throw "UI executable not found: $uiExe"
}

Start-Process -FilePath $uiExe
Write-Host "DualSense Link WinUI started: $uiExe"
