# 🖥️ Meridian 59 - Server Setup Guide (Root-Server)

## Wie du einen Root-Server einrichtest und Dateien verteilst

---

## 🎯 Die zwei Update-Typen verstehen

### ✅ Typ 1: Resource-Updates (AUTOMATISCH)

**Was wird automatisch verteilt:**
- `.rsc` - Text-Ressourcen
- `.bgf` - Grafiken
- `.ogg` / `.wav` - Sounds
- `.bsf` - Effekte

**Wie es funktioniert:**
- Client verbindet zu Port 5959
- Server sendet automatisch neue/geänderte Dateien
- **KEIN Web-Server oder FTP nötig!**

**Beispiel:**
```
Du änderst: sword.bgf
Server hat: E:\...\run\server\rsc\sword.bgf
Client lädt: Automatisch beim nächsten Login
```

---

### ❌ Typ 2: Client-EXE Updates (MANUELL)

**Was wird NICHT automatisch verteilt:**
- `meridian.exe` / `club.exe` - Die Client-Software
- Client-DLLs (z.B. `opengl32.dll`)
- Client-Konfiguration

**Wie es funktioniert:**
- Spieler muss **manuell** die neue .exe herunterladen
- Von einer Website, FTP, oder Download-Link

**Beispiel:**
```
Du änderst: meridian.exe
Server hat: NICHTS (Client-EXE läuft auf Spieler-PC)
Client lädt: Manuell von deiner Website/FTP
```

---

## 🌐 Root-Server Setup

### Szenario: Du hast einen Root-Server gemietet

```
ROOT-SERVER (z.B. 123.45.67.89)
├── Betriebssystem: Windows Server 2019
├── Dein Zugriff: RDP (Remote Desktop) oder SSH
└── Meridian 59 Server läuft hier
```

### Schritt 1: Meridian 59 auf den Server kopieren

#### Option A: Remote Desktop (Windows Server)

```
1. Verbinde mit RDP zu deinem Server
   - IP: 123.45.67.89
   - User: Administrator
   - Pass: deinPasswort

2. Kopiere Meridian59-develop\ auf den Server:
   - Via Copy-Paste in RDP
   - Oder: Via FTP/SFTP hochladen
   - Ziel: C:\Meridian59\

3. Verzeichnisstruktur auf dem Server:
   C:\Meridian59\
   ├── bin\
   │   ├── blakserv.exe
   │   └── rscmerge.exe
   ├── run\server\
   │   ├── blakserv.exe
   │   ├── blakserv.cfg  ← WICHTIG!
   │   ├── rsc\
   │   │   ├── rsc0000.rsb
   │   │   ├── *.rsc
   │   │   ├── *.bgf
   │   │   └── *.ogg
   │   └── memmap\
   │       └── *.bof
   └── kod\
```

#### Option B: Linux Server (Wine)

```bash
# 1. Verbinde via SSH
ssh root@123.45.67.89

# 2. Installiere Wine
apt-get install wine

# 3. Kopiere Dateien per SFTP
# (von deinem lokalen PC)
scp -r Meridian59-develop root@123.45.67.89:/root/meridian59/

# 4. Server starten
cd /root/meridian59/run/server
wine blakserv.exe
```

---

## ⚙️ blakserv.cfg Konfiguration

### Wichtige Einstellungen:

```ini
# C:\Meridian59\run\server\blakserv.cfg

[Socket]
# Port für Spiel-Verbindungen (hier laufen auch Resource-Downloads)
Port = 5959

[Path]
# Wo liegen die .bof Dateien?
Memmap = memmap\

# Wo liegen die Resource-Dateien für Clients?
Rsc = rsc\

# Wo liegen die Raum-Dateien?
Rooms = rooms\

[Resource]
# Resource Database Datei
RscFile = rsc\rsc0000.rsb

# KEINE weiteren Einstellungen nötig!
# blakserv.exe verteilt Dateien automatisch
```

### Vollständige Beispiel-Konfiguration:

```ini
[Socket]
Port = 5959

[Path]
Memmap = memmap\
Rsc = rsc\
Rooms = rooms\
Motd = motd\
Channel = channel\

[Resource]
RscFile = rsc\rsc0000.rsb

[Login]
MaxGuests = 100
MaxUsers = 500

[GameTime]
GameDayLength = 1440  # 24 Minuten = 1 Spieltag
```

**Das war's! Keine FTP-Einstellungen nötig.**

---

## 🔥 Windows Firewall auf dem Root-Server

### Port 5959 öffnen:

```cmd
REM Als Administrator ausführen:
netsh advfirewall firewall add rule ^
  name="Meridian 59 Server" ^
  dir=in ^
  action=allow ^
  protocol=TCP ^
  localport=5959
```

### Prüfen ob Port offen ist:

```cmd
netstat -an | findstr 5959
```

**Erwartetes Ergebnis:**
```
TCP    0.0.0.0:5959    0.0.0.0:0    LISTENING
```

---

## 📂 Dateien auf den Root-Server hochladen (für Admins)

### Du (als Admin) brauchst einen Weg, Dateien hochzuladen:

#### Option 1: Remote Desktop (RDP) - EINFACHST

```
1. Verbinde mit RDP
2. Copy-Paste direkt:
   - Von deinem PC: resource\sword.bgf
   - Auf den Server: C:\Meridian59\run\server\rsc\sword.bgf
3. rscmerge.exe ausführen auf dem Server
```

#### Option 2: FTP Server (für regelmäßige Updates)

```
1. Installiere FTP Server auf dem Root-Server
   - Windows: FileZilla Server
   - Linux: vsftpd

2. Verbinde von deinem PC aus:
   ftp://123.45.67.89
   User: admin
   Pass: deinPasswort

3. Lade Dateien hoch nach:
   /Meridian59/run/server/rsc/

4. Verbinde mit RDP/SSH und führe rscmerge aus
```

#### Option 3: SFTP (Linux) - SICHER

```bash
# Von deinem PC aus:
sftp root@123.45.67.89

sftp> cd /root/meridian59/run/server/rsc
sftp> put sword.bgf
sftp> exit

# Dann SSH und rscmerge:
ssh root@123.45.67.89
cd /root/meridian59/run/server/rsc
wine ../../../bin/rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg
```

---

## 🎮 Client-EXE Updates verteilen

### Problem: meridian.exe wird NICHT automatisch verteilt!

### Lösung 1: Download-Link auf Website

```
1. Erstelle eine Website (z.B. https://meinserver.de)

2. Stelle meridian.exe zum Download bereit:
   https://meinserver.de/downloads/meridian.exe

3. Spieler laden manuell herunter und ersetzen ihre .exe
```

### Lösung 2: FTP Download

```
1. Installiere FTP Server (öffentlich lesbar)
   ftp://meinserver.de/meridian.exe

2. Spieler verbinden und laden herunter
```

### Lösung 3: Auto-Updater (fortgeschritten)

```
Du brauchst:
1. Einen Auto-Updater (custom Tool)
2. Eine Version-Check-Datei auf deinem Web-Server

Beispiel:
- Client startet launcher.exe
- launcher.exe prüft: https://meinserver.de/version.txt
  - Aktuell: 1.0.5
  - Client hat: 1.0.4
- launcher.exe lädt: https://meinserver.de/meridian.exe
- Client startet automatisch neu
```

**Wichtig:** Meridian 59 hat standardmäßig KEINEN Auto-Updater!

---

## 📋 Kompletter Workflow: Resource-Update verteilen

### Szenario: Du änderst eine Grafik und willst sie an alle Spieler verteilen

#### Schritt 1: Auf deinem lokalen PC

```cmd
REM 1. Ändere die Grafik
notepad resource\sword.bgf

REM 2. Teste lokal ob es funktioniert
```

#### Schritt 2: Hochladen auf den Root-Server

**Via RDP:**
```
1. Remote Desktop zu 123.45.67.89
2. Copy-Paste:
   - Von: E:\Meridian59_source\...\resource\sword.bgf
   - Nach: C:\Meridian59\run\server\rsc\sword.bgf
```

**Oder via FTP:**
```
1. FileZilla verbinden zu 123.45.67.89
2. Upload nach: /Meridian59/run/server/rsc/sword.bgf
```

#### Schritt 3: Auf dem Root-Server (via RDP)

```cmd
REM Verbinde mit RDP oder SSH

REM Gehe ins rsc\ Verzeichnis
cd C:\Meridian59\run\server\rsc

REM Resource Database neu erstellen
C:\Meridian59\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav

REM Prüfen
dir rsc0000.rsb
```

#### Schritt 4: FERTIG!

```
✅ Clients laden beim nächsten Login automatisch die neue sword.bgf
✅ KEIN Server-Neustart nötig (für Grafiken)
✅ KEINE weitere Aktion nötig
```

---

## 🚨 Häufige Fehler

### Fehler 1: "Clients laden keine Updates"

**Ursache:** rsc0000.rsb nicht aktualisiert

**Lösung:**
```cmd
REM Auf dem Root-Server:
cd C:\Meridian59\run\server\rsc
del rsc0000.rsb
C:\Meridian59\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg
```

### Fehler 2: "Port 5959 nicht erreichbar"

**Ursache:** Firewall blockiert

**Lösung:**
```cmd
REM Auf dem Root-Server:
netsh advfirewall firewall add rule name="M59" dir=in action=allow protocol=TCP localport=5959

REM Prüfen:
netstat -an | findstr 5959
```

### Fehler 3: "Server findet rsc0000.rsb nicht"

**Ursache:** Falsche Pfade in blakserv.cfg

**Lösung:**
```ini
# blakserv.cfg
[Path]
Rsc = rsc\          # NICHT: C:\Meridian59\run\server\rsc\
                     # Relativer Pfad vom blakserv.exe Verzeichnis!

[Resource]
RscFile = rsc\rsc0000.rsb
```

---

## 🔐 Sicherheit

### Wichtig:

1. **Resource-Dateien sind öffentlich!**
   - Jeder der sich einloggt kann sie runterladen
   - Keine Passwörter oder Secrets in .rsc Dateien!

2. **Server-Code ist privat!**
   - .bof Dateien werden NICHT verteilt
   - Nur auf dem Server in memmap\

3. **Firewall:**
   - Nur Port 5959 öffnen
   - NICHT Port 3389 (RDP) öffentlich lassen!

---

## 📊 Zusammenfassung

### Was wird automatisch verteilt (über Port 5959):
- ✅ .rsc (Texte)
- ✅ .bgf (Grafiken)
- ✅ .ogg/.wav (Sounds)
- ✅ .bsf (Effekte)

### Was wird NICHT automatisch verteilt:
- ❌ meridian.exe (Client-Software)
- ❌ .dll Dateien
- ❌ Client-Konfiguration
- ❌ .bof Dateien (nur Server)

### Was du brauchst:

**Für Resource-Updates:**
- ✅ blakserv.exe läuft
- ✅ Port 5959 offen
- ✅ rsc0000.rsb aktuell
- ❌ KEIN FTP/Web-Server nötig!

**Für Client-EXE Updates:**
- ✅ Website ODER FTP Server
- ✅ Download-Link für Spieler
- ❌ NICHT automatisch!

### Workflow-Übersicht:

```
DU (Admin)                  ROOT-SERVER                 SPIELER
    │                            │                          │
    ├─► Via RDP/FTP             │                          │
    │   hochladen               │                          │
    │   sword.bgf  ────────────►│                          │
    │                            │                          │
    │                            ├─► rscmerge.exe          │
    │                            │   erstellt rsc0000.rsb  │
    │                            │                          │
    │                            │   Client Login ◄────────┤
    │                            │                          │
    │                            ├─► AP_GETRESOURCE ───────►│
    │                            │                          │
    │                            ├─► BP_RESOURCE ──────────►│
    │                            │   (neue sword.bgf!)      │
    │                            │                          │
    │                            ├─► BP_FILE ──────────────►│
    │                            │   [sword.bgf Download]   │
    │                            │                          │
    │                            │   ◄──────── FERTIG! ─────┤
```

---

**Erstellt:** 28. Dezember 2025
**Für:** Root-Server Setup & Updates
**Version:** 1.0 (Windows)
