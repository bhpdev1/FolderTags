# Renders the brand assets (logo PNGs, English UI mockups, README hero, GitHub social preview)
# from assets/brand/*.svg|html using headless Microsoft Edge.
#   .\scripts\render-brand.ps1              # everything
#   .\scripts\render-brand.ps1 -Only menu   # sources whose path contains "menu"
param([string]$Only)
$ErrorActionPreference = "Stop"

$root  = Split-Path $PSScriptRoot -Parent
$brand = Join-Path $root "assets\brand"
$edge  = @(
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw "Microsoft Edge not found." }

$profileDir = Join-Path $env:TEMP "foldertags-render"

function Render([string]$src, [string]$out, [int]$w, [int]$h, [double]$scale, [switch]$Transparent) {
    $url  = "file:///" + ((Join-Path $brand $src) -replace '\\', '/')
    $argv = @(
        "--headless=new", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
        "--user-data-dir=`"$profileDir`"", "--allow-file-access-from-files",
        "--window-size=$w,$h", "--force-device-scale-factor=$scale",
        "--virtual-time-budget=3000", "--screenshot=`"$out`""
    )
    if ($Transparent) { $argv += "--default-background-color=00000000" }
    $argv += $url
    Remove-Item $out -ErrorAction SilentlyContinue
    Start-Process $edge -ArgumentList $argv -Wait -NoNewWindow
    if (-not (Test-Path $out)) { throw "Render failed: $out" }
    Write-Host ("{0,-40} {1}x{2} @{3}x" -f (Split-Path $out -Leaf), $w, $h, $scale)
}

$shots = Join-Path $root "assets\screenshots"
New-Item -ItemType Directory -Force $shots | Out-Null

$jobs = @(
    # source (relative to assets/brand)   output                                         w     h    scale
    @("logo.svg",                    (Join-Path $brand "logo-256.png"),               256,  256, 1, $true),
    @("logo.svg",                    (Join-Path $brand "logo-512.png"),               256,  256, 2, $true),
    # English UI mockups (CSS px = 125 % screenshot px)
    @("mockups\context-menu.html",   (Join-Path $shots "context-menu.png"),          1000,  700, 1.6, $false),
    @("mockups\tags-window.html",    (Join-Path $shots "tags-window.png"),            640,  448, 2.5, $false),
    @("mockups\explorer.html",       (Join-Path $shots "explorer.png"),              1000,  700, 1.6, $false),
    @("mockups\desktop.html",        (Join-Path $shots "desktop.png"),               1000,  700, 1.6, $false),
    # README hero + GitHub social preview
    @("hero.html",                   (Join-Path $root  "assets\hero.png"),           1280,  640, 2, $false),
    @("hero.html",                   (Join-Path $brand "social-preview.png"),        1280,  640, 1, $false)
)
foreach ($j in $jobs) {
    if ($Only -and ($j[0] -notlike "*$Only*")) { continue }
    Render $j[0] $j[1] $j[2] $j[3] $j[4] -Transparent:$j[5]
}
