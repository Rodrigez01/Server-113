Add-Type -AssemblyName System.Drawing

$repo = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen"
$srcDir = Join-Path $repo "newbgf\new_ne\icons"
$outDir = Join-Path $srcDir "m59"
$palFile = Join-Path $repo "blakston.pal"

if (!(Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

function Load-M59Palette {
    param([string]$PaletteFile)
    $entries = @()
    foreach ($line in Get-Content -Path $PaletteFile) {
        $s = $line.Trim()
        if ($s -eq "") { continue }
        $parts = $s -split "\s+"
        if ($parts.Count -lt 3) { continue }
        $entries += [System.Drawing.Color]::FromArgb(255, [int]$parts[0], [int]$parts[1], [int]$parts[2])
    }
    if ($entries.Count -lt 256) {
        throw "Palette file has only $($entries.Count) entries; expected 256."
    }
    return ,$entries[0..255]
}

$m59Palette = Load-M59Palette -PaletteFile $palFile

function Convert-ToM59Indexed8 {
    param([string]$InputFile, [string]$OutputFile)

    $src = [System.Drawing.Bitmap]::FromFile($InputFile)
    $w = $src.Width
    $h = $src.Height

    $dst = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
    $pal = $dst.Palette
    for ($i = 0; $i -lt 256; $i++) {
        $pal.Entries[$i] = $m59Palette[$i]
    }
    $pal.Entries[254] = [System.Drawing.Color]::FromArgb(255, 0, 255, 255)
    $dst.Palette = $pal

    $rect = New-Object System.Drawing.Rectangle(0, 0, $w, $h)
    $bmpData = $dst.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $dst.PixelFormat)
    $stride = $bmpData.Stride
    $raw = New-Object byte[] ($stride * $h)

    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $px = $src.GetPixel($x, $y)
            $index = if ($px.A -lt 128) {
                254
            } else {
                $best = 0
                $bestDist = [double]::PositiveInfinity
                for ($i = 0; $i -lt 256; $i++) {
                    if ($i -eq 254) { continue }
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
    $src.Dispose()
    $dst.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $dst.Dispose()

    Write-Host "Converted $InputFile -> $OutputFile"
}

$files = Get-ChildItem -Path $srcDir -Filter "*.bmp" | Where-Object { $_.Name -match '^[0-9]+\.bmp$' }
foreach ($f in $files) {
    $outFile = Join-Path $outDir $f.Name
    Convert-ToM59Indexed8 -InputFile $f.FullName -OutputFile $outFile
}
