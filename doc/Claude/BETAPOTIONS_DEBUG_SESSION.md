# BetaPotions Debug-Session - Dokumentation
**Datum:** 27. Dezember 2025
**Status:** FUNKTIONIERT IMMER NOCH NICHT ❌

---

## 🎯 ZIEL
BetaPotions sollen beim **ersten Einloggen** eines neuen Benutzers automatisch ins Inventar kommen (ohne durch das Newbie-Portal gehen zu müssen).

**Gewünschte Items beim ersten Login:**
- 1000 Shillings
- 2x Mystic Sword
- Scale Armor
- Simple Helm
- Gauntlet
- 20x Inky Cap
- Reagenzien für Spells
- 5x Apple
- 1x Mace
- **6x BetaPotions** (eine für jede Schule: Qor, Kraanan, Faren, Riija, Jala, Shalille)

---

## 🐛 GEFUNDENE UND BEHOBENE BUGS

### Bug 1: Falsche Position des Codes
**Problem:** Code war in `UserLogonHook()`, wurde aber NACH `piLastLoginTime = GetTime()` aufgerufen
**Location:** `user.kod:3000`
**Fix:** Code nach `FirstLogon()` verschoben (Zeile 772-793)

### Bug 2: Falsche Blakod-Syntax
**Problem:** `bFirstLogin = (piLastLoginTime = 0)` ist ungültige Syntax
**Fix:** Korrekte if/else Syntax verwendet

### Bug 3: Case-Sensitivity in lore.kod
**Problem:** `pbEnableBetaPotions` (Großes B) vs `pbEnablebetaPotions` (kleines b)
**Location:** `lore.kod:79` vs `lore.kod:700`
**Fix:** Variable in Zeile 79 korrigiert zu `pbEnablebetaPotions = TRUE`

### Bug 4: Case-Sensitivity in FirstLogon()
**Problem:** `piLast_Restart_Time` (Großes R, T) vs `piLast_restart_time` (kleines r, t)
**Location:** `user.kod:772`
**Fix:** Variable korrigiert zu `piLast_restart_time`

---

## 📝 AKTUELLE IMPLEMENTIERUNG

### Datei: `user.kod` - FirstLogon() (Zeile 768-798)
```kod
FirstLogon()
"Sets the last login time and sends an appeal when a new character is created."
{
   if piLastLoginTime = 0 AND piLast_restart_time = 0
   {
      Debug("Sending first time appeal");
      Send(self,@SendFirstTimeAppeal);

      // Give beta equipment to new players on first login
      Send(self,@AddRealWorldObjects);

      // Also give BetaPotions for each school
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_QOR,
           #level=6,#giveHealth=TRUE,#healthCap=150));
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_KRAANAN,
           #level=6,#giveHealth=FALSE));
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_FAREN,
           #level=6,#giveHealth=FALSE));
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_RIIJA,
           #level=6,#giveHealth=FALSE));
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_JALA,
           #level=6,#giveHealth=FALSE));
      Send(self,@NewHold,#what=Create(&BetaPotion,#health=150,#school=SS_SHALILLE,
           #level=6,#giveHealth=FALSE));
   }

   piLastLoginTime = GetTime();

   return;
}
```

### Datei: `player.kod` - AddRealWorldObjects() (Zeile 2954-2986)
```kod
AddRealWorldObjects()
{
   local lItems,oItem,iSchool;

   // If player hasn't suicided in last 5 hours, give them cash and fancy clothes!
   if (GetTime() - piLast_restart_time) > (5*60*60)
   {
      Send(self,@NewHold,#what=Create(&Money,#number=1000));
      Send(Send(SYS,@GetStatistics),@MoneyCreated,#amount=1000);
      Send(self,@NewHold,#what=create(&InkyCap,#number=5));
      Send(self,@NewHold,#what=Create(&PantsC));
      Send(self,@NewHold,#what=Create(&Shirt,#color=XLAT_TO_GRAY));
   }

   if Send(Send(SYS,@GetLore),@BetaPotionsEnabled)
      AND Send(Send(SYS,@GetParliament),@BetaPotionsEnabled)
   {
      Send(self,@NewHold,#what=create(&MysticSword));
      Send(self,@NewHold,#what=create(&MysticSword));
      Send(self,@NewHold,#what=create(&InkyCap,#number=20));
      Send(self,@NewHold,#what=create(&SimpleHelm));
      Send(self,@NewHold,#what=create(&ScaleArmor));
      Send(self,@NewHold,#what=create(&Gauntlet));
      Send(self,@AddReagentsForSpells,#iNumCasts=20);
   }

   Send(self,@NewHold,#what=Create(&Mace));
   Send(self,@NewHold,#what=Create(&Apple,#number=5));

   return;
}
```

---

## 🔧 GEÄNDERTE DATEIEN

### Source-Dateien (kod/):
1. ✅ `kod/util/lore.kod` (Zeile 79)
2. ✅ `kod/util/parlia.kod` (Zeile 112 - war schon korrekt)
3. ✅ `kod/object/active/holder/nomoveon/battler/player/user.kod` (Zeile 772-793)

### Runtime-Dateien (run/server/kod/):
1. ✅ `run/server/kod/util/lore.kod` (Zeile 79)
2. ✅ `run/server/kod/util/parlia.kod` (Zeile 112 - war schon korrekt)
3. ✅ `run/server/kod/object/active/holder/nomoveon/battler/player/user.kod` (Zeile 772-793)

### Kompilierte Dateien (.bof):
1. ✅ `run/server/kod/util/lore.bof` - kompiliert 27.12.2025 21:27
2. ✅ `run/server/kod/util/parlia.bof` - kompiliert früher
3. ✅ `run/server/kod/object/active/holder/nomoveon/battler/player/user.bof` - kompiliert 27.12.2025 ~21:35

---

## 🔍 WICHTIGE EINSTELLUNGEN

### lore.kod (Zeile 79):
```kod
pbEnablebetaPotions = TRUE  // WICHTIG: kleines "b" in "beta"!
```

### lore.kod - BetaPotionsEnabled() (Zeile 698-701):
```kod
BetaPotionsEnabled()
{
   return pbEnablebetaPotions;  // Gibt den Wert zurück
}
```

### parlia.kod (Zeile 112):
```kod
pbHiddenSwitch = TRUE
```

### parlia.kod - BetaPotionsEnabled() (Zeile 1471-1474):
```kod
BetaPotionsEnabled()
{
   return pbHiddenSwitch;
}
```

---

## 🧪 TESTVERFAHREN

1. **Server neu starten** (sehr wichtig!)
2. **Komplett neuen Account erstellen** (neuer Username!)
3. **Neuen Charakter erstellen**
4. **Einloggen**
5. **Inventar überprüfen**

### Server-Logs beim Test:
```
Dec 27 2025 21:29:13|[user.bof (771)] Sending first time appeal
```
Diese Meldung zeigt, dass FirstLogon() aufgerufen wurde.

---

## ❌ PROBLEM: FUNKTIONIERT IMMER NOCH NICHT

### Was der Benutzer berichtet:
- **Beim ersten Test:** Nur 1000 Shillings + 1x Mace
- **Beim zweiten Test:** GAR NICHTS
- **Nach allen Fixes:** IMMER NOCH NICHTS

### Mögliche Ursachen (noch zu untersuchen):

#### 1. FirstLogon() wird möglicherweise nicht aufgerufen
- Überprüfen: Wird FirstLogon() überhaupt ausgeführt?
- Server-Logs zeigen "Sending first time appeal" → FirstLogon() läuft!

#### 2. Die Bedingung `if piLastLoginTime = 0 AND piLast_restart_time = 0` ist FALSE
**Wichtig zu prüfen:**
- Ist `piLast_restart_time` wirklich 0 bei einem neuen Charakter?
- Oder wird es irgendwo vorher gesetzt?

#### 3. AddRealWorldObjects() Bedingungen schlagen fehl
**Zwei if-Blöcke in AddRealWorldObjects():**

**Block 1:** (Zeile 2961-2968)
```kod
if (GetTime() - piLast_restart_time) > (5*60*60)
```
- Gibt: 1000 Shillings, InkyCap, Pants, Shirt
- User bekam im ersten Test 1000 Shillings → **Diese Bedingung war TRUE**

**Block 2:** (Zeile 2970-2980)
```kod
if Send(Send(SYS,@GetLore),@BetaPotionsEnabled)
   AND Send(Send(SYS,@GetParliament),@BetaPotionsEnabled)
```
- Gibt: Beta-Equipment (Mystic Swords, Armor, etc.)
- User bekam NICHTS davon → **Diese Bedingung ist FALSE!**

#### 4. BetaPotion-Objekte können nicht erstellt werden
- Fehlt die BetaPotion-Klasse?
- Datei: `kod/object/item/passitem/betap.kod` existiert ✅
- betap.bof existiert in run/server? → **MUSS GEPRÜFT WERDEN**

#### 5. Inventar/NewHold Problem
- Kann der Spieler überhaupt Items aufnehmen zu diesem Zeitpunkt?
- Ist das Inventar initialisiert wenn FirstLogon() aufgerufen wird?

---

## 🔬 NÄCHSTE DEBUG-SCHRITTE

### Schritt 1: Überprüfen ob BetaPotionsEnabled() TRUE zurückgibt
**Server-Konsole Befehle:**
```
send object 0 GetLore
show object <LORE_ID>
# Suchen nach: pbEnablebetaPotions = INT 1

send object 0 GetParliament
show object <PARLIAMENT_ID>
# Suchen nach: pbHiddenSwitch = INT 1
```

### Schritt 2: Debug-Nachrichten hinzufügen
In `FirstLogon()` nach der Bedingung:
```kod
if piLastLoginTime = 0 AND piLast_restart_time = 0
{
   Debug("DEBUG: FirstLogon conditions met!");
   Debug("DEBUG: Calling AddRealWorldObjects");
   Send(self,@AddRealWorldObjects);
   Debug("DEBUG: AddRealWorldObjects returned");

   Debug("DEBUG: Creating BetaPotions");
   Send(self,@NewHold,#what=Create(&BetaPotion,...));
   Debug("DEBUG: BetaPotions created");
}
```

### Schritt 3: AddRealWorldObjects() debuggen
Füge Debug-Ausgaben in `player.kod` hinzu:
```kod
AddRealWorldObjects()
{
   Debug("DEBUG: AddRealWorldObjects called");

   if (GetTime() - piLast_restart_time) > (5*60*60)
   {
      Debug("DEBUG: Suicide check passed");
      // Items...
   }

   if Send(Send(SYS,@GetLore),@BetaPotionsEnabled)
      AND Send(Send(SYS,@GetParliament),@BetaPotionsEnabled)
   {
      Debug("DEBUG: BetaPotions system enabled!");
      // Items...
   }
   else
   {
      Debug("DEBUG: BetaPotions system DISABLED!");
   }
}
```

### Schritt 4: Überprüfen ob betap.bof existiert
```bash
ls -la E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/kod/object/item/passitem/betap.bof
```

### Schritt 5: piLast_restart_time Wert prüfen
Füge in FirstLogon() hinzu:
```kod
Debug("DEBUG: piLastLoginTime = %i", piLastLoginTime);
Debug("DEBUG: piLast_restart_time = %i", piLast_restart_time);
```

---

## 📋 KOMPILIERUNGS-BEFEHLE

### Lore kompilieren:
```bash
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\util"
"../../bin/bc.exe" -d -I ../include -K ../kodbase.txt lore.kod
cp lore.bof "../../run/server/kod/util/"
```

### Parliament kompilieren:
```bash
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\util"
"../../bin/bc.exe" -d -I ../include -K ../kodbase.txt parlia.kod
cp parlia.bof "../../run/server/kod/util/"
```

### User kompilieren:
```bash
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\active\holder\nomoveon\battler\player"
"E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/include" -K "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/kodbase.txt" user.kod
cp user.bof "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/kod/object/active/holder/nomoveon/battler/player/"
```

### Player kompilieren (falls nötig):
```bash
cd "E:\Meridian59_source\04.08.2016\Meridian59-develop\kod\object\active\holder\nomoveon\battler"
"E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/include" -K "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/kodbase.txt" player.kod
cp player.bof "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/kod/object/active/holder/nomoveon/battler/"
```

---

## 🔑 WICHTIGE ERKENNTNISSE

1. **Case-Sensitivity ist KRITISCH** in Blakod
   - `pbEnableBetaPotions` ≠ `pbEnablebetaPotions`
   - `piLast_Restart_Time` ≠ `piLast_restart_time`

2. **Aufruf-Reihenfolge:**
   - `system.kod` → `FirstLogon()` → `UserLogon()`
   - FirstLogon() wird VOR UserLogon() aufgerufen
   - piLastLoginTime wird IN FirstLogon() gesetzt

3. **Server-Logs sind hilfreich:**
   - "Sending first time appeal" bestätigt FirstLogon() Ausführung

4. **AddRealWorldObjects() hat ZWEI Bedingungen:**
   - Suicide-Check (funktioniert - User bekam 1000 Shillings)
   - BetaPotions-Check (funktioniert NICHT - User bekam keine Beta-Items)

---

## 🎬 ZUSAMMENFASSUNG

**Was funktioniert:**
- ✅ Code wird an der richtigen Stelle ausgeführt (FirstLogon)
- ✅ Alle Case-Sensitivity Fehler behoben
- ✅ Kompilierung erfolgreich
- ✅ FirstLogon() wird aufgerufen (Server-Log bestätigt)
- ✅ Erster if-Block in AddRealWorldObjects() funktioniert

**Was NICHT funktioniert:**
- ❌ BetaPotions System scheint deaktiviert zu sein
- ❌ Zweiter if-Block in AddRealWorldObjects() wird nicht ausgeführt
- ❌ User bekommt keine Items

**Hauptverdacht:**
Die Bedingung `Send(Send(SYS,@GetLore),@BetaPotionsEnabled) AND Send(Send(SYS,@GetParliament),@BetaPotionsEnabled)` gibt FALSE zurück, obwohl wir beide auf TRUE gesetzt haben.

**Nächster Schritt morgen:**
Debug-Ausgaben hinzufügen um herauszufinden, warum BetaPotionsEnabled() FALSE zurückgibt!

---

**Ende der Session: 27. Dezember 2025, ca. 21:40 Uhr**
