// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbmonster.c
 *
 * MySQL Monster Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_MONSTERTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `monster_templates` (" \
   "  `monster_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `monster_name` VARCHAR(100) NOT NULL," \
   "  `monster_kod_class` VARCHAR(100) NOT NULL," \
   "  `monster_icon` VARCHAR(100)," \
   "  `monster_dead_icon` VARCHAR(100)," \
   "  `level` INT DEFAULT 25," \
   "  `difficulty` INT DEFAULT 0," \
   "  `max_hit_points` INT DEFAULT 1," \
   "  `max_mana` INT DEFAULT 0," \
   "  `attack_type` INT DEFAULT 0," \
   "  `attack_spell` INT DEFAULT 0," \
   "  `attack_range` INT DEFAULT 128," \
   "  `speed` INT DEFAULT 0," \
   "  `vision_distance` INT DEFAULT 10," \
   "  `cash_min` INT DEFAULT 1," \
   "  `cash_max` INT DEFAULT 10," \
   "  `treasure_type` INT DEFAULT 0," \
   "  `default_behavior` INT DEFAULT 0," \
   "  `wimpy` INT DEFAULT 0," \
   "  `karma` INT DEFAULT 0," \
   "  `is_undead` TINYINT(1) DEFAULT 0," \
   "  `is_npc` TINYINT(1) DEFAULT 0," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
   "  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`monster_template_id`)," \
   "  UNIQUE KEY `uk_monster_kod_class` (`monster_kod_class`)," \
   "  KEY `idx_monster_level` (`level`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

#define SQLQUERY_CREATETABLE_MONSTERRESISTANCES \
   "CREATE TABLE IF NOT EXISTS `monster_resistances` (" \
   "  `resistance_id` INT NOT NULL AUTO_INCREMENT," \
   "  `monster_template_id` INT NOT NULL," \
   "  `attack_type` INT NOT NULL," \
   "  `resistance_value` INT NOT NULL DEFAULT 0," \
   "  PRIMARY KEY (`resistance_id`)," \
   "  UNIQUE KEY `uk_monster_attack_type` (`monster_template_id`, `attack_type`)," \
   "  CONSTRAINT `fk_resistance_monster` FOREIGN KEY (`monster_template_id`)" \
   "    REFERENCES `monster_templates` (`monster_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

#define SQLQUERY_CREATETABLE_MONSTERSPELLS \
   "CREATE TABLE IF NOT EXISTS `monster_spells` (" \
   "  `spell_id` INT NOT NULL AUTO_INCREMENT," \
   "  `monster_template_id` INT NOT NULL," \
   "  `spell_num` INT NOT NULL," \
   "  PRIMARY KEY (`spell_id`)," \
   "  KEY `idx_monster_spell` (`monster_template_id`)," \
   "  CONSTRAINT `fk_spell_monster` FOREIGN KEY (`monster_template_id`)" \
   "    REFERENCES `monster_templates` (`monster_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_monster_count = 0;

#define MAX_INHERITANCE_DEPTH 50
#define AI_NPC 0x00000200

static Bool MonsterClassInheritsFrom(class_node *c, const char *parent_name)
{
   class_node *current = c;
   int depth = 0;

   while (current != NULL && depth < MAX_INHERITANCE_DEPTH)
   {
      if (current->class_name && _stricmp(current->class_name, parent_name) == 0)
         return True;
      current = current->super_ptr;
      depth++;
   }
   return False;
}

static int MonsterGetClassVarInt(class_node *c, const char *var_name)
{
   class_node *current = c;
   int depth = 0;

   while (current != NULL && depth < MAX_INHERITANCE_DEPTH)
   {
      classvar_name_node *cv = current->classvar_names;
      int var_id = INVALID_CLASSVAR;

      while (cv != NULL)
      {
         if (_stricmp(cv->name, var_name) == 0)
         {
            var_id = cv->id;
            break;
         }
         cv = cv->next;
      }

      if (var_id != INVALID_CLASSVAR && var_id >= 0 && current->vars != NULL && var_id < current->num_vars)
      {
         val_type val = current->vars[var_id].val;
         if (val.v.tag == TAG_INT)
            return val.v.data;
         if (val.v.tag == TAG_RESOURCE)
            return val.v.data;
         return 0;
      }
      current = current->super_ptr;
      depth++;
   }
   return 0;
}

static const char* MonsterGetClassVarResourceString(class_node *c, const char *var_name)
{
   int res_id = MonsterGetClassVarInt(c, var_name);
   if (res_id > 0)
   {
      resource_node *r = GetResourceByID(res_id);
      if (r && r->resource_val[0])
         return r->resource_val[0];
   }
   return NULL;
}

static void EscapeStringMonster(char *dest, const char *src, size_t dest_size)
{
   size_t i, j;
   if (!dest || dest_size < 1) return;
   dest[0] = '\0';
   if (!src || dest_size < 3) return;

   for (i = 0, j = 0; src[i] && j < dest_size - 2; i++)
   {
      if (src[i] == '\'' || src[i] == '\\')
      {
         if (j < dest_size - 2) dest[j++] = '\\';
      }
      if (j < dest_size - 1) dest[j++] = src[i];
   }
   dest[j] = '\0';
}

static void ExportMonsterClassToMySQL(class_node *c)
{
   char query[4096];
   const char *monster_name_raw, *monster_icon_raw, *monster_dead_icon_raw, *monster_class;
   char esc_name[256], esc_class[256], esc_icon[256], esc_dead_icon[256];
   int level, difficulty, max_hp, max_mana, attack_type, attack_spell;
   int attack_range, speed, vision_dist, cash_min, cash_max;
   int treasure_type, default_behavior, wimpy, karma, is_undead, is_npc;

   if (!c || !c->class_name) return;
   if (!mysql) return;

   // Rate limiting - pause EVERY monster to prevent overwhelming MySQL
   Sleep(20);  // 20ms pause per monster

   // Check MySQL connection and reconnect if needed
   if (mysql_ping(mysql) != 0)
   {
      eprintf("ExportMonsterClassToMySQL: MySQL connection lost, attempting reconnect...\n");
      Sleep(1000);  // Wait 1 second before retry
      if (mysql_ping(mysql) != 0)
      {
         eprintf("ExportMonsterClassToMySQL: Reconnect failed, skipping monster\n");
         return;
      }
      eprintf("ExportMonsterClassToMySQL: Reconnected successfully\n");
   }

   // Additional pause every 50 monsters
   if (export_monster_count > 0 && (export_monster_count % 50) == 0)
   {
      lprintf("ExportMonsterClassToMySQL: Progress - %d monsters exported, pausing...\n", export_monster_count);
      Sleep(500);  // 500ms pause every 50 monsters
   }

   monster_class = c->class_name;

   // Get monster name - skip if no valid name
   monster_name_raw = MonsterGetClassVarResourceString(c, "vrName");
   if (!monster_name_raw) return;
   if (strlen(monster_name_raw) == 0) return;
   if (_stricmp(monster_name_raw, "monster") == 0) return;
   if (_stricmp(monster_name_raw, "something") == 0) return;

   // Get icons (optional)
   monster_icon_raw = MonsterGetClassVarResourceString(c, "vrIcon");
   if (!monster_icon_raw) monster_icon_raw = "";

   monster_dead_icon_raw = MonsterGetClassVarResourceString(c, "vrDead_icon");
   if (!monster_dead_icon_raw) monster_dead_icon_raw = "";

   // Get integer values
   level = MonsterGetClassVarInt(c, "viLevel");
   difficulty = MonsterGetClassVarInt(c, "viDifficulty");
   max_hp = MonsterGetClassVarInt(c, "piMax_hit_points");
   max_mana = MonsterGetClassVarInt(c, "piMax_mana");
   attack_type = MonsterGetClassVarInt(c, "viAttack_type");
   attack_spell = MonsterGetClassVarInt(c, "viAttack_spell");
   attack_range = MonsterGetClassVarInt(c, "viAttackRange");
   speed = MonsterGetClassVarInt(c, "viSpeed");
   vision_dist = MonsterGetClassVarInt(c, "viVisionDistance");
   cash_min = MonsterGetClassVarInt(c, "viCashmin");
   cash_max = MonsterGetClassVarInt(c, "viCashmax");
   treasure_type = MonsterGetClassVarInt(c, "viTreasure_type");
   default_behavior = MonsterGetClassVarInt(c, "viDefault_behavior");
   wimpy = MonsterGetClassVarInt(c, "viWimpy");
   karma = MonsterGetClassVarInt(c, "viKarma");
   is_undead = MonsterGetClassVarInt(c, "vbIsUndead");
   is_npc = (default_behavior & AI_NPC) ? 1 : 0;

   // Default values
   if (level == 0) level = 1;
   if (attack_range == 0) attack_range = 128;
   if (vision_dist == 0) vision_dist = 10;

   // Escape strings
   EscapeStringMonster(esc_name, monster_name_raw, sizeof(esc_name));
   EscapeStringMonster(esc_class, monster_class, sizeof(esc_class));
   EscapeStringMonster(esc_icon, monster_icon_raw, sizeof(esc_icon));
   EscapeStringMonster(esc_dead_icon, monster_dead_icon_raw, sizeof(esc_dead_icon));

   // Build and execute query
   snprintf(query, sizeof(query),
      "INSERT INTO monster_templates (monster_name, monster_kod_class, monster_icon, "
      "monster_dead_icon, level, difficulty, max_hit_points, max_mana, attack_type, "
      "attack_spell, attack_range, speed, vision_distance, cash_min, cash_max, "
      "treasure_type, default_behavior, wimpy, karma, is_undead, is_npc) "
      "VALUES ('%s', '%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d) "
      "ON DUPLICATE KEY UPDATE monster_name='%s', monster_icon='%s', monster_dead_icon='%s', "
      "level=%d, difficulty=%d, max_hit_points=%d, max_mana=%d, attack_type=%d, "
      "attack_spell=%d, attack_range=%d, speed=%d, vision_distance=%d, cash_min=%d, cash_max=%d, "
      "treasure_type=%d, default_behavior=%d, wimpy=%d, karma=%d, is_undead=%d, is_npc=%d",
      esc_name, esc_class, esc_icon, esc_dead_icon,
      level, difficulty, max_hp, max_mana, attack_type, attack_spell, attack_range,
      speed, vision_dist, cash_min, cash_max, treasure_type, default_behavior, wimpy, karma, is_undead, is_npc,
      esc_name, esc_icon, esc_dead_icon,
      level, difficulty, max_hp, max_mana, attack_type, attack_spell, attack_range,
      speed, vision_dist, cash_min, cash_max, treasure_type, default_behavior, wimpy, karma, is_undead, is_npc);

   MySQLLock();
   if (mysql_query(mysql, query))
   {
      eprintf("ExportMonsterClassToMySQL: Failed %s: %s\n", monster_class, mysql_error(mysql));
      MySQLUnlock();
      return;
   }
   MySQLUnlock();

   export_monster_count++;
}

static void ExportMonsterClassCallback(class_node *c)
{
   if (!c || !c->class_name) return;
   if (!MonsterClassInheritsFrom(c, "Monster")) return;

   // Skip abstract base classes
   if (_stricmp(c->class_name, "Monster") == 0) return;
   if (_stricmp(c->class_name, "Battler") == 0) return;
   if (_stricmp(c->class_name, "Holder") == 0) return;
   if (_stricmp(c->class_name, "NoMoveOn") == 0) return;
   if (_stricmp(c->class_name, "ActiveObject") == 0) return;

   ExportMonsterClassToMySQL(c);
}

void _MySQLCreateMonsterSchema(void)
{
   if (!mysql) { eprintf("_MySQLCreateMonsterSchema: MySQL not connected\n"); return; }

   MySQLLock();

   lprintf("Creating Monster schema...\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_MONSTERTEMPLATES))
      eprintf("_MySQLCreateMonsterSchema: Failed monster_templates: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateMonsterSchema: monster_templates table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_MONSTERRESISTANCES))
      eprintf("_MySQLCreateMonsterSchema: Failed monster_resistances: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateMonsterSchema: monster_resistances table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_MONSTERSPELLS))
      eprintf("_MySQLCreateMonsterSchema: Failed monster_spells: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateMonsterSchema: monster_spells table ready\n");

   lprintf("Monster schema creation complete.\n");

   MySQLUnlock();
}

int MySQLExportMonsterData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportMonsterData: MySQL not ready\n");
      return 0;
   }

   export_monster_count = 0;
   lprintf("MySQLExportMonsterData: Starting Monster export...\n");

   ForEachClass(ExportMonsterClassCallback);

   lprintf("MySQLExportMonsterData: Exported %d Monsters\n", export_monster_count);
   return export_monster_count;
}
