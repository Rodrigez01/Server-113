# 🌐 Meridian 59 - Netzwerk Datei-Transfer System (Windows)

## Wie Clients Dateien vom Server bekommen

---

## 🎯 Zusammenfassung

**Meridian 59 braucht KEINEN separaten Web-Server!**

Der `blakserv.exe` Server enthält einen **eingebauten File-Server**, der Dateien direkt über das **Spiel-Protokoll** (Port 5959) überträgt.

---

## 📡 Das Protokoll

### Schritt 1: Client startet und verbindet

```
CLIENT (z.B. 123.45.67.89)
    │
    ├──► TCP-Verbindung zu: server.com:5959
    │
    └──► Verbunden!
```

### Schritt 2: Client fordert Resource-Liste an

```c
// Im Client-Code (protocol.h Zeile 54):
RequestResources()  →  ToServer(AP_GETRESOURCE, NULL)
```

**Was passiert:**
```
CLIENT                           SERVER
  │                                │
  ├──► AP_GETRESOURCE             │
  │    "Welche Dateien hast du?"  │
  │                                │
  │    ◄──────────────────────────┤
  │    rsc0000.rsb Inhalt:        │
  │    - sword.bgf [123KB] [CRC]  │
  │    - player.rsc [45KB] [CRC]  │
  │    - hit.ogg [12KB] [CRC]     │
  │    ... (alle Dateien)          │
```

### Schritt 3: Client vergleicht mit lokalen Dateien

```
CLIENT (lokal):
    ├── resource\
    │   ├── sword.bgf  [OLD VERSION]  ❌ Veraltet!
    │   ├── player.rsc [OK]           ✅ Aktuell
    │   └── hit.ogg    [FEHLT]        ❌ Nicht vorhanden

ENTSCHEIDUNG:
    → Lade: sword.bgf (veraltet)
    → Lade: hit.ogg (fehlt)
    → Skip: player.rsc (aktuell)
```

### Schritt 4: Client lädt fehlende/veraltete Dateien

```c
// Im Client-Code (protocol.h Zeile 55):
RequestAllFiles()  →  ToServer(AP_GETALL, NULL)
```

**Was passiert:**
```
CLIENT                                    SERVER
  │                                         │
  ├──► AP_GETALL                           │
  │    "Ich brauche:"                       │
  │    - sword.bgf (Checksum mismatch)      │
  │    - hit.ogg (nicht vorhanden)          │
  │                                         │
  │    ◄────────────────────────────────────┤
  │    DATEI-TRANSFER STARTET:              │
  │    [sword.bgf] Chunk 1/50 [2KB]         │
  │    [sword.bgf] Chunk 2/50 [2KB]         │
  │    ...                                   │
  │    [sword.bgf] Chunk 50/50 [1.5KB]      │
  │    [hit.ogg] Chunk 1/6 [2KB]            │
  │    ...                                   │
  │    [hit.ogg] Chunk 6/6 [2KB]            │
  │                                         │
  └──► DOWNLOAD ABGESCHLOSSEN!              │
```

### Schritt 5: Client speichert Dateien lokal

```
CLIENT speichert:
    resource\sword.bgf  ← Neue Version
    resource\hit.ogg    ← Neu heruntergeladen

    → Spiel kann starten!
```

---

## 🔧 Server-Seite: Wie blakserv.exe das macht

### 1. Server startet und lädt rsc0000.rsb

```c
// In blakserv/blakres.c (vereinfacht):
LoadResourceDatabase() {
    // Lese rsc0000.rsb
    file = fopen("rsc\\rsc0000.rsb", "rb");

    // Parse Database:
    // - Dateinamen
    // - Dateigrößen
    // - Checksums (CRC)
    // - Offsets

    // Speichere in Speicher für schnellen Zugriff
}
```

### 2. Client fordert Dateiliste an (AP_GETRESOURCE)

```c
// Server empfängt: AP_GETRESOURCE
HandleGetResource(client) {
    // Sende rsc0000.rsb Inhalt an Client
    SendResourceList(client);
}
```

**Server sendet:**
```
RESOURCE DATABASE:
├── Total Files: 1234
├── Total Size: 123MB
├── Files:
│   ├── sword.bgf
│   │   ├── Size: 123456 bytes
│   │   └── CRC: 0xABCD1234
│   ├── player.rsc
│   │   ├── Size: 45678 bytes
│   │   └── CRC: 0x1234ABCD
│   └── ...
```

### 3. Client fordert Dateien an (AP_GETALL)

```c
// Server empfängt: AP_GETALL + Liste der benötigten Dateien
HandleGetAll(client, file_list) {
    foreach (file in file_list) {
        SendFile(client, "rsc\\" + file.name);
    }
}
```

**Server sendet Datei-Chunks:**
```c
SendFile(client, filepath) {
    file = fopen(filepath, "rb");

    // Lese in 2KB Chunks
    while (!eof(file)) {
        chunk = read(file, 2048);
        SendChunk(client, chunk);
    }

    fclose(file);
}
```

---

## 🗂️ Dateistruktur auf dem Server

```
E:\Meridian59_source\...\run\server\
├── blakserv.exe           ← DER SERVER (Game + File Server)
├── blakserv.cfg           ← Konfiguration
│   └── [Resource]
│       └── (keine spezielle Config nötig!)
│
└── rsc\                   ← CLIENT-DATEIEN (öffentlich)
    ├── rsc0000.rsb       ← DATABASE (Liste aller Dateien) ⭐
    ├── sword.bgf         ← Grafiken
    ├── player.rsc        ← Text-Ressourcen
    ├── hit.ogg           ← Sounds
    └── ... (alle Client-Dateien)
```

---

## ⚙️ Wie du Dateien verteilst

### Wenn du eine Datei änderst:

```cmd
REM 1. ÄNDERE Datei (z.B. neue Grafik)
notepad resource\sword.bgf

REM 2. KOPIERE nach rsc\
copy resource\sword.bgf run\server\rsc\sword.bgf

REM 3. AKTUALISIERE rsc0000.rsb
cd run\server\rsc
..\..\..\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav

REM 4. FERTIG!
REM Server sendet beim nächsten Client-Login:
REM   - Neue rsc0000.rsb Info (sword.bgf hat neue Checksum!)
REM   - Client erkennt: "sword.bgf ist veraltet"
REM   - Client lädt: neue sword.bgf vom Server
```

### Du brauchst KEINEN Web-Server!

❌ **NICHT nötig:**
- Nginx
- Apache
- HTTP Server
- FTP Server

✅ **NUR nötig:**
- blakserv.exe läuft
- Port 5959 offen (TCP)
- Dateien in `rsc\`
- rsc0000.rsb aktuell

---

## 🔍 Wie Clients die Dateien finden

### Client-Konfiguration

Der Client muss nur wissen:
1. **Server-IP/Hostname:** z.B. `server.meridiannext.com`
2. **Port:** 5959 (Standard)

**Alle Dateien kommen automatisch über diesen Socket!**

### Beispiel-Verbindung:

```
Client-PC (123.45.67.89)
    │
    ├──► DNS-Lookup: server.meridiannext.com → 98.76.54.32
    │
    ├──► TCP-Connect: 98.76.54.32:5959
    │
    ├──► Login-Prozess
    │    ├── Username/Password
    │    └── ✅ Erfolgreich
    │
    ├──► RequestResources()
    │    └── Server sendet: rsc0000.rsb Info
    │
    ├──► Vergleich mit lokalen Dateien
    │
    ├──► RequestAllFiles()
    │    ├── Download: sword.bgf (123KB)
    │    ├── Download: hit.ogg (12KB)
    │    └── ✅ Fertig
    │
    └──► SPIEL STARTET!
```

---

## 🚀 Vorteile dieses Systems

### ✅ PRO:

1. **Einfach:** Keine extra Web-Server nötig
2. **Sicher:** Dateien nur nach Login verfügbar
3. **Effizient:** Nur geänderte Dateien werden übertragen
4. **Integriert:** Alles über ein Protokoll
5. **Automatisch:** Client managed Downloads selbst

### ⚠️ CON:

1. **Langsam:** Nur über Game-Socket, keine parallelen Downloads
2. **Keine Resume:** Download-Abbruch = von vorne
3. **Server-Last:** Jeder Client belastet den Game-Server

---

## 🔐 Firewall / Netzwerk-Anforderungen

### Server (Hosting):

```cmd
REM Windows Firewall: Port 5959 TCP öffnen
REM
REM 1. Öffne "Windows Defender Firewall mit erweiterter Sicherheit"
REM 2. Neue eingehende Regel:
REM    - Regeltyp: Port
REM    - Protokoll: TCP
REM    - Port: 5959
REM    - Aktion: Verbindung zulassen
REM    - Name: Meridian 59 Server
```

**Oder per netsh (als Administrator):**
```cmd
netsh advfirewall firewall add rule name="Meridian 59 Server" dir=in action=allow protocol=TCP localport=5959
```

### Client (Spieler):

```
Keine spezielle Konfiguration nötig!

    ├── Ausgehende Verbindung auf Port 5959 (meist erlaubt)
    └── Firewall fragt eventuell beim ersten Start
```

---

## 🎯 Praktisches Beispiel

### Szenario: Du änderst eine Grafik

**1. Vor der Änderung:**
```
SERVER: rsc\sword.bgf (Version 1, CRC: 0x12345678)
CLIENT: resource\sword.bgf (Version 1, CRC: 0x12345678)
    → Checksums stimmen überein ✅
```

**2. Du änderst die Grafik:**
```cmd
REM Neue Grafik erstellen (z.B. mit Paint.NET)
REM E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sword.bgf

REM Nach rsc\ kopieren
copy resource\sword.bgf run\server\rsc\sword.bgf

REM rsc0000.rsb neu erstellen
cd run\server\rsc
..\..\..\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg
```

**3. Nach der Änderung:**
```
SERVER: rsc\sword.bgf (Version 2, CRC: 0x87654321)  ← NEU!
CLIENT: resource\sword.bgf (Version 1, CRC: 0x12345678)  ← ALT
    → Checksums unterschiedlich ❌
```

**4. Client verbindet (neuer Login):**
```
CLIENT → Server: "RequestResources()"
SERVER → Client: "rsc0000.rsb: sword.bgf CRC=0x87654321"
CLIENT prüft lokal: "Mein sword.bgf CRC=0x12345678"
CLIENT erkennt: "VERALTET! Muss neu laden!"
CLIENT → Server: "RequestAllFiles(sword.bgf)"
SERVER → Client: [sendet neue sword.bgf]
CLIENT speichert: resource\sword.bgf (Version 2)
    → Jetzt aktuell ✅
```

**5. Spieler sieht:**
```
"Loading resources... 1 file(s)"
[████████████████] 100%
"Entering game..."
    → Neue Grafik wird angezeigt!
```

---

## 📊 Technische Details

### Protokoll-Messages:

| Message | Von | An | Zweck |
|---------|-----|----|-------|
| `AP_GETRESOURCE` | Client | Server | "Schick mir Dateiliste" |
| `BP_RESOURCE` | Server | Client | "Hier ist die Dateiliste" |
| `AP_GETALL` | Client | Server | "Ich brauche diese Dateien" |
| `BP_FILE` | Server | Client | "Hier sind Datei-Chunks" |

### Datei-Chunks:

- **Chunk-Größe:** ~2KB (variabel)
- **Übertragung:** Sequenziell (eine Datei nach der anderen)
- **Kompression:** Optional (zlib)

### rsc0000.rsb Format:

```
HEADER:
├── Magic Number: "RSB0"
├── Version: 1
├── File Count: 1234
└── Total Size: 123456789 bytes

FILE ENTRIES (für jede Datei):
├── Filename: "sword.bgf" (null-terminated)
├── File Size: 123456 bytes
├── CRC32: 0x12345678
└── Offset: 0 (für zukünftige Nutzung)

(Keine tatsächlichen Datei-Daten in .rsb!)
```

---

## 🛠️ Debugging

### Client lädt keine Updates:

```cmd
REM 1. Prüfe ob rsc0000.rsb aktuell ist:
dir run\server\rsc\rsc0000.rsb
REM Timestamp muss AKTUELL sein!

REM 2. Prüfe ob Datei in rsc\ existiert:
dir run\server\rsc\sword.bgf

REM 3. Teste rsc0000.rsb Inhalt:
..\..\..\bin\rscprint.exe run\server\rsc\rsc0000.rsb | findstr sword
REM Sollte sword.bgf mit aktueller Größe zeigen

REM 4. Server-Log prüfen (nach Server-Stop):
notepad run\server\channel\debug.txt
REM Suche nach "resource"
```

### Server sendet Dateien nicht:

```cmd
REM 1. Prüfe ob Server läuft:
tasklist | findstr blakserv

REM 2. Prüfe ob Port offen ist:
netstat -an | findstr 5959

REM 3. Prüfe Dateirechte:
dir /a run\server\rsc\sword.bgf
REM Darf nicht "schreibgeschützt" sein
```

---

## 📝 Zusammenfassung

**So funktioniert Meridian 59 File Distribution:**

1. ✅ **Eingebaut in blakserv.exe** (kein separater Web-Server)
2. ✅ **Über Spiel-Protokoll** (Port 5959, TCP)
3. ✅ **Automatisch** (Client managed selbst)
4. ✅ **Inkrementell** (nur geänderte Dateien)
5. ✅ **CRC-basiert** (Checksum-Vergleich)

**Du musst nur:**
1. Dateien nach `rsc\` kopieren
2. `rscmerge.exe` ausführen → `rsc0000.rsb` aktualisieren
3. Fertig! Clients laden automatisch beim Login

---

**Erstellt:** 28. Dezember 2025
**System:** Meridian 59 Server 105 (Windows)
**Protokoll-Version:** Standard M59 Protocol
