# Meridian 59 Client Command Injector - PowerShell
# Sendet DM-Befehle direkt an den laufenden Client über Named Pipe.
# Ersetzt unzuverlässige sendkeys-Methoden.

function Find-M59ClientPid {
    <#
    .SYNOPSIS
    Findet die Process ID des laufenden Meridian59 Clients
    #>

    $clientNames = @("meridian", "clientd3d", "client")
    $processes = Get-Process -ErrorAction SilentlyContinue

    foreach ($proc in $processes) {
        foreach ($name in $clientNames) {
            if ($proc.ProcessName.ToLower() -like "*$name*") {
                Write-Host "✓ Client gefunden: PID $($proc.Id)" -ForegroundColor Green
                return $proc.Id
            }
        }
    }

    return $null
}

function Connect-M59Client {
    <#
    .SYNOPSIS
    Verbindet zum Client via Named Pipe
    #>
    param(
        [int]$Pid
    )

    if (-not $Pid) {
        $Pid = Find-M59ClientPid
    }

    if (-not $Pid) {
        Write-Host "✗ Kein laufender Meridian 59 Client gefunden!" -ForegroundColor Red
        return $null
    }

    $pipeName = "Meridian59_Command_$Pid"

    try {
        Write-Host "→ Verbinde zu Pipe: \\.\pipe\$pipeName" -ForegroundColor Yellow

        # Erstelle Named Pipe Client
        $pipeClient = New-Object System.IO.Pipes.NamedPipeClientStream(
            ".",                                          # Server name (local)
            $pipeName,                                    # Pipe name
            [System.IO.Pipes.PipeDirection]::Out          # Write-only
        )

        # Versuche Verbindung (5 Sekunden Timeout)
        $pipeClient.Connect(5000)

        Write-Host "✓ Verbunden mit Client!" -ForegroundColor Green

        return $pipeClient
    }
    catch {
        Write-Host "✗ Verbindungsfehler: $_" -ForegroundColor Red
        Write-Host "`nHinweis: Der Client muss die Command Pipe Unterstützung kompiliert haben!" -ForegroundColor Yellow
        return $null
    }
}

function Send-M59Command {
    <#
    .SYNOPSIS
    Sendet einen Befehl an den Client
    #>
    param(
        [Parameter(Mandatory=$true)]
        $PipeClient,

        [Parameter(Mandatory=$true)]
        [string]$Command
    )

    if (-not $PipeClient -or -not $PipeClient.IsConnected) {
        Write-Host "✗ Nicht verbunden!" -ForegroundColor Red
        return $false
    }

    try {
        # Füge Newline hinzu und konvertiere zu Bytes
        $commandBytes = [System.Text.Encoding]::UTF8.GetBytes($Command + "`n")

        # Sende Befehl
        $PipeClient.Write($commandBytes, 0, $commandBytes.Length)
        $PipeClient.Flush()

        Write-Host "✓ Befehl gesendet: $Command" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "✗ Fehler beim Senden: $_" -ForegroundColor Red
        return $false
    }
}

function Disconnect-M59Client {
    <#
    .SYNOPSIS
    Trennt die Verbindung
    #>
    param(
        [Parameter(Mandatory=$true)]
        $PipeClient
    )

    if ($PipeClient) {
        $PipeClient.Close()
        $PipeClient.Dispose()
        Write-Host "✓ Verbindung getrennt" -ForegroundColor Green
    }
}

# ===== BEISPIEL-VERWENDUNG =====

Write-Host "=== Meridian 59 Client Command Injector ===`n" -ForegroundColor Magenta

# Beispiel 1: Einzelner Befehl
Write-Host "=== Beispiel 1: Einzelner Befehl ===" -ForegroundColor Cyan
$client = Connect-M59Client
if ($client) {
    Send-M59Command -PipeClient $client -Command "go rid_tos"
    Start-Sleep -Milliseconds 500
    Disconnect-M59Client -PipeClient $client
}

# Beispiel 2: Mehrere Befehle
Write-Host "`n=== Beispiel 2: Mehrere Befehle ===" -ForegroundColor Cyan
$client = Connect-M59Client
if ($client) {
    $commands = @(
        "go rid_tos",
        "dm boost stats",
        "dm get spells",
        "dm get skills"
    )

    foreach ($cmd in $commands) {
        Send-M59Command -PipeClient $client -Command $cmd
        Start-Sleep -Milliseconds 200
    }

    Disconnect-M59Client -PipeClient $client
}

# Beispiel 3: Interaktiver Modus
Write-Host "`n=== Beispiel 3: Interaktiver Modus ===" -ForegroundColor Cyan
$client = Connect-M59Client
if ($client) {
    Write-Host "`nGib Befehle ein (oder 'quit' zum Beenden):" -ForegroundColor Yellow

    while ($true) {
        $cmd = Read-Host "> "

        if ([string]::IsNullOrWhiteSpace($cmd)) {
            continue
        }

        if ($cmd.ToLower() -in @("quit", "exit", "q")) {
            break
        }

        Send-M59Command -PipeClient $client -Command $cmd
    }

    Disconnect-M59Client -PipeClient $client
}

Write-Host "`n✓ Fertig!" -ForegroundColor Green
