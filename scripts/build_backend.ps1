param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$BuildDriver
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

$checks = & "$PSScriptRoot\preflight.ps1" -RequireWdk:$BuildDriver
$repoRoot = $checks.RepoRoot
$buildDir = Join-Path $repoRoot "build"

Invoke-Checked -FilePath $checks.CMake `
    -Arguments @("-S", $repoRoot, "-B", $buildDir, "-G", "Visual Studio 17 2022", "-A", "x64") `
    -ErrorMessage "CMake configure failed."

$targets = @("dsl_service")
if($BuildDriver) {
    $targets += "dsl_driver"
}

$buildArgs = @("--build", $buildDir, "--config", $Config, "--target") + $targets
Invoke-Checked -FilePath $checks.CMake `
    -Arguments $buildArgs `
    -ErrorMessage "Backend build failed."
Write-Host "Backend build completed ($Config): $($targets -join ', ')"
