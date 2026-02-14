# QBScripts - Build & Deployment Automation für Meridian 59 Server 113/114

Dieses Verzeichnis enthält Build- und Deployment-Tools für Meridian 59 Server.

## Dateien

### PowerShell Scripts

#### **build-113.ps1** - Haupt-Build-Script
Vollautomatisches Build & Deployment System für Server 113 (Production) und 114 (Test).

**Verwendung:**
```powershell
# Server bauen
build-113.ps1 -Action build -Server production

# Server-Dateien kopieren
build-113.ps1 -Action copyserver -Server production

# Server stoppen
build-113.ps1 -Action killserver -Server production

# Server starten
build-113.ps1 -Action startserver -Server production

# Code auschecken (benötigt Git)
build-113.ps1 -Action checkout -Branch master

# Client packen (benötigt PatchListGenerator.exe)
build-113.ps1 -Action package -Server production
```

**Wichtige Parameter:**
- `-Action`: build, clean, package, checkout, killserver, startserver, copyserver
- `-Server`: production (113) oder test (114)
- `-Branch`: develop, master, release (nur für checkout)
- `-Hotfix`: true für Hotfix-Modus (nur .bof reload, kein Neustart)

**Konfiguration:**
- Zeile 11: RootPath (aktuell: C:\Users\Rod\Desktop\2\Server-104-main)
- Zeilen 19-28: Output Paths für Server 113/114
- Zeilen 31-33: Server IPs

### Perl Scripts

Alle Perl-Scripts sind **optional** und hauptsächlich für GitHub-Integration.

#### **autopatch.pl** - GitHub Pull Request Automation
Automatisiert das Mergen von GitHub Pull Requests.

**Voraussetzungen:**
```bash
cpan install LWP::Simple Pithub Git::Repository Data::Dumper
set TOKEN=dein_github_token
```

**Verwendung:**
```bash
perl autopatch.pl --change=123 --build=113 --branch=master --buildid=latest
```

**Konfiguration:**
- Zeilen 56-58: GitHub User/Repo ändern
- Zeilen 157-158, 178-180: Gleiche Änderungen

#### **automerge.pl** - Einfacher Branch Merge
Erstellt Build-Branch und merged Changelists.

**Verwendung:**
```bash
perl automerge.pl --build=113 --change=feature1 --change=feature2
```

#### **MergeWith.pl** - Branch Merge Helper
Merged einen Branch in einen anderen.

**Verwendung:**
```bash
perl MergeWith.pl --source=feature-113 --destination=master
```

#### **IsBranchMergedWith.pl** - Merge Check
Prüft ob ein Branch bereits gemerged wurde.

**Verwendung:**
```bash
perl IsBranchMergedWith.pl --source=feature-113 --destination=master
```

#### **getmilestones.pl** - GitHub Milestones
Listet GitHub Milestones.

**Verwendung:**
```bash
perl getmilestones.pl
```

**Konfiguration:**
- Zeilen 29-30: GitHub User/Repo ändern

## Quick Start

### Nur PowerShell (Empfohlen für Anfang)

1. **Server bauen:**
   ```powershell
   cd "C:\Users\Rod\Desktop\2\Server-104-main\qbscripts"
   .\build-113.ps1 -Action build -Server production
   ```

2. **Server deployen:**
   ```powershell
   .\build-113.ps1 -Action copyserver -Server production
   ```

3. **Server neu starten:**
   ```powershell
   .\build-113.ps1 -Action killserver -Server production
   .\build-113.ps1 -Action startserver -Server production
   ```

### Mit Perl Scripts (Für GitHub Integration)

1. **Perl Module installieren:**
   ```bash
   cpan install LWP::Simple Pithub Git::Repository
   ```

2. **GitHub Token setzen:**
   ```bash
   set TOKEN=dein_github_personal_access_token
   ```

3. **Perl Scripts konfigurieren:**
   - In allen .pl Dateien: Ändere 'OpenMeridian105' und 'Meridian59' zu deinem User/Repo

4. **Pull Request mergen:**
   ```bash
   perl autopatch.pl --change=123 --build=113 --branch=master
   ```

## Optionale Tools

- **psexec.exe**: Für Remote-Server-Start (nur wenn Server auf anderem PC läuft)
  - Download: https://technet.microsoft.com/en-us/sysinternals/psexec.aspx

- **PatchListGenerator.exe**: Für patchinfo.txt Generierung (Client-Updates)
  - Wird nur für `-Action package` benötigt

## Workflow Beispiel

### Standard Build & Deploy:
```powershell
# 1. Bauen
.\build-113.ps1 -Action build -Server production

# 2. Server stoppen
.\build-113.ps1 -Action killserver -Server production

# 3. Dateien kopieren
.\build-113.ps1 -Action copyserver -Server production

# 4. Server starten
.\build-113.ps1 -Action startserver -Server production
```

### Hotfix (nur Code reload, kein Neustart):
```powershell
# 1. Bauen
.\build-113.ps1 -Action build -Server production -Hotfix true

# 2. Dateien kopieren (nur .bof)
.\build-113.ps1 -Action copyserver -Server production -Hotfix true

# 3. Server reload (kein Neustart!)
.\build-113.ps1 -Action startserver -Server production -Hotfix true
```

## Troubleshooting

### "nmake not found"
```powershell
# Visual Studio Command Prompt öffnen oder:
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
```

### "Cannot connect to server at 127.0.0.1:9999"
- Server läuft nicht
- Telnet-Port 9999 ist nicht offen
- Firewall blockiert

### Perl Module fehlen
```bash
cpan install LWP::Simple Pithub Git::Repository Data::Dumper
```

## Anpassungen für eigene Umgebung

1. **build-113.ps1 öffnen**
2. **Pfade anpassen** (Zeilen 11, 19-28, 31-33)
3. **Perl Scripts anpassen** (GitHub User/Repo ändern)
4. **Testen mit -WhatIf**:
   ```powershell
   .\build-113.ps1 -Action build -Server production -WhatIf
   ```

## Support

Bei Fragen oder Problemen:
1. Zuerst `-WhatIf` Parameter verwenden um zu sehen was passieren würde
2. Log-Ausgaben prüfen
3. Pfade in build-113.ps1 kontrollieren
