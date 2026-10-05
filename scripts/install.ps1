# Installs (or updates) the shell extension for the current user, then restarts Explorer.
$ErrorActionPreference = "Stop"
# Look for FolderTags.dll in same folder (release bundle) or build folder (repo)
$src = Join-Path $PSScriptRoot "FolderTags.dll"
if (-not (Test-Path $src)) {
    $root = Split-Path $PSScriptRoot -Parent
    $src = Join-Path $root "build\FolderTags.dll"
}
if (-not (Test-Path $src)) {
    $buildScript = Join-Path $PSScriptRoot "build.ps1"
    if (Test-Path $buildScript) {
        & $buildScript
    } else {
        throw "FolderTags.dll introuvable."
    }
}

$dest = Join-Path $env:LOCALAPPDATA "FolderTags"
New-Item -ItemType Directory -Force $dest | Out-Null
$dll = Join-Path $dest "FolderTags.dll"

# Overlay handlers are loaded by every app that shows file icons, so the DLL is often locked.
# A loaded DLL can still be renamed: move it aside, copy the new one, restart Explorer.
Get-ChildItem $dest -Filter "FolderTags.*.old" -ErrorAction SilentlyContinue |
    Remove-Item -Force -ErrorAction SilentlyContinue
try {
    Copy-Item $src $dll -Force
} catch {
    Move-Item $dll (Join-Path $dest ("FolderTags.{0}.old" -f [DateTime]::Now.Ticks)) -Force
    Copy-Item $src $dll -Force
    Write-Host "DLL verrouillee, remplacement en differe..."
}

$p = Start-Process "$env:SystemRoot\System32\regsvr32.exe" -ArgumentList "/s `"$dll`"" -Wait -PassThru
if ($p.ExitCode -ne 0) { throw "regsvr32 failed ($($p.ExitCode))" }
Write-Host "FolderTags installe : $dll"

# Register icon overlays in HKLM if install-overlays.ps1 is available
$overlaysScript = Join-Path $PSScriptRoot "install-overlays.ps1"
if (Test-Path $overlaysScript) {
    try {
        & $overlaysScript
    } catch {
        Write-Warning "Enregistrement des overlays reporte : $($_.Exception.Message)"
    }
}

Write-Host "Redemarrage de l'Explorateur pour recharger les overlays..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
if (-not (Get-Process explorer -ErrorAction SilentlyContinue)) { Start-Process explorer.exe }
Write-Host "Explorateur redemarre avec succes."
