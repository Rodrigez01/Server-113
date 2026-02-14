# Meridian 59 Admin Tool - Maintenance Port Client

Zuverlässige Alternative zu sendkeys für die Kommunikation mit dem Meridian 59 Server.

## 🎯 Übersicht

Der Meridian 59 Server hat einen eingebauten **Maintenance Port** (Standard: Port 9998), der für Admin-Tools und automatisierte Verwaltung entwickelt wurde. Diese Beispiele zeigen, wie man damit arbeitet.

## ⚙️ Server-Konfiguration

In `blakserv.cfg`:

```ini
[Socket]
MaintenancePort     9998
MaintenanceMask     ::ffff:127.0.0.1
```

- **MaintenancePort**: Port-Nummer (Standard: 9998)
- **MaintenanceMask**: IP-Adressen mit Zugriff (Semikolon-getrennt, max. 15)
  - `::ffff:127.0.0.1` = nur localhost (IPv6-Mapped IPv4)
  - Für mehrere IPs: `::ffff:127.0.0.1;::ffff:192.168.1.100`

## 📁 Verfügbare Implementierungen

### 1. Python (`python_admin_client.py`)

**Verwendung:**
```bash
python python_admin_client.py
```

**Beispiel:**
```python
from python_admin_client import M59AdminClient

with M59AdminClient() as client:
    response = client.send_command("show status")
    print(response)
```

**Vorteile:**
- Einfach zu verwenden
- Cross-Platform (Windows, Linux, macOS)
- Context Manager Support
- Perfekt für Automation und Scripts

---

### 2. C# (`csharp_admin_client.cs`)

**Kompilieren:**
```bash
csc csharp_admin_client.cs
```

**Oder in Visual Studio:**
- Neue Konsolen-App erstellen
- Code einfügen und ausführen

**Verwendung:**
```csharp
using (var client = new M59AdminClient())
{
    if (client.Connect())
    {
        string response = client.SendCommand("show status");
        Console.WriteLine(response);
    }
}
```

**Vorteile:**
- Native Windows-Integration
- Typsicher und performant
- IDisposable Pattern
- Gut für GUI-Tools (WinForms, WPF)

---

### 3. PowerShell (`powershell_admin_client.ps1`)

**Verwendung:**
```powershell
.\powershell_admin_client.ps1
```

**Oder importieren als Modul:**
```powershell
. .\powershell_admin_client.ps1

$conn = Connect-M59Server
Send-M59Command -Connection $conn -Command "show status"
Disconnect-M59Server -Connection $conn
```

**Vorteile:**
- Direkt in Windows verfügbar
- Perfekt für Ad-hoc Admin-Tasks
- Einfache Integration in bestehende PowerShell-Scripts
- Keine Kompilierung nötig

---

## 📋 Verfügbare Admin-Befehle

### Account-Verwaltung
```bash
# Account erstellen (mit Character)
create automated <username> <password>

# Character hinzufügen
create user <account_id>

# Account löschen
delete account <account_id>

# Account suspendieren
suspend account <account_id> <hours>

# Account unsuspendieren
unsuspend account <account_id>

# Account-Info anzeigen
show account <username>
```

### Server-Informationen
```bash
# Server-Status
show status

# Aktive Sessions
show sessions

# Speicher-Verwendung
show memory

# Geladene Objekte
show objects
```

### System-Verwaltung
```bash
# System neu laden
reload system

# Garbage Collection
garbage

# Save Game
save game

# Server herunterfahren
shutdown
```

### Object/Class Befehle
```bash
# Object erstellen
create object <classname>

# Object anzeigen
show object <object_id>

# Object Property setzen
set object <object_id> <property> <type> <value>

# Message an Object senden
send object <object_id> <message> <parameters>
```

Vollständige Liste in der README.md (im Hauptverzeichnis).

---

## 🔒 Sicherheit

**Wichtig:**
- Der Maintenance Port hat **keine Passwort-Authentifizierung**
- Sicherheit wird über IP-Whitelist (`MaintenanceMask`) gewährleistet
- **Niemals** den Port öffentlich zugänglich machen!
- Für Remote-Zugriff: VPN oder SSH-Tunnel verwenden

**Empfohlene Konfiguration:**
```ini
# Nur localhost
MaintenanceMask     ::ffff:127.0.0.1

# Mehrere lokale IPs (für Test-Setups)
MaintenanceMask     ::ffff:127.0.0.1;::ffff:192.168.1.100
```

---

## 🚀 Vorteile gegenüber sendkeys

| Feature | sendkeys | Maintenance Port |
|---------|----------|------------------|
| Zuverlässigkeit | ❌ Unzuverlässig | ✅ TCP-basiert, robust |
| Client muss aktiv sein | ✅ Ja | ❌ Nein |
| Client muss Fokus haben | ✅ Ja | ❌ Nein |
| Geschwindigkeit | ❌ Langsam | ✅ Sehr schnell |
| Fehlerbehandlung | ❌ Schwierig | ✅ Direkte Responses |
| Automatisierung | ❌ Problematisch | ✅ Perfekt |
| Multi-Threading | ❌ Nicht möglich | ✅ Unterstützt |

---

## 📝 Erweiterte Beispiele

### Batch-Account-Erstellung (Python)
```python
from python_admin_client import M59AdminClient

users = [
    ("user1", "pass123"),
    ("user2", "pass456"),
    ("user3", "pass789")
]

with M59AdminClient() as client:
    for username, password in users:
        response = client.send_command(f"create automated {username} {password}")
        print(f"{username}: {response}")
```

### Server-Monitoring (PowerShell)
```powershell
$conn = Connect-M59Server

while ($true) {
    $status = Send-M59Command -Connection $conn -Command "show sessions"
    Write-Host $status -ForegroundColor Green
    Start-Sleep -Seconds 60
}

Disconnect-M59Server -Connection $conn
```

### GUI-Integration (C#)
```csharp
private void btnExecuteCommand_Click(object sender, EventArgs e)
{
    using (var client = new M59AdminClient())
    {
        if (client.Connect())
        {
            string response = client.SendCommand(txtCommand.Text);
            txtResponse.Text = response;
        }
    }
}
```

---

## 🐛 Troubleshooting

### "Verbindungsfehler"
- Prüfe ob der Server läuft
- Prüfe `MaintenancePort` in blakserv.cfg
- Prüfe Firewall-Einstellungen

### "Keine Antwort vom Server"
- Manche Befehle geben keine Antwort
- Erhöhe Timeout-Werte
- Prüfe Server-Logs

### "Zugriff verweigert"
- Prüfe `MaintenanceMask` in blakserv.cfg
- Stelle sicher, dass deine IP in der Whitelist ist

---

## 📚 Weitere Ressourcen

- **Admin Command Reference**: Siehe README.md im Hauptverzeichnis
- **Server Guide**: `design/Admin/server guide.htm`
- **Protocol Details**: `blakserv/maintenance.c` und `blakserv/admin.c`

---

## 💡 Tipps

1. **Connection Pooling**: Für viele Befehle, eine Verbindung wiederverwenden
2. **Error Handling**: Immer Timeouts und Exceptions abfangen
3. **Logging**: Alle Befehle und Responses loggen für Debugging
4. **Testing**: Erst auf Test-Server testen, bevor auf Production
5. **Backup**: Vor größeren Operationen Game speichern (`save game`)

---

## 🤝 Beitragen

Diese Beispiele sind Ausgangspunkte. Erweitere sie nach deinen Bedürfnissen!

Mögliche Erweiterungen:
- GUI-Frontend (WPF, Windows Forms, Electron)
- Web-Interface (Flask, ASP.NET)
- REST API Wrapper
- Discord/Telegram Bot Integration
- Automatische Backups
- Player-Statistiken Dashboard
- Auto-Moderator Tools

---

**Viel Erfolg mit deinem Admin-Tool!** 🎮
