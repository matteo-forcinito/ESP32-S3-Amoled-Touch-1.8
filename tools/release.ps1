param([Parameter(Mandatory=$true)][ValidatePattern('^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.]+)?$')][string]$Version,[string]$SourceRef='main',[string]$Gh='gh')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$commonRoot=Join-Path $projectRoot 'watch-os-common'
if (-not (Test-Path -LiteralPath (Join-Path $commonRoot 'tools/release_firmware.py'))) { $commonRoot=Join-Path (Split-Path -Parent $projectRoot) 'watch-os-common' }
python (Join-Path $commonRoot 'tools/release_firmware.py') --repo matteo-forcinito/ESP32-S3-Amoled-Touch-1.8 --product amoled --version $Version --source-ref $SourceRef --project $projectRoot --gh $Gh
if ($LASTEXITCODE -ne 0) { throw 'Release did not complete' }
