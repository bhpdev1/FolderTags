# Removes the shell extension for the current user (tags already set on folders are kept).
$ErrorActionPreference = "Stop"

# Unregister overlays if script is available
$overlaysScript = Join-Path $PSScriptRoot "install-overlays.ps1"
if (Test-Path $overlaysScript) {
    try { & $overlaysScript -Remove } catch {}
}

$dll = Join-Path $env:LOCALAPPDATA "FolderTags\FolderTags.dll"
if (Test-Path $dll) { Start-Process "$env:SystemRoot\System32\regsvr32.exe" -ArgumentList "/s /u `"$dll`"" -Wait }

Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800
Remove-Item (Split-Path $dll) -Recurse -Force -ErrorAction SilentlyContinue
if (-not (Get-Process explorer -ErrorAction SilentlyContinue)) { Start-Process explorer.exe }
Write-Host "FolderTags desinstalle."
