Add-Type -AssemblyName System.Drawing

# Pfade
$srcPath = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\newbgf\new_ne"
$dstPath = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\newbgf\new_ne"
$palettePath = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\gelmaker\res\palette.bmp"

# Lade Palette
$paletteBmp = [System.Drawing.Bitmap]::FromFile($palettePath)

function Convert-ToSmallBmp {
    param($inputFile, $outputFile, $targetWidth, $targetHeight)

    $src = [System.Drawing.Bitmap]::FromFile($inputFile)

    # Skalieren
    $dst = New-Object System.Drawing.Bitmap($targetWidth, $targetHeight)
    $graphics = [System.Drawing.Graphics]::FromImage($dst)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.DrawImage($src, 0, 0, $targetWidth, $targetHeight)
    $graphics.Dispose()

    # Speichern
    $dst.Save($outputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)

    $src.Dispose()
    $dst.Dispose()

    Write-Host "Konvertiert: $outputFile ($targetWidth x $targetHeight)"
}

# Konvertiere die Bilder auf 32x32 (wie andere Insignien)
Convert-ToSmallBmp "$srcPath\gshnec1.bmp" "$dstPath\gshnec_small.bmp" 32 32
Convert-ToSmallBmp "$srcPath\gshnec1ov.bmp" "$dstPath\gshnecov_small.bmp" 32 32

Write-Host "Fertig! Kleine BMPs erstellt."
