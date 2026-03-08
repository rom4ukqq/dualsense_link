param(
    [string]$PublishedName = ""
)

$ErrorActionPreference = "Stop"

& "$PSScriptRoot\preflight.ps1" -RequireAdmin -RequireWdk | Out-Null

if([string]::IsNullOrWhiteSpace($PublishedName)) {
    Write-Host "Installed OEM driver packages:"
    pnputil /enum-drivers | Select-String -Pattern "Published Name|Original Name|Provider Name"
    Write-Host ""
    Write-Host "Rerun with -PublishedName oemXX.inf"
    exit 0
}

Write-Host "Removing driver package: $PublishedName"
pnputil /delete-driver $PublishedName /uninstall /force

Write-Host "Done."
