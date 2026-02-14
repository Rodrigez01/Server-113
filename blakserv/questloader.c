// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * questloader.c
 *
 * MySQL Quest Template Loader
 * Lädt Quest-Templates aus MySQL und stellt sie dem KOD-System zur Verfügung
 */

#include "blakserv.h"

// External variables
extern MYSQL* mysql;
extern sql_worker_state state;

/* Quest Template Cache */
#define MAX_QUEST_TEMPLATES 100
static quest_template_t quest_templates[MAX_QUEST_TEMPLATES];
static int num_quest_templates = 0;

/* Quest Trigger Cache */
#define MAX_QUEST_TRIGGERS 500
typedef struct {
   int quest_template_id;
   char npc_class[101];
   char trigger_text[256];
   int node_order;
} quest_trigger_t;

static quest_trigger_t quest_triggers[MAX_QUEST_TRIGGERS];
static int num_quest_triggers = 0;


/* ============================================================================
 * QUEST TEMPLATE LOADING
 * ============================================================================
 */

/**
 * @brief Lädt alle Quest-Templates aus MySQL in den Speicher
 * 
 * Diese Funktion wird beim Server-Start aufgerufen und lädt alle
 * verfügbaren Quest-Templates aus der Datenbank.
 * 
 * @return Anzahl geladener Templates
 */
int LoadQuestTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[9];  // 9 columns from quest_v2_quest_templates
   int result;
   
   if (!mysql)
   {
      eprintf("LoadQuestTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }
   
   if (state < CONNECTED)
   {
      eprintf("LoadQuestTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading quest templates from MySQL...\n");
   
   MySQLLock();

   // Prepare statement
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadQuestTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   // Use quest_v2_quest_templates table (no 'active' column - all entries are active)
   const char* query = "SELECT quest_template_id, quest_name, quest_kod_class, "
                       "quest_description, quest_type, difficulty, min_level, "
                       "max_level, repeatable "
                       "FROM quest_v2_quest_templates ORDER BY quest_template_id";
   
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("LoadQuestTemplatesFromMySQL: mysql_stmt_prepare() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Execute
   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadQuestTemplatesFromMySQL: mysql_stmt_execute() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns (9 columns - no 'active' in quest_v2)
   memset(bind, 0, sizeof(bind));

   int quest_id;
   char name[101], kod_class[101], description[256], type[20], diff[20];
   int min_lvl, max_lvl;
   my_bool repeat;
   my_bool is_null[9];
   unsigned long length[9];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&quest_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = name;
   bind[1].buffer_length = sizeof(name);
   bind[1].is_null = &is_null[1];
   bind[1].length = &length[1];

   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = kod_class;
   bind[2].buffer_length = sizeof(kod_class);
   bind[2].is_null = &is_null[2];
   bind[2].length = &length[2];

   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = description;
   bind[3].buffer_length = sizeof(description);
   bind[3].is_null = &is_null[3];
   bind[3].length = &length[3];

   bind[4].buffer_type = MYSQL_TYPE_STRING;
   bind[4].buffer = type;
   bind[4].buffer_length = sizeof(type);
   bind[4].is_null = &is_null[4];
   bind[4].length = &length[4];

   bind[5].buffer_type = MYSQL_TYPE_STRING;
   bind[5].buffer = diff;
   bind[5].buffer_length = sizeof(diff);
   bind[5].is_null = &is_null[5];
   bind[5].length = &length[5];

   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&min_lvl;
   bind[6].is_null = &is_null[6];

   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&max_lvl;
   bind[7].is_null = &is_null[7];

   bind[8].buffer_type = MYSQL_TYPE_TINY;
   bind[8].buffer = (char*)&repeat;
   bind[8].is_null = &is_null[8];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadQuestTemplatesFromMySQL: mysql_stmt_bind_result() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Fetch rows
   num_quest_templates = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_quest_templates < MAX_QUEST_TEMPLATES)
   {
      quest_template_t* qt = &quest_templates[num_quest_templates];
      
      qt->quest_template_id = quest_id;
      strncpy(qt->quest_name, name, 100);
      qt->quest_name[100] = '\0';
      
      if (!is_null[2])
      {
         strncpy(qt->quest_kod_class, kod_class, 100);
         qt->quest_kod_class[100] = '\0';
      }
      else
      {
         qt->quest_kod_class[0] = '\0';
      }
      
      if (!is_null[3])
      {
         strncpy(qt->quest_description, description, 255);
         qt->quest_description[255] = '\0';
      }
      else
      {
         qt->quest_description[0] = '\0';
      }
      
      strncpy(qt->quest_type, type, 19);
      qt->quest_type[19] = '\0';
      
      strncpy(qt->difficulty, diff, 19);
      qt->difficulty[19] = '\0';
      
      qt->min_level = min_lvl;
      qt->max_level = is_null[7] ? 999 : max_lvl;
      qt->repeatable = repeat;
      qt->active = 1;  // All quest_v2 entries are active
      
      dprintf("Loaded quest %d: %s (%s, %s)\n", 
              qt->quest_template_id, qt->quest_name, qt->quest_type, qt->difficulty);
      
      num_quest_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   
   lprintf("LoadQuestTemplatesFromMySQL: Loaded %d quest templates\n", num_quest_templates);
   
   // Load quest triggers after templates
   LoadQuestTriggersFromMySQL();
   
   return num_quest_templates;
}


/**
 * @brief Findet ein Quest-Template by ID
 * 
 * @param quest_id Quest Template ID (QST_ID)
 * @return Pointer zu Quest-Template oder NULL
 */
quest_template_t* GetQuestTemplateByID(int quest_id)
{
   for (int i = 0; i < num_quest_templates; i++)
   {
      if (quest_templates[i].quest_template_id == quest_id)
      {
         return &quest_templates[i];
      }
   }
   return NULL;
}


/**
 * @brief Gibt alle verfügbaren Quest-Templates zurück
 * 
 * @param count Output: Anzahl Templates
 * @return Array von Quest-Templates
 */
quest_template_t* GetAllQuestTemplates(int* count)
{
   *count = num_quest_templates;
   return quest_templates;
}


/**
 * @brief Prüft ob eine Quest für einen Spieler verfügbar ist
 * 
 * @param quest_id Quest Template ID
 * @param player_level Spieler-Level
 * @return 1 wenn verfügbar, 0 wenn nicht
 */
int IsQuestAvailableForPlayer(int quest_id, int player_level)
{
   quest_template_t* qt = GetQuestTemplateByID(quest_id);
   if (!qt)
      return 0;
   
   if (!qt->active)
      return 0;
   
   if (player_level < qt->min_level)
      return 0;
   
   if (qt->max_level > 0 && player_level > qt->max_level)
      return 0;
   
   return 1;
}


/**
 * @brief Gibt Quest-Templates nach Typ zurück
 * 
 * @param quest_type Quest Type (z.B. "faction", "spell", "combat")
 * @param results Output: Array von Quest-IDs
 * @param max_results Maximale Anzahl Ergebnisse
 * @return Anzahl gefundener Quests
 */
int GetQuestTemplatesByType(const char* quest_type, int* results, int max_results)
{
   int count = 0;
   
   for (int i = 0; i < num_quest_templates && count < max_results; i++)
   {
      if (strcmp(quest_templates[i].quest_type, quest_type) == 0)
      {
         results[count++] = quest_templates[i].quest_template_id;
      }
   }
   
   return count;
}


/* ============================================================================
 * QUEST TRIGGER LOADING
 * ============================================================================
 */

/**
 * @brief Lädt alle Quest-Trigger aus MySQL in den Speicher
 * 
 * Lädt NPCs und deren Trigger-Texte für alle aktiven Quests.
 * Wird nach LoadQuestTemplatesFromMySQL() aufgerufen.
 * 
 * @return Anzahl geladener Trigger
 */
int LoadQuestTriggersFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[4];
   int result;
   
   if (!mysql)
   {
      eprintf("LoadQuestTriggersFromMySQL: MySQL not available\n");
      return 0;
   }
   
   if (state < CONNECTED)
   {
      eprintf("LoadQuestTriggersFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading quest triggers from MySQL...\n");
   
   MySQLLock();

   // Prepare statement
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadQuestTriggersFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   // Query: Alle Trigger für aktive Quests mit quest_giver NPCs (quest_v2 tables)
   const char* query =
      "SELECT qt.quest_template_id, npc.npc_class, c.cargo_text, n.node_order "
      "FROM quest_v2_quest_templates qt "
      "JOIN quest_v2_quest_node_templates n ON qt.quest_template_id = n.quest_template_id "
      "JOIN quest_v2_quest_node_npcs npc ON n.quest_node_id = npc.quest_node_id "
      "JOIN quest_v2_quest_node_cargo c ON n.quest_node_id = c.quest_node_id "
      "WHERE npc.npc_role = 'quest_giver' "
      "  AND c.cargo_type = 'trigger' "
      "ORDER BY qt.quest_template_id, n.node_order";
   
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("LoadQuestTriggersFromMySQL: mysql_stmt_prepare() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Execute
   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadQuestTriggersFromMySQL: mysql_stmt_execute() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));
   
   int quest_id, node_order;
   char npc_class[101], trigger_text[256];
   my_bool is_null[4];
   unsigned long length[4];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&quest_id;
   bind[0].is_null = &is_null[0];
   
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = npc_class;
   bind[1].buffer_length = sizeof(npc_class);
   bind[1].is_null = &is_null[1];
   bind[1].length = &length[1];
   
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = trigger_text;
   bind[2].buffer_length = sizeof(trigger_text);
   bind[2].is_null = &is_null[2];
   bind[2].length = &length[2];
   
   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&node_order;
   bind[3].is_null = &is_null[3];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadQuestTriggersFromMySQL: mysql_stmt_bind_result() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Fetch rows
   num_quest_triggers = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_quest_triggers < MAX_QUEST_TRIGGERS)
   {
      quest_trigger_t* qt = &quest_triggers[num_quest_triggers];
      
      qt->quest_template_id = quest_id;
      qt->node_order = node_order;
      
      strncpy(qt->npc_class, npc_class, 100);
      qt->npc_class[100] = '\0';
      
      strncpy(qt->trigger_text, trigger_text, 255);
      qt->trigger_text[255] = '\0';
      
      dprintf("Loaded trigger: Quest %d, NPC=%s, Trigger=\"%s\"\n", 
              qt->quest_template_id, qt->npc_class, qt->trigger_text);
      
      num_quest_triggers++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   
   lprintf("LoadQuestTriggersFromMySQL: Loaded %d quest triggers\n", num_quest_triggers);
   return num_quest_triggers;
}


/**
 * @brief Sucht nach einem Quest-Trigger
 * 
 * Prüft ob ein NPC mit einem bestimmten Trigger-Text eine Quest startet.
 * Nutzt den in-memory Cache - KEINE MySQL-Query!
 * 
 * @param npc_class NPC-Klasse (z.B. "BarloqueBartender")
 * @param trigger_text Trigger-Text (case-insensitive, z.B. "Kuckuck")
 * @return Quest-Template-ID oder 0 wenn kein Match
 */
int CheckQuestTrigger(const char* npc_class, const char* trigger_text)
{
   if (!npc_class || !trigger_text)
      return 0;
   
   // Durchsuche Cache
   for (int i = 0; i < num_quest_triggers; i++)
   {
      quest_trigger_t* qt = &quest_triggers[i];
      
      // NPC-Klasse matchen
      if (strcmp(qt->npc_class, npc_class) != 0)
         continue;
      
      // Trigger-Text matchen (case-insensitive)
      if (_stricmp(qt->trigger_text, trigger_text) != 0)
         continue;
      
      // Match gefunden!
      dprintf("CheckQuestTrigger: Match found! Quest %d for NPC %s with trigger \"%s\"\n",
              qt->quest_template_id, npc_class, trigger_text);
      
      return qt->quest_template_id;
   }
   
   return 0; // Kein Match
}


/**
 * @brief Gibt alle Trigger für einen NPC zurück
 * 
 * @param npc_class NPC-Klasse
 * @param results Output: Array von Quest-IDs
 * @param max_results Maximale Anzahl Ergebnisse
 * @return Anzahl gefundener Trigger
 */
int GetTriggersForNPC(const char* npc_class, int* results, int max_results)
{
   int count = 0;
   
   if (!npc_class || !results)
      return 0;
   
   for (int i = 0; i < num_quest_triggers && count < max_results; i++)
   {
      if (strcmp(quest_triggers[i].npc_class, npc_class) == 0)
      {
         results[count++] = quest_triggers[i].quest_template_id;
      }
   }
   
   return count;
}

