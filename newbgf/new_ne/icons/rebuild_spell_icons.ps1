Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = "Stop"

$repo = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen"
$srcDir = Join-Path $repo "newbgf\new_ne\icons"
$workDir = Join-Path $srcDir "_rebuilt_32"
$palFile = Join-Path $repo "blakston.pal"
$makebgf = Join-Path $repo "bin\makebgf.exe"

$destDirs = @(
    (Join-Path $repo "run\localclient\resource"),
    (Join-Path $repo "resource\graphics"),
    (Join-Path $repo "run\server\rsc")
)

if (-not (Test-Path $workDir)) {
    New-Item -ItemType Directory -Path $workDir | Out-Null
}

function Load-M59Palette {
    param([string]$PaletteFile)
    $entries = New-Object System.Collections.Generic.List[System.Drawing.Color]
    foreach ($line in Get-Content -Path $PaletteFile) {
        $s = $line.Trim()
        if ($s -eq "") { continue }
        $parts = $s -split "\s+"
        if ($parts.Count -lt 3) { continue }
        $entries.Add([System.Drawing.Color]::FromArgb(255, [int]$parts[0], [int]$parts[1], [int]$parts[2]))
    }
    if ($entries.Count -lt 256) {
        throw "Palette file has only $($entries.Count) entries; expected 256."
    }
    return ,$entries[0..255]
}

function Convert-Icon {
    param(
        [string]$InputFile,
        [string]$OutputFile,
        [System.Drawing.Color[]]$Palette
    )

    $size = 32
    $src = [System.Drawing.Bitmap]::FromFile($InputFile)

    # Treat magenta background as transparent and auto-crop.
    $minX = $src.Width
    $minY = $src.Height
    $maxX = -1
    $maxY = -1
    for ($y = 0; $y -lt $src.Height; $y++) {
        for ($x = 0; $x -lt $src.Width; $x++) {
            $p = $src.GetPixel($x, $y)
            $isMagenta = ($p.R -ge 245 -and $p.G -le 10 -and $p.B -ge 245)
            if (-not $isMagenta) {
                if ($x -lt $minX) { $minX = $x }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    if ($maxX -lt $minX -or $maxY -lt $minY) {
        $minX = 0; $minY = 0; $maxX = $src.Width - 1; $maxY = $src.Height - 1
    }

    $cropW = $maxX - $minX + 1
    $cropH = $maxY - $minY + 1
    $fit = [Math]::Min(($size - 2) / $cropW, ($size - 2) / $cropH)
    $drawW = [int][Math]::Round($cropW * $fit)
    $drawH = [int][Math]::Round($cropH * $fit)
    $drawX = [int][Math]::Round(($size - $drawW) / 2.0)
    $drawY = [int][Math]::Round(($size - $drawH) / 2.0)

    $canvas = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($canvas)
    $g.Clear([System.Drawing.Color]::FromArgb(255, 255, 0, 255))
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $srcRect = New-Object System.Drawing.Rectangle($minX, $minY, $cropW, $cropH)
    $dstRect = New-Object System.Drawing.Rectangle($drawX, $drawY, $drawW, $drawH)
    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    $src.Dispose()

    $dst = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
    $pal = $dst.Palette
    for ($i = 0; $i -lt 256; $i++) {
        $pal.Entries[$i] = $Palette[$i]
    }
    # M59 transparency index.
    $pal.Entries[254] = [System.Drawing.Color]::FromArgb(255, 255, 0, 255)
    $dst.Palette = $pal

    $rect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
    $data = $dst.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $dst.PixelFormat)
    $stride = $data.Stride
    $raw = New-Object byte[] ($stride * $size)

    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            $p = $canvas.GetPixel($x, $y)
            $isMagenta = ($p.R -ge 245 -and $p.G -le 10 -and $p.B -ge 245)
            if ($isMagenta) {
                $raw[$y * $stride + $x] = [byte]254
                continue
            }

            $best = 0
            $bestDist = [double]::PositiveInfinity
            for ($i = 0; $i -lt 256; $i++) {
                if ($i -eq 254) { continue }
                $c = $Palette[$i]
                $dr = $p.R - $c.R
                $dg = $p.G - $c.G
                $db = $p.B - $c.B
                $dist = ($dr * $dr) + ($dg * $dg) + ($db * $db)
                if ($dist -lt $bestDist) {
                    $bestDist = $dist
                    $best = $i
                }
            }
            $raw[$y * $stride + $x] = [byte]$best
        }
    }

    [System.Runtime.InteropServices.Marshal]::Copy($raw, 0, $data.Scan0, $raw.Length)
    $dst.UnlockBits($data)

    $canvas.Dispose()
    $dst.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $dst.Dispose()
}

$palette = Load-M59Palette -PaletteFile $palFile

# Build all numeric icons present in source directory.
$iconFiles = Get-ChildItem -Path $srcDir -File -Filter "*.bmp" |
    Where-Object { $_.BaseName -match '^\d+$' } |
    Sort-Object { [int]$_.BaseName }

foreach ($f in $iconFiles) {
    $n = $f.BaseName
    $bmpOut = Join-Path $workDir "$n.bmp"
    Convert-Icon -InputFile $f.FullName -OutputFile $bmpOut -Palette $palette

    $bbg = Join-Path $workDir "$n.bbg"
    @(
        "-s 1",
        "2",
        "$n.bmp",
        "$n.bmp",
        "2",
        "1 1",
        "1 2"
    ) | Set-Content -Path $bbg -Encoding ASCII

    $bgfOut = Join-Path $workDir "inecro$n.bgf"
    Push-Location $workDir
    & $makebgf -o $bgfOut "@$n.bbg"
    Pop-Location
    if ($LASTEXITCODE -ne 0) {
        throw "makebgf failed for icon $n"
    }

    foreach ($d in $destDirs) {
        Copy-Item -Path $bgfOut -Destination (Join-Path $d "inecro$n.bgf") -Force
    }
    Write-Host "Built inecro$n.bgf"
}

Write-Host "Done."
