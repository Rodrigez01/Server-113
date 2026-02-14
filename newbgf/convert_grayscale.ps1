Add-Type -AssemblyName System.Drawing

$srcPath = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\newbgf\new_ne"
$dstPath = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\newbgf\new_ne"

function Convert-ToGrayscale8bit {
    param($inputFile, $outputFile, $size)

    $src = [System.Drawing.Bitmap]::FromFile($inputFile)

    # Skalieren auf Zielgröße
    $scaled = New-Object System.Drawing.Bitmap($size, $size)
    $g = [System.Drawing.Graphics]::FromImage($scaled)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($src, 0, 0, $size, $size)
    $g.Dispose()
    $src.Dispose()

    # In Graustufen konvertieren
    $gray = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)

    # Graustufenpalette erstellen
    $palette = $gray.Palette
    for ($i = 0; $i -lt 256; $i++) {
        $palette.Entries[$i] = [System.Drawing.Color]::FromArgb(255, $i, $i, $i)
    }
    # Cyan für Transparenz (Index 254)
    $palette.Entries[254] = [System.Drawing.Color]::FromArgb(255, 0, 255, 255)
    $gray.Palette = $palette

    # Pixel kopieren und in Graustufen umwandeln
    $rect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
    $bmpData = $gray.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $gray.PixelFormat)

    $bytes = New-Object byte[] ($size * $size)

    for ($y = 0; $y -lt $size; $y++) {
        for ($x = 0; $x -lt $size; $x++) {
            $pixel = $scaled.GetPixel($x, $y)
            if ($pixel.A -lt 128) {
                # Transparent -> Cyan (Index 254)
                $bytes[$y * $size + $x] = 254
            } else {
                # Graustufe berechnen
                $grayValue = [int](0.299 * $pixel.R + 0.587 * $pixel.G + 0.114 * $pixel.B)
                # Dunkler machen (mehr schwarz)
                $grayValue = [int]($grayValue * 0.5)
                if ($grayValue -gt 253) { $grayValue = 253 }
                $bytes[$y * $size + $x] = $grayValue
            }
        }
    }

    [System.Runtime.InteropServices.Marshal]::Copy($bytes, 0, $bmpData.Scan0, $bytes.Length)
    $gray.UnlockBits($bmpData)

    $scaled.Dispose()

    # Speichern
    $gray.Save($outputFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $gray.Dispose()

    Write-Host "Erstellt: $outputFile ($size x $size, 8-bit Graustufen)"
}

# Konvertiere beide PNGs
Convert-ToGrayscale8bit "$srcPath\Necro.png" "$dstPath\gshnec_gray.bmp" 32
Convert-ToGrayscale8bit "$srcPath\gshnec1.png" "$dstPath\gshnecov_gray.bmp" 32

Write-Host "Fertig!"
