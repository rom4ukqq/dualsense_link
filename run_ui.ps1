param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$SkipBuild,
    [switch]$SkipUiRestore
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $repoRoot "scripts\run_app.ps1") -Config $Config -SkipBuild:$SkipBuild -SkipUiRestore:$SkipUiRestore
