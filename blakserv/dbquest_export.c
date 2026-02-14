// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbquest_export.c
 *
 * Quest Data Export - Exports QuestTemplate classes from KOD to MySQL
 * Uses quest_v2_quest_templates table
 */

#include "blakserv.h"

// External variables
extern MYSQL* mysql;
extern sql_worker_state state;

// Counters
static int export_quest_count = 0;

/* Maximum inheritance depth to prevent infinite loops */
#define MAX_INHERITANCE_DEPTH 50

/**
 * @brief Checks if a class inherits from QuestTemplate
 */
static Bool ClassInheritsFromQuestTemplate(class_node *c)
{
   class_node *current = c;
   int depth = 0;

   while (current != NULL && depth < MAX_INHERITANCE_DEPTH)
   {
      if (current->class_name && _stricmp(current->class_name, "QuestTemplate") == 0)
         return True;
      current = current->super_ptr;
      depth++;
   }
   return False;
}

/**
 * @brief Gets a class variable integer value
 */
static int GetClassVarIntQuest(class_node *c, const char *var_name)
{
   class_node *current = c;
   classvar_name_node *cv;
   int depth = 0;

   while (current != NULL && depth < MAX_INHERITANCE_DEPTH)
   {
      cv = current->classvar_names;
      while (cv != NULL)
      {
         if (cv->name && _stricmp(cv->name, var_name) == 0)
         {
            if (current->vars && cv->id >= 0 && cv->id < current->num_vars)
            {
               val_type val = current->vars[cv->id].val;
               if (val.v.tag == TAG_INT)
                  return val.v.data;
            }
            return 0;
         }
         cv = cv->next;
      }
      current = current->super_ptr;
      depth++;
   }
   return 0;
}

/**
 * @brief Gets a class variable resource string
 */
static const char* GetClassVarResourceQuest(class_node *c, const char *var_name)
{
   class_node *current = c;
   classvar_name_node *cv;
   resource_node *r;
   int depth = 0;

   while (current != NULL && depth < MAX_INHERITANCE_DEPTH)
   {
      cv = current->classvar_names;
      while (cv != NULL)
      {
         if (cv->name && _stricmp(cv->name, var_name) == 0)
         {
            if (current->vars && cv->id >= 0 && cv->id < current->num_vars)
            {
               val_type val = current->vars[cv->id].val;
               if (val.v.tag == TAG_RESOURCE && val.v.data > 0)
               {
                  r = GetResourceByID(val.v.data);
                  if (r && r->resource_val && r->resource_val[0])
                     return r->resource_val[0];
               }
            }
            return NULL;
         }
         cv = cv->next;
      }
      current = current->super_ptr;
      depth++;
   }
   return NULL;
}

/**
 * @brief Determines quest type string from piQuestType
 * Must match ENUM: 'faction', 'spell', 'item', 'delivery', 'combat', 'crafting', 'other'
 */
static const char* GetQuestTypeString(int quest_type)
{
   // Quest types from blakston.khd
   switch (quest_type)
   {
      case 1: return "faction";
      case 2: return "spell";
      case 3: return "combat";
      case 4: return "delivery";
      case 5: return "item";      // escort -> item
      case 6: return "crafting";  // collect -> crafting
      default: return "other";
   }
}

/**
 * @brief Determines difficulty from player restrictions
 */
static const char* GetQuestDifficulty(int player_restrict, int time_limit)
{
   // Simple heuristic based on restrictions and time limit
   if (time_limit > 0 && time_limit < 1800) // < 30 min
      return "hard";
   if (player_restrict > 100)
      return "hard";
   if (player_restrict > 50)
      return "medium";
   return "easy";
}

/**
 * @brief Escapes a string for MySQL insertion
 */
static void EscapeStringForMySQL(char *dest, const char *src, size_t max_len)
{
   if (!src)
   {
      dest[0] = '\0';
      return;
   }

   size_t src_len = strlen(src);
   if (src_len > max_len / 2 - 1)
      src_len = max_len / 2 - 1;

   MySQLLock();
   mysql_real_escape_string(mysql, dest, src, (unsigned long)src_len);
   MySQLUnlock();
}

/**
 * @brief Callback to export a single quest class
 * Uses mysql_query() instead of prepared statements to avoid crashes
 */
static void ExportQuestClassToMySQL(class_node *c)
{
   const char *quest_name, *quest_desc, *quest_class;
   const char *quest_type_str, *difficulty_str;
   int quest_id, quest_type, player_restrict, time_limit;
   int min_level, max_level, repeatable;
   char desc_buffer[512];
   char query[4096];
   char esc_name[512], esc_class[256], esc_desc[2048];
   char esc_type[64], esc_diff[64];

   if (!c || !c->class_name)
      return;

   // Skip base QuestTemplate class
   if (_stricmp(c->class_name, "QuestTemplate") == 0)
      return;

   // Check if inherits from QuestTemplate
   if (!ClassInheritsFromQuestTemplate(c))
      return;

   quest_class = c->class_name;

   // Get quest properties
   quest_id = GetClassVarIntQuest(c, "viQuestID");
   if (quest_id == 0)
   {
      dprintf("ExportQuestClassToMySQL: Skipping %s (no viQuestID)\n", quest_class);
      return;
   }

   quest_name = GetClassVarResourceQuest(c, "vrName");
   quest_desc = GetClassVarResourceQuest(c, "vrDesc");
   quest_type = GetClassVarIntQuest(c, "piQuestType");
   player_restrict = GetClassVarIntQuest(c, "piPlayerRestrict");
   time_limit = GetClassVarIntQuest(c, "piTimeLimit");

   // Default values
   if (!quest_name || quest_name[0] == '\0')
      quest_name = quest_class;

   if (!quest_desc)
   {
      snprintf(desc_buffer, sizeof(desc_buffer), "Quest: %s", quest_name);
      quest_desc = desc_buffer;
   }

   quest_type_str = GetQuestTypeString(quest_type);
   difficulty_str = GetQuestDifficulty(player_restrict, time_limit);

   // Level restrictions
   min_level = 1;
   max_level = 150;
   repeatable = (player_restrict & 0x10) ? 0 : 1; // Q_PLAYER_NOTSUCCEEDED = can't repeat

   dprintf("ExportQuestClassToMySQL: Exporting %s (ID=%d, type=%s)\n",
           quest_class, quest_id, quest_type_str);

   // Escape strings for SQL
   EscapeStringForMySQL(esc_name, quest_name, sizeof(esc_name));
   EscapeStringForMySQL(esc_class, quest_class, sizeof(esc_class));
   EscapeStringForMySQL(esc_desc, quest_desc, sizeof(esc_desc));
   EscapeStringForMySQL(esc_type, quest_type_str, sizeof(esc_type));
   EscapeStringForMySQL(esc_diff, difficulty_str, sizeof(esc_diff));

   // Build INSERT query for quest_v2_quest_templates
   snprintf(query, sizeof(query),
      "INSERT INTO quest_v2_quest_templates "
      "(quest_template_id, quest_name, quest_kod_class, quest_description, "
      "quest_type, difficulty, min_level, max_level, repeatable, time_limit) "
      "VALUES (%d, '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d) "
      "ON DUPLICATE KEY UPDATE "
      "quest_name='%s', quest_description='%s', quest_type='%s', "
      "difficulty='%s', min_level=%d, max_level=%d, repeatable=%d, time_limit=%d",
      quest_id, esc_name, esc_class, esc_desc, esc_type, esc_diff,
      min_level, max_level, repeatable, time_limit,
      esc_name, esc_desc, esc_type, esc_diff, min_level, max_level, repeatable, time_limit);

   // Execute query
   MySQLLock();
   if (mysql_query(mysql, query) != 0)
   {
      eprintf("ExportQuestClassToMySQL: query failed for %s: %s\n",
              quest_class, mysql_error(mysql));
   }
   else
   {
      export_quest_count++;
   }
   MySQLUnlock();
}

/**
 * @brief Exports all quest templates from KOD to MySQL
 *
 * Iterates through all loaded classes, finds QuestTemplate subclasses,
 * and exports their data to the quest_v2_quest_templates table.
 *
 * @return Number of exported quests
 */
int MySQLExportQuestData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportQuestData: MySQL not available\n");
      return 0;
   }

   lprintf("MySQLExportQuestData: Starting quest export to quest_v2_quest_templates...\n");

   // Clear old non-KOD-based entries (keep entries with valid quest_template_id from KOD)
   // We use ON DUPLICATE KEY UPDATE, so existing entries will be updated

   export_quest_count = 0;

   // Iterate all classes and export quests
   ForEachClass(ExportQuestClassToMySQL);

   lprintf("MySQLExportQuestData: Exported %d quest templates to quest_v2\n", export_quest_count);

   return export_quest_count;
}
