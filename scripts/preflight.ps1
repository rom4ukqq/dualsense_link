param(
    [switch]$RequireAdmin,
    [switch]$RequireWdk,
    [switch]$RequireWinUi,
    [switch]$RequireTestSigning
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

$cmake = Get-CMakePath
$msbuild = Get-MSBuildPath
$isAdmin = Test-IsAdministrator
$hasWdk = Test-WdkInstalled
$hasWinUi = Test-WinUiToolchain
$isTestSigning = Test-TestSigningMode

Write-Host "Preflight checks:"
Write-Host ("  CMake:        " + ($(if($cmake) { $cmake } else { "not found" })))
Write-Host ("  MSBuild:      " + ($(if($msbuild) { $msbuild } else { "not found" })))
Write-Host ("  Admin:        " + ($(if($isAdmin) { "yes" } else { "no" })))
Write-Host ("  WDK:          " + ($(if($hasWdk) { "yes" } else { "no" })))
Write-Host ("  WinUI toolset:" + ($(if($hasWinUi) { "yes" } else { "no" })))
Write-Host ("  Test-signing: " + ($(if($isTestSigning) { "enabled" } else { "disabled" })))

if(-not $cmake) {
    throw "CMake not found. Install CMake and rerun."
}
if(-not $msbuild) {
    throw "MSBuild not found. Install Visual Studio Build Tools and rerun."
}
if($RequireAdmin -and -not $isAdmin) {
    throw "Administrator rights are required for this action."
}
if($RequireWdk -and -not $hasWdk) {
    throw "WDK not detected. Install Windows Driver Kit and rerun."
}
if($RequireWinUi -and -not $hasWinUi) {
    throw "WinUI toolchain not detected (Windows App SDK / WindowsXaml targets missing)."
}
if($RequireTestSigning -and -not $isTestSigning) {
    throw "Test-signing mode is disabled. Enable with: bcdedit /set testsigning on"
}

[PSCustomObject]@{
    RepoRoot = Get-RepoRoot
    CMake = $cmake
    MSBuild = $msbuild
    IsAdmin = $isAdmin
    WdkInstalled = $hasWdk
    WinUiToolchain = $hasWinUi
    TestSigning = $isTestSigning
}
