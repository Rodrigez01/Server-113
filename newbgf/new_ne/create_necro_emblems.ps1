Add-Type -AssemblyName System.Drawing

$repo = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen"
$input = Join-Path $repo "newbgf\new_ne\Necro.png"
$outMale = Join-Path $repo "resource\graphics\necro_emblm_nm.bmp"
$outFemale = Join-Path $repo "resource\graphics\necro_emblm_nf.bmp"
$palFile = Join-Path $repo "blakston.pal"
$scaleBoost = 1.18

function Load-M59Palette {
    param([string]$PaletteFile)

    $entries = @()
    foreach ($line in Get-Content -Path $PaletteFile) {
        $s = $line.Trim()
        if ($s -eq "") { continue }
        $parts = $s -split "\s+"
        if ($parts.Count -lt 3) { continue }

        $r = [int]$parts[0]
        $g = [int]$parts[1]
        $b = [int]$parts[2]
        $entries += [System.Drawing.Color]::FromArgb(255, $r, $g, $b)
    }

    if ($entries.Count -lt 256) {
        throw "Palette file has only $($entries.Count) entries; expected 256."
    }

    return ,$entries[0..255]
}

function Convert-ToIndexed8 {
    param(
        [string]$InputFile,
        [string]$OutputFile,
        [int]$Size
    )

    $src = [System.Drawing.Bitmap]::FromFile($InputFile)
    $srcW = $src.Width
    $srcH = $src.Height

    # Auto-crop transparent border so the emblem occupies more visual space.
    $minX = $srcW
    $minY = $srcH
    $maxX = -1
    $maxY = -1
    for ($y = 0; $y -lt $srcH; $y++) {
        for ($x = 0; $x -lt $srcW; $x++) {
            $p = $src.GetPixel($x, $y)
            if ($p.A -ge 16) {
                if ($x -lt $minX) { $minX = $x }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }
    if ($maxX -lt $minX -or $maxY -lt $minY) {
        $minX = 0; $minY = 0; $maxX = $srcW - 1; $maxY = $srcH - 1
    }

    $cropW = $maxX - $minX + 1
    $cropH = $maxY - $minY + 1
    $fit = [Math]::Min($Size / $cropW, $Size / $cropH) * $scaleBoost
    $drawW = [int][Math]::Round($cropW * $fit)
    $drawH = [int][Math]::Round($cropH * $fit)
    $drawX = [int][Math]::Round(($Size - $drawW) / 2.0)
    $drawY = [int][Math]::Round(($Size - $drawH) / 2.0)

    $scaled = New-Object System.Drawing.Bitmap($Size, $Size)
    $g = [System.Drawing.Graphics]::FromImage($scaled)
    $g.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $srcRect = New-Object System.Drawing.Rectangle($minX, $minY, $cropW, $cropH)
    $dstRect = New-Object System.Drawing.Rectangle($drawX, $drawY, $drawW, $drawH)
    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    $src.Dispose()

    $m59Palette = Load-M59Palette -PaletteFile $palFile
    $dst = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
    $palette = $dst.Palette
    for ($i = 0; $i -lt 256; $i++) {
        $palette.Entries[$i] = $m59Palette[$i]
    }
    # Transparency color used by M59 artwork.
    $palette.Entries[254] = [System.Drawing.Color]::FromArgb(255, 0, 255, 255)
    $dst.Palette = $palette

    $rect = New-Object System.Drawing.Rectangle(0, 0, $Size, $Size)
    $bmpData = $dst.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $dst.PixelFormat)
    $stride = $bmpData.Stride
    $raw = New-Object byte[] ($stride * $Size)

    for ($y = 0; $y -lt $Size; $y++) {
        for ($x = 0; $x -lt $Size; $x++) {
            $px = $scaled.GetPixel($x, $y)
            $index = if ($px.A -lt 128) {
                254
            } else {
                $best = 0
                $bestDist = [double]::PositiveInfinity
                for ($i = 0; $i -lt 256; $i++) {
                    if ($i -eq 254) { continue } # transparency index
                    $c = $m59Palette[$i]
                    $dr = $px.R - $c.R
                    $dg = $px.G - $c.G
                    $db = $px.B - $c.B
                    $dist = ($dr * $dr) + ($dg * $dg) + ($db * $db)
                    if ($dist -lt $bestDist) {
                        $bestDist = $dist
                        $best = $i
                    }
                }
                $best
            }
            $raw[$y * $stride + $x] = [byte]$index
        }
    }

    [System.Runtime.InteropServices.Marshal]::Copy($raw, 0, $bmpData.Scan0, $raw.Length)
    $dst.UnlockBits($bmpData)

    $scaled.Dispose()
    $dst.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $dst.Dispose()
    Write-Host "Created $OutputFile"
}

Convert-ToIndexed8 -InputFile $input -OutputFile $outMale -Size 32
Convert-ToIndexed8 -InputFile $input -OutputFile $outFemale -Size 32
