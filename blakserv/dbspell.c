// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbspell.c
 *
 * MySQL Spell Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_SPELLTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `spell_templates` (" \
   "  `spell_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `spell_name` VARCHAR(100) NOT NULL," \
   "  `spell_kod_class` VARCHAR(100) NOT NULL," \
   "  `spell_icon` VARCHAR(100)," \
   "  `spell_num` INT DEFAULT 0," \
   "  `school` INT DEFAULT 1," \
   "  `spell_level` INT DEFAULT 1," \
   "  `mana_cost` INT DEFAULT 1," \
   "  `exertion` INT DEFAULT 1," \
   "  `cast_time` INT DEFAULT 0," \
   "  `post_cast_time` INT DEFAULT 1," \
   "  `chance_to_increase` INT DEFAULT 20," \
   "  `meditate_ratio` INT DEFAULT 100," \
   "  `is_harmful` TINYINT(1) DEFAULT 0," \
   "  `is_outlaw` TINYINT(1) DEFAULT 0," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
   "  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`spell_template_id`)," \
   "  UNIQUE KEY `uk_spell_kod_class` (`spell_kod_class`)," \
   "  KEY `idx_spell_school` (`school`)," \
   "  KEY `idx_spell_level` (`spell_level`)," \
   "  KEY `idx_spell_num` (`spell_num`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

#define SQLQUERY_CREATETABLE_SPELLREAGENTS \
   "CREATE TABLE IF NOT EXISTS `spell_reagents` (" \
   "  `reagent_id` INT NOT NULL AUTO_INCREMENT," \
   "  `spell_template_id` INT NOT NULL," \
   "  `reagent_kod_class` VARCHAR(100) NOT NULL," \
   "  `amount` INT NOT NULL DEFAULT 1," \
   "  PRIMARY KEY (`reagent_id`)," \
   "  KEY `idx_spell_reagent` (`spell_template_id`)," \
   "  CONSTRAINT `fk_reagent_spell` FOREIGN KEY (`spell_template_id`)" \
   "    REFERENCES `spell_templates` (`spell_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_spell_count = 0;

#define MAX_INHERITANCE_DEPTH 50

static Bool SpellClassInheritsFrom(class_node *c, const char *parent_name)
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

static int SpellGetClassVarInt(class_node *c, const char *var_name)
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

static const char* SpellGetClassVarResourceString(class_node *c, const char *var_name)
{
   int res_id = SpellGetClassVarInt(c, var_name);
   if (res_id > 0)
   {
      resource_node *r = GetResourceByID(res_id);
      if (r && r->resource_val[0])
         return r->resource_val[0];
   }
   return NULL;
}

static void EscapeString(char *dest, const char *src, size_t dest_size)
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

static void ExportSpellClassToMySQL(class_node *c)
{
   char query[2048];
   const char *spell_name_raw, *spell_icon_raw, *spell_class;
   char esc_name[128], esc_class[128], esc_icon[128];
   int spell_num, school, spell_level, mana_cost, exertion;
   int cast_time, post_cast_time, chance_to_increase, meditate_ratio;
   int is_harmful, is_outlaw;

   if (!c || !c->class_name) return;
   if (!mysql) return;

   spell_class = c->class_name;

   // Get spell name - skip if no valid name
   spell_name_raw = SpellGetClassVarResourceString(c, "vrName");
   if (!spell_name_raw) return;
   if (strlen(spell_name_raw) == 0) return;
   if (_stricmp(spell_name_raw, "spell") == 0) return;
   if (_stricmp(spell_name_raw, "something") == 0) return;

   // Get icon (optional)
   spell_icon_raw = SpellGetClassVarResourceString(c, "vrIcon");
   if (!spell_icon_raw) spell_icon_raw = "";

   // Get integer values
   spell_num = SpellGetClassVarInt(c, "viSpell_Num");
   school = SpellGetClassVarInt(c, "viSchool");
   spell_level = SpellGetClassVarInt(c, "viSpell_level");
   mana_cost = SpellGetClassVarInt(c, "viMana");
   exertion = SpellGetClassVarInt(c, "viSpellExertion");
   cast_time = SpellGetClassVarInt(c, "viCast_time");
   post_cast_time = SpellGetClassVarInt(c, "viPostCast_time");
   chance_to_increase = SpellGetClassVarInt(c, "viChance_To_Increase");
   meditate_ratio = SpellGetClassVarInt(c, "viMeditate_ratio");
   is_harmful = SpellGetClassVarInt(c, "viHarmful");
   is_outlaw = SpellGetClassVarInt(c, "viOutlaw");

   // Skip DM spells without spell_num
   if (school == 7 && spell_num == 0) return;

   // Default values
   if (spell_level == 0) spell_level = 1;
   if (mana_cost == 0) mana_cost = 1;
   if (exertion == 0) exertion = 1;
   if (chance_to_increase == 0) chance_to_increase = 20;
   if (meditate_ratio == 0) meditate_ratio = 100;
   if (post_cast_time == 0) post_cast_time = 1;

   // Escape strings
   EscapeString(esc_name, spell_name_raw, sizeof(esc_name));
   EscapeString(esc_class, spell_class, sizeof(esc_class));
   EscapeString(esc_icon, spell_icon_raw, sizeof(esc_icon));

   // Build query
   snprintf(query, sizeof(query),
      "INSERT INTO spell_templates (spell_name, spell_kod_class, spell_icon, "
      "spell_num, school, spell_level, mana_cost, exertion, cast_time, post_cast_time, "
      "chance_to_increase, meditate_ratio, is_harmful, is_outlaw) "
      "VALUES ('%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d) "
      "ON DUPLICATE KEY UPDATE spell_name='%s', spell_icon='%s', "
      "spell_num=%d, school=%d, spell_level=%d, mana_cost=%d, exertion=%d, cast_time=%d, "
      "post_cast_time=%d, chance_to_increase=%d, meditate_ratio=%d, is_harmful=%d, is_outlaw=%d",
      esc_name, esc_class, esc_icon,
      spell_num, school, spell_level, mana_cost, exertion, cast_time, post_cast_time,
      chance_to_increase, meditate_ratio, is_harmful, is_outlaw,
      esc_name, esc_icon,
      spell_num, school, spell_level, mana_cost, exertion, cast_time, post_cast_time,
      chance_to_increase, meditate_ratio, is_harmful, is_outlaw);

   MySQLLock();
   if (mysql_query(mysql, query))
      eprintf("ExportSpellClassToMySQL: Failed %s: %s\n", spell_class, mysql_error(mysql));
   else
      export_spell_count++;
   MySQLUnlock();
}

static void ExportSpellClassCallback(class_node *c)
{
   if (!c || !c->class_name) return;
   if (!SpellClassInheritsFrom(c, "Spell")) return;

   // Skip abstract base classes
   if (_stricmp(c->class_name, "Spell") == 0) return;
   if (_stricmp(c->class_name, "PassiveObject") == 0) return;
   if (_stricmp(c->class_name, "AttackSpell") == 0) return;
   if (_stricmp(c->class_name, "BoltSpell") == 0) return;
   if (_stricmp(c->class_name, "RadiusProjectile") == 0) return;
   if (_stricmp(c->class_name, "Debuff") == 0) return;
   if (_stricmp(c->class_name, "Disease") == 0) return;
   if (_stricmp(c->class_name, "Boon") == 0) return;
   if (_stricmp(c->class_name, "DMSpell") == 0) return;
   if (_stricmp(c->class_name, "Crafting") == 0) return;
   if (_stricmp(c->class_name, "WallSpell") == 0) return;
   if (_stricmp(c->class_name, "TeleportSpell") == 0) return;

   ExportSpellClassToMySQL(c);
}

void _MySQLCreateSpellSchema(void)
{
   if (!mysql) { eprintf("_MySQLCreateSpellSchema: MySQL not connected\n"); return; }

   MySQLLock();

   lprintf("Creating Spell schema...\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_SPELLTEMPLATES))
      eprintf("_MySQLCreateSpellSchema: Failed spell_templates: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateSpellSchema: spell_templates table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_SPELLREAGENTS))
      eprintf("_MySQLCreateSpellSchema: Failed spell_reagents: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateSpellSchema: spell_reagents table ready\n");

   lprintf("Spell schema creation complete.\n");

   MySQLUnlock();
}

int MySQLExportSpellData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportSpellData: MySQL not ready\n");
      return 0;
   }

   export_spell_count = 0;
   lprintf("MySQLExportSpellData: Starting Spell export...\n");

   ForEachClass(ExportSpellClassCallback);

   lprintf("MySQLExportSpellData: Exported %d Spells\n", export_spell_count);
   return export_spell_count;
}
