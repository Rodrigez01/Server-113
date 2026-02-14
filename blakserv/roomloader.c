// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * roomloader.c
 *
 * Loads room templates from MySQL database into memory
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define MAX_ROOM_TEMPLATES 600
static room_template_t room_templates[MAX_ROOM_TEMPLATES];
static int num_room_templates = 0;

int LoadRoomTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[4];
   int result;
   int room_id;
   my_bool active;
   char name[101], kod_class[101];
   my_bool is_null[4];
   unsigned long length[4];

   num_room_templates = 0;

   if (!mysql)
   {
      lprintf("LoadRoomTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      lprintf("LoadRoomTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading Room templates from MySQL...\n");

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadRoomTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query =
      "SELECT room_template_id, room_name, room_kod_class, active "
      "FROM room_templates WHERE active = 1 ORDER BY room_name";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadRoomTemplatesFromMySQL: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadRoomTemplatesFromMySQL: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&room_id;
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

   bind[3].buffer_type = MYSQL_TYPE_TINY;
   bind[3].buffer = (char*)&active;
   bind[3].is_null = &is_null[3];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadRoomTemplatesFromMySQL: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0 && num_room_templates < MAX_ROOM_TEMPLATES)
   {
      room_template_t* rt = &room_templates[num_room_templates];

      memset(rt, 0, sizeof(room_template_t));

      rt->room_template_id = room_id;

      strncpy(rt->room_name, name, 100);
      rt->room_name[100] = '\0';

      if (!is_null[2])
      {
         strncpy(rt->room_kod_class, kod_class, 100);
         rt->room_kod_class[100] = '\0';
      }

      rt->active = is_null[3] ? 1 : active;

      num_room_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadRoomTemplatesFromMySQL: Loaded %d Room templates\n", num_room_templates);
   return num_room_templates;
}

room_template_t* GetRoomTemplateByID(int room_id)
{
   int i;
   for (i = 0; i < num_room_templates; i++)
   {
      if (room_templates[i].room_template_id == room_id)
         return &room_templates[i];
   }
   return NULL;
}

room_template_t* GetRoomTemplateByClass(const char* class_name)
{
   int i;

   if (!class_name)
      return NULL;

   for (i = 0; i < num_room_templates; i++)
   {
      if (_stricmp(room_templates[i].room_kod_class, class_name) == 0)
         return &room_templates[i];
   }
   return NULL;
}

room_template_t* GetRoomTemplateByNum(int room_num)
{
   int i;
   for (i = 0; i < num_room_templates; i++)
   {
      if (room_templates[i].room_num == room_num)
         return &room_templates[i];
   }
   return NULL;
}

room_template_t* GetAllRoomTemplates(int* count)
{
   if (count)
      *count = num_room_templates;
   return room_templates;
}
