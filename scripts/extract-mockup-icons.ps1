# Extracts real Windows icons (with the real FolderTags badges) from the reference screenshots
# into assets/brand/mockups/icons, for the English UI mockups. Run once after replacing a screenshot.
#   desktop_badges.png  : black desktop -> un-blended to transparent PNGs (alpha = max channel)
#   explorer_sidebar.png: nav-pane icons cropped as-is on the Explorer dark background (#191919)
Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$out  = Join-Path $root "assets\brand\mockups\icons"
New-Item -ItemType Directory -Force $out | Out-Null

function Save-Crop($src, [int]$x, [int]$y, [int]$w, [int]$h, $file, [switch]$Unblend) {
    $bmp = New-Object Drawing.Bitmap $w, $h, ([Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($j = 0; $j -lt $h; $j++) {
        for ($i = 0; $i -lt $w; $i++) {
            $p = $src.GetPixel($x + $i, $y + $j)
            if ($Unblend) {
                # Pixel was composited over black: c = a * color  ->  a = max(c), color = c / a
                $a = [Math]::Max($p.R, [Math]::Max($p.G, $p.B))
                if ($a -lt 6) { $bmp.SetPixel($i, $j, [Drawing.Color]::FromArgb(0, 0, 0, 0)); continue }
                $k = 255.0 / $a
                $bmp.SetPixel($i, $j, [Drawing.Color]::FromArgb($a, [int][Math]::Min(255, $p.R * $k),
                    [int][Math]::Min(255, $p.G * $k), [int][Math]::Min(255, $p.B * $k)))
            } else {
                $bmp.SetPixel($i, $j, $p)
            }
        }
    }
    $bmp.Save((Join-Path $out $file), [Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

# Desktop grid: cells of 116x125 starting at (15,22); icon occupies the top 62 px of a cell.
$desk = [Drawing.Bitmap]::FromFile((Join-Path $root "assets\reference\desktop_badges.png"))
$cells = [ordered]@{
    "projects" = 0, 0; "documents" = 1, 0; "designs" = 2, 0; "archives" = 3, 0
    "workspace" = 0, 1; "backups" = 2, 1; "notes" = 3, 1; "personal" = 4, 1
    "downloads" = 1, 2; "urgent" = 2, 2; "todo" = 3, 2; "music" = 4, 2
    "invoices" = 0, 3; "recyclebin" = 1, 3; "readme" = 3, 3; "family" = 4, 3
    "utilities" = 0, 4; "templates" = 3, 4
}
foreach ($name in $cells.Keys) {
    $c, $r = $cells[$name]
    Save-Crop $desk (15 + 116 * $c + 18) (22 + 125 * $r) 80 62 "d_$name.png" -Unblend
}
$desk.Dispose()

# Explorer nav pane (22x22 crops at 125% scaling) and content-list shortcut icons.
$exp = [Drawing.Bitmap]::FromFile((Join-Path $root "assets\reference\explorer_sidebar.png"))
$nav = [ordered]@{
    "home" = 42, 178; "gallery" = 42, 218; "tags" = 42, 258
    "desktop" = 42, 618; "downloads" = 42, 658; "documents" = 42, 698
    "pictures" = 42, 738; "music" = 42, 778; "videos" = 42, 818; "folder" = 42, 858
}
foreach ($name in $nav.Keys) { $x, $y = $nav[$name]; Save-Crop $exp $x $y 22 22 "e_$name.png" }
Save-Crop $exp 221 208 18 19 "e_link.png"
$exp.Dispose()
Write-Host "Icons -> $out"
