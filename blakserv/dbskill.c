// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbskill.c
 *
 * MySQL Skill Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_SKILLTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `skill_templates` (" \
   "  `skill_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `skill_name` VARCHAR(100) NOT NULL," \
   "  `skill_kod_class` VARCHAR(100) NOT NULL," \
   "  `skill_icon` VARCHAR(100)," \
   "  `skill_num` INT DEFAULT 0," \
   "  `school` INT DEFAULT 1," \
   "  `skill_level` INT DEFAULT 1," \
   "  `exertion` INT DEFAULT 1," \
   "  `meditate_ratio` INT DEFAULT 100," \
   "  `is_combat` TINYINT(1) DEFAULT 0," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
   "  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`skill_template_id`)," \
   "  UNIQUE KEY `uk_skill_kod_class` (`skill_kod_class`)," \
   "  KEY `idx_skill_school` (`school`)," \
   "  KEY `idx_skill_level` (`skill_level`)," \
   "  KEY `idx_skill_num` (`skill_num`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_skill_count = 0;

#define MAX_INHERITANCE_DEPTH 50

static Bool SkillClassInheritsFrom(class_node *c, const char *parent_name)
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

static int SkillGetClassVarInt(class_node *c, const char *var_name)
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

static const char* SkillGetClassVarResourceString(class_node *c, const char *var_name)
{
   int res_id = SkillGetClassVarInt(c, var_name);
   if (res_id > 0)
   {
      resource_node *r = GetResourceByID(res_id);
      if (r && r->resource_val[0])
         return r->resource_val[0];
   }
   return NULL;
}

static void EscapeStringSkill(char *dest, const char *src, size_t dest_size)
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

static void ExportSkillClassToMySQL(class_node *c)
{
   char query[2048];
   const char *skill_name_raw, *skill_icon_raw, *skill_class;
   char esc_name[128], esc_class[128], esc_icon[128];
   int skill_num, school, skill_level, exertion, meditate_ratio;
   int is_combat;

   if (!c || !c->class_name) return;
   if (!mysql) return;

   skill_class = c->class_name;

   // Get skill name - skip if no valid name
   skill_name_raw = SkillGetClassVarResourceString(c, "vrName");
   if (!skill_name_raw) return;
   if (strlen(skill_name_raw) == 0) return;
   if (_stricmp(skill_name_raw, "skill") == 0) return;
   if (_stricmp(skill_name_raw, "something") == 0) return;

   // Get icon (optional)
   skill_icon_raw = SkillGetClassVarResourceString(c, "vrIcon");
   if (!skill_icon_raw) skill_icon_raw = "";

   // Get integer values
   skill_num = SkillGetClassVarInt(c, "viSkill_num");
   school = SkillGetClassVarInt(c, "viSchool");
   skill_level = SkillGetClassVarInt(c, "viSkill_level");
   exertion = SkillGetClassVarInt(c, "viSpellExertion");
   meditate_ratio = SkillGetClassVarInt(c, "viMeditate_ratio");
   is_combat = SkillGetClassVarInt(c, "viCombat");

   // Default values
   if (skill_level == 0) skill_level = 1;
   if (exertion == 0) exertion = 1;
   if (meditate_ratio == 0) meditate_ratio = 100;

   // Escape strings
   EscapeStringSkill(esc_name, skill_name_raw, sizeof(esc_name));
   EscapeStringSkill(esc_class, skill_class, sizeof(esc_class));
   EscapeStringSkill(esc_icon, skill_icon_raw, sizeof(esc_icon));

   // Build query
   snprintf(query, sizeof(query),
      "INSERT INTO skill_templates (skill_name, skill_kod_class, skill_icon, "
      "skill_num, school, skill_level, exertion, meditate_ratio, is_combat) "
      "VALUES ('%s', '%s', '%s', %d, %d, %d, %d, %d, %d) "
      "ON DUPLICATE KEY UPDATE skill_name='%s', skill_icon='%s', "
      "skill_num=%d, school=%d, skill_level=%d, exertion=%d, meditate_ratio=%d, is_combat=%d",
      esc_name, esc_class, esc_icon,
      skill_num, school, skill_level, exertion, meditate_ratio, is_combat,
      esc_name, esc_icon,
      skill_num, school, skill_level, exertion, meditate_ratio, is_combat);

   MySQLLock();
   if (mysql_query(mysql, query))
      eprintf("ExportSkillClassToMySQL: Failed %s: %s\n", skill_class, mysql_error(mysql));
   else
      export_skill_count++;
   MySQLUnlock();
}

static void ExportSkillClassCallback(class_node *c)
{
   if (!c || !c->class_name) return;
   if (!SkillClassInheritsFrom(c, "Skill")) return;

   // Skip abstract base classes
   if (_stricmp(c->class_name, "Skill") == 0) return;
   if (_stricmp(c->class_name, "PassiveObject") == 0) return;

   ExportSkillClassToMySQL(c);
}

void _MySQLCreateSkillSchema(void)
{
   if (!mysql) { eprintf("_MySQLCreateSkillSchema: MySQL not connected\n"); return; }

   MySQLLock();

   lprintf("Creating Skill schema...\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_SKILLTEMPLATES))
      eprintf("_MySQLCreateSkillSchema: Failed skill_templates: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateSkillSchema: skill_templates table ready\n");

   lprintf("Skill schema creation complete.\n");

   MySQLUnlock();
}

int MySQLExportSkillData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportSkillData: MySQL not ready\n");
      return 0;
   }

   export_skill_count = 0;
   lprintf("MySQLExportSkillData: Starting Skill export...\n");

   ForEachClass(ExportSkillClassCallback);

   lprintf("MySQLExportSkillData: Exported %d Skills\n", export_skill_count);
   return export_skill_count;
}
