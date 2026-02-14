# Build Server Script for Meridian59 113/114
# Adapted from build-105.ps1 for Server-104-main
# Telnet code: http://thesurlyadmin.com/2013/04/04/using-powershell-as-a-telnet-client/
# Requires psexec.exe: https://technet.microsoft.com/en-us/sysinternals/psexec.aspx (OPTIONAL - nur für Remote-Server)
Param(
  #Source Repository Paths
  # OPTIONAL: Pfad zu PatchListGenerator.exe (auskommentiert, falls nicht vorhanden)
  # [string]$PatchListGen="C:\qbscripts\PatchListGenerator.exe",
  [string]$PatchListGen="",
  [string]$RootPath="C:\Users\Rod\Desktop\2\Server-104-main",
  [string]$SolutionPath="\",
  [string]$MeridianSolution="Meridian59.sln",
  [string]$ClientPath="\run\localclient\",
  [string]$ServerPath="\run\server\",
  [string]$ResourcePath="\resource\",

  #Destination Client Paths
  [string]$OutputPackagePath="C:\patchserver\114\clientpatch\", # Root Path for Uncompressed Client
  [string]$ProdPackagePath="C:\patchserver\113\clientpatch\",
  [string]$TestPackagePath="C:\patchserver\114\clientpatch\",
  [string]$OutputParentPath="C:\patchserver\114", # Root path for patchinfo.txt
  [string]$ProdParentPath="C:\patchserver\113",
  [string]$TestParentPath="C:\patchserver\114",
  [string]$PackageResourcePath="resource\", #Resources subdirectory
  [string]$LatestZip="latest.zip",

  #Destination Server Paths
  [string]$OutputServerPath="C:\Server114\",
  [string]$ProdServerPath="C:\Server113\",
  [string]$TestServerPath="C:\Server114\",

  #DNS
  [string]$OutputIP="127.0.0.1",
  [string]$ProdServerIP="127.0.0.1",  # Ändere zu deiner Server-IP falls remote
  [string]$TestServerIP="127.0.0.1",

  #Control Parameters
  [string]$BuildID="latest",
  [ValidateSet('build','clean','package','checkout','killserver','startserver','copyserver')]$Action,
  [ValidateSet('develop','master','release')]$Branch,
  [ValidateSet('production','test')]$Server,
  [ValidateSet('true','false')]$Hotfix,
  [switch]$WhatIf
)

function DisplaySyntax
{
   Write-Host
   Write-Host "Usage Scenarios:"
   Write-Host "1) Build or Clean Project and Platform"
   Write-Host "   build-113.ps1 -Action [build|clean] -Server [production|test]"
   Write-Host
   Write-Host "2) Package Client for distribution (requires PatchListGenerator.exe)"
   Write-Host "   build-113.ps1 -Action package -Server [production|test]"
   Write-Host
   Write-Host "3) Checkout latest code (requires git repository)"
   Write-Host "   build-113.ps1 -Action checkout -Branch [develop|master|release]"
   Write-Host
   Write-Host "4) Copy server files to destination"
   Write-Host "   build-113.ps1 -Action copyserver -Server [production|test]"
   Write-Host
   Write-Host "5) Shutdown server"
   Write-Host "   build-113.ps1 -Action killserver -Server [production|test]"
   Write-Host
   Write-Host "6) Start server and recreate, load npcdlg.txt"
   Write-Host "   build-113.ps1 -Action startserver -Server [production|test]"
   Write-Host
   Write-Host "Optional Parameters:"
   Write-Host "-PatchListGen the executable file to generate patchinfo.txt"
   Write-Host "-RootPath the root path to work under for all operations"
   Write-Host "-SolutionPath the path to the client solution files"
   Write-Host "-MeridianSolution the file name of the Meridian solution file"
   Write-Host "-ClientPath the path to copy client files from"
   Write-Host "-ResourcePath the path to copy client resources from"
   Write-Host "-OutputPackagePath the path to output a packaged client"
   Write-Host "-OutputParentPath the path to output patchinfo.txt"
   Write-Host "-PackageResourcePath the path to copy resources to"
   Write-Host "-Branch which branch to build from (develop, master, release)"
   Write-Host "-Server either production (113) or test (114)"
   Write-Host "-Hotfix true if production server should reuse kodbase.txt and not restart"
   Write-Host "-BuildID used by the build server to name output files"
   Write-Host "-WhatIf simulates the actions and outputs resulting command to be run instead of performing said command"
   Write-Host
   Write-Host "Examples:"
   Write-Host "  build-113.ps1 -Action build -Server production"
   Write-Host "  build-113.ps1 -Action copyserver -Server production"
   Write-Host "  build-113.ps1 -Action startserver -Server production"
   Write-Host
}

########### Path Creation Code ###########
function TestRootPath
{
   if (-not (Test-Path -Path $RootPath -PathType container))
   {
      Write-Host "$RootPath does not exist, creating"
      New-Item -Path $RootPath -ItemType Directory
   }
}

function TestPackagePath
{
   if (-not (Test-Path -Path "$OutputPackagePath"-PathType container))
   {
      Write-Host "$OutputPackagePath does not exist, creating"
      New-Item -Path "$OutputPackagePath" -ItemType Directory
   }
}

function TestResourcesPath
{
   if (-not (Test-Path -Path "$OutputPackagePath$PackageResourcePath"-PathType container))
   {
      Write-Host "$OutputPackagePath$PackageResourcePath does not exist, creating"
      New-Item -Path "$OutputPackagePath$PackageResourcePath" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\mail\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\mail\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\mail\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\resource\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\resource\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\resource\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\ads\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\ads\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\ads\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\download\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\download\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\download\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputPackagePath\\help\\"-PathType container))
   {
      Write-Host "$OutputPackagePath\\help\\ does not exist, creating"
      New-Item -Path "$OutputPackagePath\\help\\" -ItemType Directory
   }
}

########### Checkout Code ###########
function CheckoutAction
{
   Set-Location -Path $RootPath
   TestRootPath
   CheckoutAndUpdateBranch
}

function CheckoutAndUpdateBranch
{
   $Command = "git checkout " + $Branch
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }

   $Command = "git pull origin " + $Branch
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

########### Build Action Code ###########
function Get-Batchfile ($file) {
   $cmd = "`"$file`" & set"
   cmd /c $cmd | Foreach-Object {
      $p, $v = $_.split('=')
      Set-Item -path env:$p -value $v
   }
}

function VsVars32()
{
   # Versuche verschiedene Visual Studio Versionen zu finden
   # VS140 = Visual Studio 2015
   # VS120 = Visual Studio 2013
   # VS110 = Visual Studio 2012

   $vsVersions = @("VS140COMNTOOLS", "VS120COMNTOOLS", "VS110COMNTOOLS")
   $vsFound = $false

   foreach ($vsVar in $vsVersions)
   {
      try
      {
         $vsPath = (Get-ChildItem "env:$vsVar" -ErrorAction SilentlyContinue).Value
         if ($vsPath)
         {
            Write-Host "Gefunden: $vsVar = $vsPath"
            $batchFile = [System.IO.Path]::Combine($vsPath, "vsvars32.bat")
            if (Test-Path $batchFile)
            {
               Write-Host "Lade Visual Studio Umgebung von: $batchFile"
               Get-Batchfile $batchFile
               $vsFound = $true
               break
            }
         }
      }
      catch
      {
         # Ignoriere Fehler und versuche nächste Version
      }
   }

   if (-not $vsFound)
   {
      # Fallback: Versuche den bekannten Pfad für VS 2015
      $fallbackPath = "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
      if (Test-Path $fallbackPath)
      {
         Write-Host "Verwende Fallback-Pfad: $fallbackPath"
         Get-Batchfile $fallbackPath
      }
      else
      {
         Write-Error "Visual Studio konnte nicht gefunden werden! Bitte installiere Visual Studio 2013 oder 2015."
      }
   }
}

function CopyProdKodbase
{
   if ($Hotfix -eq "true")
   {
      Write-Host "Using production kodbase.txt"

      Copy-Item "$ProdServerPath\kodbase.txt" -Destination "$RootPath\kod" -Recurse -Force
   }
   else
   {
      Write-Host "Making new kodbase.txt"
   }
}

function BuildCodebase
{
   $Command = "nmake release=1 nocopyfiles=1"
   if (-not ($ENV:INCLUDE))   { VsVars32 }
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

function BuildAction
{
   CheckServerParams
   CopyProdKodbase
   BuildCodebase
}

########### Clean Action Code ###########
function CleanAction
{
   $Command = "nmake clean release=1"

   if (-not ($ENV:INCLUDE))   { VsVars32 }
   if ($WhatIf) { Write-Host $Command }
   else
   {
      Write-Host "Running: $Command"
      powershell "& $Command "
   }
}

########### Parameter Checking ###########
function CheckServerParams
{
   if (($Server -ne "production") -and ($Server -ne "test"))
   {
      Write-Error "Incorrect -Server parameter!"
      DisplaySyntax
      Exit -1
   }

   switch ($Server)
   {
      production
      {
         $script:OutputPackagePath = $ProdPackagePath
         $script:OutputParentPath = $ProdParentPath
         $script:OutputServerPath = $ProdServerPath
         $script:OutputIP = $ProdServerIP
      }
      test
      {
         $script:OutputPackagePath = $TestPackagePath
         $script:OutputParentPath = $TestParentPath
         $script:OutputServerPath = $TestServerPath
         $script:OutputIP = $TestServerIP
      }
   }
}

########### Copy Server Action Code ###########
function TestServerDirs
{
   if (-not (Test-Path -Path $OutputServerPath -PathType container))
   {
      Write-Host "$OutputServerPath does not exist, creating"
      New-Item -Path $OutputServerPath -ItemType Directory
   }

   if (-not (Test-Path -Path "$OutputServerPath\\channel\\"-PathType container))
   {
      Write-Host "$OutputServerPath\\channel\\ does not exist, creating"
      New-Item -Path "$OutputServerPath\\channel\\" -ItemType Directory
   }
   if (-not (Test-Path -Path "$OutputServerPath\\savegame\\"-PathType container))
   {
      Write-Host "$OutputServerPath\\savegame\\ does not exist, creating"
      New-Item -Path "$OutputServerPath\\savegame\\" -ItemType Directory
   }
}

function CopyServerFiles
{
   # Pflichtdateien (müssen existieren)
   $RequiredItems=@("blakserv.exe","blakston.khd","kodbase.txt","protocol.khd")

   # Optionale Dateien (werden kopiert falls vorhanden)
   $OptionalItems=@("jansson.dll","libcurl.dll","libmysql.dll","packages.txt")

   $BinarySource = "$RootPath$ServerPath"
   $BinaryDestination = "$OutputServerPath"

   # Kopiere Pflichtdateien
   foreach ($file in $RequiredItems)
   {
      $sourcePath = "$BinarySource$file"
      if (Test-Path $sourcePath)
      {
         Write-Host "Kopiere: $file"
         Copy-Item $sourcePath -destination $BinaryDestination -Force
      }
      else
      {
         Write-Error "FEHLER: Pflichtdatei nicht gefunden: $sourcePath"
      }
   }

   # Kopiere optionale Dateien (ohne Fehler wenn nicht vorhanden)
   foreach ($file in $OptionalItems)
   {
      $sourcePath = "$BinarySource$file"
      if (Test-Path $sourcePath)
      {
         Write-Host "Kopiere (optional): $file"
         Copy-Item $sourcePath -destination $BinaryDestination -Force
      }
      else
      {
         Write-Host "Überspringe (nicht vorhanden): $file"
      }
   }
}

function CopyServerBofs
{
   $ServerSource = "$RootPath$ServerPath"
   $ServerDest = "$OutputServerPath"
   $BofDir = "loadkod"

   Write-Host "$ServerDest"

   $GitIgnore = Get-ChildItem -Path "$ServerSource$BofDir" -Filter .gitignore
   Copy-Item "$ServerSource$BofDir" -Destination $ServerDest -Recurse -Force -exclude $GitIgnore
}

function CopyServerDirs
{
   $ServerDirs=@("loadkod","memmap","rooms","rsc")

   $ServerSource = "$RootPath$ServerPath"
   $ServerDest = "$OutputServerPath"

   foreach ($folder in $ServerDirs)
   {
      $GitIgnore = Get-ChildItem -Path "$ServerSource$folder" -Filter .gitignore
      Copy-Item "$ServerSource$folder" -Destination $ServerDest -Recurse -Force -exclude $GitIgnore
   }
}

function CopySaveGame
{
   if ($Server -eq "test")
   {
      $SaveLocation = "savegame\lastsave.txt"
      $ServerSource = "$ProdServerPath"
      $ServerDest = "$OutputServerPath"

      Remove-Item "$ServerDest\savegame\*"

      $SaveFile = (Get-Content $ServerSource$SaveLocation)[12]
      $SaveExt = $SaveFile.Replace("LASTSAVE ", ".")

      Write-Host "Copying save game $SaveExt to test"

      $SaveFiles=@("accounts","dynarscs","gameuser","striings")
      foreach ($file in $SaveFiles)
      {
         Copy-Item "$ServerSource\savegame\$file$SaveExt" -Destination "$ServerDest\savegame" -Recurse -Force
      }
      Copy-Item "$ServerSource$SaveLocation" -Destination "$ServerDest\savegame" -Recurse -Force
   }
}

function CopyServerKodbase
{
   Copy-Item "$RootPath\kod\kodbase.txt" -Destination "$ProdServerPath" -Recurse -Force
}

function CopyServerAction
{
   CheckServerParams
   TestServerDirs

   if (($Hotfix -eq "true") -and ($Server -eq "production"))
   {
      Write-Host "Production hotfix"
      CopyServerBofs
      CopyServerKodbase
   }
   else
   {
      CopyServerFiles
      CopyServerDirs
      CopySaveGame
   }
}

########### Package Action Code ###########
function CopyFiles
{
   $BinaryItems=@("club.exe","m59bind.exe","meridian.exe","ikpMP3.dll",
                  "irrKlang.dll","Heidelb1.ttf","license.rtf")

   $BinarySource = "$RootPath$ClientPath"
   $BinaryDestination = "$OutputPackagePath"

   foreach ($file in $BinaryItems)
   {
      Copy-Item "$BinarySource$file" -destination $BinaryDestination -Force
   }
}

function CopyResources
{
   $ResourceSource = "$RootPath$ClientPath$PackageResourcePath"
   $ResourceDestination = "$OutputPackagePath$PackageResourcePath"

   $GitIgnore = Get-ChildItem -Path $ResourceSource -Filter .gitignore
   $files = Get-ChildItem -Path $ResourceSource -Exclude $GitIgnore
   $files | Copy-Item -Destination $ResourceDestination -Force
}

function CompressAll
{
   if (Test-Path $OutputPackagePath$LatestZip)
   {
      Remove-Item $OutputPackagePath$LatestZip
   }

   # HINWEIS: Benötigt PowerShell Community Extensions oder Compress-Archive
   # Get-ChildItem -Recurse $OutputPackagePath |
   # Write-Zip -OutputPath $OutputPackagePath$LatestZip -IncludeEmptyDirectories -EntryPathRoot $OutputPackagePath

   # Alternative mit Compress-Archive (PowerShell 5.0+):
   Compress-Archive -Path "$OutputPackagePath\*" -DestinationPath "$OutputPackagePath$LatestZip" -Force
}

function PatchListGen
{
   # OPTIONAL: Auskommentiert, falls PatchListGenerator.exe nicht vorhanden
   if ($PatchListGen -eq "")
   {
      Write-Host "PatchListGenerator.exe nicht konfiguriert, überspringe patchinfo.txt Generierung"
      return
   }

   $Args = "--client=$OutputPackagePath", "--outfile=$OutputParentPath\patchinfo.txt", "--type=classic"

   if ($WhatIf) { Write-Host $PatchListGen $Args }
   else
   {
      Write-Host "Running: $PatchListGen $Args"
      powershell "& $PatchListGen $Args "
   }
}

function PackageAction
{
   CheckServerParams
   TestPackagePath
   TestResourcesPath
   CopyFiles
   CopyResources
   CompressAll
   PatchListGen
}

########### KillServer Action Code ###########
function KillServerAction
{
   CheckServerParams

   $Commands=@("save game","terminate save")

   try
   {
      $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
      if ($Socket)
      {
         $Stream = $Socket.GetStream()
         $Writer = New-Object System.IO.StreamWriter($Stream)
         $Buffer = New-Object System.Byte[] 1024
         $Encoding = New-Object System.Text.AsciiEncoding

         foreach ($Command in $Commands)
         {
            Write-Host "Sending command: $Command"
            $Writer.WriteLine($Command)
            $Writer.Flush()
            Start-Sleep -Milliseconds (4000)
         }
         $Writer.Close()
         $Stream.Close()
         $Socket.Close()
      }
   }
   catch
   {
      Write-Error "Could not connect to server at $OutputIP:9999 - Is the server running?"
   }
}

########### Start Server Action Code ###########
function StartServer
{
   if ($Server -eq "production")
   {
      # OPTIONAL: Auskommentiert, da psexec.exe möglicherweise nicht vorhanden
      # Start-Process -FilePath "c:\qbscripts\psexec.exe" -ArgumentList ('-i 2', '-d', '\\127.0.0.1', '-u username', '-p password', "cmd /c start /D $ProdServerPath blakserv.exe") -Wait

      # Alternative: Lokaler Start
      Write-Host "Starting production server locally at $ProdServerPath"
      Start-Process -WorkingDirectory $ProdServerPath -FilePath "$ProdServerPath\blakserv.exe"
      Start-Sleep -Milliseconds (2000)
   }
   else
   {
      Write-Host "Starting test server at $OutputServerPath"
      Start-Process -WorkingDirectory $OutputServerPath -FilePath "$OutputServerPath\blakserv.exe"
      Start-Sleep -Milliseconds (2000)
   }
}

function RecreateServer
{
   $Commands=@("lock updating","send o 0 recreateall","read npcdlg.txt","unlock")

   try
   {
      $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
      if ($Socket)
      {
         $Stream = $Socket.GetStream()
         $Writer = New-Object System.IO.StreamWriter($Stream)
         $Buffer = New-Object System.Byte[] 1024
         $Encoding = New-Object System.Text.AsciiEncoding

         foreach ($Command in $Commands)
         {
            Write-Host "Sending command: $Command"
            $Writer.WriteLine($Command)
            $Writer.Flush()
            Start-Sleep -Milliseconds (8000)
         }
         $Writer.Close()
         $Stream.Close()
         $Socket.Close()
      }
   }
   catch
   {
      Write-Error "Could not connect to server at $OutputIP:9999"
   }
}

function ReloadServer
{
   $Commands=@("reload system")

   try
   {
      $Socket = New-Object System.Net.Sockets.TcpClient("$OutputIP", 9999)
      if ($Socket)
      {
         $Stream = $Socket.GetStream()
         $Writer = New-Object System.IO.StreamWriter($Stream)
         $Buffer = New-Object System.Byte[] 1024
         $Encoding = New-Object System.Text.AsciiEncoding

         foreach ($Command in $Commands)
         {
            Write-Host "Sending command: $Command"
            $Writer.WriteLine($Command)
            $Writer.Flush()
            Start-Sleep -Milliseconds (1000)
         }
         $Writer.Close()
         $Stream.Close()
         $Socket.Close()
      }
   }
   catch
   {
      Write-Error "Could not connect to server at $OutputIP:9999"
   }
}

function StartServerAction
{
   CheckServerParams
   if (($Hotfix -eq "true") -and ($Server -eq "production"))
   {
      Write-Host "Production hotfix"
      ReloadServer
   }
   else
   {
      StartServer
      RecreateServer
   }
}

########### Main Code ###########
if (($Action -ne "build") -and ($Action -ne "clean") -and ($Action -ne "package") -and ($Action -ne "checkout") -and ($Action -ne "startserver") -and ($Action -ne "killserver") -and ($Action -ne "copyserver"))
{
   Write-Error "Incorrect -Action parameter!"
   DisplaySyntax
   Exit -1
}

switch ($Action)
{
   checkout { CheckoutAction }
   build { BuildAction }
   clean { CleanAction }
   package { PackageAction }
   startserver { StartServerAction }
   killserver { KillServerAction }
   copyserver { CopyServerAction }
}
