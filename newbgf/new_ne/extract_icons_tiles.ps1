Add-Type -AssemblyName System.Drawing

$base = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen\newbgf\new_ne"
$inputFile = Join-Path $base "icons.bmp"
$tileW = 32
$tileH = 32

if (!(Test-Path $inputFile)) {
    throw "Input file not found: $inputFile"
}

$bmp = [System.Drawing.Bitmap]::FromFile($inputFile)
try {
    $cols = [int]($bmp.Width / $tileW)
    $rows = [int]($bmp.Height / $tileH)
    $count = 0

    $csv = Join-Path $base "spell_icon_tiles_index.csv"
    "index,row,col,file" | Out-File -FilePath $csv -Encoding ascii

    for ([int]$r = 0; $r -lt $rows; $r++) {
        for ([int]$col = 0; $col -lt $cols; $col++) {
            $x = [int]($col * $tileW)
            $y = [int]($r * $tileH)
            $rect = New-Object System.Drawing.Rectangle($x, $y, [int]$tileW, [int]$tileH)
            $tile = $bmp.Clone($rect, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            try {
                $count++
                $name = ("spell_icon_tile_{0:D4}.bmp" -f $count)
                $out = Join-Path $base $name
                $tile.Save($out, [System.Drawing.Imaging.ImageFormat]::Bmp)
                "$count,$r,$col,$name" | Out-File -FilePath $csv -Encoding ascii -Append
            }
            finally {
                $tile.Dispose()
            }
        }
    }

    Write-Host "Extracted $count tiles to $base"
    Write-Host "Index written: $csv"
}
finally {
    $bmp.Dispose()
}
