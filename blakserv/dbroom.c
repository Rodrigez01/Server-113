// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbroom.c
 *
 * MySQL Room Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_ROOMTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `room_templates` (" \
   "  `room_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `room_name` VARCHAR(100) NOT NULL," \
   "  `room_kod_class` VARCHAR(100) NOT NULL," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`room_template_id`)," \
   "  UNIQUE KEY `uk_room_kod_class` (`room_kod_class`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_room_count = 0;

void _MySQLCreateRoomSchema(void)
{
   if (!mysql)
   {
      eprintf("_MySQLCreateRoomSchema: MySQL not connected\n");
      return;
   }

   MySQLLock();

   lprintf("Creating Room schema...\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ROOMTEMPLATES))
   {
      eprintf("_MySQLCreateRoomSchema: Failed room_templates: %s\n", mysql_error(mysql));
   }
   else
   {
      dprintf("_MySQLCreateRoomSchema: room_templates table ready\n");
   }

   lprintf("Room schema creation complete.\n");

   MySQLUnlock();
}

static void ExportRoomCallback(class_node *c)
{
   char query[512];
   char esc_name[64];
   char esc_class[64];
   const char *room_name = NULL;
   const char *room_class = NULL;
   class_node *parent;
   class_node *cur;
   classvar_name_node *cv;
   int depth;
   int is_room;
   int res_id;
   resource_node *r;
   size_t i, j;

   // Basic null checks
   if (!c)
      return;
   if (!c->class_name)
      return;
   if (!mysql)
      return;

   // Check inheritance from Room
   is_room = 0;
   parent = c;
   depth = 0;
   while (parent && depth < 50)
   {
      if (parent->class_name && _stricmp(parent->class_name, "Room") == 0)
      {
         is_room = 1;
         break;
      }
      parent = parent->super_ptr;
      depth++;
   }

   if (!is_room)
      return;

   // Skip base Room class
   if (_stricmp(c->class_name, "Room") == 0)
      return;

   room_class = c->class_name;

   // Find vrName
   cur = c;
   depth = 0;
   while (cur && depth < 50 && !room_name)
   {
      cv = cur->classvar_names;
      while (cv)
      {
         if (cv->name && _stricmp(cv->name, "vrName") == 0)
         {
            if (cur->vars && cv->id >= 0 && cv->id < cur->num_vars)
            {
               val_type val = cur->vars[cv->id].val;
               if (val.v.tag == TAG_RESOURCE && val.v.data > 0)
               {
                  res_id = val.v.data;
                  r = GetResourceByID(res_id);
                  if (r && r->resource_val && r->resource_val[0])
                     room_name = r->resource_val[0];
               }
            }
            break;
         }
         cv = cv->next;
      }
      cur = cur->super_ptr;
      depth++;
   }

   // Skip invalid names
   if (!room_name)
      return;
   if (room_name[0] == '\0')
      return;
   if (_stricmp(room_name, "room") == 0)
      return;
   if (_stricmp(room_name, "something") == 0)
      return;

   // Escape room_name
   esc_name[0] = '\0';
   for (i = 0, j = 0; room_name[i] && j < 60; i++)
   {
      if (room_name[i] == '\'' || room_name[i] == '\\')
      {
         esc_name[j++] = '\\';
      }
      esc_name[j++] = room_name[i];
   }
   esc_name[j] = '\0';

   // Escape room_class
   esc_class[0] = '\0';
   for (i = 0, j = 0; room_class[i] && j < 60; i++)
   {
      if (room_class[i] == '\'' || room_class[i] == '\\')
      {
         esc_class[j++] = '\\';
      }
      esc_class[j++] = room_class[i];
   }
   esc_class[j] = '\0';

   // Execute query
   snprintf(query, sizeof(query),
      "INSERT INTO room_templates (room_name, room_kod_class) "
      "VALUES ('%s', '%s') "
      "ON DUPLICATE KEY UPDATE room_name='%s'",
      esc_name, esc_class, esc_name);

   MySQLLock();
   if (mysql_query(mysql, query))
   {
      eprintf("MySQLExportRoomData: Failed %s: %s\n", esc_class, mysql_error(mysql));
   }
   else
   {
      export_room_count++;
   }
   MySQLUnlock();
}

int MySQLExportRoomData(void)
{
   if (!mysql)
   {
      eprintf("MySQLExportRoomData: MySQL not connected\n");
      return 0;
   }

   if (state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportRoomData: MySQL not ready\n");
      return 0;
   }

   export_room_count = 0;
   lprintf("MySQLExportRoomData: Starting Room export...\n");

   ForEachClass(ExportRoomCallback);

   lprintf("MySQLExportRoomData: Exported %d Rooms\n", export_room_count);
   return export_room_count;
}
