# 🎮 SCHRITT-FÜR-SCHRITT: Client Updates verteilen (Windows)

## ✅ VORAUSSETZUNGEN

Bevor du anfängst, prüfe ob diese Dinge vorhanden sind:

### 1. Prüfe ob rscmerge.exe existiert:
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe"
```
**Erwartetes Ergebnis:**
```
... 123.456 rscmerge.exe
```
❌ **Falls nicht vorhanden:** Du musst erst das Projekt kompilieren (makefile.mak)

### 2. Prüfe ob rsc\ Verzeichnis existiert:
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
```
**Erwartetes Ergebnis:**
```
Verzeichnis von E:\Meridian59_source\...\run\server\rsc
```

### 3. Prüfe ob rsc0000.rsb existiert:
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\rsc0000.rsb"
```
**Erwartetes Ergebnis:**
```
28.12.2025  17:00    1.700.000 rsc0000.rsb
```

✅ **Wenn alle 3 Prüfungen OK sind, kannst du weitermachen!**

---

## 📝 SZENARIO 1: Text-Ressource ändern (z.B. Item-Beschreibung)

### Was du brauchst:
- Eine .kod Datei die du ändern willst
- bc.exe (Blakod Compiler)

### Schritt-für-Schritt:

#### Schritt 1: Finde die richtige .kod Datei
```cmd
REM Beispiel: Du willst die Beschreibung von "Mystic Sword" ändern
dir /s /b "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\*mystic*"
```

**Ergebnis notieren:** z.B. `kod\object\item\weapon\mysticsword.kod`

#### Schritt 2: .kod Datei editieren
```cmd
REM Öffne die Datei mit Notepad++, VSCode oder Notepad
notepad "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon\mysticsword.kod"

REM Ändere z.B. die Beschreibung:
REM MysticSword_desc_rsc = "This is a magical sword."
REM →
REM MysticSword_desc_rsc = "This is my AWESOME magical sword!"
```

**SPEICHERN NICHT VERGESSEN!**

#### Schritt 3: Kompilieren
```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon"

"E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\bc.exe" ^
  -d ^
  -I "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\include" ^
  -K "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\kodbase.txt" ^
  mysticsword.kod
```

**Erwartetes Ergebnis:**
```
(keine Fehlermeldung)
```

**Prüfung ob es funktioniert hat:**
```cmd
dir mysticsword.bof mysticsword.rsc
```
**Du solltest sehen:**
```
... mysticsword.bof  (Server-Code)
... mysticsword.rsc  (Client-Ressource)
```

#### Schritt 4: .bof nach memmap\ kopieren (Server-Update)
```cmd
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon\mysticsword.bof" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\memmap\mysticsw.bof"
```

**Prüfung:**
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\memmap\mysticsw.bof"
```
**Timestamp muss AKTUELL sein!**

#### Schritt 5: .rsc nach rsc\ kopieren (Client-Update)
```cmd
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon\mysticsword.rsc" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.rsc"
```

**Prüfung:**
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.rsc"
```

#### Schritt 6: Resource Database NEU ERSTELLEN (WICHTIG!)
```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"

"E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe" ^
  -o rsc0000.rsb ^
  *.rsc *.bgf *.ogg *.wav *.mp3
```

**Erwartetes Ergebnis:**
```
(Programm läuft kurz, keine Fehlermeldung)
```

**Prüfung ob es funktioniert hat:**
```cmd
dir rsc0000.rsb
```
**Timestamp muss AKTUELL sein (gerade eben)!**

#### Schritt 7: Server neu laden
**Im Spiel als Admin:**
```
Send o 0 RecreateAll
```

**Oder Server komplett neu starten**

#### Schritt 8: Testen
1. **Neuer Client verbindet** → Lädt automatisch neue mysticsw.rsc
2. **Im Spiel:** Schau dir das Mystic Sword an
3. **Die Beschreibung sollte geändert sein!**

---

## 🎨 SZENARIO 2: Grafik ändern (z.B. Schwert-Sprite)

### Was du brauchst:
- Eine .bgf Datei (Grafik)
- Bildbearbeitungsprogramm (optional)

### Schritt-für-Schritt:

#### Schritt 1: Finde die Grafik-Datei
```cmd
dir /s /b "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\*.bgf" | findstr /i sword
```

**Beispiel-Ergebnis:** `resource\mysticsw.bgf`

#### Schritt 2: Grafik ändern
**Option A: Mit BGF-Editor** (falls vorhanden)
**Option B: Ersetze mit neuer .bgf Datei**

```cmd
REM Backup erstellen
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\mysticsw.bgf" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\mysticsw.bgf.backup"

REM Neue Datei reinkopieren
REM (von wo auch immer du sie hast)
```

#### Schritt 3: Nach rsc\ kopieren
```cmd
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\mysticsw.bgf" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.bgf"
```

**Prüfung:**
```cmd
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.bgf"
```
**Timestamp muss AKTUELL sein!**

#### Schritt 4: Resource Database neu erstellen
```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"

"E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe" ^
  -o rsc0000.rsb ^
  *.rsc *.bgf *.ogg *.wav *.mp3
```

**Prüfung:**
```cmd
dir rsc0000.rsb
```
**Timestamp muss AKTUELL sein!**

#### Schritt 5: KEIN Server-Neustart nötig!
❗ **Für Grafik-Änderungen brauchst du KEINEN Server-Neustart!**

Clients laden beim nächsten Login automatisch die neue Grafik.

#### Schritt 6: Testen
1. **Neuer Client verbindet**
2. **Client lädt automatisch neue mysticsw.bgf**
3. **Im Spiel sollte das Schwert anders aussehen!**

---

## 🔊 SZENARIO 3: Sound ändern (z.B. Schwert-Schwing-Sound)

### Schritt-für-Schritt:

#### Schritt 1: Finde die Sound-Datei
```cmd
dir /s /b "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\*.ogg" | findstr /i sword
```

**Beispiel:** `resource\sounds\swordswing.ogg`

#### Schritt 2: Sound ersetzen
```cmd
REM Backup
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sounds\swordswing.ogg" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sounds\swordswing.ogg.backup"

REM Neue Datei reinkopieren (von deinem Audio-Editor)
```

#### Schritt 3: Nach rsc\ kopieren
```cmd
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\sounds\swordswing.ogg" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\swordswing.ogg"
```

#### Schritt 4: Resource Database neu erstellen
```cmd
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"

"E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe" ^
  -o rsc0000.rsb ^
  *.rsc *.bgf *.ogg *.wav *.mp3
```

#### Schritt 5: Testen
Clients laden beim nächsten Login automatisch den neuen Sound.

---

## 🚨 FEHLERBEHEBUNG

### Problem 1: "rscmerge.exe not found"

**Symptom:**
```cmd
Der Befehl "rscmerge.exe" ist entweder falsch geschrieben oder konnte nicht gefunden werden.
```

**Lösung:**
```cmd
REM Prüfe ob die Datei existiert
dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe"

REM Falls nicht: Projekt kompilieren
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop"
REM (Makefile ausführen)
```

### Problem 2: "rsc0000.rsb hat alten Timestamp"

**Symptom:**
```cmd
dir run\server\rsc\rsc0000.rsb
REM Zeigt altes Datum
```

**Lösung:**
```cmd
REM Database löschen und neu erstellen
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
del rsc0000.rsb

"E:\Meridian59_source\04.08.2016\Meridian59-develop\bin\rscmerge.exe" ^
  -o rsc0000.rsb ^
  *.rsc *.bgf *.ogg *.wav *.mp3

REM Prüfen
dir rsc0000.rsb
```

### Problem 3: "Client lädt keine Updates"

**Mögliche Ursachen:**

1. **rsc0000.rsb nicht aktualisiert**
   ```cmd
   cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
   dir rsc0000.rsb
   REM Timestamp prüfen!
   ```

2. **Datei nicht in rsc\ kopiert**
   ```cmd
   dir "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.rsc"
   REM Muss existieren!
   ```

3. **Server läuft nicht**
   ```cmd
   REM Prüfe ob Server läuft
   tasklist | findstr blakserv
   ```

### Problem 4: "Permission denied beim Kopieren"

**Lösung:**
```cmd
REM Windows: Als Administrator ausführen
REM Rechtsklick auf cmd.exe -> "Als Administrator ausführen"

REM Oder: Prüfe ob Datei schreibgeschützt ist
attrib "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.rsc"
```

---

## 📋 CHECKLISTE: Habe ich alles richtig gemacht?

Nach jedem Update:

- [ ] ✅ .kod Datei editiert und gespeichert
- [ ] ✅ Mit bc.exe kompiliert → .bof und .rsc erstellt
- [ ] ✅ .bof nach memmap\ kopiert
- [ ] ✅ .rsc nach rsc\ kopiert
- [ ] ✅ rscmerge.exe ausgeführt → rsc0000.rsb aktualisiert
- [ ] ✅ rsc0000.rsb Timestamp ist AKTUELL
- [ ] ✅ Server neu geladen (RecreateAll oder Neustart)
- [ ] ✅ Im Spiel getestet

---

## 🎯 QUICK REFERENCE

### Kompilieren & Verteilen (Copy-Paste):
```cmd
REM 1. KOMPILIEREN
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\item\weapon"
"..\..\..\bin\bc.exe" -d -I "..\..\..\kod\include" -K "..\..\..\kod\kodbase.txt" mysticsword.kod

REM 2. KOPIEREN (Server)
copy mysticsword.bof "..\..\..\run\server\memmap\mysticsw.bof"

REM 3. KOPIEREN (Client)
copy mysticsword.rsc "..\..\..\run\server\rsc\mysticsw.rsc"

REM 4. RESOURCE DATABASE
cd "..\..\..\run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav *.mp3

REM 5. PRÜFEN
dir rsc0000.rsb

REM 6. SERVER NEU LADEN
REM Im Spiel: Send o 0 RecreateAll
```

### Nur Grafik ändern (Copy-Paste):
```cmd
REM 1. KOPIEREN
copy "E:\Meridian59_source\04.08.2016\Meridian59-develop\resource\mysticsw.bgf" ^
     "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc\mysticsw.bgf"

REM 2. RESOURCE DATABASE
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\rsc"
"..\..\..\bin\rscmerge.exe" -o rsc0000.rsb *.rsc *.bgf *.ogg *.wav *.mp3

REM 3. FERTIG (kein Server-Neustart nötig)
```

---

## ⚙️ WICHTIGE DATEINAMEN-KONVENTIONEN

Meridian 59 nutzt **8.3 Dateinamen** (DOS-Kompatibilität):

| Original | In memmap\ / rsc\ |
|----------|-------------------|
| mysticsword.kod | mysticsw.bof |
| mysticsword.rsc | mysticsw.rsc |
| escapedconvict.kod | escapedc.bof |
| betapotion.kod | betap.bof |

**WICHTIG:** Achte auf die korrekten Kurznamen beim Kopieren!

---

## 📞 SUPPORT

Falls etwas nicht klappt:

1. **Prüfe die Logs:**
   ```cmd
   REM PowerShell:
   Get-Content -Tail 50 "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\channel\error.txt"

   REM Oder öffne mit Notepad:
   notepad "E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\channel\error.txt"
   ```

2. **Prüfe Timestamps:**
   ```cmd
   dir "run\server\rsc\rsc0000.rsb"
   dir "run\server\memmap\*.bof"
   ```

3. **Teste rscmerge manuell:**
   ```cmd
   cd run\server\rsc
   ..\..\..\bin\rscmerge.exe -o test.rsb mysticsw.rsc
   dir test.rsb
   ```

---

**Erstellt:** 28. Dezember 2025
**Getestet:** ✅ Ja
**Version:** 1.1 (Windows)
