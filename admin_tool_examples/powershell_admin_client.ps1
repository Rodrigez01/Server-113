# Meridian 59 Admin Tool - PowerShell Client
# Verbindet sich zum Maintenance Port des Servers und sendet Admin-Befehle.
# Ersetzt unzuverlässige sendkeys-Methoden.

function Connect-M59Server {
    param(
        [string]$Host = "127.0.0.1",
        [int]$Port = 9998
    )

    try {
        $client = New-Object System.Net.Sockets.TcpClient
        $client.Connect($Host, $Port)
        $stream = $client.GetStream()
        $stream.ReadTimeout = 10000
        $stream.WriteTimeout = 10000

        Write-Host "✓ Verbunden mit ${Host}:${Port}" -ForegroundColor Green

        # Optional: Begrüßungsnachricht empfangen
        Start-Sleep -Milliseconds 100
        $response = Receive-M59Response -Stream $stream
        if ($response) {
            Write-Host "Server: $response" -ForegroundColor Cyan
        }

        return @{
            Client = $client
            Stream = $stream
        }
    }
    catch {
        Write-Host "✗ Verbindungsfehler: $_" -ForegroundColor Red
        return $null
    }
}

function Send-M59Command {
    param(
        [Parameter(Mandatory=$true)]
        $Connection,

        [Parameter(Mandatory=$true)]
        [string]$Command
    )

    if (-not $Connection -or -not $Connection.Stream) {
        Write-Host "✗ Nicht verbunden!" -ForegroundColor Red
        return $null
    }

    try {
        # Befehl mit Carriage Return abschließen
        $commandBytes = [System.Text.Encoding]::UTF8.GetBytes($Command + "`r`n")
        $Connection.Stream.Write($commandBytes, 0, $commandBytes.Length)
        $Connection.Stream.Flush()

        Write-Host "→ Gesendet: $Command" -ForegroundColor Yellow

        # Warte auf Antwort
        Start-Sleep -Milliseconds 100
        $response = Receive-M59Response -Stream $Connection.Stream

        return $response
    }
    catch {
        Write-Host "✗ Fehler beim Senden: $_" -ForegroundColor Red
        return $null
    }
}

function Receive-M59Response {
    param(
        [Parameter(Mandatory=$true)]
        $Stream,

        [int]$BufferSize = 4096
    )

    try {
        if ($Stream.DataAvailable) {
            $buffer = New-Object byte[] $BufferSize
            $bytesRead = $Stream.Read($buffer, 0, $BufferSize)

            if ($bytesRead -gt 0) {
                $response = [System.Text.Encoding]::UTF8.GetString($buffer, 0, $bytesRead)
                return $response.Trim()
            }
        }
    }
    catch {
        Write-Host "✗ Fehler beim Empfangen: $_" -ForegroundColor Red
    }

    return $null
}

function Disconnect-M59Server {
    param(
        [Parameter(Mandatory=$true)]
        $Connection
    )

    try {
        if ($Connection.Stream) {
            Send-M59Command -Connection $Connection -Command "QUIT" | Out-Null
            $Connection.Stream.Close()
        }

        if ($Connection.Client) {
            $Connection.Client.Close()
        }

        Write-Host "✓ Verbindung getrennt" -ForegroundColor Green
    }
    catch {
        Write-Host "Fehler beim Trennen: $_" -ForegroundColor Yellow
    }
}

# ===== BEISPIEL-VERWENDUNG =====

# Beispiel 1: Einfacher Befehl
Write-Host "`n=== Beispiel 1: Einzelner Befehl ===" -ForegroundColor Magenta
$conn = Connect-M59Server
if ($conn) {
    $response = Send-M59Command -Connection $conn -Command "show status"
    if ($response) {
        Write-Host "← Antwort:`n$response`n" -ForegroundColor Cyan
    }
    Disconnect-M59Server -Connection $conn
}

# Beispiel 2: Mehrere Befehle
Write-Host "`n=== Beispiel 2: Mehrere Befehle ===" -ForegroundColor Magenta
$conn = Connect-M59Server
if ($conn) {
    $commands = @(
        "show status",
        "show sessions",
        "show memory"
    )

    foreach ($cmd in $commands) {
        $response = Send-M59Command -Connection $conn -Command $cmd
        if ($response) {
            Write-Host "← Antwort: $response`n" -ForegroundColor Cyan
        }
        Start-Sleep -Milliseconds 500
    }

    Disconnect-M59Server -Connection $conn
}

# Beispiel 3: Account erstellen
Write-Host "`n=== Beispiel 3: Account erstellen ===" -ForegroundColor Magenta
$conn = Connect-M59Server
if ($conn) {
    # Account erstellen
    $response = Send-M59Command -Connection $conn -Command "create automated testuser testpass123"
    if ($response) {
        Write-Host "← Antwort: $response" -ForegroundColor Cyan
    }

    Start-Sleep -Milliseconds 500

    # Status prüfen
    $response = Send-M59Command -Connection $conn -Command "show account testuser"
    if ($response) {
        Write-Host "← Antwort: $response" -ForegroundColor Cyan
    }

    Disconnect-M59Server -Connection $conn
}

Write-Host "`n✓ Fertig!" -ForegroundColor Green
