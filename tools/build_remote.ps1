# Build the board-specific Remote image, then package it for SD or web upload.
param(
    [ValidateSet('watcher', 'amoled', 'amoled175', 'knob')]
    [string]$Board = 'watcher',
    [switch]$Development,
    [switch]$ReuseDevelopmentBuild
)
$ErrorActionPreference = 'Stop'
$env:PYTHONUTF8 = '1'
function Write-IfChanged([string]$Path, [string]$Text)
{
    if (-not (Test-Path -LiteralPath $Path) -or [IO.File]::ReadAllText($Path) -cne $Text)
    {
        [IO.File]::WriteAllText($Path, $Text)
    }
}
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
$familyRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$projects = @{
    watcher = 'SenseCap-Watcher-W1-A'
    amoled = 'ESP32-S3-Amoled-Touch-1.8'
    amoled175 = 'ESP32-S3-Touch-AMOLED-1.75'
    knob = 'ESP32-S3-Knob-Touch-LCD-1.8'
}
$projectRoot = Join-Path $familyRoot $projects[$Board]
$env:WATCH_OS_COMMON_DIR = (Join-Path $familyRoot 'watch-os-common').Replace('\', '/')
$appRoot = Join-Path $projectRoot 'external_apps/remote'
$buildRoot = Join-Path $appRoot $(if ($Development -or $ReuseDevelopmentBuild) { 'build-remote-dev' } else { 'build-remote' })
New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
$configPath = (Join-Path $buildRoot 'sdkconfig').Replace('\', '/')
$defaults = (Join-Path $appRoot 'sdkconfig.defaults').Replace('\', '/')
if ($Development)
{
    if (Test-Path -LiteralPath $configPath)
    {
        $config = [IO.File]::ReadAllText($configPath)
        $config = $config -replace '(?m)^# CONFIG_WATCH_DEV_CONSOLE is not set\r?$', 'CONFIG_WATCH_DEV_CONSOLE=y'
        Write-IfChanged $configPath $config
    }
    $extra = Join-Path $buildRoot 'console.defaults'
    Write-IfChanged $extra "CONFIG_WATCH_DEV_CONSOLE=y`nCONFIG_WATCH_MONKEY_TEST=n`n"
    $defaults += ';' + $extra.Replace('\', '/')
}
elseif ($ReuseDevelopmentBuild)
{
    # Keep compiled dependencies, but explicitly disable the development console.
    if (-not (Test-Path -LiteralPath $configPath)) { throw 'No development configuration to reuse' }
    $config = [IO.File]::ReadAllText($configPath)
    $config = $config -replace '(?m)^CONFIG_WATCH_DEV_CONSOLE=y\r?$', '# CONFIG_WATCH_DEV_CONSOLE is not set'
    Write-IfChanged $configPath $config
}
$logPath = Join-Path $buildRoot 'build.log'
$ErrorActionPreference = 'Continue'
idf.py -C $appRoot -B $buildRoot -D "SDKCONFIG=$configPath" -D "SDKCONFIG_DEFAULTS=$defaults" build *> $logPath
$buildExit = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
Get-Content -LiteralPath $logPath -Tail 24
if ($buildExit -ne 0) { exit $buildExit }
if (-not $Development -and (Select-String -LiteralPath $configPath -Pattern '^CONFIG_WATCH_DEV_CONSOLE=y$' -Quiet))
{
    throw 'Refusing to package a firmware with the development console enabled'
}
if (-not $Development)
{
    $package = Join-Path $projectRoot 'dist/remote/apps/Remote'
    New-Item -ItemType Directory -Path $package -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $buildRoot 'remote.bin') -Destination (Join-Path $package 'Remote.bin')
    Copy-Item -LiteralPath (Join-Path $appRoot 'manifest.json'), (Join-Path $appRoot 'icon.png') -Destination $package
    $zipPath = Join-Path $projectRoot "dist/remote-$Board-1.0.0.zip"
    Compress-Archive -LiteralPath (Join-Path $projectRoot 'dist/remote/apps') -DestinationPath $zipPath -Force
    $hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText("$zipPath.sha256", "$hash  $([IO.Path]::GetFileName($zipPath))`n")
    Write-Host "Pacchetto: $zipPath"
}
exit 0
