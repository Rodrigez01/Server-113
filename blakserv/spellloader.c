// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * spellloader.c
 *
 * Loads spell templates from MySQL database into memory
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define MAX_SPELL_TEMPLATES 500
static spell_template_t spell_templates[MAX_SPELL_TEMPLATES];
static int num_spell_templates = 0;

int LoadSpellTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[16];
   int result;

   num_spell_templates = 0;

   if (!mysql)
   {
      lprintf("LoadSpellTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      lprintf("LoadSpellTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading Spell templates from MySQL...\n");

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadSpellTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query =
      "SELECT spell_template_id, spell_name, spell_kod_class, spell_icon, "
      "spell_num, school, spell_level, mana_cost, exertion, "
      "cast_time, post_cast_time, chance_to_increase, meditate_ratio, "
      "is_harmful, is_outlaw, active "
      "FROM spell_templates WHERE active = 1 ORDER BY school, spell_level, spell_name";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadSpellTemplatesFromMySQL: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadSpellTemplatesFromMySQL: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));

   int spell_id, spell_num, school, spell_level, mana_cost, exertion;
   int cast_time, post_cast_time, chance_to_increase, meditate_ratio;
   my_bool is_harmful, is_outlaw, active;
   char name[101], kod_class[101], icon[101];
   my_bool is_null[16];
   unsigned long length[16];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&spell_id;
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

   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&spell_num;
   bind[4].is_null = &is_null[4];

   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&school;
   bind[5].is_null = &is_null[5];

   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&spell_level;
   bind[6].is_null = &is_null[6];

   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&mana_cost;
   bind[7].is_null = &is_null[7];

   bind[8].buffer_type = MYSQL_TYPE_LONG;
   bind[8].buffer = (char*)&exertion;
   bind[8].is_null = &is_null[8];

   bind[9].buffer_type = MYSQL_TYPE_LONG;
   bind[9].buffer = (char*)&cast_time;
   bind[9].is_null = &is_null[9];

   bind[10].buffer_type = MYSQL_TYPE_LONG;
   bind[10].buffer = (char*)&post_cast_time;
   bind[10].is_null = &is_null[10];

   bind[11].buffer_type = MYSQL_TYPE_LONG;
   bind[11].buffer = (char*)&chance_to_increase;
   bind[11].is_null = &is_null[11];

   bind[12].buffer_type = MYSQL_TYPE_LONG;
   bind[12].buffer = (char*)&meditate_ratio;
   bind[12].is_null = &is_null[12];

   bind[13].buffer_type = MYSQL_TYPE_TINY;
   bind[13].buffer = (char*)&is_harmful;
   bind[13].is_null = &is_null[13];

   bind[14].buffer_type = MYSQL_TYPE_TINY;
   bind[14].buffer = (char*)&is_outlaw;
   bind[14].is_null = &is_null[14];

   bind[15].buffer_type = MYSQL_TYPE_TINY;
   bind[15].buffer = (char*)&active;
   bind[15].is_null = &is_null[15];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadSpellTemplatesFromMySQL: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0 && num_spell_templates < MAX_SPELL_TEMPLATES)
   {
      spell_template_t* st = &spell_templates[num_spell_templates];

      st->spell_template_id = spell_id;

      strncpy(st->spell_name, name, 100);
      st->spell_name[100] = '\0';

      if (!is_null[2]) { strncpy(st->spell_kod_class, kod_class, 100); st->spell_kod_class[100] = '\0'; }
      else { st->spell_kod_class[0] = '\0'; }

      if (!is_null[3]) { strncpy(st->spell_icon, icon, 100); st->spell_icon[100] = '\0'; }
      else { st->spell_icon[0] = '\0'; }

      st->spell_description[0] = '\0';

      st->spell_num = is_null[4] ? 0 : spell_num;
      st->school = is_null[5] ? 1 : school;
      st->spell_level = is_null[6] ? 1 : spell_level;
      st->mana_cost = is_null[7] ? 1 : mana_cost;
      st->exertion = is_null[8] ? 1 : exertion;
      st->cast_time = is_null[9] ? 0 : cast_time;
      st->post_cast_time = is_null[10] ? 1 : post_cast_time;
      st->chance_to_increase = is_null[11] ? 20 : chance_to_increase;
      st->meditate_ratio = is_null[12] ? 100 : meditate_ratio;
      st->is_harmful = is_null[13] ? 0 : is_harmful;
      st->is_outlaw = is_null[14] ? 0 : is_outlaw;
      st->is_personal_ench = 1;
      st->active = is_null[15] ? 1 : active;

      st->num_reagents = 0;

      num_spell_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadSpellTemplatesFromMySQL: Loaded %d Spell templates\n", num_spell_templates);
   return num_spell_templates;
}

spell_template_t* GetSpellTemplateByID(int spell_id)
{
   for (int i = 0; i < num_spell_templates; i++)
   {
      if (spell_templates[i].spell_template_id == spell_id)
         return &spell_templates[i];
   }
   return NULL;
}

spell_template_t* GetSpellTemplateByClass(const char* class_name)
{
   if (!class_name)
      return NULL;

   for (int i = 0; i < num_spell_templates; i++)
   {
      if (_stricmp(spell_templates[i].spell_kod_class, class_name) == 0)
         return &spell_templates[i];
   }
   return NULL;
}

spell_template_t* GetSpellTemplateByNum(int spell_num)
{
   for (int i = 0; i < num_spell_templates; i++)
   {
      if (spell_templates[i].spell_num == spell_num)
         return &spell_templates[i];
   }
   return NULL;
}

spell_template_t* GetAllSpellTemplates(int* count)
{
   if (count)
      *count = num_spell_templates;
   return spell_templates;
}

int GetSpellsBySchool(int school, int* results, int max_results)
{
   int count = 0;

   if (!results || max_results <= 0)
      return 0;

   for (int i = 0; i < num_spell_templates && count < max_results; i++)
   {
      if (spell_templates[i].school == school)
      {
         results[count++] = spell_templates[i].spell_template_id;
      }
   }
   return count;
}
