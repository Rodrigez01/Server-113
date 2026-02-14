// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* database.h
*

 BASIC USAGE:
 --------------------------
  * You must not call any method marked with '_' prefix from outside database.c
  * You should call methods MySQLRecordXY to record data to DB
  * Do not start adding includes to blakserv.h or other M59 related headers/stuff

 STEPS TO ADD A RECORD:
 --------------------------
  1) Design a SQL table layout and add the SQL query text to section SQL
  2) Design a stored procedure and add the SQL query text to section SQL
  3) Add the queries from (1) and (2) to _MySQLVerifySchema()
  4) --- TEST IF THEY GET CREATED ---
  5) Add a sql_record_xy enum and typedef below (matching your table design)
     Note: Don't use anything except simple datatypes (int, float, ..) and char* as parameters.
  6) Add a new sql_recordtype below (same entry must go to kod\include\blakston.khd)
  7) Create a MySQLRecordXY method to enqueue this kind of record (see examples)
	 Note: You must _strdup any char* parameters
  8) Create a _MySQLWriteXY method to write this kind of record (see examples)
	 Note: You must free all _strdups here
  9) Add the _MySQLWriterXY case to the 'switch' in _MySQLWriteNode()

*/

#ifndef _DATABASE_H
#define _DATABASE_H

#include <Windows.h>
#include <mysql.h>
#include <process.h>

#pragma region Structs/Enums
typedef struct sql_queue_node sql_queue_node;
typedef struct sql_queue sql_queue;
typedef struct sql_record_totalmoney sql_record_totalmoney;
typedef struct sql_record_moneycreated sql_record_moneycreated;
typedef struct sql_record_playerlogin sql_record_playerlogin;
typedef struct sql_record_playerassessdamage sql_record_playerassessdamage;
typedef struct sql_record_playerdeath sql_record_playerdeath;
typedef struct sql_record_player sql_record_player;
typedef struct sql_record_playersuicide sql_record_playersuicide;
typedef struct sql_record_guild sql_record_guild;
typedef struct sql_record_guilddisband sql_record_guilddisband;
typedef struct sql_record_account sql_record_account;
typedef enum sql_recordtype sql_recordtype;
typedef enum sql_worker_state sql_worker_state;

struct sql_queue_node
{
	sql_recordtype type;
	void* data;
	sql_queue_node* next;
};

struct sql_queue
{
	HANDLE mutex;
	int count;
	sql_queue_node* first;
	sql_queue_node* last;
};

struct sql_record_totalmoney
{
	int total_money;
};

struct sql_record_moneycreated
{
	int money_created;
};

struct sql_record_playerlogin
{
	char* account;
	char* character;
	char* ip;
};

struct sql_record_playerassessdamage
{
	char* who;
	char* attacker;
	int aspell;
	int atype;
	int applied;
	int original;
	char* weapon;
};

struct sql_record_playerdeath
{
   char* victim;
   char* killer;
   char* room;
   char* attack;
   int ispvp;
};

struct sql_record_player
{
   int account_id;
   char* name;
   char* home;
   char* bind;
   char* guild;
   int max_health;
   int max_mana;
   int might;
   int p_int;
   int myst;
   int stam;
   int aim;
   int agil;
};

struct sql_record_playersuicide
{
   int account_id;
   char* name;
};

struct sql_record_guild
{
   char* name;
   char* leader;
   char* hall;
};

struct sql_record_guilddisband
{
   char* name;
};

struct sql_record_account
{
   int account_id;
   char* name;
   char* password;
   char* email;
   int type;
   int credits;
   int last_login_time;
   int suspend_time;
};

enum sql_recordtype
{
	STAT_TOTALMONEY		= 1,
	STAT_MONEYCREATED	= 2,
	STAT_PLAYERLOGIN	= 3,
	STAT_ASSESS_DAM		= 4,
   STAT_PLAYERDEATH = 5,
   STAT_PLAYER = 6,
   STAT_PLAYERSUICIDE = 7,
   STAT_GUILD = 8,
   STAT_GUILDDISBAND = 9,
   STAT_ACCOUNT = 10
};

enum sql_worker_state
{
	STOPPED			= 0,
	STOPPING		= 1,
	STARTING		= 2,
	INITIALIZED		= 3,
	CONNECTED		= 4,
	SCHEMAVERIFIED	= 5
};
#pragma endregion

void MySQLInit(char* Host, char* User, char* Password, char* DB);
void MySQLEnd();
void MySQLLock(void);
void MySQLUnlock(void);
BOOL MySQLIsReady(void);
void MySQLAutoExport(int force_export);
BOOL MySQLRecordTotalMoney(int total_money);
BOOL MySQLRecordMoneyCreated(int money_created);
BOOL MySQLRecordPlayerLogin(char* account, char* character, char* ip);
BOOL MySQLRecordPlayerAssessDamage(char* who, char* attacker, int aspell, int atype, int applied, int original, char* weapon);
BOOL MySQLRecordPlayerDeath(char* victim, char* killer, char* room, char* attack, int ispvp);
BOOL MySQLRecordPlayer(int account_id, char* name, char* home, char* bind, char* guild,
   int max_health, int max_mana, int might, int p_int, int myst,
   int stam, int agil, int aim);
BOOL MySQLRecordPlayerSuicide(int account_id, char* name);
BOOL MySQLRecordGuild(char* name, char* leader, char* hall);
BOOL MySQLRecordGuildDisband(char* name);
BOOL MySQLSaveAccount(int account_id, char* name, char* password, char* email,
   int type, int credits, int last_login_time, int suspend_time);
BOOL MySQLLoadAccounts(void);
BOOL MySQLSaveAllAccounts(void);

void __cdecl _MySQLWorker(void* Parameters);
void _MySQLVerifySchema();
void _MySQLCreateAccountSchema(void);
void _MySQLCreateQuestSchema(void);
BOOL _MySQLEnqueue(sql_queue_node* Node);
BOOL _MySQLDequeue(BOOL processNode);
void _MySQLCallProc(char* Name, MYSQL_BIND Parameters[]);
void _MySQLWriteNode(sql_queue_node* Node, BOOL ProcessNode);
void _MySQLWriteTotalMoney(sql_record_totalmoney* Data, BOOL ProcessNode);
void _MySQLWriteMoneyCreated(sql_record_moneycreated* Data, BOOL ProcessNode);
void _MySQLWritePlayerLogin(sql_record_playerlogin* Data, BOOL ProcessNode);
void _MySQLWritePlayerAssessDamage(sql_record_playerassessdamage* Data, BOOL ProcessNode);
void _MySQLWritePlayerDeath(sql_record_playerdeath* Data, BOOL ProcessNode);
void _MySQLWritePlayer(sql_record_player* Data, BOOL ProcessNode);
void _MySQLWritePlayerSuicide(sql_record_playersuicide* Data, BOOL ProcessNode);
void _MySQLWriteGuild(sql_record_guild* Data, BOOL ProcessNode);
void _MySQLWriteGuildDisband(sql_record_guilddisband* Data, BOOL ProcessNode);
void _MySQLWriteAccount(sql_record_account* Data, BOOL ProcessNode);

/* Quest System Functions (dbquest.c) */
int MySQLExportQuestData(void);

/* Quest Loader (questloader.c) */
typedef struct {
   int quest_template_id;
   char quest_name[101];
   char quest_kod_class[101];
   char quest_description[256];
   char quest_type[20];
   char difficulty[20];
   int min_level;
   int max_level;
   int repeatable;  /* 0=False, 1=True */
   int active;      /* 0=False, 1=True */
} quest_template_t;

int LoadQuestTemplatesFromMySQL(void);
quest_template_t* GetQuestTemplateByID(int quest_id);
quest_template_t* GetAllQuestTemplates(int* count);
int IsQuestAvailableForPlayer(int quest_id, int player_level);
int GetQuestTemplatesByType(const char* quest_type, int* results, int max_results);

/* Quest Trigger Functions (questloader.c) */
int LoadQuestTriggersFromMySQL(void);
int CheckQuestTrigger(const char* npc_class, const char* trigger_text);
int GetTriggersForNPC(const char* npc_class, int* results, int max_results);

/* Quest Node Loader (questnodeloader.c) */
struct quest_node_data_s {
   int quest_node_id;
   int quest_node_index;
   int quest_template_id;
   int node_order;
   char node_type[51];
   int time_limit;
   char npc_modifier[51];
};
typedef struct quest_node_data_s quest_node_data_t;

struct quest_node_npc_s {
   int quest_node_id;
   char npc_class[101];
   char npc_role[51];
};
typedef struct quest_node_npc_s quest_node_npc_t;

struct quest_dialog_s {
   int quest_node_id;
   char dialog_type[51];
   char dialog_context[101];
   char dialog_text[2048];
};
typedef struct quest_dialog_s quest_dialog_t;

/* Deprecated - Quest Node functions removed (questnodeloader.c removed) */
/* Quest triggers now handled by in-memory cache in questloader.c */

/* ============================================================================
 * ITEM TEMPLATE SYSTEM
 * ============================================================================
 */

/* Item category enum - matches SQL ENUM */
typedef enum {
   ITEM_CAT_WEAPON = 0,
   ITEM_CAT_ARMOR,
   ITEM_CAT_SHIELD,
   ITEM_CAT_HELMET,
   ITEM_CAT_PANTS,
   ITEM_CAT_SHIRT,
   ITEM_CAT_STACKABLE,
   ITEM_CAT_CONSUMABLE,
   ITEM_CAT_QUEST_ITEM,
   ITEM_CAT_MISC
} item_category_t;

/* Weapon type enum */
typedef enum {
   WEAPON_TYPE_THRUST = 0,
   WEAPON_TYPE_SLASH,
   WEAPON_TYPE_BLUDGEON
} weapon_type_t;

/* Weapon quality enum */
typedef enum {
   WEAPON_QUALITY_LOW = 0,
   WEAPON_QUALITY_NORMAL,
   WEAPON_QUALITY_HIGH,
   WEAPON_QUALITY_NERUDITE
} weapon_quality_t;

/* Item Template structure - base for all items */
typedef struct {
   int item_template_id;
   char item_name[101];
   char item_kod_class[101];
   char item_description[256];
   char item_icon[101];

   /* Base statistics */
   int weight;
   int bulk;
   int value_average;
   int hits_init_min;
   int hits_init_max;

   /* Type flags */
   int use_type;
   int item_type;

   /* Category */
   item_category_t item_category;

   /* Status */
   int active;
} item_template_t;

/* Weapon-specific data */
typedef struct {
   int item_template_id;
   weapon_type_t weapon_type;
   weapon_quality_t weapon_quality;
   int proficiency_needed;
   int weapon_range;
   int attack_type;
} item_weapon_t;

/* Defense modifier data (armor, shield, helmet, etc.) */
typedef struct {
   int item_template_id;
   int defense_base;
   int damage_base;
   int spell_modifier;
   int layer;
   /* Resistances stored as simple array [fire, cold, acid, shock, magic] */
   int resistances[5];
} item_defense_t;

/* Stackable item data */
typedef struct {
   int item_template_id;
   int max_stack;
   int is_reagent;
} item_stackable_t;

/* Item Loader Functions (itemloader.c) */
int LoadItemTemplatesFromMySQL(void);
item_template_t* GetItemTemplateByID(int item_id);
item_template_t* GetItemTemplateByClass(const char* class_name);
item_template_t* GetAllItemTemplates(int* count);
int IsItemActive(int item_id);
int GetItemsByCategory(item_category_t category, int* results, int max_results);

/* Item Weapon Functions (itemloader.c) */
item_weapon_t* GetItemWeaponData(int item_id);

/* Item Defense Functions (itemloader.c) */
item_defense_t* GetItemDefenseData(int item_id);

/* Item Stackable Functions (itemloader.c) */
item_stackable_t* GetItemStackableData(int item_id);

/* Item Schema Creation (database.c) */
void _MySQLCreateItemSchema(void);

/* Item Export (dbitem.c) */
int MySQLExportItemData(void);

/* ============================================================================
 * MONSTER TEMPLATE SYSTEM
 * ============================================================================
 */

/* Maximum number of resistances per monster */
#define MAX_MONSTER_RESISTANCES 20

/* Maximum number of spells per monster */
#define MAX_MONSTER_SPELLS 10

/* Monster Resistance Entry */
typedef struct {
    int attack_type;        /* ATCK_WEAP_* or -ATCK_SPELL_* */
    int value;              /* -100 to +100 */
} monster_resistance_t;

/* Monster Spell Entry */
typedef struct {
    int spell_num;          /* SID_* */
    int mana_cost;
    int trigger_chance;     /* 1-100 */
} monster_spell_t;

/* Monster Template structure - main structure for all monsters */
typedef struct {
    int monster_template_id;
    char monster_name[101];
    char monster_kod_class[101];
    char monster_icon[101];
    char monster_dead_icon[101];

    /* Combat Stats */
    int level;              /* viLevel */
    int difficulty;         /* viDifficulty */
    int max_hit_points;     /* piMax_hit_points */
    int max_mana;           /* piMax_mana */

    /* Combat Properties */
    int attack_type;        /* viAttack_type */
    int attack_spell;       /* viAttack_spell */
    int attack_range;       /* viAttackRange */
    int speed;              /* viSpeed */
    int vision_distance;    /* viVisionDistance */

    /* Loot/Economy */
    int cash_min;           /* viCashmin */
    int cash_max;           /* viCashmax */
    int treasure_type;      /* viTreasure_type */

    /* AI/Behavior */
    int default_behavior;   /* viDefault_behavior */
    int wimpy;              /* viWimpy */
    int spell_chance;       /* viSpellChance */
    int spell_power;        /* piSpellPower */

    /* Classification */
    int karma;              /* viKarma */
    int is_undead;          /* vbIsUndead */
    int is_npc;             /* AI_NPC flag */

    /* Sounds */
    char sound_hit[101];
    char sound_miss[101];
    char sound_aware[101];
    char sound_death[101];

    /* Status */
    int active;

    /* Inline arrays for fast access */
    monster_resistance_t resistances[MAX_MONSTER_RESISTANCES];
    int num_resistances;

    monster_spell_t spells[MAX_MONSTER_SPELLS];
    int num_spells;

} monster_template_t;

/* Monster Loader Functions (monsterloader.c) */
int LoadMonsterTemplatesFromMySQL(void);
monster_template_t* GetMonsterTemplateByID(int monster_id);
monster_template_t* GetMonsterTemplateByClass(const char* class_name);
monster_template_t* GetAllMonsterTemplates(int* count);
int IsMonsterActive(int monster_id);
int GetMonstersByLevel(int min_level, int max_level, int* results, int max_results);

/* Monster Schema Creation (database.c) */
void _MySQLCreateMonsterSchema(void);

/* Monster Export (dbmonster.c) */
int MySQLExportMonsterData(void);

/* ============================================================================
 * NPC System Structures and Functions (Simplified - Phase 1)
 * ============================================================================ */

/* NPC Occupation Constants (match blakston.khd) */
#define NPC_ROLE_NONE        0
#define NPC_ROLE_BLACKSMITH  1
#define NPC_ROLE_APOTHECARY  2
#define NPC_ROLE_INNKEEPER   3
#define NPC_ROLE_BANKER      4
#define NPC_ROLE_BARTENDER   5
#define NPC_ROLE_MERCHANT    6
#define NPC_ROLE_ELDER       7
#define NPC_ROLE_SCHOLAR     8
#define NPC_ROLE_TRAINER     9

/* Simplified NPC Template Structure - similar size to monster_template_t */
typedef struct {
    int npc_template_id;
    char npc_name[101];
    char npc_kod_class[101];
    char npc_icon[101];

    /* Location */
    char town[51];
    char room_kod_class[101];

    /* Role/Occupation */
    int occupation;

    /* Merchant Settings */
    int merchant_markup;
    int is_seller;
    int is_buyer;
    int is_teacher;
    int is_banker;

    /* Faction */
    int is_lawful;
    int faction;

    int active;
} npc_template_t;

/* NPC Loader Functions (npcloader.c) */
int LoadNPCTemplatesFromMySQL(void);
npc_template_t* GetNPCTemplateByID(int npc_id);
npc_template_t* GetNPCTemplateByClass(const char* class_name);
npc_template_t* GetAllNPCTemplates(int* count);

/* NPC Schema Creation (database.c) */
void _MySQLCreateNPCSchema(void);

/* NPC Export (dbnpc.c) */
int MySQLExportNPCData(void);

/* ============================================================================
 * SPELL TEMPLATE SYSTEM
 * ============================================================================
 */

/* Spell School Constants (match blakston.khd) */
#define SS_SHALILLE   1
#define SS_QOR        2
#define SS_KRAANAN    3
#define SS_FAREN      4
#define SS_RIIJA      5
#define SS_JALA       6
#define SS_DM_COMMAND 7
#define SS_CRAFTING   8
#define SS_NECROMANTIE 9

/* Maximum reagents per spell */
#define MAX_SPELL_REAGENTS 5

/* Spell Reagent Entry */
typedef struct {
    char reagent_class[101];
    int amount;
} spell_reagent_t;

/* Spell Template Structure */
typedef struct {
    int spell_template_id;
    char spell_name[101];
    char spell_kod_class[101];
    char spell_icon[101];
    char spell_description[256];

    /* Identification */
    int spell_num;              /* viSpell_Num (SID_*) */
    int school;                 /* viSchool */
    int spell_level;            /* viSpell_level (1-6) */

    /* Costs */
    int mana_cost;              /* viMana */
    int exertion;               /* viSpellExertion */

    /* Timing */
    int cast_time;              /* viCast_time (ms) */
    int post_cast_time;         /* viPostCast_time (sec) */

    /* Learning */
    int chance_to_increase;     /* viChance_To_Increase */
    int meditate_ratio;         /* viMeditate_ratio */

    /* Flags */
    int is_harmful;             /* viHarmful */
    int is_outlaw;              /* viOutlaw */
    int is_personal_ench;       /* viPersonal_ench */

    /* Status */
    int active;

    /* Reagents */
    spell_reagent_t reagents[MAX_SPELL_REAGENTS];
    int num_reagents;

} spell_template_t;

/* Spell Loader Functions (spellloader.c) */
int LoadSpellTemplatesFromMySQL(void);
spell_template_t* GetSpellTemplateByID(int spell_id);
spell_template_t* GetSpellTemplateByClass(const char* class_name);
spell_template_t* GetSpellTemplateByNum(int spell_num);
spell_template_t* GetAllSpellTemplates(int* count);
int GetSpellsBySchool(int school, int* results, int max_results);

/* Spell Schema Creation (database.c) */
void _MySQLCreateSpellSchema(void);

/* Spell Export (dbspell.c) */
int MySQLExportSpellData(void);

/* ============================================================================
 * SKILL TEMPLATE SYSTEM
 * ============================================================================
 */

/* Skill Template Structure */
typedef struct {
    int skill_template_id;
    char skill_name[101];
    char skill_kod_class[101];
    char skill_icon[101];

    /* Identification */
    int skill_num;              /* viSkill_num (SKID_*) */
    int school;                 /* viSchool */
    int skill_level;            /* viSkill_level (1-6) */

    /* Costs */
    int exertion;               /* viSpellExertion */
    int meditate_ratio;         /* viMeditate_ratio */

    /* Flags */
    int is_combat;              /* Combat skill flag */

    /* Status */
    int active;

} skill_template_t;

/* Skill Loader Functions (skillloader.c) */
int LoadSkillTemplatesFromMySQL(void);
skill_template_t* GetSkillTemplateByID(int skill_id);
skill_template_t* GetSkillTemplateByClass(const char* class_name);
skill_template_t* GetSkillTemplateByNum(int skill_num);
skill_template_t* GetAllSkillTemplates(int* count);
int GetSkillsBySchool(int school, int* results, int max_results);

/* Skill Schema Creation (database.c) */
void _MySQLCreateSkillSchema(void);

/* Skill Export (dbskill.c) */
int MySQLExportSkillData(void);

/* ============================================================================
 * ROOM TEMPLATE SYSTEM
 * ============================================================================
 */

/* Room Template Structure */
typedef struct {
    int room_template_id;
    char room_name[101];
    char room_kod_class[101];
    int room_num;
    int active;
} room_template_t;

/* Room Loader Functions (roomloader.c) */
int LoadRoomTemplatesFromMySQL(void);
room_template_t* GetRoomTemplateByID(int room_id);
room_template_t* GetRoomTemplateByClass(const char* class_name);
room_template_t* GetRoomTemplateByNum(int room_num);
room_template_t* GetAllRoomTemplates(int* count);

/* Room Schema Creation (database.c) */
void _MySQLCreateRoomSchema(void);

/* Room Export (dbroom.c) */
int MySQLExportRoomData(void);

/* ============================================================================
 * PLAYER CHARACTER SYSTEM
 * ============================================================================
 */

/* Player Template Structure - represents a player character from MySQL */
typedef struct {
    int player_id;              /* idplayer from MySQL */
    int account_id;             /* player_account_id */
    char player_name[46];       /* player_name */
    char player_home[256];      /* player_home */
    char player_bind[256];      /* player_bind */
    char player_guild[46];      /* player_guild */

    /* Stats */
    int max_health;             /* player_max_health */
    int max_mana;               /* player_max_mana */
    int might;                  /* player_might */
    int intellect;              /* player_int */
    int mysticism;              /* player_myst */
    int stamina;                /* player_stam */
    int agility;                /* player_agil */
    int aim;                    /* player_aim */

    /* Runtime */
    int object_id;              /* Associated game object ID (0 if not found) */

} player_template_t;

/* Player Loader Functions (playerloader.c) */
BOOL LoadPlayerTemplatesFromMySQL(void);
void AssociatePlayersWithAccounts(void);
player_template_t* GetPlayerTemplateByID(int player_id);
player_template_t* GetPlayerTemplateByAccountID(int account_id);
player_template_t* GetPlayerTemplateByName(const char* name);
player_template_t* GetPlayerTemplateByIndex(int index);
int GetNumPlayerTemplates(void);

#endif
