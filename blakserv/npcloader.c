// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * npcloader.c
 *
 * TEST 4: Adding MySQL query (prepared statement)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define MAX_NPC_TEMPLATES 100
static npc_template_t npc_templates[MAX_NPC_TEMPLATES];
static int num_npc_templates = 0;

int LoadNPCTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[14];
   int result;

   num_npc_templates = 0;

   if (!mysql)
   {
      lprintf("LoadNPCTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      lprintf("LoadNPCTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading NPC templates from MySQL...\n");

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadNPCTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query =
      "SELECT npc_template_id, npc_name, npc_kod_class, npc_icon, "
      "town, room_kod_class, occupation, merchant_markup, "
      "is_seller, is_buyer, is_teacher, is_banker, "
      "is_lawful, faction "
      "FROM npc_templates WHERE active = 1 ORDER BY npc_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadNPCTemplatesFromMySQL: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadNPCTemplatesFromMySQL: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));

   int npc_id, occupation, merchant_markup, faction;
   my_bool is_seller, is_buyer, is_teacher, is_banker, is_lawful;
   char name[101], kod_class[101], icon[101], town[51], room_class[101];
   my_bool is_null[14];
   unsigned long length[14];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&npc_id;
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
   bind[3].buffer = icon;
   bind[3].buffer_length = sizeof(icon);
   bind[3].is_null = &is_null[3];
   bind[3].length = &length[3];

   bind[4].buffer_type = MYSQL_TYPE_STRING;
   bind[4].buffer = town;
   bind[4].buffer_length = sizeof(town);
   bind[4].is_null = &is_null[4];
   bind[4].length = &length[4];

   bind[5].buffer_type = MYSQL_TYPE_STRING;
   bind[5].buffer = room_class;
   bind[5].buffer_length = sizeof(room_class);
   bind[5].is_null = &is_null[5];
   bind[5].length = &length[5];

   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&occupation;
   bind[6].is_null = &is_null[6];

   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&merchant_markup;
   bind[7].is_null = &is_null[7];

   bind[8].buffer_type = MYSQL_TYPE_TINY;
   bind[8].buffer = (char*)&is_seller;
   bind[8].is_null = &is_null[8];

   bind[9].buffer_type = MYSQL_TYPE_TINY;
   bind[9].buffer = (char*)&is_buyer;
   bind[9].is_null = &is_null[9];

   bind[10].buffer_type = MYSQL_TYPE_TINY;
   bind[10].buffer = (char*)&is_teacher;
   bind[10].is_null = &is_null[10];

   bind[11].buffer_type = MYSQL_TYPE_TINY;
   bind[11].buffer = (char*)&is_banker;
   bind[11].is_null = &is_null[11];

   bind[12].buffer_type = MYSQL_TYPE_TINY;
   bind[12].buffer = (char*)&is_lawful;
   bind[12].is_null = &is_null[12];

   bind[13].buffer_type = MYSQL_TYPE_LONG;
   bind[13].buffer = (char*)&faction;
   bind[13].is_null = &is_null[13];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadNPCTemplatesFromMySQL: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0 && num_npc_templates < MAX_NPC_TEMPLATES)
   {
      npc_template_t* nt = &npc_templates[num_npc_templates];

      nt->npc_template_id = npc_id;
      strncpy(nt->npc_name, name, 100);
      nt->npc_name[100] = '\0';

      if (!is_null[2]) { strncpy(nt->npc_kod_class, kod_class, 100); nt->npc_kod_class[100] = '\0'; }
      else { nt->npc_kod_class[0] = '\0'; }

      if (!is_null[3]) { strncpy(nt->npc_icon, icon, 100); nt->npc_icon[100] = '\0'; }
      else { nt->npc_icon[0] = '\0'; }

      if (!is_null[4]) { strncpy(nt->town, town, 50); nt->town[50] = '\0'; }
      else { nt->town[0] = '\0'; }

      if (!is_null[5]) { strncpy(nt->room_kod_class, room_class, 100); nt->room_kod_class[100] = '\0'; }
      else { nt->room_kod_class[0] = '\0'; }

      nt->occupation = is_null[6] ? 0 : occupation;
      nt->merchant_markup = is_null[7] ? 100 : merchant_markup;
      nt->is_seller = is_null[8] ? 0 : is_seller;
      nt->is_buyer = is_null[9] ? 0 : is_buyer;
      nt->is_teacher = is_null[10] ? 0 : is_teacher;
      nt->is_banker = is_null[11] ? 0 : is_banker;
      nt->is_lawful = is_null[12] ? 1 : is_lawful;
      nt->faction = is_null[13] ? 0 : faction;
      nt->active = 1;

      num_npc_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadNPCTemplatesFromMySQL: Loaded %d NPC templates\n", num_npc_templates);
   return num_npc_templates;
}

npc_template_t* GetNPCTemplateByID(int npc_id)
{
   for (int i = 0; i < num_npc_templates; i++)
   {
      if (npc_templates[i].npc_template_id == npc_id)
         return &npc_templates[i];
   }
   return NULL;
}

npc_template_t* GetNPCTemplateByClass(const char* class_name)
{
   if (!class_name)
      return NULL;

   for (int i = 0; i < num_npc_templates; i++)
   {
      if (_stricmp(npc_templates[i].npc_kod_class, class_name) == 0)
         return &npc_templates[i];
   }
   return NULL;
}

npc_template_t* GetAllNPCTemplates(int* count)
{
   if (count)
      *count = num_npc_templates;
   return npc_templates;
}
