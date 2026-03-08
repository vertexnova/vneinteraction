#==============================================================================
# VneInteraction Windows Build Script (PowerShell)
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Minimal: configure, build, test. Uses CMake with default generator.
#==============================================================================

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Debug",
    [ValidateSet("shared", "static")]
    [string]$LibType = "shared",
    [ValidateSet("configure", "build", "configure_and_build", "test")]
    [string]$Action = "configure_and_build",
    [switch]$Clean,
    [int]$Jobs = 10
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Prefer Visual Studio generator if available
$Generator = ""
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath 2>$null
    if ($vsPath) {
        $Generator = "-G `"Visual Studio 17 2022`" -A x64"
    }
}
if (-not $Generator) {
    $Generator = "-G `"Ninja`" -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe"
}

# Build dir: build/<LibType>/<BuildType>/build-windows-msvc (matches bash script layout)
$BuildDir = Join-Path $ProjectRoot "build\$LibType\$BuildType\build-windows-msvc"
$ConfigureCmd = "cmake -B `"$BuildDir`" -S `"$ProjectRoot`" -DCMAKE_BUILD_TYPE=$BuildType -DVNE_TEMPLATE_LIB_TYPE=$LibType -DVNE_TEMPLATE_TESTS=ON $Generator"
$BuildCmd = "cmake --build `"$BuildDir`" --config $BuildType --parallel $Jobs"
$TestCmd = "ctest --test-dir `"$BuildDir`" --output-on-failure -C $BuildType"

function Clean-Build {
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

function Ensure-BuildDir {
    if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null }
}

Write-Host "Windows :: MSVC ($BuildType, $LibType)"
Write-Host ""

Set-Location $ProjectRoot
switch ($Action) {
    "configure" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Expression $ConfigureCmd
    }
    "build" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Expression $ConfigureCmd
        Invoke-Expression $BuildCmd
    }
    "configure_and_build" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Expression $ConfigureCmd
        Invoke-Expression $BuildCmd
    }
    "test" {
        if ($Clean) { Clean-Build }; Ensure-BuildDir | Out-Null
        Invoke-Expression $ConfigureCmd
        Invoke-Expression $BuildCmd
        Invoke-Expression $TestCmd
    }
    default {
        Write-Host "Usage: .\build_windows.ps1 [-BuildType Debug|Release|...] [-LibType shared|static] [-Action configure|build|configure_and_build|test] [-Clean] [-Jobs N]"
        exit 1
    }
}

Write-Host ""
Write-Host "=== Build completed successfully ==="
Write-Host "Build directory: $BuildDir"
