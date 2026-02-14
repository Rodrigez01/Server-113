// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * playerloader.c
 *
 * MySQL Player Character Loading
 * Loads player characters from the MySQL player table and associates
 * them with their accounts.
 */

#include "blakserv.h"

// External MySQL connection
extern MYSQL* mysql;
extern sql_worker_state state;

// Player template storage
static player_template_t* player_templates = NULL;
static int num_player_templates = 0;
static int max_player_templates = 0;

#define INITIAL_PLAYER_TEMPLATES 100
#define PLAYER_TEMPLATE_GROW 50

/**
 * Initialize player template storage
 */
void InitPlayerTemplates(void)
{
    if (player_templates != NULL)
    {
        FreeMemory(MALLOC_ID_PLAYER_TEMPLATE, player_templates,
                   max_player_templates * sizeof(player_template_t));
    }

    max_player_templates = INITIAL_PLAYER_TEMPLATES;
    player_templates = (player_template_t*)AllocateMemory(MALLOC_ID_PLAYER_TEMPLATE,
                        max_player_templates * sizeof(player_template_t));
    num_player_templates = 0;
}

/**
 * Add a player template to storage
 */
static player_template_t* AddPlayerTemplate(void)
{
    if (num_player_templates >= max_player_templates)
    {
        int new_max = max_player_templates + PLAYER_TEMPLATE_GROW;
        player_template_t* new_templates = (player_template_t*)AllocateMemory(
            MALLOC_ID_PLAYER_TEMPLATE, new_max * sizeof(player_template_t));

        memcpy(new_templates, player_templates,
               num_player_templates * sizeof(player_template_t));

        FreeMemory(MALLOC_ID_PLAYER_TEMPLATE, player_templates,
                   max_player_templates * sizeof(player_template_t));

        player_templates = new_templates;
        max_player_templates = new_max;
    }

    return &player_templates[num_player_templates++];
}

/**
 * Load player templates from MySQL
 */
BOOL LoadPlayerTemplatesFromMySQL(void)
{
    MYSQL_STMT* stmt;
    MYSQL_BIND bind[14];
    int status;

    // Result variables
    int idplayer;
    int player_account_id;
    char player_name[46];
    char player_home[256];
    char player_bind[256];
    char player_guild[46];
    int player_max_health;
    int player_max_mana;
    int player_might;
    int player_int;
    int player_myst;
    int player_stam;
    int player_agil;
    int player_aim;

    unsigned long name_len, home_len, bind_len, guild_len;
    my_bool is_null[14];
    my_bool error[14];

    if (!mysql || state < SCHEMAVERIFIED)
    {
        lprintf("LoadPlayerTemplatesFromMySQL: MySQL not available\n");
        return FALSE;
    }

    lprintf("Loading player characters from MySQL...\n");

    InitPlayerTemplates();

    MySQLLock();

    stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        MySQLUnlock();
        return FALSE;
    }

    const char* query =
        "SELECT idplayer, player_account_id, player_name, "
        "COALESCE(player_home, ''), COALESCE(player_bind, ''), "
        "COALESCE(player_guild, ''), "
        "COALESCE(player_max_health, 100), COALESCE(player_max_mana, 100), "
        "COALESCE(player_might, 50), COALESCE(player_int, 50), "
        "COALESCE(player_myst, 50), COALESCE(player_stam, 50), "
        "COALESCE(player_agil, 50), COALESCE(player_aim, 50) "
        "FROM player ORDER BY idplayer";

    status = mysql_stmt_prepare(stmt, query, strlen(query));
    if (status != 0)
    {
        eprintf("LoadPlayerTemplatesFromMySQL: Failed to prepare: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return FALSE;
    }

    status = mysql_stmt_execute(stmt);
    if (status != 0)
    {
        eprintf("LoadPlayerTemplatesFromMySQL: Failed to execute: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return FALSE;
    }

    memset(bind, 0, sizeof(bind));

    // idplayer
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &idplayer;
    bind[0].is_null = &is_null[0];
    bind[0].error = &error[0];

    // player_account_id
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &player_account_id;
    bind[1].is_null = &is_null[1];
    bind[1].error = &error[1];

    // player_name
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = player_name;
    bind[2].buffer_length = sizeof(player_name);
    bind[2].length = &name_len;
    bind[2].is_null = &is_null[2];
    bind[2].error = &error[2];

    // player_home
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = player_home;
    bind[3].buffer_length = sizeof(player_home);
    bind[3].length = &home_len;
    bind[3].is_null = &is_null[3];
    bind[3].error = &error[3];

    // player_bind
    bind[4].buffer_type = MYSQL_TYPE_STRING;
    bind[4].buffer = player_bind;
    bind[4].buffer_length = sizeof(player_bind);
    bind[4].length = &bind_len;
    bind[4].is_null = &is_null[4];
    bind[4].error = &error[4];

    // player_guild
    bind[5].buffer_type = MYSQL_TYPE_STRING;
    bind[5].buffer = player_guild;
    bind[5].buffer_length = sizeof(player_guild);
    bind[5].length = &guild_len;
    bind[5].is_null = &is_null[5];
    bind[5].error = &error[5];

    // Stats
    bind[6].buffer_type = MYSQL_TYPE_LONG;
    bind[6].buffer = &player_max_health;
    bind[6].is_null = &is_null[6];
    bind[6].error = &error[6];

    bind[7].buffer_type = MYSQL_TYPE_LONG;
    bind[7].buffer = &player_max_mana;
    bind[7].is_null = &is_null[7];
    bind[7].error = &error[7];

    bind[8].buffer_type = MYSQL_TYPE_LONG;
    bind[8].buffer = &player_might;
    bind[8].is_null = &is_null[8];
    bind[8].error = &error[8];

    bind[9].buffer_type = MYSQL_TYPE_LONG;
    bind[9].buffer = &player_int;
    bind[9].is_null = &is_null[9];
    bind[9].error = &error[9];

    bind[10].buffer_type = MYSQL_TYPE_LONG;
    bind[10].buffer = &player_myst;
    bind[10].is_null = &is_null[10];
    bind[10].error = &error[10];

    bind[11].buffer_type = MYSQL_TYPE_LONG;
    bind[11].buffer = &player_stam;
    bind[11].is_null = &is_null[11];
    bind[11].error = &error[11];

    bind[12].buffer_type = MYSQL_TYPE_LONG;
    bind[12].buffer = &player_agil;
    bind[12].is_null = &is_null[12];
    bind[12].error = &error[12];

    bind[13].buffer_type = MYSQL_TYPE_LONG;
    bind[13].buffer = &player_aim;
    bind[13].is_null = &is_null[13];
    bind[13].error = &error[13];

    status = mysql_stmt_bind_result(stmt, bind);
    if (status != 0)
    {
        eprintf("LoadPlayerTemplatesFromMySQL: Failed to bind: %s\n",
                mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return FALSE;
    }

    while (mysql_stmt_fetch(stmt) == 0)
    {
        player_template_t* pt = AddPlayerTemplate();

        player_name[name_len] = '\0';
        player_home[home_len] = '\0';
        player_bind[bind_len] = '\0';
        player_guild[guild_len] = '\0';

        pt->player_id = idplayer;
        pt->account_id = player_account_id;
        strncpy(pt->player_name, player_name, sizeof(pt->player_name) - 1);
        pt->player_name[sizeof(pt->player_name) - 1] = '\0';
        strncpy(pt->player_home, player_home, sizeof(pt->player_home) - 1);
        pt->player_home[sizeof(pt->player_home) - 1] = '\0';
        strncpy(pt->player_bind, player_bind, sizeof(pt->player_bind) - 1);
        pt->player_bind[sizeof(pt->player_bind) - 1] = '\0';
        strncpy(pt->player_guild, player_guild, sizeof(pt->player_guild) - 1);
        pt->player_guild[sizeof(pt->player_guild) - 1] = '\0';

        pt->max_health = player_max_health;
        pt->max_mana = player_max_mana;
        pt->might = player_might;
        pt->intellect = player_int;
        pt->mysticism = player_myst;
        pt->stamina = player_stam;
        pt->agility = player_agil;
        pt->aim = player_aim;

        pt->object_id = 0; // Will be set when player object is found/created

        dprintf("LoadPlayerTemplatesFromMySQL: Loaded player %s (ID=%d, Account=%d)\n",
                pt->player_name, pt->player_id, pt->account_id);
    }

    mysql_stmt_close(stmt);
    MySQLUnlock();

    lprintf("LoadPlayerTemplatesFromMySQL: Loaded %d player characters\n",
            num_player_templates);

    return TRUE;
}

/**
 * Get player template by ID
 */
player_template_t* GetPlayerTemplateByID(int player_id)
{
    for (int i = 0; i < num_player_templates; i++)
    {
        if (player_templates[i].player_id == player_id)
            return &player_templates[i];
    }
    return NULL;
}

/**
 * Get player template by account ID
 */
player_template_t* GetPlayerTemplateByAccountID(int account_id)
{
    for (int i = 0; i < num_player_templates; i++)
    {
        if (player_templates[i].account_id == account_id)
            return &player_templates[i];
    }
    return NULL;
}

/**
 * Get player template by name
 */
player_template_t* GetPlayerTemplateByName(const char* name)
{
    for (int i = 0; i < num_player_templates; i++)
    {
        if (stricmp(player_templates[i].player_name, name) == 0)
            return &player_templates[i];
    }
    return NULL;
}

/**
 * Get number of loaded player templates
 */
int GetNumPlayerTemplates(void)
{
    return num_player_templates;
}

/**
 * Get player template by index
 */
player_template_t* GetPlayerTemplateByIndex(int index)
{
    if (index < 0 || index >= num_player_templates)
        return NULL;
    return &player_templates[index];
}

/**
 * Associate loaded player templates with accounts
 * Called after game state is loaded
 */
void AssociatePlayersWithAccounts(void)
{
    int associated = 0;

    lprintf("AssociatePlayersWithAccounts: Checking %d players...\n", num_player_templates);

    for (int i = 0; i < num_player_templates; i++)
    {
        player_template_t* pt = &player_templates[i];

        // Check if account exists
        account_node* account = GetAccountByID(pt->account_id);
        if (!account)
        {
            dprintf("AssociatePlayersWithAccounts: Account %d not found for player %s\n",
                    pt->account_id, pt->player_name);
            continue;
        }

        // Check if user already exists for this account
        user_node* existing = GetFirstUserByAccountID(pt->account_id);
        if (existing)
        {
            dprintf("AssociatePlayersWithAccounts: Account %d already has user (object %d)\n",
                    pt->account_id, existing->object_id);
            pt->object_id = existing->object_id;
            associated++;
            continue;
        }

        // No user exists - we need to create one
        // This requires creating a player object, which is complex
        // For now, just log that the player needs to be created
        lprintf("AssociatePlayersWithAccounts: Player %s (Account %d) needs to be created in game\n",
                pt->player_name, pt->account_id);
    }

    lprintf("AssociatePlayersWithAccounts: %d players associated with accounts\n", associated);
}
