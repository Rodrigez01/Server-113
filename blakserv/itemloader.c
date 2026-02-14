// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * itemloader.c
 *
 * MySQL Item Template Loader
 * Loads item templates from MySQL and provides them to the KOD system
 */

#include "blakserv.h"

// External variables
extern MYSQL* mysql;
extern sql_worker_state state;

// Forward declarations
int LoadItemWeaponsFromMySQL(void);
int LoadItemDefenseFromMySQL(void);
int LoadItemStackableFromMySQL(void);

/* Item Template Cache */
#define MAX_ITEM_TEMPLATES 500
static item_template_t item_templates[MAX_ITEM_TEMPLATES];
static int num_item_templates = 0;

/* Item Weapon Cache */
#define MAX_ITEM_WEAPONS 200
static item_weapon_t item_weapons[MAX_ITEM_WEAPONS];
static int num_item_weapons = 0;

/* Item Defense Cache */
#define MAX_ITEM_DEFENSE 200
static item_defense_t item_defense[MAX_ITEM_DEFENSE];
static int num_item_defense = 0;

/* Item Stackable Cache */
#define MAX_ITEM_STACKABLE 200
static item_stackable_t item_stackable[MAX_ITEM_STACKABLE];
static int num_item_stackable = 0;

/* Category name to enum mapping */
static item_category_t ParseItemCategory(const char* category)
{
   if (!category) return ITEM_CAT_MISC;

   if (strcmp(category, "weapon") == 0) return ITEM_CAT_WEAPON;
   if (strcmp(category, "armor") == 0) return ITEM_CAT_ARMOR;
   if (strcmp(category, "shield") == 0) return ITEM_CAT_SHIELD;
   if (strcmp(category, "helmet") == 0) return ITEM_CAT_HELMET;
   if (strcmp(category, "pants") == 0) return ITEM_CAT_PANTS;
   if (strcmp(category, "shirt") == 0) return ITEM_CAT_SHIRT;
   if (strcmp(category, "stackable") == 0) return ITEM_CAT_STACKABLE;
   if (strcmp(category, "consumable") == 0) return ITEM_CAT_CONSUMABLE;
   if (strcmp(category, "quest_item") == 0) return ITEM_CAT_QUEST_ITEM;

   return ITEM_CAT_MISC;
}

/* Weapon type string to enum */
static weapon_type_t ParseWeaponType(const char* type)
{
   if (!type) return WEAPON_TYPE_THRUST;

   if (strcmp(type, "thrust") == 0) return WEAPON_TYPE_THRUST;
   if (strcmp(type, "slash") == 0) return WEAPON_TYPE_SLASH;
   if (strcmp(type, "bludgeon") == 0) return WEAPON_TYPE_BLUDGEON;

   return WEAPON_TYPE_THRUST;
}

/* Weapon quality string to enum */
static weapon_quality_t ParseWeaponQuality(const char* quality)
{
   if (!quality) return WEAPON_QUALITY_NORMAL;

   if (strcmp(quality, "low") == 0) return WEAPON_QUALITY_LOW;
   if (strcmp(quality, "normal") == 0) return WEAPON_QUALITY_NORMAL;
   if (strcmp(quality, "high") == 0) return WEAPON_QUALITY_HIGH;
   if (strcmp(quality, "nerudite") == 0) return WEAPON_QUALITY_NERUDITE;

   return WEAPON_QUALITY_NORMAL;
}


/* ============================================================================
 * ITEM TEMPLATE LOADING
 * ============================================================================
 */

/**
 * @brief Loads all item templates from MySQL into memory
 *
 * Called during server startup to load all available item templates.
 *
 * @return Number of loaded templates
 */
int LoadItemTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[13];
   int result;

   if (!mysql)
   {
      eprintf("LoadItemTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      eprintf("LoadItemTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading item templates from MySQL...\n");

   MySQLLock();

   // Prepare statement
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadItemTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT item_template_id, item_name, item_kod_class, "
                       "item_description, item_icon, weight, bulk, value_average, "
                       "hits_init_min, hits_init_max, use_type, item_type, "
                       "item_category "
                       "FROM item_templates WHERE active = 1 ORDER BY item_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadItemTemplatesFromMySQL: mysql_stmt_prepare() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Execute
   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadItemTemplatesFromMySQL: mysql_stmt_execute() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));

   int item_id;
   char name[101], kod_class[101], description[256], icon[101], category[20];
   int weight, bulk, value_avg, hits_min, hits_max, use_type, item_type;
   my_bool is_null[13];
   unsigned long length[13];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&item_id;
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
   bind[4].buffer = icon;
   bind[4].buffer_length = sizeof(icon);
   bind[4].is_null = &is_null[4];
   bind[4].length = &length[4];

   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&weight;
   bind[5].is_null = &is_null[5];

   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&bulk;
   bind[6].is_null = &is_null[6];

   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&value_avg;
   bind[7].is_null = &is_null[7];

   bind[8].buffer_type = MYSQL_TYPE_LONG;
   bind[8].buffer = (char*)&hits_min;
   bind[8].is_null = &is_null[8];

   bind[9].buffer_type = MYSQL_TYPE_LONG;
   bind[9].buffer = (char*)&hits_max;
   bind[9].is_null = &is_null[9];

   bind[10].buffer_type = MYSQL_TYPE_LONG;
   bind[10].buffer = (char*)&use_type;
   bind[10].is_null = &is_null[10];

   bind[11].buffer_type = MYSQL_TYPE_LONG;
   bind[11].buffer = (char*)&item_type;
   bind[11].is_null = &is_null[11];

   bind[12].buffer_type = MYSQL_TYPE_STRING;
   bind[12].buffer = category;
   bind[12].buffer_length = sizeof(category);
   bind[12].is_null = &is_null[12];
   bind[12].length = &length[12];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadItemTemplatesFromMySQL: mysql_stmt_bind_result() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Fetch rows
   num_item_templates = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_item_templates < MAX_ITEM_TEMPLATES)
   {
      item_template_t* it = &item_templates[num_item_templates];

      it->item_template_id = item_id;

      strncpy(it->item_name, name, 100);
      it->item_name[100] = '\0';

      if (!is_null[2])
      {
         strncpy(it->item_kod_class, kod_class, 100);
         it->item_kod_class[100] = '\0';
      }
      else
      {
         it->item_kod_class[0] = '\0';
      }

      if (!is_null[3])
      {
         strncpy(it->item_description, description, 255);
         it->item_description[255] = '\0';
      }
      else
      {
         it->item_description[0] = '\0';
      }

      if (!is_null[4])
      {
         strncpy(it->item_icon, icon, 100);
         it->item_icon[100] = '\0';
      }
      else
      {
         it->item_icon[0] = '\0';
      }

      it->weight = is_null[5] ? 0 : weight;
      it->bulk = is_null[6] ? 0 : bulk;
      it->value_average = is_null[7] ? 0 : value_avg;
      it->hits_init_min = is_null[8] ? 0 : hits_min;
      it->hits_init_max = is_null[9] ? 0 : hits_max;
      it->use_type = is_null[10] ? 0 : use_type;
      it->item_type = is_null[11] ? 0 : item_type;

      if (!is_null[12])
      {
         category[length[12]] = '\0';
         it->item_category = ParseItemCategory(category);
      }
      else
      {
         it->item_category = ITEM_CAT_MISC;
      }

      it->active = 1;

      dprintf("Loaded item %d: %s (%s)\n",
              it->item_template_id, it->item_name, it->item_kod_class);

      num_item_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadItemTemplatesFromMySQL: Loaded %d item templates\n", num_item_templates);

   // Load weapon data
   LoadItemWeaponsFromMySQL();

   // Load defense data
   LoadItemDefenseFromMySQL();

   // Load stackable data
   LoadItemStackableFromMySQL();

   return num_item_templates;
}


/**
 * @brief Loads weapon-specific data for items
 *
 * @return Number of loaded weapon entries
 */
int LoadItemWeaponsFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[6];
   int result;

   if (!mysql || state < CONNECTED)
      return 0;

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT item_template_id, weapon_type, weapon_quality, "
                       "proficiency_needed, weapon_range, attack_type "
                       "FROM item_weapons ORDER BY item_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(bind, 0, sizeof(bind));

   int item_id, prof_needed, weapon_range, attack_type;
   char weapon_type[20], weapon_quality[20];
   my_bool is_null[6];
   unsigned long length[6];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&item_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = weapon_type;
   bind[1].buffer_length = sizeof(weapon_type);
   bind[1].is_null = &is_null[1];
   bind[1].length = &length[1];

   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = weapon_quality;
   bind[2].buffer_length = sizeof(weapon_quality);
   bind[2].is_null = &is_null[2];
   bind[2].length = &length[2];

   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&prof_needed;
   bind[3].is_null = &is_null[3];

   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&weapon_range;
   bind[4].is_null = &is_null[4];

   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&attack_type;
   bind[5].is_null = &is_null[5];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   num_item_weapons = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_item_weapons < MAX_ITEM_WEAPONS)
   {
      item_weapon_t* iw = &item_weapons[num_item_weapons];

      iw->item_template_id = item_id;

      if (!is_null[1])
      {
         weapon_type[length[1]] = '\0';
         iw->weapon_type = ParseWeaponType(weapon_type);
      }
      else
      {
         iw->weapon_type = WEAPON_TYPE_THRUST;
      }

      if (!is_null[2])
      {
         weapon_quality[length[2]] = '\0';
         iw->weapon_quality = ParseWeaponQuality(weapon_quality);
      }
      else
      {
         iw->weapon_quality = WEAPON_QUALITY_NORMAL;
      }

      iw->proficiency_needed = is_null[3] ? 0 : prof_needed;
      iw->weapon_range = is_null[4] ? 192 : weapon_range;  // Default 3 * FINENESS
      iw->attack_type = is_null[5] ? 0 : attack_type;

      num_item_weapons++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   dprintf("LoadItemWeaponsFromMySQL: Loaded %d weapon entries\n", num_item_weapons);
   return num_item_weapons;
}


/**
 * @brief Loads defense-specific data for items
 *
 * @return Number of loaded defense entries
 */
int LoadItemDefenseFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[5];
   int result;

   if (!mysql || state < CONNECTED)
      return 0;

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT item_template_id, defense_base, damage_base, "
                       "spell_modifier, layer "
                       "FROM item_defense ORDER BY item_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(bind, 0, sizeof(bind));

   int item_id, def_base, dmg_base, spell_mod, layer;
   my_bool is_null[5];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&item_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&def_base;
   bind[1].is_null = &is_null[1];

   bind[2].buffer_type = MYSQL_TYPE_LONG;
   bind[2].buffer = (char*)&dmg_base;
   bind[2].is_null = &is_null[2];

   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&spell_mod;
   bind[3].is_null = &is_null[3];

   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&layer;
   bind[4].is_null = &is_null[4];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   num_item_defense = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_item_defense < MAX_ITEM_DEFENSE)
   {
      item_defense_t* id = &item_defense[num_item_defense];

      id->item_template_id = item_id;
      id->defense_base = is_null[1] ? 0 : def_base;
      id->damage_base = is_null[2] ? 0 : dmg_base;
      id->spell_modifier = is_null[3] ? 0 : spell_mod;
      id->layer = is_null[4] ? 0 : layer;

      // Initialize resistances to 0
      memset(id->resistances, 0, sizeof(id->resistances));

      num_item_defense++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   dprintf("LoadItemDefenseFromMySQL: Loaded %d defense entries\n", num_item_defense);
   return num_item_defense;
}


/**
 * @brief Loads stackable-specific data for items
 *
 * @return Number of loaded stackable entries
 */
int LoadItemStackableFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[3];
   int result;

   if (!mysql || state < CONNECTED)
      return 0;

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return 0;
   }

   const char* query = "SELECT item_template_id, max_stack, is_reagent "
                       "FROM item_stackable ORDER BY item_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   memset(bind, 0, sizeof(bind));

   int item_id, max_stack;
   my_bool is_reagent;
   my_bool is_null[3];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&item_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&max_stack;
   bind[1].is_null = &is_null[1];

   bind[2].buffer_type = MYSQL_TYPE_TINY;
   bind[2].buffer = (char*)&is_reagent;
   bind[2].is_null = &is_null[2];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   num_item_stackable = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_item_stackable < MAX_ITEM_STACKABLE)
   {
      item_stackable_t* is = &item_stackable[num_item_stackable];

      is->item_template_id = item_id;
      is->max_stack = is_null[1] ? 9999 : max_stack;
      is->is_reagent = is_null[2] ? 0 : is_reagent;

      num_item_stackable++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   dprintf("LoadItemStackableFromMySQL: Loaded %d stackable entries\n", num_item_stackable);
   return num_item_stackable;
}


/* ============================================================================
 * ITEM TEMPLATE ACCESS FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Finds an item template by ID
 *
 * @param item_id Item Template ID
 * @return Pointer to item template or NULL
 */
item_template_t* GetItemTemplateByID(int item_id)
{
   for (int i = 0; i < num_item_templates; i++)
   {
      if (item_templates[i].item_template_id == item_id)
      {
         return &item_templates[i];
      }
   }
   return NULL;
}


/**
 * @brief Finds an item template by KOD class name
 *
 * @param class_name KOD class name (e.g., "LongSword")
 * @return Pointer to item template or NULL
 */
item_template_t* GetItemTemplateByClass(const char* class_name)
{
   if (!class_name)
      return NULL;

   for (int i = 0; i < num_item_templates; i++)
   {
      if (_stricmp(item_templates[i].item_kod_class, class_name) == 0)
      {
         return &item_templates[i];
      }
   }
   return NULL;
}


/**
 * @brief Returns all available item templates
 *
 * @param count Output: Number of templates
 * @return Array of item templates
 */
item_template_t* GetAllItemTemplates(int* count)
{
   *count = num_item_templates;
   return item_templates;
}


/**
 * @brief Checks if an item is active
 *
 * @param item_id Item Template ID
 * @return 1 if active, 0 if not or not found
 */
int IsItemActive(int item_id)
{
   item_template_t* it = GetItemTemplateByID(item_id);
   if (!it)
      return 0;

   return it->active;
}


/**
 * @brief Returns item templates by category
 *
 * @param category Item category
 * @param results Output: Array of item IDs
 * @param max_results Maximum number of results
 * @return Number of found items
 */
int GetItemsByCategory(item_category_t category, int* results, int max_results)
{
   int count = 0;

   for (int i = 0; i < num_item_templates && count < max_results; i++)
   {
      if (item_templates[i].item_category == category)
      {
         results[count++] = item_templates[i].item_template_id;
      }
   }

   return count;
}


/**
 * @brief Gets weapon-specific data for an item
 *
 * @param item_id Item Template ID
 * @return Pointer to weapon data or NULL
 */
item_weapon_t* GetItemWeaponData(int item_id)
{
   for (int i = 0; i < num_item_weapons; i++)
   {
      if (item_weapons[i].item_template_id == item_id)
      {
         return &item_weapons[i];
      }
   }
   return NULL;
}


/**
 * @brief Gets defense-specific data for an item
 *
 * @param item_id Item Template ID
 * @return Pointer to defense data or NULL
 */
item_defense_t* GetItemDefenseData(int item_id)
{
   for (int i = 0; i < num_item_defense; i++)
   {
      if (item_defense[i].item_template_id == item_id)
      {
         return &item_defense[i];
      }
   }
   return NULL;
}


/**
 * @brief Gets stackable-specific data for an item
 *
 * @param item_id Item Template ID
 * @return Pointer to stackable data or NULL
 */
item_stackable_t* GetItemStackableData(int item_id)
{
   for (int i = 0; i < num_item_stackable; i++)
   {
      if (item_stackable[i].item_template_id == item_id)
      {
         return &item_stackable[i];
      }
   }
   return NULL;
}
