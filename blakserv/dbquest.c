// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbquest.c
 *
 * MySQL Quest System Integration
 * Handles quest history and active quests in MySQL database
 */

#include "blakserv.h"

// External variables from database.c
extern MYSQL* mysql;
extern sql_worker_state state;

/* Forward declarations */
void _MySQLCreateQuestSchema(void);


/* ============================================================================
 * QUEST HISTORY Functions
 * ============================================================================
 */

/**
 * MySQLSaveQuestHistory - Speichert Quest-History in MySQL
 * @param account_id Account-ID
 * @param quest_template_id Quest Template ID (QST_ID)
 * @param quest_name Quest-Name (kann NULL sein)
 * @param status "success" oder "failure"
 * @param success_time Zeit bei Erfolg (logged in time)
 * @param failure_time Zeit bei Fehlschlag (logged in time)
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLSaveQuestHistory(int account_id, int quest_template_id, const char* quest_name,
                           const char* status, int success_time, int failure_time)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[6];
   int result;
   unsigned long name_len, status_len;

   if (!mysql || state < SCHEMAVERIFIED)
   {
      dprintf("MySQLSaveQuestHistory: MySQL not available\n");
      return FALSE;
   }

   if (!status)
      return FALSE;

   // Quest-Name kann NULL sein
   if (!quest_name)
      quest_name = "";

   dprintf("MySQLSaveQuestHistory: Saving quest %d for account %d (status: %s)\n", 
           quest_template_id, account_id, status);

   // Statement initialisieren
   MySQLLock();
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   // CALL SaveQuestHistory(?,?,?,?,?,?)
   const char* query = "CALL SaveQuestHistory(?,?,?,?,?,?)";
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("MySQLSaveQuestHistory: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // String-Längen
   name_len = strlen(quest_name);
   status_len = strlen(status);

   // Bind-Parameter vorbereiten
   memset(bind, 0, sizeof(bind));

   // p_account_id
   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&account_id;

   // p_quest_template_id
   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&quest_template_id;

   // p_status
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = (char*)status;
   bind[2].buffer_length = status_len;
   bind[2].length = &status_len;

   // p_success_time
   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&success_time;

   // p_failure_time
   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&failure_time;

   // p_quest_name
   bind[5].buffer_type = MYSQL_TYPE_STRING;
   bind[5].buffer = (char*)quest_name;
   bind[5].buffer_length = name_len;
   bind[5].length = &name_len;

   // Parameter binden und ausführen
   result = mysql_stmt_bind_param(stmt, bind);
   if (result != 0)
   {
      eprintf("MySQLSaveQuestHistory: bind_param failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("MySQLSaveQuestHistory: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return FALSE;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   dprintf("MySQLSaveQuestHistory: Quest history saved successfully\n");
   return TRUE;
}


/**
 * MySQLLoadQuestHistory - Lädt Quest-History aus MySQL
 * @param account_id Account-ID
 * @param callback Callback-Funktion die für jede Quest aufgerufen wird
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLLoadQuestHistory(int account_id, void (*callback)(int account_id, int quest_id, 
                           const char* status, int success_time, int failure_time))
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind_param[1];
   MYSQL_BIND bind_result[5];
   int result;
   int row_count = 0;

   // Result-Variablen
   int quest_template_id;
   char quest_name[101];
   char status[20];
   int success_time;
   int failure_time;

   unsigned long name_len, status_len;
   my_bool is_null[5];
   my_bool error[5];

   if (!mysql || state < SCHEMAVERIFIED)
   {
      lprintf("MySQLLoadQuestHistory: MySQL not available\n");
      return FALSE;
   }

   if (!callback)
      return FALSE;

   lprintf("MySQLLoadQuestHistory: Loading quest history for account %d...\n", account_id);

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   // CALL LoadQuestHistory(?)
   const char* query = "CALL LoadQuestHistory(?)";
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("MySQLLoadQuestHistory: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Bind input parameter
   memset(bind_param, 0, sizeof(bind_param));
   bind_param[0].buffer_type = MYSQL_TYPE_LONG;
   bind_param[0].buffer = (char*)&account_id;

   result = mysql_stmt_bind_param(stmt, bind_param);
   if (result != 0)
   {
      eprintf("MySQLLoadQuestHistory: bind_param failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Execute
   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("MySQLLoadQuestHistory: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Bind result
   memset(bind_result, 0, sizeof(bind_result));

   // quest_template_id
   bind_result[0].buffer_type = MYSQL_TYPE_LONG;
   bind_result[0].buffer = (char*)&quest_template_id;
   bind_result[0].is_null = &is_null[0];
   bind_result[0].error = &error[0];

   // quest_name (skip - we don't need it for loading)
   bind_result[1].buffer_type = MYSQL_TYPE_STRING;
   bind_result[1].buffer = quest_name;
   bind_result[1].buffer_length = sizeof(quest_name);
   bind_result[1].length = &name_len;
   bind_result[1].is_null = &is_null[1];
   bind_result[1].error = &error[1];

   // status
   bind_result[2].buffer_type = MYSQL_TYPE_STRING;
   bind_result[2].buffer = status;
   bind_result[2].buffer_length = sizeof(status);
   bind_result[2].length = &status_len;
   bind_result[2].is_null = &is_null[2];
   bind_result[2].error = &error[2];

   // success_time
   bind_result[3].buffer_type = MYSQL_TYPE_LONG;
   bind_result[3].buffer = (char*)&success_time;
   bind_result[3].is_null = &is_null[3];
   bind_result[3].error = &error[3];

   // failure_time
   bind_result[4].buffer_type = MYSQL_TYPE_LONG;
   bind_result[4].buffer = (char*)&failure_time;
   bind_result[4].is_null = &is_null[4];
   bind_result[4].error = &error[4];

   result = mysql_stmt_bind_result(stmt, bind_result);
   if (result != 0)
   {
      eprintf("MySQLLoadQuestHistory: bind_result failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Fetch all rows
   while (mysql_stmt_fetch(stmt) == 0)
   {
      // Null-Terminierung
      status[status_len] = '\0';

      // Callback aufrufen
      callback(account_id, quest_template_id, status, success_time, failure_time);
      row_count++;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   lprintf("MySQLLoadQuestHistory: Loaded %d quest history entries\n", row_count);
   return TRUE;
}


/* ============================================================================
 * ACTIVE QUESTS Functions
 * ============================================================================
 */

/**
 * MySQLSaveActiveQuest - Speichert eine aktive Quest
 * @param account_id Account-ID
 * @param quest_template_id Quest Template ID
 * @param quest_node_template_id Aktueller Quest Node Template ID
 * @param quest_node_counter Schritt-Nummer in der Quest-Kette
 * @param quest_deadline Unix-Timestamp des Ablaufdatums
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLSaveActiveQuest(int account_id, int quest_template_id, int quest_node_template_id,
                          int quest_node_counter, int quest_deadline)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[6];
   int result;

   if (!mysql || state < SCHEMAVERIFIED)
   {
      dprintf("MySQLSaveActiveQuest: MySQL not available\n");
      return FALSE;
   }

   dprintf("MySQLSaveActiveQuest: Saving active quest %d for account %d (node %d)\n",
           quest_template_id, account_id, quest_node_template_id);

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   // CALL SaveActiveQuest(?,?,?,?,?,?)
   const char* query = "CALL SaveActiveQuest(?,?,?,?,?,?)";
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      eprintf("MySQLSaveActiveQuest: prepare failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Bind-Parameter
   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&account_id;

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&quest_template_id;

   bind[2].buffer_type = MYSQL_TYPE_LONG;
   bind[2].buffer = (char*)&quest_node_template_id;

   bind[3].buffer_type = MYSQL_TYPE_LONG;
   bind[3].buffer = (char*)&quest_node_counter;

   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&quest_deadline;

   // quest_data (JSON) - vorerst NULL
   bind[5].buffer_type = MYSQL_TYPE_NULL;

   result = mysql_stmt_bind_param(stmt, bind);
   if (result != 0)
   {
      eprintf("MySQLSaveActiveQuest: bind_param failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      eprintf("MySQLSaveActiveQuest: execute failed: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   return TRUE;
}


/**
 * MySQLRemoveActiveQuest - Entfernt eine aktive Quest
 * @param account_id Account-ID
 * @param quest_template_id Quest Template ID
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLRemoveActiveQuest(int account_id, int quest_template_id)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[2];
   int result;

   if (!mysql || state < SCHEMAVERIFIED)
   {
      dprintf("MySQLRemoveActiveQuest: MySQL not available\n");
      return FALSE;
   }

   dprintf("MySQLRemoveActiveQuest: Removing quest %d for account %d\n",
           quest_template_id, account_id);

   MySQLLock();

   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   const char* query = "CALL RemoveActiveQuest(?,?)";
   result = mysql_stmt_prepare(stmt, query, strlen(query));
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&account_id;

   bind[1].buffer_type = MYSQL_TYPE_LONG;
   bind[1].buffer = (char*)&quest_template_id;

   result = mysql_stmt_bind_param(stmt, bind);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   result = mysql_stmt_execute(stmt);
   if (result != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   mysql_stmt_close(stmt);
   MySQLUnlock();
   return TRUE;
}


/* ============================================================================
 * SCHEMA CREATION
 * ============================================================================
 */

/**
 * _MySQLCreateQuestSchema - Erstellt Quest-Tabellen und Procedures
 * Diese Funktion wird von _MySQLVerifySchema() in database.c aufgerufen
 */
void _MySQLCreateQuestSchema(void)
{
   if (!mysql)
      return;

   lprintf("MySQL: Creating quest schema...\n");

   // SQL-Schema aus create_quests_tables.sql muss manuell ausgeführt werden
   // Hier nur prüfen ob Tabellen existieren

   // Prüfe ob quest_history Tabelle existiert
   MySQLLock();
   if (mysql_query(mysql, "SELECT 1 FROM quest_history LIMIT 1") == 0)
   {
      mysql_free_result(mysql_store_result(mysql));
      lprintf("MySQL: Quest tables already exist\n");
   }
   else
   {
      lprintf("MySQL: Quest tables not found - please run sql/create_quests_tables.sql\n");
   }
   MySQLUnlock();
}


/* ============================================================================
 * QUEST TRIGGER Functions
 * ============================================================================
 */

/**
 * NOTE: Quest trigger checking is now handled by the in-memory cache in questloader.c
 * See: CheckQuestTrigger() and LoadQuestTriggersFromMySQL()
 * This eliminates the need for real-time MySQL queries on every player message.
 */

