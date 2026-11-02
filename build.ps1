# WiseEngine build entry point.
# How to Run:
#   .\build.ps1 -Config Debug -Target all -Tests
#   .\build.ps1 -Config Release -Target all -Tests -BuildDirectory build/clean-release
# Prerequisites: CMake 3.31+, Ninja, VS2022 C++ x64 workload; bootstrap SDKs first.
param(
    [ValidateSet('Debug','Release','RelWithDebInfo')][string]$Config = 'Debug',
    [ValidateSet('clean','configure','build','all')][string]$Target = 'all',
    [ValidateRange(1,128)][int]$Jobs = 6,
    [switch]$Tests,
    [string]$BuildDirectory = '',
    [string[]]$CMakeArgs = @()
)
$ErrorActionPreference = 'Stop'
$engineRoot = $PSScriptRoot
$preset = $Config.ToLowerInvariant()
if (-not $BuildDirectory) { $BuildDirectory = "build/$preset" }
$buildPath = [IO.Path]::GetFullPath((Join-Path $engineRoot $BuildDirectory))
$buildParent = [IO.Path]::GetFullPath((Join-Path $engineRoot 'build')) + [IO.Path]::DirectorySeparatorChar
if (-not $buildPath.StartsWith($buildParent, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildDirectory must be a child of this checkout build directory.'
}
# Prerequisites: import the toolchain environment only for this PowerShell process.
foreach ($tool in @('cmake','ninja')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing prerequisite: $tool" }
}
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ build tools are required.' }
    $installation = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) { throw 'Install the VS2022 Desktop development with C++ workload.' }
    $vcvars = Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
    $lines = & $env:ComSpec /d /c "call `"$vcvars`" >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'MSVC x64 environment initialization failed.' }
    foreach ($line in $lines) {
        if ($line -match '^([^=]+)=(.*)$') { Set-Item -LiteralPath "Env:$($matches[1])" -Value $matches[2] }
    }
}
Push-Location -LiteralPath $engineRoot
try {
    # Explicit cleanup is confined to one resolved build directory, never a source or SDK tree.
    if ($Target -eq 'clean') {
        if (Test-Path -LiteralPath $buildPath) {
            $resolved = (Resolve-Path -LiteralPath $buildPath).Path
            if (-not $resolved.StartsWith($buildParent,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe cleanup target.' }
            Remove-Item -LiteralPath $resolved -Recurse -Force
        }
        return
    }
    if ($Target -in @('configure','all')) {
        $arguments = @('--preset',$preset,'-B',$buildPath) + $CMakeArgs
        if ($Tests) { $arguments += '-DWISEENGINE_BUILD_TESTS=ON' }
        & cmake @arguments
        if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
    }
    if ($Target -in @('build','all')) {
        & cmake --build $buildPath --parallel $Jobs
        if ($LASTEXITCODE -ne 0) { throw 'CMake build failed.' }
        if ($Tests) {
            & ctest --test-dir $buildPath --output-on-failure --no-tests=error
            if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }
        }
    }
} finally { Pop-Location }
