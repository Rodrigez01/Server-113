// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * monsterloader.c
 *
 * MySQL Monster Template Loader
 * Loads monster templates from MySQL and provides them to the KOD system
 */

#include "blakserv.h"

// External variables
extern MYSQL* mysql;
extern sql_worker_state state;

// Forward declarations
static int LoadMonsterResistancesFromMySQL(void);
static int LoadMonsterSpellsFromMySQL(void);

/* Monster Template Cache */
#define MAX_MONSTER_TEMPLATES 500
static monster_template_t monster_templates[MAX_MONSTER_TEMPLATES];
static int num_monster_templates = 0;


/* ============================================================================
 * MONSTER TEMPLATE LOADING
 * ============================================================================
 */

/**
 * @brief Loads all monster templates from MySQL into memory
 *
 * Called during server startup to load all available monster templates.
 *
 * @return Number of loaded templates
 */
int LoadMonsterTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[28];  // 28 columns in query (was 24 - buffer overflow bug!)
   int result;

   if (!mysql)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading monster templates from MySQL...\n");

   MySQLLock();

   // Prepare statement
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query =
      "SELECT monster_template_id, monster_name, monster_kod_class, "
      "monster_icon, monster_dead_icon, level, difficulty, "
      "max_hit_points, max_mana, attack_type, attack_spell, attack_range, "
      "speed, vision_distance, cash_min, cash_max, treasure_type, "
      "default_behavior, wimpy, spell_chance, spell_power, "
      "karma, is_undead, is_npc, "
      "sound_hit, sound_miss, sound_aware, sound_death "
      "FROM monster_templates WHERE active = 1 ORDER BY monster_template_id";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: mysql_stmt_prepare() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Execute
   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: mysql_stmt_execute() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));

   int monster_id, level, difficulty, max_hp, max_mana;
   int attack_type, attack_spell, attack_range, speed, vision_dist;
   int cash_min, cash_max, treasure_type;
   int default_behavior, wimpy, spell_chance, spell_power;
   int karma;
   my_bool is_undead, is_npc;
   char name[101], kod_class[101], icon[101], dead_icon[101];
   char sound_hit[101], sound_miss[101], sound_aware[101], sound_death[101];
   my_bool is_null[28];
   unsigned long length[28];

   // monster_template_id
   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&monster_id;
   bind[0].is_null = &is_null[0];

   // monster_name
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = name;
   bind[1].buffer_length = sizeof(name);
   bind[1].is_null = &is_null[1];
   bind[1].length = &length[1];

   // monster_kod_class
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = kod_class;
   bind[2].buffer_length = sizeof(kod_class);
   bind[2].is_null = &is_null[2];
   bind[2].length = &length[2];

   // monster_icon
   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = icon;
   bind[3].buffer_length = sizeof(icon);
   bind[3].is_null = &is_null[3];
   bind[3].length = &length[3];

   // monster_dead_icon
   bind[4].buffer_type = MYSQL_TYPE_STRING;
   bind[4].buffer = dead_icon;
   bind[4].buffer_length = sizeof(dead_icon);
   bind[4].is_null = &is_null[4];
   bind[4].length = &length[4];

   // level
   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&level;
   bind[5].is_null = &is_null[5];

   // difficulty
   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&difficulty;
   bind[6].is_null = &is_null[6];

   // max_hit_points
   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&max_hp;
   bind[7].is_null = &is_null[7];

   // max_mana
   bind[8].buffer_type = MYSQL_TYPE_LONG;
   bind[8].buffer = (char*)&max_mana;
   bind[8].is_null = &is_null[8];

   // attack_type
   bind[9].buffer_type = MYSQL_TYPE_LONG;
   bind[9].buffer = (char*)&attack_type;
   bind[9].is_null = &is_null[9];

   // attack_spell
   bind[10].buffer_type = MYSQL_TYPE_LONG;
   bind[10].buffer = (char*)&attack_spell;
   bind[10].is_null = &is_null[10];

   // attack_range
   bind[11].buffer_type = MYSQL_TYPE_LONG;
   bind[11].buffer = (char*)&attack_range;
   bind[11].is_null = &is_null[11];

   // speed
   bind[12].buffer_type = MYSQL_TYPE_LONG;
   bind[12].buffer = (char*)&speed;
   bind[12].is_null = &is_null[12];

   // vision_distance
   bind[13].buffer_type = MYSQL_TYPE_LONG;
   bind[13].buffer = (char*)&vision_dist;
   bind[13].is_null = &is_null[13];

   // cash_min
   bind[14].buffer_type = MYSQL_TYPE_LONG;
   bind[14].buffer = (char*)&cash_min;
   bind[14].is_null = &is_null[14];

   // cash_max
   bind[15].buffer_type = MYSQL_TYPE_LONG;
   bind[15].buffer = (char*)&cash_max;
   bind[15].is_null = &is_null[15];

   // treasure_type
   bind[16].buffer_type = MYSQL_TYPE_LONG;
   bind[16].buffer = (char*)&treasure_type;
   bind[16].is_null = &is_null[16];

   // default_behavior
   bind[17].buffer_type = MYSQL_TYPE_LONG;
   bind[17].buffer = (char*)&default_behavior;
   bind[17].is_null = &is_null[17];

   // wimpy
   bind[18].buffer_type = MYSQL_TYPE_LONG;
   bind[18].buffer = (char*)&wimpy;
   bind[18].is_null = &is_null[18];

   // spell_chance
   bind[19].buffer_type = MYSQL_TYPE_LONG;
   bind[19].buffer = (char*)&spell_chance;
   bind[19].is_null = &is_null[19];

   // spell_power
   bind[20].buffer_type = MYSQL_TYPE_LONG;
   bind[20].buffer = (char*)&spell_power;
   bind[20].is_null = &is_null[20];

   // karma
   bind[21].buffer_type = MYSQL_TYPE_LONG;
   bind[21].buffer = (char*)&karma;
   bind[21].is_null = &is_null[21];

   // is_undead
   bind[22].buffer_type = MYSQL_TYPE_TINY;
   bind[22].buffer = (char*)&is_undead;
   bind[22].is_null = &is_null[22];

   // is_npc
   bind[23].buffer_type = MYSQL_TYPE_TINY;
   bind[23].buffer = (char*)&is_npc;
   bind[23].is_null = &is_null[23];

   // sound_hit
   bind[24].buffer_type = MYSQL_TYPE_STRING;
   bind[24].buffer = sound_hit;
   bind[24].buffer_length = sizeof(sound_hit);
   bind[24].is_null = &is_null[24];
   bind[24].length = &length[24];

   // sound_miss
   bind[25].buffer_type = MYSQL_TYPE_STRING;
   bind[25].buffer = sound_miss;
   bind[25].buffer_length = sizeof(sound_miss);
   bind[25].is_null = &is_null[25];
   bind[25].length = &length[25];

   // sound_aware
   bind[26].buffer_type = MYSQL_TYPE_STRING;
   bind[26].buffer = sound_aware;
   bind[26].buffer_length = sizeof(sound_aware);
   bind[26].is_null = &is_null[26];
   bind[26].length = &length[26];

   // sound_death
   bind[27].buffer_type = MYSQL_TYPE_STRING;
   bind[27].buffer = sound_death;
   bind[27].buffer_length = sizeof(sound_death);
   bind[27].is_null = &is_null[27];
   bind[27].length = &length[27];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadMonsterTemplatesFromMySQL: mysql_stmt_bind_result() failed: %s\n",
              mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Fetch rows
   num_monster_templates = 0;
   while (mysql_stmt_fetch(stmt) == 0 && num_monster_templates < MAX_MONSTER_TEMPLATES)
   {
      monster_template_t* mt = &monster_templates[num_monster_templates];

      mt->monster_template_id = monster_id;

      // Strings
      strncpy(mt->monster_name, name, 100);
      mt->monster_name[100] = '\0';

      if (!is_null[2])
      {
         strncpy(mt->monster_kod_class, kod_class, 100);
         mt->monster_kod_class[100] = '\0';
      }
      else
      {
         mt->monster_kod_class[0] = '\0';
      }

      if (!is_null[3])
      {
         strncpy(mt->monster_icon, icon, 100);
         mt->monster_icon[100] = '\0';
      }
      else
      {
         mt->monster_icon[0] = '\0';
      }

      if (!is_null[4])
      {
         strncpy(mt->monster_dead_icon, dead_icon, 100);
         mt->monster_dead_icon[100] = '\0';
      }
      else
      {
         mt->monster_dead_icon[0] = '\0';
      }

      // Combat stats
      mt->level = is_null[5] ? 25 : level;
      mt->difficulty = is_null[6] ? 0 : difficulty;
      mt->max_hit_points = is_null[7] ? 1 : max_hp;
      mt->max_mana = is_null[8] ? 0 : max_mana;

      // Combat properties
      mt->attack_type = is_null[9] ? 0 : attack_type;
      mt->attack_spell = is_null[10] ? 0 : attack_spell;
      mt->attack_range = is_null[11] ? 128 : attack_range;
      mt->speed = is_null[12] ? 0 : speed;
      mt->vision_distance = is_null[13] ? 10 : vision_dist;

      // Loot
      mt->cash_min = is_null[14] ? 1 : cash_min;
      mt->cash_max = is_null[15] ? 10 : cash_max;
      mt->treasure_type = is_null[16] ? 0 : treasure_type;

      // AI
      mt->default_behavior = is_null[17] ? 0 : default_behavior;
      mt->wimpy = is_null[18] ? 0 : wimpy;
      mt->spell_chance = is_null[19] ? 10 : spell_chance;
      mt->spell_power = is_null[20] ? 50 : spell_power;

      // Classification
      mt->karma = is_null[21] ? 0 : karma;
      mt->is_undead = is_null[22] ? 0 : is_undead;
      mt->is_npc = is_null[23] ? 0 : is_npc;

      // Sounds
      if (!is_null[24])
      {
         strncpy(mt->sound_hit, sound_hit, 100);
         mt->sound_hit[100] = '\0';
      }
      else
      {
         mt->sound_hit[0] = '\0';
      }

      if (!is_null[25])
      {
         strncpy(mt->sound_miss, sound_miss, 100);
         mt->sound_miss[100] = '\0';
      }
      else
      {
         mt->sound_miss[0] = '\0';
      }

      if (!is_null[26])
      {
         strncpy(mt->sound_aware, sound_aware, 100);
         mt->sound_aware[100] = '\0';
      }
      else
      {
         mt->sound_aware[0] = '\0';
      }

      if (!is_null[27])
      {
         strncpy(mt->sound_death, sound_death, 100);
         mt->sound_death[100] = '\0';
      }
      else
      {
         mt->sound_death[0] = '\0';
      }

      mt->active = 1;

      // Initialize arrays
      mt->num_resistances = 0;
      mt->num_spells = 0;

      dprintf("Loaded monster %d: %s (%s) Level %d\n",
              mt->monster_template_id, mt->monster_name, mt->monster_kod_class, mt->level);

      num_monster_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadMonsterTemplatesFromMySQL: Loaded %d monster templates\n", num_monster_templates);

   // Load resistances
   LoadMonsterResistancesFromMySQL();

   // Load spells
   LoadMonsterSpellsFromMySQL();

   return num_monster_templates;
}


/**
 * @brief Loads monster resistances from MySQL
 *
 * @return Number of loaded resistance entries
 */
static int LoadMonsterResistancesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[3];
   int result;
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

   const char* query = "SELECT monster_template_id, attack_type, resistance_value "
                       "FROM monster_resistances ORDER BY monster_template_id";

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

   int monster_id, attack_type, res_value;
   my_bool is_null[3];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&monster_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&attack_type;
   bind[1].is_null = &is_null[1];

   bind[2].buffer_type = MYSQL_TYPE_LONG;
   bind[2].buffer = (char*)&res_value;
   bind[2].is_null = &is_null[2];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0)
   {
      monster_template_t* mt = GetMonsterTemplateByID(monster_id);
      if (mt && mt->num_resistances < MAX_MONSTER_RESISTANCES)
      {
         mt->resistances[mt->num_resistances].attack_type = attack_type;
         mt->resistances[mt->num_resistances].value = res_value;
         mt->num_resistances++;
         count++;
      }
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   dprintf("LoadMonsterResistancesFromMySQL: Loaded %d resistance entries\n", count);
   return count;
}


/**
 * @brief Loads monster spells from MySQL
 *
 * @return Number of loaded spell entries
 */
static int LoadMonsterSpellsFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[4];
   int result;
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

   const char* query = "SELECT monster_template_id, spell_num, mana_cost, trigger_chance "
                       "FROM monster_spells ORDER BY monster_template_id";

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

   int monster_id, spell_num, mana_cost, trigger_chance;
   my_bool is_null[4];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&monster_id;
   bind[0].is_null = &is_null[0];

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&spell_num;
   bind[1].is_null = &is_null[1];

   bind[2].buffer_type = MYSQL_TYPE_LONG;
   bind[2].buffer = (char*)&mana_cost;
   bind[2].is_null = &is_null[2];

   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&trigger_chance;
   bind[3].is_null = &is_null[3];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0)
   {
      monster_template_t* mt = GetMonsterTemplateByID(monster_id);
      if (mt && mt->num_spells < MAX_MONSTER_SPELLS)
      {
         mt->spells[mt->num_spells].spell_num = spell_num;
         mt->spells[mt->num_spells].mana_cost = mana_cost;
         mt->spells[mt->num_spells].trigger_chance = trigger_chance;
         mt->num_spells++;
         count++;
      }
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   dprintf("LoadMonsterSpellsFromMySQL: Loaded %d spell entries\n", count);
   return count;
}


/* ============================================================================
 * MONSTER TEMPLATE ACCESS FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Finds a monster template by ID
 *
 * @param monster_id Monster Template ID
 * @return Pointer to monster template or NULL
 */
monster_template_t* GetMonsterTemplateByID(int monster_id)
{
   for (int i = 0; i < num_monster_templates; i++)
   {
      if (monster_templates[i].monster_template_id == monster_id)
      {
         return &monster_templates[i];
      }
   }
   return NULL;
}


/**
 * @brief Finds a monster template by KOD class name
 *
 * @param class_name KOD class name (e.g., "Orc", "Lich")
 * @return Pointer to monster template or NULL
 */
monster_template_t* GetMonsterTemplateByClass(const char* class_name)
{
   if (!class_name)
      return NULL;

   for (int i = 0; i < num_monster_templates; i++)
   {
      if (_stricmp(monster_templates[i].monster_kod_class, class_name) == 0)
      {
         return &monster_templates[i];
      }
   }
   return NULL;
}


/**
 * @brief Returns all available monster templates
 *
 * @param count Output: Number of templates
 * @return Array of monster templates
 */
monster_template_t* GetAllMonsterTemplates(int* count)
{
   *count = num_monster_templates;
   return monster_templates;
}


/**
 * @brief Checks if a monster is active
 *
 * @param monster_id Monster Template ID
 * @return 1 if active, 0 if not or not found
 */
int IsMonsterActive(int monster_id)
{
   monster_template_t* mt = GetMonsterTemplateByID(monster_id);
   if (!mt)
      return 0;

   return mt->active;
}


/**
 * @brief Returns monster templates by level range
 *
 * @param min_level Minimum level
 * @param max_level Maximum level
 * @param results Output: Array of monster IDs
 * @param max_results Maximum number of results
 * @return Number of found monsters
 */
int GetMonstersByLevel(int min_level, int max_level, int* results, int max_results)
{
   int count = 0;

   for (int i = 0; i < num_monster_templates && count < max_results; i++)
   {
      if (monster_templates[i].level >= min_level &&
          monster_templates[i].level <= max_level)
      {
         results[count++] = monster_templates[i].monster_template_id;
      }
   }

   return count;
}
