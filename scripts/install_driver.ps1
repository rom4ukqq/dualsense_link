param(
    [string]$InfPath = ".\driver\dualsense_link_vhf.inf",
    [string]$DriverBinaryPath = ".\build\driver\Debug\dsl_driver.sys"
)

$ErrorActionPreference = "Stop"

$checks = & "$PSScriptRoot\preflight.ps1" -RequireAdmin -RequireWdk -RequireTestSigning

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
$inf = Join-Path $repoRoot $InfPath
$sys = Join-Path $repoRoot $DriverBinaryPath

if(-not (Test-Path $inf)) {
    throw "INF file not found: $inf"
}

if(-not (Test-Path $sys)) {
    throw "Driver binary not found: $sys. Build/sign dsl_driver.sys first, then rerun."
}

Write-Host "Installing driver package from: $inf"
pnputil /add-driver $inf /install

Write-Host "Done. If needed, reboot Windows."
