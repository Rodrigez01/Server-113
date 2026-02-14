// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * questtrigger.c
 *
 * MySQL Quest Trigger System
 * Enables NPCs to automatically trigger quests from MySQL database
 */

#include "blakserv.h"

// External MySQL connection
extern MYSQL* mysql;
extern sql_worker_state state;

/* ============================================================================
 * C_GetQuestsForNPC - Returns list of quest IDs where this NPC is a quest_giver
 *
 * Parameters: npc_class (resource)
 * Returns: List of quest_ids, or NIL if none found
 * ============================================================================
 */
int C_GetQuestsForNPC(int object_id, local_var_type *local_vars,
                      int num_normal_parms, parm_node normal_parm_array[],
                      int num_name_parms, parm_node name_parm_array[])
{
    val_type ret_val, param_val;
    val_type list_val, elem_val;
    MYSQL_STMT* stmt;
    MYSQL_BIND bind_params[1];
    MYSQL_BIND bind_result[1];
    resource_node *r;
    const char* npc_class;
    int npc_class_len;

    // Result buffer
    int quest_id;
    my_bool is_null;
    my_bool error_flag;

    ret_val.v.tag = TAG_NIL;
    ret_val.v.data = 0;

    // Check parameters
    if (num_normal_parms != 1)
    {
        bprintf("C_GetQuestsForNPC requires 1 parameter (npc_class)\n");
        return ret_val.int_val;
    }

    param_val.int_val = normal_parm_array[0].value;
    if (param_val.v.tag != TAG_RESOURCE)
    {
        bprintf("C_GetQuestsForNPC: npc_class must be a resource\n");
        return ret_val.int_val;
    }

    r = GetResourceByID(param_val.v.data);
    if (r == NULL || r->resource_val[0] == NULL)
    {
        bprintf("C_GetQuestsForNPC: invalid resource\n");
        return ret_val.int_val;
    }

    npc_class = r->resource_val[0];
    npc_class_len = (int)strlen(npc_class);

    if (!mysql || state < SCHEMAVERIFIED)
    {
        eprintf("C_GetQuestsForNPC: MySQL not available\n");
        return ret_val.int_val;
    }

    // Simple query - just get quest IDs for this NPC class
    const char* query =
        "SELECT DISTINCT qt.quest_template_id "
        "FROM quest_templates qt "
        "WHERE qt.quest_kod_class = ? "
        "AND qt.active = 1 "
        "ORDER BY qt.quest_template_id";

    MySQLLock();

    stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        eprintf("C_GetQuestsForNPC: mysql_stmt_init failed\n");
        MySQLUnlock();
        return ret_val.int_val;
    }

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)))
    {
        eprintf("C_GetQuestsForNPC: prepare failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind parameters
    memset(bind_params, 0, sizeof(bind_params));

    bind_params[0].buffer_type = MYSQL_TYPE_STRING;
    bind_params[0].buffer = (char*)npc_class;
    bind_params[0].buffer_length = npc_class_len;

    if (mysql_stmt_bind_param(stmt, bind_params))
    {
        eprintf("C_GetQuestsForNPC: bind_param failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Execute query
    if (mysql_stmt_execute(stmt))
    {
        eprintf("C_GetQuestsForNPC: execute failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind results
    memset(bind_result, 0, sizeof(bind_result));

    bind_result[0].buffer_type = MYSQL_TYPE_LONG;
    bind_result[0].buffer = (char*)&quest_id;
    bind_result[0].is_null = &is_null;
    bind_result[0].error = &error_flag;

    if (mysql_stmt_bind_result(stmt, bind_result))
    {
        eprintf("C_GetQuestsForNPC: bind_result failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Build result list
    list_val.v.tag = TAG_NIL;
    list_val.v.data = NIL;

    int row_count = 0;
    while (mysql_stmt_fetch(stmt) == 0)
    {
        if (!is_null)
        {
            elem_val.v.tag = TAG_INT;
            elem_val.v.data = quest_id;

            list_val.int_val = Cons(elem_val, list_val);
            row_count++;
        }
    }

    mysql_stmt_close(stmt);
    MySQLUnlock();

    dprintf("C_GetQuestsForNPC: Found %d quests for NPC '%s'\n", row_count, npc_class);

    return list_val.int_val;
}

/* ============================================================================
 * C_GetQuestTriggers - Returns list of trigger phrases for a quest
 *
 * Parameters: quest_id (int)
 * Returns: List of trigger text resource IDs
 * ============================================================================
 */
int C_GetQuestTriggers(int object_id, local_var_type *local_vars,
                       int num_normal_parms, parm_node normal_parm_array[],
                       int num_name_parms, parm_node name_parm_array[])
{
    val_type ret_val, param_val;
    val_type list_val, elem_val;
    MYSQL_STMT* stmt;
    MYSQL_BIND bind_params[1];
    MYSQL_BIND bind_result[1];
    int quest_id;

    // Result buffer
    char trigger_text[256];
    unsigned long trigger_len;
    my_bool is_null;
    my_bool error_flag;

    ret_val.v.tag = TAG_NIL;
    ret_val.v.data = 0;

    // Check parameters
    if (num_normal_parms != 1)
    {
        bprintf("C_GetQuestTriggers requires 1 parameter (quest_id)\n");
        return ret_val.int_val;
    }

    param_val.int_val = normal_parm_array[0].value;
    if (param_val.v.tag != TAG_INT)
    {
        bprintf("C_GetQuestTriggers: quest_id must be an integer\n");
        return ret_val.int_val;
    }

    quest_id = param_val.v.data;

    if (!mysql || state < SCHEMAVERIFIED)
    {
        eprintf("C_GetQuestTriggers: MySQL not available\n");
        return ret_val.int_val;
    }

    // Query for quest triggers (simplified - just use quest_description as trigger)
    const char* query =
        "SELECT quest_description "
        "FROM quest_templates "
        "WHERE quest_template_id = ? "
        "AND active = 1";

    MySQLLock();

    stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        eprintf("C_GetQuestTriggers: mysql_stmt_init failed\n");
        MySQLUnlock();
        return ret_val.int_val;
    }

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)))
    {
        eprintf("C_GetQuestTriggers: prepare failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind parameters
    memset(bind_params, 0, sizeof(bind_params));

    bind_params[0].buffer_type = MYSQL_TYPE_LONG;
    bind_params[0].buffer = (char*)&quest_id;

    if (mysql_stmt_bind_param(stmt, bind_params))
    {
        eprintf("C_GetQuestTriggers: bind_param failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Execute query
    if (mysql_stmt_execute(stmt))
    {
        eprintf("C_GetQuestTriggers: execute failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind results
    memset(bind_result, 0, sizeof(bind_result));

    bind_result[0].buffer_type = MYSQL_TYPE_STRING;
    bind_result[0].buffer = trigger_text;
    bind_result[0].buffer_length = sizeof(trigger_text) - 1;
    bind_result[0].length = &trigger_len;
    bind_result[0].is_null = &is_null;
    bind_result[0].error = &error_flag;

    if (mysql_stmt_bind_result(stmt, bind_result))
    {
        eprintf("C_GetQuestTriggers: bind_result failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Build result list
    list_val.v.tag = TAG_NIL;
    list_val.v.data = NIL;

    while (mysql_stmt_fetch(stmt) == 0)
    {
        if (!is_null && trigger_len > 0)
        {
            trigger_text[trigger_len] = '\0';

            // Create a dynamic resource for the trigger text
            int res_id = AddDynamicResource(trigger_text);

            elem_val.v.tag = TAG_RESOURCE;
            elem_val.v.data = res_id;

            list_val.int_val = Cons(elem_val, list_val);
        }
    }

    mysql_stmt_close(stmt);
    MySQLUnlock();

    return list_val.int_val;
}

/* ============================================================================
 * C_CheckQuestTrigger - Check if string matches any quest trigger for NPC
 *
 * Parameters: npc_class (resource), trigger_text (resource)
 * Returns: quest_id if match found, NIL otherwise
 * ============================================================================
 */
int C_CheckQuestTrigger(int object_id, local_var_type *local_vars,
                        int num_normal_parms, parm_node normal_parm_array[],
                        int num_name_parms, parm_node name_parm_array[])
{
    val_type ret_val, param_val1, param_val2;
    MYSQL_STMT* stmt;
    MYSQL_BIND bind_params[2];
    MYSQL_BIND bind_result[1];
    resource_node *r1, *r2;
    const char* npc_class;
    const char* trigger_text;
    int npc_class_len, trigger_len;

    // Result buffer
    int quest_id;
    my_bool is_null;
    my_bool error_flag;

    ret_val.v.tag = TAG_NIL;
    ret_val.v.data = 0;

    // Check parameters
    if (num_normal_parms != 2)
    {
        bprintf("C_CheckQuestTrigger requires 2 parameters (npc_class, trigger_text)\n");
        return ret_val.int_val;
    }

    param_val1.int_val = normal_parm_array[0].value;
    param_val2.int_val = normal_parm_array[1].value;

    if (param_val1.v.tag != TAG_RESOURCE || param_val2.v.tag != TAG_RESOURCE)
    {
        bprintf("C_CheckQuestTrigger: parameters must be resources\n");
        return ret_val.int_val;
    }

    r1 = GetResourceByID(param_val1.v.data);
    r2 = GetResourceByID(param_val2.v.data);

    if (r1 == NULL || r1->resource_val[0] == NULL ||
        r2 == NULL || r2->resource_val[0] == NULL)
    {
        bprintf("C_CheckQuestTrigger: invalid resources\n");
        return ret_val.int_val;
    }

    npc_class = r1->resource_val[0];
    trigger_text = r2->resource_val[0];
    npc_class_len = (int)strlen(npc_class);
    trigger_len = (int)strlen(trigger_text);

    if (!mysql || state < SCHEMAVERIFIED)
    {
        eprintf("C_CheckQuestTrigger: MySQL not available\n");
        return ret_val.int_val;
    }

    // Query - check if trigger text matches quest for this NPC
    const char* query =
        "SELECT quest_template_id "
        "FROM quest_templates "
        "WHERE quest_kod_class = ? "
        "AND LOWER(quest_description) LIKE CONCAT('%', LOWER(?), '%') "
        "AND active = 1 "
        "LIMIT 1";

    MySQLLock();

    stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        eprintf("C_CheckQuestTrigger: mysql_stmt_init failed\n");
        MySQLUnlock();
        return ret_val.int_val;
    }

    if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)))
    {
        eprintf("C_CheckQuestTrigger: prepare failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind parameters
    memset(bind_params, 0, sizeof(bind_params));

    bind_params[0].buffer_type = MYSQL_TYPE_STRING;
    bind_params[0].buffer = (char*)npc_class;
    bind_params[0].buffer_length = npc_class_len;

    bind_params[1].buffer_type = MYSQL_TYPE_STRING;
    bind_params[1].buffer = (char*)trigger_text;
    bind_params[1].buffer_length = trigger_len;

    if (mysql_stmt_bind_param(stmt, bind_params))
    {
        eprintf("C_CheckQuestTrigger: bind_param failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Execute query
    if (mysql_stmt_execute(stmt))
    {
        eprintf("C_CheckQuestTrigger: execute failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Bind result
    memset(bind_result, 0, sizeof(bind_result));

    bind_result[0].buffer_type = MYSQL_TYPE_LONG;
    bind_result[0].buffer = (char*)&quest_id;
    bind_result[0].is_null = &is_null;
    bind_result[0].error = &error_flag;

    if (mysql_stmt_bind_result(stmt, bind_result))
    {
        eprintf("C_CheckQuestTrigger: bind_result failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        MySQLUnlock();
        return ret_val.int_val;
    }

    // Fetch result
    if (mysql_stmt_fetch(stmt) == 0 && !is_null)
    {
        ret_val.v.tag = TAG_INT;
        ret_val.v.data = quest_id;

        dprintf("C_CheckQuestTrigger: Found quest %d for NPC '%s' trigger '%s'\n",
                quest_id, npc_class, trigger_text);
    }

    mysql_stmt_close(stmt);
    MySQLUnlock();

    return ret_val.int_val;
}
