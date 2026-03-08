param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$SkipRestore
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\common.ps1"

$checks = & "$PSScriptRoot\preflight.ps1" -RequireWinUi
$repoRoot = $checks.RepoRoot
$project = Join-Path $repoRoot "ui\winui3\DualSenseLinkUI.vcxproj"

if(-not (Test-Path $project)) {
    throw "WinUI project not found: $project"
}

if($SkipRestore) {
    $target = "/t:Build"
} else {
    $target = "/t:Restore,Build"
}

Invoke-Checked -FilePath $checks.MSBuild `
    -Arguments @(
        $project,
        $target,
        "/p:Configuration=$Config",
        "/p:Platform=x64",
        "/m"
    ) `
    -ErrorMessage "WinUI build failed."

Write-Host "WinUI build completed ($Config): $project"
