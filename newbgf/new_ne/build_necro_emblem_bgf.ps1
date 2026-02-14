 $ErrorActionPreference = "Stop"

 $repo = "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen"
 $gfx = Join-Path $repo "resource\graphics"
 $makebgf = Join-Path $repo "bin\makebgf.exe"
 $tmp = Join-Path $repo "newbgf\new_ne\_emblem_build"

 if (-not (Test-Path $tmp)) {
     New-Item -ItemType Directory -Path $tmp | Out-Null
 }

 function Build-Emblem {
     param(
         [string]$BmpName,
         [string]$BgfName
     )

     $bmpSrc = Join-Path $gfx $BmpName
     $bmpDst = Join-Path $tmp $BmpName
     Copy-Item -Path $bmpSrc -Destination $bmpDst -Force

     $bbg = Join-Path $tmp ($BgfName -replace '\.bgf$', '.bbg')
     $lines = @()
     $lines += "-s 1"
     $lines += "1"
     $lines += $BmpName
     $lines += "21"
     for ($i = 1; $i -le 21; $i++) {
         $lines += "1 1"
     }
     Set-Content -Path $bbg -Encoding ASCII -Value $lines

     $bgfOut = Join-Path $tmp $BgfName
     Push-Location $tmp
     & $makebgf -o $bgfOut "@$($BgfName -replace '\.bgf$', '.bbg')"
     Pop-Location
     if ($LASTEXITCODE -ne 0) {
         throw "makebgf failed for $BgfName"
     }

     $targets = @(
         (Join-Path $repo "run\localclient\resource"),
         (Join-Path $repo "resource\graphics"),
         (Join-Path $repo "run\server\rsc")
     )

     $backup = Join-Path $repo "run\localclient\resource - Kopie"
     if (Test-Path $backup) {
         $targets += $backup
     }

     foreach ($t in $targets) {
         Copy-Item -Path $bgfOut -Destination (Join-Path $t $BgfName) -Force
     }

     Write-Host "Built $BgfName"
 }

 Build-Emblem -BmpName "necro_emblm_nm.bmp" -BgfName "emblm-nm.bgf"
 Build-Emblem -BmpName "necro_emblm_nf.bmp" -BgfName "emblm-nf.bgf"

 Write-Host "Done."
