// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * dbaccount.c
 *
 * MySQL Account Management Integration
 * Handles loading/saving accounts to MySQL database instead of flat files
 */

#include "blakserv.h"

// External variables from database.c
extern MYSQL* mysql;
extern sql_worker_state state;

// SQL Queries für Account-Tabelle
#define SQLQUERY_CREATETABLE_ACCOUNTS \
   "CREATE TABLE IF NOT EXISTS `accounts` ( \
      `account_id` INT(11) NOT NULL PRIMARY KEY, \
      `account_name` VARCHAR(50) NOT NULL UNIQUE, \
      `account_password` VARCHAR(255) NOT NULL, \
      `account_email` VARCHAR(100) DEFAULT '', \
      `account_type` INT(1) NOT NULL DEFAULT 0, \
      `account_credits` INT(11) NOT NULL DEFAULT 0, \
      `account_last_login_time` INT(11) NOT NULL DEFAULT 0, \
      `account_suspend_time` INT(11) NOT NULL DEFAULT 0, \
      `account_created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP, \
      `account_updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, \
      INDEX `idx_account_name` (`account_name`), \
      INDEX `idx_account_email` (`account_email`) \
   ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;"

#define SQLQUERY_CREATEPROC_SAVEACCOUNT \
   "CREATE PROCEDURE SaveAccount( \
      IN p_account_id INT(11), \
      IN p_name VARCHAR(50), \
      IN p_password VARCHAR(255), \
      IN p_email VARCHAR(100), \
      IN p_type INT(1), \
      IN p_credits INT(11), \
      IN p_last_login_time INT(11), \
      IN p_suspend_time INT(11)) \
   BEGIN \
      INSERT INTO `accounts` ( \
         `account_id`, `account_name`, `account_password`, `account_email`, \
         `account_type`, `account_credits`, `account_last_login_time`, `account_suspend_time`) \
      VALUES (p_account_id, p_name, p_password, p_email, p_type, p_credits, p_last_login_time, p_suspend_time) \
      ON DUPLICATE KEY UPDATE \
         `account_name` = p_name, \
         `account_password` = p_password, \
         `account_email` = p_email, \
         `account_type` = p_type, \
         `account_credits` = p_credits, \
         `account_last_login_time` = p_last_login_time, \
         `account_suspend_time` = p_suspend_time; \
   END"

#define SQLQUERY_DROPPROC_SAVEACCOUNT "DROP PROCEDURE IF EXISTS SaveAccount;"
#define SQLQUERY_CALL_SAVEACCOUNT "CALL SaveAccount(?,?,?,?,?,?,?,?);"

/* Forward declarations */
void _MySQLCreateAccountSchema(void);
void _MySQLSaveAccountToDatabase(account_node *a);
void _MySQLLoadAccountCallback(account_node *a);

/* Public Functions */

/**
 * MySQLSaveAccount - Speichert einen Account in die MySQL-Datenbank
 * @param account_id Account-ID
 * @param name Account-Name
 * @param password Verschlüsseltes Passwort
 * @param email E-Mail-Adresse
 * @param type Account-Typ (0=Normal, 1=Admin, 2=DM, 3=Guest)
 * @param credits Anzahl Credits (in 1/100)
 * @param last_login_time Letzter Login (Unix-Timestamp)
 * @param suspend_time Suspend-Zeit (Unix-Timestamp)
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLSaveAccount(int account_id, char* name, char* password, char* email,
                      int type, int credits, int last_login_time, int suspend_time)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[8];
   int status;
   unsigned long name_len, password_len, email_len;

   if (!mysql || state < SCHEMAVERIFIED)
   {
      dprintf("MySQLSaveAccount: MySQL not available (mysql=%p, state=%d)\n", mysql, state);
      return FALSE;
   }

   if (!name || !password)
      return FALSE;

   dprintf("MySQLSaveAccount: Saving account %s (ID=%d)\n", name, account_id);

   // E-Mail kann NULL sein
   if (!email)
      email = "";

   MySQLLock();

   // Statement initialisieren
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   // Query vorbereiten
   status = mysql_stmt_prepare(stmt, SQLQUERY_CALL_SAVEACCOUNT, 
                                strlen(SQLQUERY_CALL_SAVEACCOUNT));
   if (status != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // String-Längen berechnen
   name_len = strlen(name);
   password_len = strlen(password);
   email_len = strlen(email);

   // Bind-Parameter vorbereiten
   memset(bind, 0, sizeof(bind));

   // account_id (INT)
   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&account_id;

   // name (VARCHAR)
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = (char*)name;
   bind[1].buffer_length = name_len;
   bind[1].length = &name_len;

   // password (VARCHAR)
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = (char*)password;
   bind[2].buffer_length = password_len;
   bind[2].length = &password_len;

   // email (VARCHAR)
   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = (char*)email;
   bind[3].buffer_length = email_len;
   bind[3].length = &email_len;

   // type (INT)
   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&type;

   // credits (INT)
   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&credits;

   // last_login_time (INT)
   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&last_login_time;

   // suspend_time (INT)
   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&suspend_time;

   // Parameter binden
   status = mysql_stmt_bind_param(stmt, bind);
   if (status != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Query ausführen
   status = mysql_stmt_execute(stmt);
   if (status != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Statement schließen
   mysql_stmt_close(stmt);

   MySQLUnlock();

   return TRUE;
}

/**
 * MySQLLoadAccounts - Lädt alle Accounts aus der MySQL-Datenbank
 * @return TRUE bei Erfolg, FALSE bei Fehler
 */
BOOL MySQLLoadAccounts(void)
{
   MYSQL_STMT* stmt;
   MYSQL_BIND bind[8];
   int status;

   // Account-Daten
   int account_id;
   char name[51];
   char password[256];
   char email[101];
   int type;
   int credits;
   int last_login_time;
   int suspend_time;

   // String-Längen
   unsigned long name_len, password_len, email_len;
   my_bool is_null[8];
   my_bool error[8];

   if (!mysql || state < SCHEMAVERIFIED)
   {
      lprintf("MySQLLoadAccounts: MySQL not available (mysql=%p, state=%d)\n", mysql, state);
      return FALSE;
   }

   lprintf("MySQLLoadAccounts: Starting to load accounts from database...\n");

   MySQLLock();

   // Statement initialisieren
   stmt = mysql_stmt_init(mysql);
   if (!stmt)
   {
      MySQLUnlock();
      return FALSE;
   }

   // Query vorbereiten
   const char* query = "SELECT account_id, account_name, account_password, account_email, "
                      "account_type, account_credits, account_last_login_time, account_suspend_time "
                      "FROM accounts ORDER BY account_id ASC";

   status = mysql_stmt_prepare(stmt, query, strlen(query));
   if (status != 0)
   {
      eprintf("MySQLLoadAccounts: Failed to prepare statement: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Query ausführen
   status = mysql_stmt_execute(stmt);
   if (status != 0)
   {
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Result-Set vorbereiten
   memset(bind, 0, sizeof(bind));

   // account_id
   bind[0].buffer_type = MYSQL_TYPE_LONG;
   bind[0].buffer = (char*)&account_id;
   bind[0].is_null = &is_null[0];
   bind[0].error = &error[0];

   // account_name
   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = (char*)name;
   bind[1].buffer_length = sizeof(name);
   bind[1].length = &name_len;
   bind[1].is_null = &is_null[1];
   bind[1].error = &error[1];

   // account_password
   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = (char*)password;
   bind[2].buffer_length = sizeof(password);
   bind[2].length = &password_len;
   bind[2].is_null = &is_null[2];
   bind[2].error = &error[2];

   // account_email
   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = (char*)email;
   bind[3].buffer_length = sizeof(email);
   bind[3].length = &email_len;
   bind[3].is_null = &is_null[3];
   bind[3].error = &error[3];

   // account_type
   bind[4].buffer_type = MYSQL_TYPE_LONG;
   bind[4].buffer = (char*)&type;
   bind[4].is_null = &is_null[4];
   bind[4].error = &error[4];

   // account_credits
   bind[5].buffer_type = MYSQL_TYPE_LONG;
   bind[5].buffer = (char*)&credits;
   bind[5].is_null = &is_null[5];
   bind[5].error = &error[5];

   // account_last_login_time
   bind[6].buffer_type = MYSQL_TYPE_LONG;
   bind[6].buffer = (char*)&last_login_time;
   bind[6].is_null = &is_null[6];
   bind[6].error = &error[6];

   // account_suspend_time
   bind[7].buffer_type = MYSQL_TYPE_LONG;
   bind[7].buffer = (char*)&suspend_time;
   bind[7].is_null = &is_null[7];
   bind[7].error = &error[7];

   // Result binden
   status = mysql_stmt_bind_result(stmt, bind);
   if (status != 0)
   {
      eprintf("MySQLLoadAccounts: Failed to bind result: %s\n", mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      MySQLUnlock();
      return FALSE;
   }

   // Alle Rows fetchen
   int row_count = 0;
   while (mysql_stmt_fetch(stmt) == 0)
   {
      // Null-Terminierung sicherstellen
      name[name_len] = '\0';
      password[password_len] = '\0';
      email[email_len] = '\0';

      // Account in Memory laden
      LoadAccount(account_id, name, password, email, type, 
                  last_login_time, suspend_time, credits);
      row_count++;
      dprintf("MySQLLoadAccounts: Loaded account #%d: %s (ID=%d)\n", row_count, name, account_id);
   }

   // Statement schließen
   mysql_stmt_close(stmt);

   lprintf("MySQLLoadAccounts: Successfully loaded %d accounts from database\n", row_count);
   MySQLUnlock();
   return TRUE;
}

/**
 * MySQLSaveAllAccounts - Speichert alle Accounts in die Datenbank
 */
BOOL MySQLSaveAllAccounts(void)
{
   if (!mysql || state < SCHEMAVERIFIED)
      return FALSE;

   ForEachAccount(_MySQLSaveAccountToDatabase);
   return TRUE;
}

/* Internal Functions */

/**
 * _MySQLSaveAccountToDatabase - Callback zum Speichern eines Accounts
 */
void _MySQLSaveAccountToDatabase(account_node *a)
{
   if (!a)
      return;

   MySQLSaveAccount(a->account_id, a->name, a->password, a->email,
                    a->type, a->credits, a->last_login_time, a->suspend_time);
}

/**
 * _MySQLCreateAccountSchema - Erstellt Account-Tabellen und Procedures
 * Diese Funktion wird von _MySQLVerifySchema() aufgerufen
 */
void _MySQLCreateAccountSchema(void)
{
   if (!mysql)
      return;

   MySQLLock();

   lprintf("MySQL: Creating account schema...\n");

   // Tabelle erstellen
   if (mysql_query(mysql, SQLQUERY_CREATETABLE_ACCOUNTS) != 0)
   {
      eprintf("MySQL: Failed to create accounts table: %s\n", mysql_error(mysql));
      MySQLUnlock();
      return;
   }

   // Alte Procedure löschen
   mysql_query(mysql, SQLQUERY_DROPPROC_SAVEACCOUNT);

   // Neue Procedure erstellen
   if (mysql_query(mysql, SQLQUERY_CREATEPROC_SAVEACCOUNT) != 0)
   {
      eprintf("MySQL: Failed to create SaveAccount procedure: %s\n", mysql_error(mysql));
      MySQLUnlock();
      return;
   }

   dprintf("MySQL: Account schema created successfully\n");
   MySQLUnlock();
}
