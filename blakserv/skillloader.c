// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * skillloader.c
 *
 * Loads skill templates from MySQL database into memory
 */

#include "blakserv.h"

extern MYSQL* mysql;
extern sql_worker_state state;

#define MAX_SKILL_TEMPLATES 100
static skill_template_t skill_templates[MAX_SKILL_TEMPLATES];
static int num_skill_templates = 0;

int LoadSkillTemplatesFromMySQL(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[11];
   int result;

   num_skill_templates = 0;

   if (!mysql)
   {
      lprintf("LoadSkillTemplatesFromMySQL: MySQL not available\n");
      return 0;
   }

   if (state < CONNECTED)
   {
      lprintf("LoadSkillTemplatesFromMySQL: MySQL not connected yet\n");
      return 0;
   }

   lprintf("Loading Skill templates from MySQL...\n");

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      eprintf("LoadSkillTemplatesFromMySQL: mysql_stmt_init() failed\n");
      MySQLUnlock();
      return 0;
   }

   const char* query =
      "SELECT skill_template_id, skill_name, skill_kod_class, skill_icon, "
      "skill_num, school, skill_level, exertion, meditate_ratio, "
      "is_combat, active "
      "FROM skill_templates WHERE active = 1 ORDER BY school, skill_level, skill_name";

   result = mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query));
   if (result != 0)
   {
      eprintf("LoadSkillTemplatesFromMySQL: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("LoadSkillTemplatesFromMySQL: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   // Bind result columns
   memset(bind, 0, sizeof(bind));

   int skill_id, skill_num, school, skill_level, exertion, meditate_ratio;
   my_bool is_combat, active;
   char name[101], kod_class[101], icon[101];
   my_bool is_null[11];
   unsigned long length[11];

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&skill_id;
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
   bind[4].buffer = (char*)&skill_num;
   bind[4].is_null = &is_null[4];

   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&school;
   bind[5].is_null = &is_null[5];

   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&skill_level;
   bind[6].is_null = &is_null[6];

   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&exertion;
   bind[7].is_null = &is_null[7];

   bind[8].buffer_type = MYSQL_TYPE_LONG;
   bind[8].buffer = (char*)&meditate_ratio;
   bind[8].is_null = &is_null[8];

   bind[9].buffer_type = MYSQL_TYPE_TINY;
   bind[9].buffer = (char*)&is_combat;
   bind[9].is_null = &is_null[9];

   bind[10].buffer_type = MYSQL_TYPE_TINY;
   bind[10].buffer = (char*)&active;
   bind[10].is_null = &is_null[10];

   result = mysql_stmt_bind_result(stmt, bind);
   if (result != 0)
   {
      eprintf("LoadSkillTemplatesFromMySQL: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return 0;
   }

   while (mysql_stmt_fetch(stmt) == 0 && num_skill_templates < MAX_SKILL_TEMPLATES)
   {
      skill_template_t* st = &skill_templates[num_skill_templates];

      st->skill_template_id = skill_id;

      strncpy(st->skill_name, name, 100);
      st->skill_name[100] = '\0';

      if (!is_null[2]) { strncpy(st->skill_kod_class, kod_class, 100); st->skill_kod_class[100] = '\0'; }
      else { st->skill_kod_class[0] = '\0'; }

      if (!is_null[3]) { strncpy(st->skill_icon, icon, 100); st->skill_icon[100] = '\0'; }
      else { st->skill_icon[0] = '\0'; }

      st->skill_num = is_null[4] ? 0 : skill_num;
      st->school = is_null[5] ? 1 : school;
      st->skill_level = is_null[6] ? 1 : skill_level;
      st->exertion = is_null[7] ? 1 : exertion;
      st->meditate_ratio = is_null[8] ? 100 : meditate_ratio;
      st->is_combat = is_null[9] ? 0 : is_combat;
      st->active = is_null[10] ? 1 : active;

      num_skill_templates++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();

   lprintf("LoadSkillTemplatesFromMySQL: Loaded %d Skill templates\n", num_skill_templates);
   return num_skill_templates;
}

skill_template_t* GetSkillTemplateByID(int skill_id)
{
   for (int i = 0; i < num_skill_templates; i++)
   {
      if (skill_templates[i].skill_template_id == skill_id)
         return &skill_templates[i];
   }
   return NULL;
}

skill_template_t* GetSkillTemplateByClass(const char* class_name)
{
   if (!class_name)
      return NULL;

   for (int i = 0; i < num_skill_templates; i++)
   {
      if (_stricmp(skill_templates[i].skill_kod_class, class_name) == 0)
         return &skill_templates[i];
   }
   return NULL;
}

skill_template_t* GetSkillTemplateByNum(int skill_num)
{
   for (int i = 0; i < num_skill_templates; i++)
   {
      if (skill_templates[i].skill_num == skill_num)
         return &skill_templates[i];
   }
   return NULL;
}

skill_template_t* GetAllSkillTemplates(int* count)
{
   if (count)
      *count = num_skill_templates;
   return skill_templates;
}

int GetSkillsBySchool(int school, int* results, int max_results)
{
   int count = 0;

   if (!results || max_results <= 0)
      return 0;

   for (int i = 0; i < num_skill_templates && count < max_results; i++)
   {
      if (skill_templates[i].school == school)
      {
         results[count++] = skill_templates[i].skill_template_id;
      }
   }
   return count;
}
