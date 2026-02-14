// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbnpc.c
 *
 * MySQL NPC Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_NPCTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `npc_templates` (" \
   "  `npc_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `npc_name` VARCHAR(100) NOT NULL," \
   "  `npc_kod_class` VARCHAR(100) NOT NULL," \
   "  `npc_icon` VARCHAR(100)," \
   "  `town` VARCHAR(50)," \
   "  `room_kod_class` VARCHAR(100)," \
   "  `occupation` INT DEFAULT 0," \
   "  `merchant_markup` INT DEFAULT 100," \
   "  `is_seller` TINYINT(1) DEFAULT 0," \
   "  `is_buyer` TINYINT(1) DEFAULT 0," \
   "  `is_teacher` TINYINT(1) DEFAULT 0," \
   "  `is_banker` TINYINT(1) DEFAULT 0," \
   "  `is_lawful` TINYINT(1) DEFAULT 1," \
   "  `faction` INT DEFAULT 0," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP," \
   "  `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`npc_template_id`)," \
   "  UNIQUE KEY `uk_npc_kod_class` (`npc_kod_class`)," \
   "  KEY `idx_npc_town` (`town`)," \
   "  KEY `idx_npc_occupation` (`occupation`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_npc_count = 0;

#define MAX_INHERITANCE_DEPTH 50

static Bool NPCClassInheritsFrom(class_node *c, const char *parent_name)
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

static int NPCGetClassVarInt(class_node *c, const char *var_name)
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

static const char* NPCGetClassVarResourceString(class_node *c, const char *var_name)
{
   int res_id = NPCGetClassVarInt(c, var_name);
   if (res_id > 0)
   {
      resource_node *r = GetResourceByID(res_id);
      if (r && r->resource_val[0])
         return r->resource_val[0];
   }
   return NULL;
}

static const char* DetermineTownFromClass(const char* class_name)
{
   if (!class_name) return "";
   if (_strnicmp(class_name, "Tos", 3) == 0) return "Tos";
   if (_strnicmp(class_name, "Barloque", 8) == 0) return "Barloque";
   if (_strnicmp(class_name, "Marion", 6) == 0) return "Marion";
   if (_strnicmp(class_name, "Jasper", 6) == 0) return "Jasper";
   if (_strnicmp(class_name, "CorNoth", 7) == 0) return "Cor Noth";
   if (_strnicmp(class_name, "Raza", 4) == 0) return "Raza";
   if (_strnicmp(class_name, "Kocatan", 7) == 0) return "Kocatan";
   return "";
}

static void EscapeString(char *dest, const char *src, size_t dest_size)
{
   size_t i, j;
   if (!src || !dest || dest_size < 3) { if (dest && dest_size > 0) dest[0] = '\0'; return; }
   for (i = 0, j = 0; src[i] && j < dest_size - 2; i++)
   {
      if (src[i] == '\'' || src[i] == '\\') dest[j++] = '\\';
      if (j < dest_size - 1) dest[j++] = src[i];
   }
   dest[j] = '\0';
}

static void ExportNPCClassToMySQL(class_node *c)
{
   char query[2048];
   const char *npc_name, *npc_icon, *npc_class, *town;
   char esc_name[256], esc_class[256], esc_icon[256], esc_town[128];

   if (!c || !c->class_name) return;
   npc_class = c->class_name;

   npc_name = NPCGetClassVarResourceString(c, "vrName");
   if (!npc_name || strlen(npc_name) == 0 || _stricmp(npc_name, "something") == 0) return;

   npc_icon = NPCGetClassVarResourceString(c, "vrIcon");
   if (!npc_icon) npc_icon = "";

   int occupation = NPCGetClassVarInt(c, "viOccupation");
   int merchant_markup = NPCGetClassVarInt(c, "viMerchant_markup");
   if (merchant_markup == 0) merchant_markup = 100;

   int attributes = NPCGetClassVarInt(c, "viAttributes");
   int is_seller = (attributes & 0x10) ? 1 : 0;
   int is_buyer = (attributes & 0x20) ? 1 : 0;
   int is_teacher = (attributes & 0x100) ? 1 : 0;
   int is_banker = (attributes & 0x1000) ? 1 : 0;
   int is_lawful = (attributes & 0x10000) ? 1 : 0;
   int faction = NPCGetClassVarInt(c, "viFaction");

   town = DetermineTownFromClass(npc_class);

   if (!mysql) return;

   EscapeString(esc_name, npc_name, sizeof(esc_name));
   EscapeString(esc_class, npc_class, sizeof(esc_class));
   EscapeString(esc_icon, npc_icon, sizeof(esc_icon));
   EscapeString(esc_town, town, sizeof(esc_town));

   snprintf(query, sizeof(query),
      "INSERT INTO npc_templates (npc_name, npc_kod_class, npc_icon, town, occupation, "
      "merchant_markup, is_seller, is_buyer, is_teacher, is_banker, is_lawful, faction) "
      "VALUES ('%s', '%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %d) "
      "ON DUPLICATE KEY UPDATE npc_name='%s', npc_icon='%s', town='%s', occupation=%d, "
      "merchant_markup=%d, is_seller=%d, is_buyer=%d, is_teacher=%d, is_banker=%d, is_lawful=%d, faction=%d",
      esc_name, esc_class, esc_icon, esc_town, occupation, merchant_markup,
      is_seller, is_buyer, is_teacher, is_banker, is_lawful, faction,
      esc_name, esc_icon, esc_town, occupation, merchant_markup,
      is_seller, is_buyer, is_teacher, is_banker, is_lawful, faction);

   MySQLLock();
   if (mysql_query(mysql, query))
      eprintf("ExportNPCClassToMySQL: Failed %s: %s\n", npc_class, mysql_error(mysql));
   else
      export_npc_count++;
   MySQLUnlock();
}

static void ExportNPCClassCallback(class_node *c)
{
   if (!c || !c->class_name) return;
   if (!NPCClassInheritsFrom(c, "Towns")) return;
   if (_stricmp(c->class_name, "Towns") == 0 || _stricmp(c->class_name, "Wanderer") == 0) return;

   ExportNPCClassToMySQL(c);
}

void _MySQLCreateNPCSchema(void)
{
   if (!mysql) { eprintf("_MySQLCreateNPCSchema: MySQL not connected\n"); return; }

   MySQLLock();

   lprintf("Creating NPC schema...\n");
   if (mysql_query(mysql, SQLQUERY_CREATETABLE_NPCTEMPLATES))
      eprintf("_MySQLCreateNPCSchema: Failed: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateNPCSchema: npc_templates table ready\n");
   lprintf("NPC schema creation complete.\n");

   MySQLUnlock();
}

int MySQLExportNPCData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportNPCData: MySQL not ready\n");
      return 0;
   }

   export_npc_count = 0;
   lprintf("MySQLExportNPCData: Starting NPC export...\n");

   ForEachClass(ExportNPCClassCallback);

   lprintf("MySQLExportNPCData: Exported %d NPCs\n", export_npc_count);
   return export_npc_count;
}
