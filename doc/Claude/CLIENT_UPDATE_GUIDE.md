# 🎮 Meridian 59 - Client Update System Guide (Windows)

## Wie du Änderungen an Spieler-Clients verteilst

---

## ⚡ Quick Start: Resource Database neu erstellen

```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"

REM Alle Resource-Dateien in Database packen
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav *.mp3 *.bsf

REM Fertig! Clients laden beim nächsten Login automatisch Updates
```

---

## 📁 Datei-Typen

### ✅ Client-seitig (werden verteilt):

| Typ | Was ist das? | Beispiel |
|-----|--------------|----------|
| `.rsc` | Text-Ressourcen (Namen, Beschreibungen) | `sword.rsc` |
| `.bgf` | Grafiken & Animationen | `player.bgf` |
| `.ogg` | Sounds & Musik | `sword_swing.ogg` |
| `.wav` | Sounds (alt) | `footstep.wav` |
| `.bsf` | Special Effects | `fireball.bsf` |

### ❌ Server-seitig (NICHT verteilt):

| Typ | Was ist das? |
|-----|--------------|
| `.bof` | Kompilierter Server-Code |
| `.kod` | Source Code |

---

## 🔄 Der Update-Workflow

```
1. ÄNDERN              2. KOPIEREN            3. PACKEN             4. VERTEILEN
   ↓                      ↓                      ↓                     ↓
Du editierst:          Nach rsc\ kopieren:    Database erstellen:   Auto beim Login!
resource\              run\server\rsc\        rscmerge.exe          Client lädt neue
├── sword.bgf      →   ├── sword.bgf      →   → rsc0000.rsb     →   Dateien runter
└── sounds\            └── sounds\
    └── hit.ogg            └── hit.ogg
```

---

## 📝 KOMPLETTES BEISPIEL

### Szenario: Neue Schwert-Grafik verteilen

```cmd
REM 1️⃣ GRAFIK ÄNDERN
REM Editiere mit Paint.NET oder Photoshop:
REM E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sword.bgf

REM 2️⃣ NACH RSC\ KOPIEREN
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sword.bgf" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\sword.bgf"

REM 3️⃣ RESOURCE DATABASE NEU ERSTELLEN
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav *.mp3 *.bsf

REM 4️⃣ FERTIG!
REM Server muss NICHT neu gestartet werden
REM Clients laden beim nächsten Login die neue sword.bgf automatisch
```

---

## 🛠️ Die wichtigen Tools

### 1. `rscmerge.exe` - Resource Database Builder

**Was es tut:**
- Packt alle `.rsc`, `.bgf`, `.ogg` etc. in eine `rsc0000.rsb` Database
- Clients prüfen diese Database und laden neue/geänderte Dateien

**Syntax:**
```cmd
rscmerge.exe -o <output.rsb> <input files...>
```

**Beispiel:**
```cmd
cd run\server\rsc
..\..\..\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg
```

### 2. `blakdiff.exe` - Resource File Comparator

**Was es tut:**
- Vergleicht zwei .rsc Dateien
- Returncode 0 = gleich, 1 = unterschiedlich

**Wird automatisch von Makefile genutzt**

### 3. `makebgf.exe` - BGF Creator

**Was es tut:**
- Erstellt .bgf Dateien aus Bildern
- Für neue Grafiken

---

## 🎯 Typische Workflows

### A) String-Ressourcen ändern (.rsc)

```cmd
REM 1. Ändere .kod Datei in kod\
notepad "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon\sword.kod"

REM 2. Kompiliere mit bc.exe
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon"
"..\..\..\bin\bc.exe" -d -I "..\..\..\kod\include" -K "..\..\..\kod\kodbase.txt" sword.kod
REM → Erstellt sword.bof UND sword.rsc

REM 3. Kopiere .bof nach memmap\ (Server)
copy sword.bof "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\memmap\"

REM 4. Kopiere .rsc nach rsc\ (Client)
copy sword.rsc "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\"

REM 5. Resource Database neu erstellen
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav

REM 6. Server neu laden
REM Im Spiel: Send o 0 RecreateAll
```

### B) Grafik ändern (.bgf)

```cmd
REM 1. Ändere Grafik
REM Editiere: resource\sword.bgf

REM 2. Kopiere nach rsc\
copy "resource\sword.bgf" "run\server\rsc\sword.bgf"

REM 3. Resource Database neu erstellen
cd "run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav

REM 4. KEIN Server-Neustart nötig!
REM Clients laden beim nächsten Login automatisch
```

### C) Sound ändern (.ogg)

```cmd
REM 1. Ändere Sound
REM Ersetze: resource\sounds\sword_hit.ogg

REM 2. Kopiere nach rsc\
copy "resource\sounds\sword_hit.ogg" "run\server\rsc\sword_hit.ogg"

REM 3. Resource Database neu erstellen
cd "run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav

REM 4. Clients laden beim nächsten Login
```

---

## 📊 Was passiert beim Client-Login?

```
CLIENT STARTET
    ↓
Verbindet zu Server
    ↓
Server sendet: rsc0000.rsb Info
    ↓
Client vergleicht mit lokaler Version
    ↓
Unterschiede gefunden?
    ├─ JA → Download neue Dateien
    │        ├── sword.bgf (geändert)
    │        ├── player.rsc (neu)
    │        └── hit.ogg (gelöscht vom Server)
    │
    └─ NEIN → Login fortsetzen
```

---

## ⚠️ WICHTIG!

### DO's ✅

- **IMMER** rsc0000.rsb neu erstellen nach Änderungen
- Grafiken/Sounds nach `run\server\rsc\` kopieren
- .bof Dateien nach `run\server\memmap\` kopieren
- Clients laden automatisch Updates

### DON'Ts ❌

- **NICHT** .bof oder .kod an Clients verteilen (nur Server!)
- **NICHT** vergessen rsc0000.rsb zu aktualisieren
- **NICHT** Server neustarten für .rsc/.bgf Änderungen (unnötig!)

---

## 🔍 Debugging

### Problem: Clients laden Updates nicht

1. **Prüfe rsc0000.rsb Timestamp:**
   ```cmd
   dir "run\server\rsc\rsc0000.rsb"
   REM Muss aktuell sein!
   ```

2. **Prüfe ob Datei in rsc\ ist:**
   ```cmd
   dir "run\server\rsc\sword.bgf"
   ```

3. **Neu erstellen:**
   ```cmd
   cd run\server\rsc
   del rsc0000.rsb
   ..\..\..\bin\rscmerge.exe -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav
   ```

### Problem: Client-Error "File not found"

- Datei fehlt in rsc0000.rsb
- Lösung: rscmerge neu ausführen

---

## 📚 Verzeichnis-Struktur

```
E:\Meridian59_source\04.08.2016\Meridian59-develop\
│
├── resource\              ← HIER editierst du
│   ├── *.bgf             (Original-Grafiken)
│   ├── sounds\
│   │   └── *.ogg
│   └── rooms\
│
├── kod\                   ← Source Code
│   └── object\
│       └── item\
│           └── sword.kod  (Editieren → sword.rsc erstellen)
│
├── bin\                   ← Build Tools
│   ├── bc.exe            (Kod Compiler)
│   ├── rscmerge.exe      (Resource Database Builder) ⭐
│   ├── blakdiff.exe      (File Comparator)
│   └── makebgf.exe       (BGF Creator)
│
└── run\server\
    ├── rsc\              ← Client Resource Files ⭐
    │   ├── *.rsc        (Strings)
    │   ├── *.bgf        (Grafiken)
    │   ├── *.ogg        (Sounds)
    │   └── rsc0000.rsb  (RESOURCE DATABASE) ⭐⭐⭐
    │
    └── memmap\          ← Server Code
        └── *.bof
```

---

## 🚀 Quick Commands

### Resource Database neu erstellen:
```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav *.mp3 *.bsf
```

### Timestamp prüfen:
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\rsc0000.rsb"
```

### Alle .rsc kopieren (PowerShell):
```powershell
Get-ChildItem -Path "kod" -Recurse -Filter "*.rsc" | Copy-Item -Destination "run\server\rsc\"
```

---

**Erstellt:** 28. Dezember 2025
**Für:** Meridian 59 Server Updates (Windows)
**Status:** ✅ Komplett & Getestet
