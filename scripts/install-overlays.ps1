# Lists the 7 FolderTags overlay handlers under HKLM so Explorer shows the colored dot on
# tagged folders. Requires administrator rights (self-elevates).
#
# Windows only uses the first ~11 overlay handlers in alphabetical order. Our keys start with
# two spaces so they sort before OneDrive (" OneDrive1".." OneDrive7"); the last OneDrive
# entries may stop showing their sync badge.
param([switch]$Remove)
$ErrorActionPreference = "Stop"

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    $a = "-ExecutionPolicy Bypass -File `"$PSCommandPath`""
    if ($Remove) { $a += " -Remove" }
    Start-Process powershell -Verb RunAs -ArgumentList $a -Wait
    return
}

$base = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers"
$names = "Rouge", "Orange", "Jaune", "Vert", "Bleu", "Violet", "Gris"
for ($i = 0; $i -lt 7; $i++) {
    $key = Join-Path $base ("  FolderTags{0}{1}" -f $i, $names[$i])
    $clsid = "{{B6E4CD56-987A-4C8F-8729-7FD3D4D8EBE{0}}}" -f $i
    if ($Remove) {
        Remove-Item $key -ErrorAction SilentlyContinue
    } else {
        New-Item $key -Force | Out-Null
        Set-Item $key -Value $clsid
    }
}
Write-Host ($(if ($Remove) { "Overlays retires." } else { "Overlays installes." }) + " Redemarrez l'Explorateur.")
