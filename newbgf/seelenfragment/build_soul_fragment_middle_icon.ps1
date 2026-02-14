Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\\..")).Path
$srcPng = Join-Path $PSScriptRoot "SeelenfraghmentMittel.png"
$workBmp = Join-Path $PSScriptRoot "isoulfragm.bmp"
$palFile = Join-Path $repo "blakston.pal"
$makebgf = Join-Path $repo "bin\\makebgf.exe"
$bgfName = "isoulfragm.bgf"

$destDirs = @(
    (Join-Path $repo "resource\\graphics"),
    (Join-Path $repo "run\\localclient\\resource"),
    (Join-Path $repo "run\\server\\rsc")
)

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
    if ($entries.Count -lt 256) { throw "Palette file has only $($entries.Count) entries; expected 256." }
    return ,$entries[0..255]
}

function Convert-ToM59IconBmp {
    param([string]$InputFile, [string]$OutputFile, [System.Drawing.Color[]]$Palette)

    $size = 32
    $bgTolerance = 28
    $src = [System.Drawing.Bitmap]::FromFile($InputFile)
    $bg = $src.GetPixel(0, 0)

    function Is-BackgroundPixel {
        param([System.Drawing.Color]$px, [System.Drawing.Color]$bgColor, [int]$tol)
        if ($px.A -lt 128) { return $true }
        $dr = [Math]::Abs($px.R - $bgColor.R)
        $dg = [Math]::Abs($px.G - $bgColor.G)
        $db = [Math]::Abs($px.B - $bgColor.B)
        return ($dr -le $tol -and $dg -le $tol -and $db -le $tol)
    }

    $minX = $src.Width; $minY = $src.Height; $maxX = -1; $maxY = -1
    for ($y = 0; $y -lt $src.Height; $y++) {
        for ($x = 0; $x -lt $src.Width; $x++) {
            $p = $src.GetPixel($x, $y)
            if (-not (Is-BackgroundPixel -px $p -bgColor $bg -tol $bgTolerance)) {
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
    $g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255, 255))
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $srcRect = New-Object System.Drawing.Rectangle($minX, $minY, $cropW, $cropH)
    $dstRect = New-Object System.Drawing.Rectangle($drawX, $drawY, $drawW, $drawH)
    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    $src.Dispose()

    $dst = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
    $pal = $dst.Palette
    for ($i = 0; $i -lt 256; $i++) { $pal.Entries[$i] = $Palette[$i] }
    $pal.Entries[254] = [System.Drawing.Color]::FromArgb(255, 0, 255, 255)
    $dst.Palette = $pal

    $rect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
    $data = $dst.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $dst.PixelFormat)
    $stride = $data.Stride
    $raw = New-Object byte[] ($stride * $size)

    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            $p = $canvas.GetPixel($x, $y)
            $isNearBlack = ($p.R -le 60 -and $p.G -le 60 -and $p.B -le 60)
            if (Is-BackgroundPixel -px $p -bgColor $bg -tol $bgTolerance -or $isNearBlack -or ($p.G -ge 245 -and $p.B -ge 245 -and $p.R -le 10)) {
                $raw[$y * $stride + $x] = [byte]254
                continue
            }
            $best = 0; $bestDist = [double]::PositiveInfinity
            for ($i = 0; $i -lt 256; $i++) {
                if ($i -eq 254) { continue }
                $c = $Palette[$i]
                $dr = $p.R - $c.R; $dg = $p.G - $c.G; $db = $p.B - $c.B
                $dist = ($dr * $dr) + ($dg * $dg) + ($db * $db)
                if ($dist -lt $bestDist) { $bestDist = $dist; $best = $i }
            }
            $raw[$y * $stride + $x] = [byte]$best
        }
    }

    # Force transparent background: flood-fill from edges using corner index.
    $cornerIdx = @(
        [int]$raw[0],
        [int]$raw[$size - 1],
        [int]$raw[(($size - 1) * $stride)],
        [int]$raw[(($size - 1) * $stride) + ($size - 1)]
    )
    $bgIndex = ($cornerIdx | Group-Object | Sort-Object Count -Descending | Select-Object -First 1).Name -as [int]
    $visited = New-Object bool[] ($size * $size)
    $q = New-Object System.Collections.Generic.Queue[int]

    function Enqueue-IfBg {
        param([int]$xx, [int]$yy)
        if ($xx -lt 0 -or $yy -lt 0 -or $xx -ge $size -or $yy -ge $size) { return }
        $id = $yy * $size + $xx
        if ($visited[$id]) { return }
        $visited[$id] = $true
        if ([int]$raw[$yy * $stride + $xx] -eq $bgIndex) { $q.Enqueue($id) }
    }

    for ($x = 0; $x -lt $size; $x++) {
        Enqueue-IfBg -xx $x -yy 0
        Enqueue-IfBg -xx $x -yy ($size - 1)
    }
    for ($y = 0; $y -lt $size; $y++) {
        Enqueue-IfBg -xx 0 -yy $y
        Enqueue-IfBg -xx ($size - 1) -yy $y
    }

    while ($q.Count -gt 0) {
        $id = $q.Dequeue()
        $cx = $id % $size
        $cy = [int](($id - $cx) / $size)
        $raw[$cy * $stride + $cx] = [byte]254
        Enqueue-IfBg -xx ($cx - 1) -yy $cy
        Enqueue-IfBg -xx ($cx + 1) -yy $cy
        Enqueue-IfBg -xx $cx -yy ($cy - 1)
        Enqueue-IfBg -xx $cx -yy ($cy + 1)
    }

    [System.Runtime.InteropServices.Marshal]::Copy($raw, 0, $data.Scan0, $raw.Length)
    $dst.UnlockBits($data)
    $canvas.Dispose()
    $dst.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $dst.Dispose()
}

$palette = Load-M59Palette -PaletteFile $palFile
Convert-ToM59IconBmp -InputFile $srcPng -OutputFile $workBmp -Palette $palette

$graphicsDir = Join-Path $repo "resource\\graphics"
$bgfOut = Join-Path $graphicsDir $bgfName
Push-Location $graphicsDir
& $makebgf -o $bgfOut -n "adept soul fragment" 1 $workBmp 1 1 1
Pop-Location
if ($LASTEXITCODE -ne 0) { throw "makebgf failed for $bgfName" }

foreach ($d in $destDirs) {
    $dst = Join-Path $d $bgfName
    if ($dst -ne $bgfOut) { Copy-Item -Path $bgfOut -Destination $dst -Force }
}

Write-Host "Built and deployed $bgfName"
