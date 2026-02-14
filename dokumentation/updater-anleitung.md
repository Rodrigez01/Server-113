# Meridian 59 - Updater & Patchsystem Anleitung

## Übersicht

Das Update-System besteht aus drei Komponenten:
- **meridian.exe** (Client) - erkennt Updates und lädt club.exe herunter
- **club.exe** (Updater) - lädt geänderte Dateien vom Webserver
- **blakserv** (Gameserver) - entscheidet ob ein Update nötig ist

## 1. clientpatch.txt erstellen

Das Tool `clientpatch.exe` scannt das Client-Verzeichnis und erzeugt eine JSON-Patchliste
mit MD5-Hashes, Dateigrößen und Pfaden für alle Client-Dateien.

### Aufruf:
```
clientpatch.exe <Ausgabedatei> <Client-Verzeichnis>
```

### Beispiel:
```
clientpatch.exe C:\Server-104\run\localclient\clientpatch.txt C:\Server-104\run\localclient\
```

Das Tool liegt in: `bin\clientpatch.exe` oder `util\release\clientpatch.exe`

### Wann neu generieren:
- Wenn eine Datei im Client geändert wurde (.bgf, .roo, .exe, .dll etc.)
- Wenn neue Dateien hinzugefügt wurden
- Nach jedem Neukompilieren von club.exe oder meridian.exe

### Format der clientpatch.txt:
```json
[
 {
  "Filename": "D3DX9_43.dll",
  "Basepath": "\\",
  "MyHash": "86E39E9161C3D930D93822F1563C280D",
  "Version": 3,
  "Download": true,
  "Length": 1998168
 },
 {
  "Filename": "1skyA.bgf",
  "Basepath": "\\resource\\",
  "MyHash": "6B28AD285ED31F7B50BE22233AF0D308",
  "Version": 3,
  "Download": true,
  "Length": 36495
 }
]
```

## 2. Versionssystem

### Client-Version (clientd3d\client.h):
```c
#define MAJOR_REV 50   // Classic Client = 50, Ogre Client = 90
#define MINOR_REV 55   // Deine Version, 0-99
```

Die Versionsnummer wird berechnet als:
```
client_version = MAJOR_REV * 100 + MINOR_REV = 5055
```

### Server-Konfiguration (blakserv.cfg):
```ini
[Login]
MinClassicVersion    5055

[Update]
DownloadReason          <Ein Update ist verfügbar und erforderlich.>
ClassicPatchHost        www.meridian59.rocks
ClassicPatchPath        /meridian59
ClassicPatchCachePath   /meridian59/
ClassicPatchTxt         clientpatch.txt
ClassicClubExe          club.exe
```

### Entscheidungslogik des Servers:
```
Client verbindet sich und schickt seine Version (z.B. 5055)

1. Version < InvalidVersion (100)?  --> Verbindung trennen
2. Version < MinClassicVersion?     --> Update erzwingen (club.exe starten)
3. RSB-Hash stimmt nicht überein?   --> Update erzwingen
4. Alles OK                         --> Login durchlassen
```

## 3. Update-Ablauf

```
1. meridian.exe verbindet sich zum Gameserver
2. Server vergleicht Client-Version mit MinClassicVersion
3. Server schickt Update-Befehl mit: PatchHost, PatchPath, PatchTxt, ClubExe
4. meridian.exe lädt club.exe von https://www.meridian59.rocks/meridian59/club.exe
5. meridian.exe startet club.exe und beendet sich
6. club.exe lädt clientpatch.txt von https://www.meridian59.rocks/meridian59/clientpatch.txt
7. club.exe vergleicht MD5-Hashes aller Dateien mit lokalen Dateien
8. club.exe lädt nur geänderte Dateien herunter
9. club.exe startet meridian.exe neu
```

## 4. Workflow: Neues Update veröffentlichen

### Schritt 1: Client-Version erhöhen
In `clientd3d\client.h`:
```c
#define MINOR_REV 56   // von 55 auf 56 erhöht
```

### Schritt 2: Kompilieren
- meridian.exe neu kompilieren (clientd3d)
- club.exe neu kompilieren (club)

### Schritt 3: Dateien vorbereiten
Neue/geänderte Dateien nach `run\localclient\` kopieren.
Insbesondere die neue club.exe:
```
copy club\release\club.exe run\localclient\club.exe
```

### Schritt 4: clientpatch.txt generieren
```
clientpatch.exe "run\localclient\clientpatch.txt" "run\localclient\"
```

### Schritt 5: Auf Webserver hochladen
Folgende Dateien auf https://www.meridian59.rocks/meridian59/ hochladen:
- clientpatch.txt (immer)
- club.exe (wenn geändert)
- meridian.exe (wenn geändert)
- Alle weiteren geänderten Dateien (.bgf, .roo, .dll etc.)

### Schritt 6: Gameserver konfigurieren
In `blakserv.cfg`:
```ini
[Login]
MinClassicVersion    5056
```

### Schritt 7: Gameserver neustarten
Damit die neue MinClassicVersion aktiv wird.

Ab jetzt bekommen alle Clients mit Version < 5056 die Update-Aufforderung.
Clients mit Version 5056 loggen sich direkt ein.

## 5. Webserver-Voraussetzungen (IIS)

### Verzeichnisstruktur auf dem Server:
```
C:\inetpub\wwwroot\meridian59\
    ├── web.config              (MIME-Types)
    ├── clientpatch.txt
    ├── club.exe
    ├── meridian.exe
    ├── D3DX9_43.dll
    ├── de\
    ├── resource\               (alle .bgf, .roo, .rsb, .ogg)
    ├── help\
    ├── mail\
    ├── download\
    └── ads\
```

### web.config (Minimal - nur MIME-Types):
IIS blockiert unbekannte Dateitypen. Die web.config registriert alle
Meridian 59 Formate (.bgf, .roo, .rsb, .bsf, .ogg, .khd, .pal etc.)
als application/octet-stream.

Datei liegt in: `run\server\iis\web.config`

### HTTPS:
Der Client und club.exe nutzen HTTPS (Port 443).
Ein gültiges SSL-Zertifikat muss im IIS gebunden sein.

## 6. Wichtige Dateipfade

| Datei | Pfad |
|---|---|
| Client-Version | `clientd3d\client.h` (MAJOR_REV, MINOR_REV) |
| Server-Config | `run\server\blakserv.cfg` ([Login] und [Update]) |
| Updater-Quellcode | `club\club.c`, `club\transfer.c` |
| Client-Download-Code | `clientd3d\download.c`, `clientd3d\transfer.c` |
| Patchlisten-Generator | `bin\clientpatch.exe` (Quellcode: `util\clientpatch.c`) |
| Patchlisten-Header | `include\clientpatch.h` (Hash-Vergleich, JSON-Format) |
| Generierte Patchliste | `run\localclient\clientpatch.txt` |
| IIS MIME-Config | `run\server\iis\web.config` |
