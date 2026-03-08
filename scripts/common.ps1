Set-StrictMode -Version Latest

function Get-RepoRoot {
    return (Split-Path -Parent $PSScriptRoot)
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Get-CMakePath {
    $command = Get-Command cmake -ErrorAction SilentlyContinue
    if($null -ne $command) {
        return $command.Source
    }

    $fallback = "C:\Program Files\CMake\bin\cmake.exe"
    if(Test-Path $fallback) {
        return $fallback
    }
    return $null
}

function Get-MSBuildPath {
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )

    foreach($path in $candidates) {
        if(Test-Path $path) {
            return $path
        }
    }

    $fromPath = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if($null -ne $fromPath) {
        return $fromPath.Source
    }
    return $null
}

function Test-WdkInstalled {
    $candidates = @(
        "C:\Program Files (x86)\Windows Kits\10\build\WindowsDriver.Common.targets",
        "C:\Program Files (x86)\Windows Kits\10\Include\wdf\kmdf\1.33\wdf.h"
    )
    foreach($path in $candidates) {
        if(Test-Path $path) {
            return $true
        }
    }
    return $false
}

function Test-WinUiToolchain {
    $xamlTargets = Test-Path "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Microsoft\WindowsXaml\v17.0\Microsoft.Windows.UI.Xaml.CPP.Targets"
    $appSdkNuget = Test-Path "$env:USERPROFILE\.nuget\packages\microsoft.windowsappsdk"
    return $xamlTargets -or $appSdkNuget
}

function Test-TestSigningMode {
    $output = bcdedit /enum 2>$null
    if($LASTEXITCODE -ne 0) {
        return $false
    }

    $line = $output | Where-Object { $_ -match '^\s*testsigning\s+' } | Select-Object -First 1
    if($null -eq $line) {
        return $false
    }
    return ($line -match '(yes|on|да)\s*$')
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$ErrorMessage
    )

    & $FilePath @Arguments
    if($LASTEXITCODE -ne 0) {
        throw $ErrorMessage
    }
}
