# BetaPotion Auto-Distribution Implementation

## Status: ✅ ERFOLGREICH IMPLEMENTIERT (28.12.2025)

Automatische Vergabe von BetaPotions (6 Stück, eine pro Schule) beim ersten Login funktioniert!

---

## Die Lösung: UserLogonHook statt FirstLogon

### Warum UserLogonHook()?
- `FirstLogon()` wird während Charaktererstellung aufgerufen → Spieler noch nicht vollständig initialisiert
- `UserLogonHook()` wird NACH vollständigem Login aufgerufen → Spieler in Raum platziert und bereit
- NewHold funktioniert nur in UserLogonHook() korrekt

---

## Modifizierte Dateien

### 1. user.kod - UserLogonHook() (Zeile 3008-3074)
**Dies ist die HAUPTÄNDERUNG für BetaPotions!**

```blakod
UserLogonHook()
{
   local i, oPotion;

   // Give beta potions on first login only
   if piLast_restart_time = 0
   {
      Debug("UserLogonHook: First login detected, creating BetaPotions");

      // Create 6 beta potions, one for each school
      oPotion = Create(&BetaPotion,#school=SS_QOR,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Qor potion created"); Send(self,@Newhold,#what=oPotion); }

      oPotion = Create(&BetaPotion,#school=SS_KRAANAN,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Kraanan potion created"); Send(self,@Newhold,#what=oPotion); }

      oPotion = Create(&BetaPotion,#school=SS_FAREN,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Faren potion created"); Send(self,@Newhold,#what=oPotion); }

      oPotion = Create(&BetaPotion,#school=SS_RIIJA,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Riija potion created"); Send(self,@Newhold,#what=oPotion); }

      oPotion = Create(&BetaPotion,#school=SS_JALA,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Jala potion created"); Send(self,@Newhold,#what=oPotion); }

      oPotion = Create(&BetaPotion,#school=SS_SHALILLE,#level=6,#health=150,#giveHealth=TRUE,#healthCap=150);
      if oPotion <> $ { Debug("UserLogonHook: Shalille potion created"); Send(self,@Newhold,#what=oPotion); }

      Debug("UserLogonHook: All BetaPotions processed");
   }

   foreach i in Send(SYS,@GetBackgroundObjects)
   {
      Send(i,@AddBackgroundObject,#who=self);
   }

   Send(self,@LoadMailNews);
   piLastMoveUpdateTime = GetTickCount();

   propagate;
}
```

### 2. user.kod - FirstLogon() (Zeile 768-797)
**Beta-Equipment wird hier erstellt (nicht BetaPotions!):**

```blakod
FirstLogon()
{
   if piLastLoginTime = 0 AND piLast_restart_time = 0
   {
      Send(self,@SendFirstTimeAppeal);
      Send(self,@Newhold,#what=Create(&MysticSword));
      Send(self,@Newhold,#what=Create(&MysticSword));
      Send(self,@Newhold,#what=Create(&ScaleArmor));
      Send(self,@Newhold,#what=Create(&SimpleHelm));
      Send(self,@Newhold,#what=Create(&Gauntlet));
   }
   piLastLoginTime = GetTime();
   return;
}
```

### 3. lore.kod (Zeile 79)
```blakod
pbEnablebetaPotions = TRUE  // WICHTIG: kleines 'b'!
```

---

## Kompilierung & Installation

```bash
# 1. Kompiliere user.kod
cd "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player"
"E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "../../../../../../kod/include" -K "../../../../../../kod/kodbase.txt" user.kod

# 2. Kompiliere Subklassen
cd user
for f in dm.kod guest.kod escapedconvict.kod; do "E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "../../../../../../kod/include" -K "../../../../../../kod/kodbase.txt" "$f"; done
cd dm
"E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "../../../../../../kod/include" -K "../../../../../../kod/kodbase.txt" admin.kod
cd admin
"E:/Meridian59_source/04.08.2016/Meridian59-develop/bin/bc.exe" -d -I "../../../../../../kod/include" -K "../../../../../../kod/kodbase.txt" creator.kod

# 3. Kopiere nach memmap/
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user/dm.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user/guest.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user/escapedconvict.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user/dm/admin.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"
cp "E:/Meridian59_source/04.08.2016/Meridian59-develop/kod/object/active/holder/nomoveon/battler/player/user/dm/admin/creator.bof" "E:/Meridian59_source/04.08.2016/Meridian59-develop/run/server/memmap/"

# 4. Server neu laden
# Im Spiel: Send o 0 RecreateAll
```

---

## Was man erhält

**Beim ersten Login:**
- 2x Mystic Sword
- 1x Scale Armor
- 1x Simple Helm
- 1x Gauntlet
- 1x Mace
- 1000 Shillings
- 6x BetaPotions (Qor, Kraanan, Faren, Riija, Jala, Shalille)
  - Jede gibt +150 HP (max 150 HP Cap)
  - Jede gibt alle Zauber bis Level 6 in ihrer Schule

---

## Fehlersuche

1. **BetaPotions erscheinen nicht** → Debug-Log prüfen: `run/server/channel/debug.txt` (nach Server-Stop)
2. **Compile-Fehler** → Case-Sensitivity: `piLast_restart_time` (nicht mit großem R/T)
3. **Items nicht im Inventar** → memmap/ Dateien aktuell? Timestamp prüfen!

---

**Letzte Änderung:** 28.12.2025 17:15
**Status:** ✅ GETESTET UND FUNKTIONIERT
