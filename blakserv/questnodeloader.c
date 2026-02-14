// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * questnodeloader.c
 *
 * MySQL Quest Node Loader
 * Lädt vollständige Quest-Node-Daten aus MySQL (Nodes, NPCs, Rewards, Dialogs)
 */

#include "blakserv.h"

// External variables
extern MYSQL* mysql;
extern sql_worker_state state;

/* Quest Node Cache */
#define MAX_QUEST_NODES 500

/* Structs defined in database.h: quest_node_data_t, quest_node_npc_t, quest_dialog_t */

typedef struct {
   int quest_node_id;
   char cargo_type[51];        // ITEM, MESSAGE, TRIGGER_PHRASE
   char cargo_class[101];
   char cargo_text[1024];
   int cargo_quantity;
} quest_node_cargo_t;

typedef struct {
   int quest_node_id;
   char reward_type[51];       // ITEMCLASS, BOON, SPELL, SKILL, XP, GOLD, NPC_RESPONSE
   char reward_class[101];
   int reward_quantity;
   int reward_value;
   int reward_param1;
   int reward_param2;
} quest_node_reward_t;

/* quest_dialog_t defined in database.h */

static quest_node_data_t quest_nodes[MAX_QUEST_NODES];
static int num_quest_nodes = 0;

/* ============================================================================
 * QUEST NODE LOADING
 * ============================================================================
 */

/**
 * @brief Lädt alle Quest-Nodes aus MySQL
 * 
 * @return Anzahl der geladenen Nodes
 */
int LoadQuestNodesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[7];
   int result;
   
   if (!mysql || state < CONNECTED)
   {
      eprintf("LoadQuestNodesFromMySQL: MySQL not connected\n");
      return 0;
   }

   lprintf("Loading quest nodes from MySQL...\n");
   
   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadQuestNodesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT quest_node_id, quest_node_index, quest_template_id, "
                       "node_order, node_type, time_limit, npc_modifier "
                       "FROM quest_node_templates "
                       "ORDER BY quest_template_id, node_order";
   
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("LoadQuestNodesFromMySQL: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadQuestNodesFromMySQL: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));
   
   int node_id, node_index, template_id, order, time_limit;
   char node_type[51], npc_mod[51];
   my_bool is_null[7];
   unsigned long length[7];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&node_id;
   bind[0].is_null = &is_null[0];
   
   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&node_index;
   bind[1].is_null = &is_null[1];
   
   bind[2].buffer_type = MYSQL_TYPE_LONG;
   bind[2].buffer = (char*)&template_id;
   bind[2].is_null = &is_null[2];
   
   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&order;
   bind[3].is_null = &is_null[3];
   
   bind[4].buffer_type = MYSQL_TYPE_STRING;
   bind[4].buffer = node_type;
   bind[4].buffer_length = 50;
   bind[4].length = &length[4];
   bind[4].is_null = &is_null[4];
   
   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&time_limit;
   bind[5].is_null = &is_null[5];
   
   bind[6].buffer_type = MYSQL_TYPE_STRING;
   bind[6].buffer = npc_mod;
   bind[6].buffer_length = 50;
   bind[6].length = &length[6];
   bind[6].is_null = &is_null[6];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadQuestNodesFromMySQL: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Fetch rows
   num_quest_nodes = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_quest_nodes < MAX_QUEST_NODES)
   {
      quest_node_data_t* qn = &quest_nodes[num_quest_nodes];
      
      qn->quest_node_id = node_id;
      qn->quest_node_index = node_index;
      qn->quest_template_id = template_id;
      qn->node_order = order;
      qn->time_limit = is_null[5] ? 0 : time_limit;
      
      strncpy(qn->node_type, node_type, 50);
      qn->node_type[50] = '\0';
      
      if (!is_null[6])
      {
         strncpy(qn->npc_modifier, npc_mod, 50);
         qn->npc_modifier[50] = '\0';
      }
      else
      {
         strcpy(qn->npc_modifier, "NONE");
      }
      
      num_quest_nodes++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   
   lprintf("LoadQuestNodesFromMySQL: Loaded %d quest nodes\n", num_quest_nodes);
   return num_quest_nodes;
}

/**
 * @brief Findet Quest-Node by Index (QNT_ID)
 * 
 * @param quest_node_index QNT_ID from blakston.khd
 * @return Pointer zu Quest-Node oder NULL
 */
quest_node_data_t* GetQuestNodeByIndex(int quest_node_index)
{
   for (int i = 0; i < num_quest_nodes; i++)
   {
      if (quest_nodes[i].quest_node_index == quest_node_index)
      {
         return &quest_nodes[i];
      }
   }
   return NULL;
}

/**
 * @brief Lädt NPCs für einen Quest-Node
 * 
 * @param quest_node_id Database ID
 * @param npcs Array für Ergebnisse
 * @param max_npcs Maximale Anzahl
 * @return Anzahl gefundener NPCs
 */
int GetQuestNodeNPCs(int quest_node_id, quest_node_npc_t* npcs, int max_npcs)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[3];
   MYSQL_BIND param_bind[1];
   int count = 0;
   
   if (!mysql || state < CONNECTED)
      return 0;

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT quest_node_id, npc_class, npc_role "
                       "FROM quest_node_npcs WHERE quest_node_id = ?";
   
   if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind parameter
   memset(param_bind, 0, sizeof(param_bind));
   param_bind[0].buffer_type = MYSQL_TYPE_LONG;
   param_bind[0].buffer = (char*)&quest_node_id;
   
   mysql_stmt_bind_param(stmt, param_bind);
   
   if (mysql_stmt_execute(stmt) != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind results
   memset(bind, 0, sizeof(bind));
   
   int node_id;
   char npc_class[101], npc_role[51];
   my_bool is_null[3];
   unsigned long length[3];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&node_id;
   bind[0].is_null = &is_null[0];
   
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = npc_class;
   bind[1].buffer_length = 100;
   bind[1].length = &length[1];
   bind[1].is_null = &is_null[1];
   
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = npc_role;
   bind[2].buffer_length = 50;
   bind[2].length = &length[2];
   bind[2].is_null = &is_null[2];

   mysql_stmt_bind_result(stmt, bind);

   while (mysql_stmt_fetch(stmt) == 0 && count < max_npcs)
   {
      npcs[count].quest_node_id = node_id;
      strncpy(npcs[count].npc_class, npc_class, 100);
      npcs[count].npc_class[100] = '\0';
      strncpy(npcs[count].npc_role, npc_role, 50);
      npcs[count].npc_role[50] = '\0';
      count++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   return count;
}

/**
 * @brief Lädt Dialogs für einen Quest-Node
 * 
 * @param quest_node_id Database ID
 * @param dialogs Array für Ergebnisse
 * @param max_dialogs Maximale Anzahl
 * @return Anzahl gefundener Dialogs
 */
int GetQuestNodeDialogs(int quest_node_id, quest_dialog_t* dialogs, int max_dialogs)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[3];
   MYSQL_BIND param_bind[1];
   int count = 0;
   
   if (!mysql || state < CONNECTED)
      return 0;

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT dialog_type, dialog_context, dialog_text "
                       "FROM quest_dialogs WHERE quest_node_id = ?";
   
   if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(param_bind, 0, sizeof(param_bind));
   param_bind[0].buffer_type = MYSQL_TYPE_LONG;
   param_bind[0].buffer = (char*)&quest_node_id;
   
   mysql_stmt_bind_param(stmt, param_bind);
   
   if (mysql_stmt_execute(stmt) != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(bind, 0, sizeof(bind));
   
   char dialog_type[51], dialog_context[101], dialog_text[2048];
   my_bool is_null[3];
   unsigned long length[3];

   bind[0].buffer_type = MYSQL_TYPE_STRING;
   bind[0].buffer = dialog_type;
   bind[0].buffer_length = 50;
   bind[0].length = &length[0];
   bind[0].is_null = &is_null[0];
   
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = dialog_context;
   bind[1].buffer_length = 100;
   bind[1].length = &length[1];
   bind[1].is_null = &is_null[1];
   
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = dialog_text;
   bind[2].buffer_length = 2047;
   bind[2].length = &length[2];
   bind[2].is_null = &is_null[2];

   mysql_stmt_bind_result(stmt, bind);

   while (mysql_stmt_fetch(stmt) == 0 && count < max_dialogs)
   {
      dialogs[count].quest_node_id = quest_node_id;
      strncpy(dialogs[count].dialog_type, dialog_type, 50);
      dialogs[count].dialog_type[50] = '\0';
      
      if (!is_null[1])
      {
         strncpy(dialogs[count].dialog_context, dialog_context, 100);
         dialogs[count].dialog_context[100] = '\0';
      }
      else
      {
         dialogs[count].dialog_context[0] = '\0';
      }
      
      strncpy(dialogs[count].dialog_text, dialog_text, 2047);
      dialogs[count].dialog_text[2047] = '\0';
      count++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   return count;
}

/**
 * @brief Lädt alle Quest-Nodes für eine Quest-Template
 * 
 * @param quest_template_id Template ID
 * @param nodes Array für Ergebnisse
 * @param max_nodes Maximale Anzahl
 * @return Anzahl gefundener Nodes
 */
int GetQuestNodesByTemplate(int quest_template_id, quest_node_data_t* nodes, int max_nodes)
{
   int count = 0;
   
   for (int i = 0; i < num_quest_nodes && count < max_nodes; i++)
   {
      if (quest_nodes[i].quest_template_id == quest_template_id)
      {
         memcpy(&nodes[count], &quest_nodes[i], sizeof(quest_node_data_t));
         count++;
      }
   }
   
   return count;
}
