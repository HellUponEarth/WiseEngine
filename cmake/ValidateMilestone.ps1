# WiseEngine clean milestone validation.
# How to Run: .\cmake\ValidateMilestone.ps1 -Jobs 6
# Each invocation uses new Debug/Release build directories; logs and JSON stay in build/validation.
param([ValidateRange(1,128)][int]$Jobs = 6)
$ErrorActionPreference = 'Stop'
$engineRoot = Split-Path -Parent $PSScriptRoot
foreach ($tool in @('python','git','cmake','ninja')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing prerequisite: $tool" }
}
Push-Location -LiteralPath $engineRoot
try {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $relative = "build/validation/$stamp"
    $output = Join-Path $engineRoot $relative
    if (Test-Path -LiteralPath $output) { throw 'Validation output already exists; run again with a new timestamp.' }
    New-Item -ItemType Directory -Path $output | Out-Null
    & python cmake/VerifyThirdParty.py *> (Join-Path $output 'dependencies.log')
    if ($LASTEXITCODE -ne 0) { throw "SDK verification failed; inspect $output/dependencies.log" }
    $lanes = @()
    foreach ($config in @('Debug','Release')) {
        $lower = $config.ToLowerInvariant()
        $lane = "$relative/$lower"
        & "$engineRoot/build.ps1" -Config $config -Target all -Tests -Jobs $Jobs -BuildDirectory $lane *> (Join-Path $output "$lower.log")
        if (-not $?) { throw "$config build failed; inspect $output/$lower.log" }
        $exe = Join-Path $engineRoot "$lane/bin/WiseEngine.exe"
        $identity = & $exe --build-info
        if ($LASTEXITCODE -ne 0) { throw "$config build identity check failed" }
        $lanes += @{ configuration=$config; result='PASS'; build=$lane; identity=$identity; log="$lower.log" }
    }
    # Use the executable identity so future milestone reports carry the tested version.
    $version = ($lanes[0].identity | Where-Object { $_ -like 'version=*' }) -replace '^version=', ''
    if (-not $version) { throw 'Executable version is missing from build identity.' }
    $report = @{
        milestone=$version; completedUtc=[DateTime]::UtcNow.ToString('o'); lanes=$lanes
        dependencyLockSha256=(Get-FileHash thirdparty/dependencies.lock.json -Algorithm SHA256).Hash.ToLowerInvariant()
        scope='Clean build directories and automated CTest coverage. Desktop interaction is validated separately.'
    }
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'results.json') -Encoding UTF8
    Write-Output "Milestone validation passed: $output/results.json"
} finally { Pop-Location }
