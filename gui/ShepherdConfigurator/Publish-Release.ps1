param(
    [switch]$Launch
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectFile = Join-Path $projectRoot "ShepherdConfigurator.csproj"
$releaseDir = Join-Path $projectRoot "artifacts\\release\\ShepherdConfigurator"
$exePath = Join-Path $releaseDir "ShepherdConfigurator.exe"

Write-Host "Publishing ShepherdPatch Configurator release..." -ForegroundColor DarkYellow
dotnet publish $projectFile -c Release /p:PublishProfile=win-x64

Write-Host ""
Write-Host "Release published to:" -ForegroundColor DarkYellow
Write-Host "  $releaseDir"

if (Test-Path $exePath) {
    Write-Host ""
    Write-Host "Launch with:" -ForegroundColor DarkYellow
    Write-Host "  $exePath"
}

if ($Launch -and (Test-Path $exePath)) {
    Start-Process $exePath
}
