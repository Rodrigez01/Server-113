// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbitem.c
 *
 * MySQL Item Database Functions (using mysql_query, not prepared statements)
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define SQLQUERY_CREATETABLE_ITEMTEMPLATES \
   "CREATE TABLE IF NOT EXISTS `item_templates` (" \
   "  `item_template_id` INT NOT NULL AUTO_INCREMENT," \
   "  `item_name` VARCHAR(100) NOT NULL," \
   "  `item_name_de` VARCHAR(100) DEFAULT NULL," \
   "  `item_kod_class` VARCHAR(100) NOT NULL," \
   "  `item_description` TEXT DEFAULT NULL," \
   "  `item_desc_de` TEXT DEFAULT NULL," \
   "  `item_icon` VARCHAR(100) DEFAULT NULL," \
   "  `weight` INT DEFAULT 0," \
   "  `bulk` INT DEFAULT 0," \
   "  `value_average` INT DEFAULT 0," \
   "  `hits_init_min` INT DEFAULT 0," \
   "  `hits_init_max` INT DEFAULT 0," \
   "  `use_type` INT DEFAULT 0," \
   "  `item_type` INT DEFAULT 0," \
   "  `item_category` ENUM('weapon','armor','shield','helmet','pants','shirt','stackable','consumable','quest_item','misc') NOT NULL DEFAULT 'misc'," \
   "  `active` TINYINT(1) DEFAULT 1," \
   "  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP," \
   "  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP," \
   "  PRIMARY KEY (`item_template_id`)," \
   "  UNIQUE KEY `uk_item_kod_class` (`item_kod_class`)," \
   "  KEY `idx_item_category` (`item_category`)," \
   "  KEY `idx_item_active` (`active`)" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;"

#define SQLQUERY_CREATETABLE_ITEMWEAPONS \
   "CREATE TABLE IF NOT EXISTS `item_weapons` (" \
   "  `item_template_id` INT NOT NULL," \
   "  `weapon_type` VARCHAR(20) DEFAULT 'thrust'," \
   "  `weapon_quality` VARCHAR(20) DEFAULT 'normal'," \
   "  `proficiency_needed` INT DEFAULT 0," \
   "  `weapon_range` INT DEFAULT 192," \
   "  `attack_type` INT DEFAULT 0," \
   "  PRIMARY KEY (`item_template_id`)," \
   "  CONSTRAINT `fk_weapon_template` FOREIGN KEY (`item_template_id`)" \
   "    REFERENCES `item_templates` (`item_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

#define SQLQUERY_CREATETABLE_ITEMDEFENSE \
   "CREATE TABLE IF NOT EXISTS `item_defense` (" \
   "  `item_template_id` INT NOT NULL," \
   "  `defense_base` INT DEFAULT 0," \
   "  `damage_base` INT DEFAULT 0," \
   "  `spell_modifier` INT DEFAULT 0," \
   "  `layer` INT DEFAULT 0," \
   "  PRIMARY KEY (`item_template_id`)," \
   "  CONSTRAINT `fk_defense_template` FOREIGN KEY (`item_template_id`)" \
   "    REFERENCES `item_templates` (`item_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

#define SQLQUERY_CREATETABLE_ITEMSTACKABLE \
   "CREATE TABLE IF NOT EXISTS `item_stackable` (" \
   "  `item_template_id` INT NOT NULL," \
   "  `max_stack` INT DEFAULT 9999," \
   "  `is_reagent` TINYINT(1) DEFAULT 0," \
   "  PRIMARY KEY (`item_template_id`)," \
   "  CONSTRAINT `fk_stackable_template` FOREIGN KEY (`item_template_id`)" \
   "    REFERENCES `item_templates` (`item_template_id`) ON DELETE CASCADE" \
   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"

static int export_item_count = 0;
static int export_weapon_count = 0;
static int export_defense_count = 0;
static int export_stackable_count = 0;

#define MAX_INHERITANCE_DEPTH 50

static Bool ItemClassInheritsFrom(class_node *c, const char *parent_name)
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

static int ItemGetClassVarInt(class_node *c, const char *var_name)
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

static const char* ItemGetClassVarResourceString(class_node *c, const char *var_name)
{
   int res_id = ItemGetClassVarInt(c, var_name);
   if (res_id > 0)
   {
      resource_node *r = GetResourceByID(res_id);
      if (r && r->resource_val[0])
         return r->resource_val[0];
   }
   return NULL;
}

static void EscapeStringItem(char *dest, const char *src, size_t dest_size)
{
   size_t i, j;

   if (!dest || dest_size < 1)
      return;

   dest[0] = '\0';

   if (!src || dest_size < 3)
      return;

   for (i = 0, j = 0; src[i] && j < dest_size - 2; i++)
   {
      if (src[i] == '\'' || src[i] == '\\' || src[i] == '"')
      {
         dest[j++] = '\\';
      }
      dest[j++] = src[i];
   }
   dest[j] = '\0';
}

static const char* DetermineItemCategory(class_node *c)
{
   if (ItemClassInheritsFrom(c, "Weapon") || ItemClassInheritsFrom(c, "Ranged"))
      return "weapon";
   if (ItemClassInheritsFrom(c, "Armor"))
      return "armor";
   if (ItemClassInheritsFrom(c, "Shield"))
      return "shield";
   if (ItemClassInheritsFrom(c, "Helmet"))
      return "helmet";
   if (ItemClassInheritsFrom(c, "Pants"))
      return "pants";
   if (ItemClassInheritsFrom(c, "ShirtBase") || ItemClassInheritsFrom(c, "Shirt"))
      return "shirt";
   if (ItemClassInheritsFrom(c, "NumberItem"))
      return "stackable";
   if (ItemClassInheritsFrom(c, "Food") || ItemClassInheritsFrom(c, "Potion"))
      return "consumable";
   return "misc";
}

static const char* GetWeaponTypeString(int weapon_type)
{
   switch (weapon_type)
   {
      case 1: return "thrust";
      case 2: return "slash";
      case 3: return "bludgeon";
      default: return "thrust";
   }
}

static const char* GetWeaponQualityString(int quality)
{
   switch (quality)
   {
      case 1: return "low";
      case 2: return "normal";
      case 3: return "high";
      case 4: return "nerudite";
      default: return "normal";
   }
}

static void ExportItemClassToMySQL(class_node *c)
{
   char query[4096];
   const char *item_name_raw, *item_icon_raw, *item_class, *category;
   char esc_name[512], esc_class[256], esc_icon[512];
   int weight, bulk, value_average, hits_min, hits_max, use_type, item_type;

   if (!c || !c->class_name)
      return;

   item_class = c->class_name;

   // Get item name - skip if no valid name
   item_name_raw = ItemGetClassVarResourceString(c, "vrName");
   if (!item_name_raw) return;
   if (strlen(item_name_raw) == 0) return;
   if (_stricmp(item_name_raw, "item") == 0) return;
   if (_stricmp(item_name_raw, "something") == 0) return;

   // Get icon (optional)
   item_icon_raw = ItemGetClassVarResourceString(c, "vrIcon");
   if (!item_icon_raw) item_icon_raw = "";

   // Get category
   category = DetermineItemCategory(c);

   // Get integer values
   weight = ItemGetClassVarInt(c, "viWeight");
   bulk = ItemGetClassVarInt(c, "viBulk");
   value_average = ItemGetClassVarInt(c, "viValue_average");
   hits_min = ItemGetClassVarInt(c, "viHits_init_min");
   hits_max = ItemGetClassVarInt(c, "viHits_init_max");
   use_type = ItemGetClassVarInt(c, "viUse_type");
   item_type = ItemGetClassVarInt(c, "viItem_type");

   // Escape strings
   EscapeStringItem(esc_name, item_name_raw, sizeof(esc_name));
   EscapeStringItem(esc_class, item_class, sizeof(esc_class));
   EscapeStringItem(esc_icon, item_icon_raw, sizeof(esc_icon));

   // Build and execute query
   snprintf(query, sizeof(query),
      "INSERT INTO item_templates (item_name, item_kod_class, item_icon, "
      "weight, bulk, value_average, hits_init_min, hits_init_max, "
      "use_type, item_type, item_category) "
      "VALUES ('%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, '%s') "
      "ON DUPLICATE KEY UPDATE item_name='%s', item_icon='%s', "
      "weight=%d, bulk=%d, value_average=%d, hits_init_min=%d, hits_init_max=%d, "
      "use_type=%d, item_type=%d, item_category='%s'",
      esc_name, esc_class, esc_icon,
      weight, bulk, value_average, hits_min, hits_max, use_type, item_type, category,
      esc_name, esc_icon,
      weight, bulk, value_average, hits_min, hits_max, use_type, item_type, category);

   MySQLLock();
   if (mysql_query(mysql, query) != 0)
   {
      eprintf("ExportItemClassToMySQL: query failed for %s: %s\n", item_class, mysql_error(mysql));
      MySQLUnlock();
      return;
   }
   MySQLUnlock();

   export_item_count++;

   // TODO: Sub-tables temporarily disabled for debugging
   // Weapon, Defense, Stackable exports commented out
}

static void ExportItemClassCallback(class_node *c)
{
   if (!c || !c->class_name)
      return;

   if (!ItemClassInheritsFrom(c, "Item"))
      return;

   // Skip abstract base classes
   if (_stricmp(c->class_name, "Item") == 0) return;
   if (_stricmp(c->class_name, "PassiveItem") == 0) return;
   if (_stricmp(c->class_name, "ActiveItem") == 0) return;
   if (_stricmp(c->class_name, "PassiveObject") == 0) return;
   if (_stricmp(c->class_name, "Weapon") == 0) return;
   if (_stricmp(c->class_name, "DefenseModifier") == 0) return;
   if (_stricmp(c->class_name, "NumberItem") == 0) return;

   ExportItemClassToMySQL(c);
}

void _MySQLCreateItemSchema(void)
{
   if (!mysql) { eprintf("_MySQLCreateItemSchema: MySQL not connected\n"); return; }

   MySQLLock();

   lprintf("Creating Item schema...\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ITEMTEMPLATES))
      eprintf("_MySQLCreateItemSchema: Failed item_templates: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateItemSchema: item_templates table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ITEMWEAPONS))
      eprintf("_MySQLCreateItemSchema: Failed item_weapons: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateItemSchema: item_weapons table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ITEMDEFENSE))
      eprintf("_MySQLCreateItemSchema: Failed item_defense: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateItemSchema: item_defense table ready\n");

   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ITEMSTACKABLE))
      eprintf("_MySQLCreateItemSchema: Failed item_stackable: %s\n", mysql_error(mysql));
   else
      dprintf("_MySQLCreateItemSchema: item_stackable table ready\n");

   lprintf("Item schema creation complete.\n");

   MySQLUnlock();
}

int MySQLExportItemData(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
   {
      eprintf("MySQLExportItemData: MySQL not available\n");
      return 0;
   }

   lprintf("MySQLExportItemData: Starting Item export...\n");

   export_item_count = 0;
   export_weapon_count = 0;
   export_defense_count = 0;
   export_stackable_count = 0;

   ForEachClass(ExportItemClassCallback);

   lprintf("MySQLExportItemData: Exported %d Items (%d weapons, %d defense, %d stackable)\n",
           export_item_count, export_weapon_count, export_defense_count, export_stackable_count);

   return export_item_count;
}
