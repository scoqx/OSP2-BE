#ifndef CG_SUPERHUD_PRIVATE_H
#define CG_SUPERHUD_PRIVATE_H

#include "../cg_local.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUPERHUD_DEFAULT_FADEDELAY 1000.0
#define SUPERHUD_EXECUTE_MAX_LEN   1024

/* sbhc/sbac style bitmask: bit0 = color by total HP+armor; bit1 = cap shown number at 999 */
#define SHUD_SBCOUNT_STYLE_TOTALHP_COLOR (1 << 0)
#define SHUD_SBCOUNT_STYLE_CAP999        (1 << 1)

typedef enum
{
	SUPERHUD_ALIGNH_LEFT,
	SUPERHUD_ALIGNH_CENTER,
	SUPERHUD_ALIGNH_RIGHT,
} superhudAlignH_t;

typedef enum
{
	SUPERHUD_ALIGNV_TOP,
	SUPERHUD_ALIGNV_CENTER,
	SUPERHUD_ALIGNV_BOTTOM,
} superhudAlignV_t;

typedef enum
{
	SUPERHUD_COLOR_RGBA,
	SUPERHUD_COLOR_T,
	SUPERHUD_COLOR_E,
	SUPERHUD_COLOR_I,
} superhudColorType_t;

typedef struct
{
	superhudColorType_t type;
	vec4_t rgba;
} superhudColor_t;

typedef enum
{
	SUPERHUD_DIR_LEFT_TO_RIGHT,
	SUPERHUD_DIR_RIGHT_TO_LEFT,
	SUPERHUD_DIR_TOP_TO_BOTTOM,
	SUPERHUD_DIR_BOTTOM_TO_TOP,
} superhudDirection_t;

typedef enum
{
	SUPERHUD_ITTEAM_BLUE,
	SUPERHUD_ITTEAM_RED,
	SUPERHUD_ITTEAM_NEUTRAL,
	SUPERHUD_ITTEAM_OWN,
	SUPERHUD_ITTEAM_ENEMY,
} superhudItTeam_t;

typedef struct
{
	struct
	{
		superhudAlignH_t value;
		qboolean isSet;
	} alignH;
	struct
	{
		superhudAlignV_t value;
		qboolean isSet;
	} alignV;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} angles;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} bgcolor;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} bgcolor2;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} border;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} borderColor;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} borderColor2;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} hlcolor;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} color;
	struct
	{
		superhudColor_t value;
		qboolean isSet;
	} color2;
	struct
	{
		superhudDirection_t value;
		qboolean isSet;
	} direction;
	struct
	{
		qboolean isSet;
	} doublebar;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} fade;
	struct
	{
		int value;
		qboolean isSet;
	} fadedelay;
	struct
	{
		qboolean isSet;
	} fill;
	struct
	{
		char value[MAX_QPATH];
		qboolean isSet;
	} font;
	struct
	{
		vec2_t value;
		qboolean isSet;
	} fontsize;
	struct
	{
		char value[MAX_QPATH];
		qboolean isSet;
	} image;
	struct
	{
		char value[MAX_QPATH];
		qboolean isSet;
	} image2;
	struct
	{
		superhudAlignH_t value;
		qboolean isSet;
	} imageAlignH;
	struct
	{
		superhudAlignV_t value;
		qboolean isSet;
	} imageAlignV;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} imagetc;
	struct
	{
		superhudItTeam_t value;
		qboolean isSet;
	} itTeam;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} margins;
	struct
	{
		char value[MAX_QPATH];
		qboolean isSet;
	} model;
	struct
	{
		qboolean isSet;
	} monospace;
	struct
	{
		vec3_t value;
		qboolean isSet;
	} offset;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} rect;
	struct
	{
		vec4_t value;
		qboolean isSet;
	} shadowColor;
	struct
	{
		int value;
		qboolean isSet;
	} style;
	struct
	{
		char value[MAX_QPATH];
		qboolean isSet;
	} text;
	struct
	{
		superhudAlignH_t value;
		qboolean isSet;
	} textAlign;
	struct
	{
		vec2_t value;
		qboolean isSet;
	} textOffset;
	struct
	{
		int value;
		qboolean isSet;
	} textStyle;
	struct
	{
		int value;
		qboolean isSet;
	} time;
	struct
	{
		int value;
		int valueNeg;
		int valueExt;
		int valueNegExt;
		qboolean isSet;
	} visflags;
	struct
	{
		int value;
		qboolean isSet;
	} hlsize;
	struct
	{
		int value;
		int valueNeg;
		qboolean isSet;
	} visiblity;
	struct
	{
		char value[SUPERHUD_EXECUTE_MAX_LEN];
		qboolean isSet;
	} execute;
	struct
	{
		char value[256];
		qboolean isSet;
	} weaponMaxValue;
} superhudConfig_t;

typedef struct configFileLine_s
{
	char* line;
	int size;
	int line_number;
	struct configFileLine_s* next;
} configFileLine_t;

typedef struct configFileInfo_s
{
	int pos; /* position in line */
	configFileLine_t* root;
	configFileLine_t* last_line;
} configFileInfo_t;

typedef enum
{
	SUPERHUD_CONFIG_OK,
	SUPERHUD_CONFIG_UNEXPECTED_CHARACTER,
	SUPERHUD_CONFIG_END_OF_FILE,
	SUPERHUD_CONFIG_END_OF_ELEMENT,
	SUPERHUD_CONFIG_WRONG_ELEMENT_NAME,
	SUPERHUD_CONFIG_WRONG_COMMAND_NAME,
	SUPERHUD_CONFIG_LOST_ELEMENT_BODY,
} superhudConfigParseStatus_t;

#define SE_IM         (1 << 0)  // 0x00000001
#define SE_IM_STR "im"
#define SE_TEAM_ONLY  (1 << 1)  // 0x00000002
#define SE_TEAM_ONLY_STR "teamonly"
#define SE_SPECT      (1 << 2)  // 0x00000004
#define SE_SPECT_STR      "spectator"
#define SE_DEAD       (1 << 3)  // 0x00000008
#define SE_DEAD_STR       "dead"
#define SE_DEMO_HIDE  (1 << 4)  // 0x00000010
#define SE_DEMO_HIDE_STR  "demohide"
#define SE_SCORES_HIDE  (1 << 5)  // 0x00000020
#define SE_SCORES_HIDE_STR  "scoreshide"
#define SE_KEY1_SHOW  (1 << 6)  // 0x00000040
#define SE_KEY1_SHOW_STR  "key1show"
#define SE_KEY2_SHOW  (1 << 7)  // 0x00000080
#define SE_KEY2_SHOW_STR  "key2show"
#define SE_KEY3_SHOW  (1 << 8)  // 0x00000100
#define SE_KEY3_SHOW_STR  "key3show"
#define SE_KEY4_SHOW  (1 << 9)  // 0x00000200
#define SE_KEY4_SHOW_STR  "key4show"
#define SE_KEY5_SHOW  (1U << 27)  // 0x08000000
#define SE_KEY5_SHOW_STR  "key5show"
#define SE_KEY6_SHOW  (1U << 28)  // 0x10000000
#define SE_KEY6_SHOW_STR  "key6show"
#define SE_KEY7_SHOW  (1U << 29)  // 0x20000000
#define SE_KEY7_SHOW_STR  "key7show"
#define SE_KEY8_SHOW  (1U << 30)  // 0x40000000
#define SE_KEY8_SHOW_STR  "key8show"
#define SE_SHOW_EMPTY  (1 << 10) // 0x00000400
#define SE_SHOW_EMPTY_STR  "showempty"
#define SE_GT_FFA   (1 << 11) // 0x00000800
#define SE_GT_FFA_STR "gt_ffa"
#define SE_GT_TOURNEY   (1 << 12) // 0x00001000
#define SE_GT_TOURNEY_STR "gt_tourney"
#define SE_GT_TDM      (1 << 13) // 0x00002000
#define SE_GT_TDM_STR  "gt_tdm"
#define SE_GT_CTF       (1 << 14) // 0x00004000
#define SE_GT_CTF_STR   "gt_ctf"
#define SE_GT_FREEZETAG (1 << 15) // 0x00008000
#define SE_GT_FREEZETAG_STR "gt_freezetag"
#define SE_GT_CLANARENA (1 << 16) // 0x00010000
#define SE_GT_CLANARENA_STR "gt_clanarena"
#define SE_WEAPON_GAUNTLET (1 << 17) // 0x00020000
#define SE_WEAPON_GAUNTLET_STR "gauntlet"
#define SE_WEAPON_MACHINEGUN (1 << 18) // 0x00040000
#define SE_WEAPON_MACHINEGUN_STR "machinegun"
#define SE_WEAPON_SHOTGUN (1 << 19) // 0x00080000
#define SE_WEAPON_SHOTGUN_STR "shotgun"
#define SE_WEAPON_GRENADE_LAUNCHER (1 << 20) // 0x00100000
#define SE_WEAPON_GRENADE_LAUNCHER_STR "grenade"
#define SE_WEAPON_ROCKET_LAUNCHER (1 << 21) // 0x00200000
#define SE_WEAPON_ROCKET_LAUNCHER_STR "rocket"
#define SE_WEAPON_LIGHTNING (1 << 22) // 0x00400000
#define SE_WEAPON_LIGHTNING_STR "lightning"
#define SE_WEAPON_RAILGUN (1 << 23) // 0x00800000
#define SE_WEAPON_RAILGUN_STR "railgun"
#define SE_WEAPON_PLASMAGUN (1 << 24) // 0x01000000
#define SE_WEAPON_PLASMAGUN_STR "plasma"
#define SE_WEAPON_BFG (1 << 25) // 0x02000000
#define SE_WEAPON_BFG_STR "bfg"
#define SE_HEALTH_LOW (1 << 26) // 0x04000000
#define SE_HEALTH_LOW_STR "lowhp"
#define SE_WARMUP (1U << 31)
#define SE_WARMUP_STR "warmup"

/* Extended visflags (second bitmask; main SE_* uses all 32 bits) */
#define SE_ENTERCHAT (1 << 0)
#define SE_ENTERCHAT_STR "enterchat"
#define SE_SHOWCHAT (1 << 1)
#define SE_SHOWCHAT_STR "showchat"
#define SE_PRINTUI (1 << 2)
#define SE_PRINTUI_STR "printui"
#define SE_BESTATS (1 << 3)
#define SE_BESTATS_STR "bestats"

#define SE_SHUD_STYLE1 (1 << 0)
#define SE_SHUD_STYLE1_STR "style1"
#define SE_SHUD_STYLE2 (1 << 1)
#define SE_SHUD_STYLE2_STR "style2"
#define SE_SHUD_STYLE3 (1 << 2)
#define SE_SHUD_STYLE3_STR "style3"
#define SE_SHUD_STYLE4 (1 << 3)
#define SE_SHUD_STYLE4_STR "style4"
#define SE_SHUD_STYLE_ANY (SE_SHUD_STYLE1 | SE_SHUD_STYLE2 | SE_SHUD_STYLE3 | SE_SHUD_STYLE4)

#define SE_GT_ANY (SE_GT_FFA | SE_GT_TOURNEY | SE_GT_TDM | SE_GT_CTF | SE_GT_FREEZETAG | SE_GT_CLANARENA)

#define SE_KEY_ANY (SE_KEY1_SHOW | SE_KEY2_SHOW | SE_KEY3_SHOW | SE_KEY4_SHOW | \
	SE_KEY5_SHOW | SE_KEY6_SHOW | SE_KEY7_SHOW | SE_KEY8_SHOW)

#define SHUD_CHECK_SHOW_EMPTY(element) ( \
    ((element) != NULL) && \
    ((element)->config.visflags.isSet) && \
    (((element)->config.visflags.value & SE_SHOW_EMPTY) != 0) && \
    (((element)->config.visflags.valueNeg & SE_SHOW_EMPTY) == 0) \
)

typedef struct superHUDConfigElement_s
{
	const char* name;
	int visibility;
	void* (*create)(const superhudConfig_t* config);
	void (*routine)(void* context);
	void (*destroy)(void* context);
	void* context;
	int order;
	qboolean isFragmessage;
	int visibilityNeg;
	int visibilityExt;
	int visibilityNegExt;
} superHUDConfigElement_t;

typedef struct superhudElementDictMember_s
{
	const superHUDConfigElement_t* element;
	struct superhudElementDictMember_s* next;
}
superhudElementDictMember_t;

/*
 * Visibility data for one HUD element (filled at config load, read each frame).
 *
 * vflags / vflagsNeg — copy of element visflags (see SE_* in this header).
 * vflagsExt / vflagsNegExt — extended visflags (enterchat/showchat/printui/bestats).
 *
 * showInIntermission, showWhenDead, showWhenSpect — without the matching positive
 * visflags bit the element is hidden in that state (default HUD behavior).
 *
 * frameChecks — internal bitmask: which optional per-frame rules still apply
 * (team, scores, weapon, keys, …). Built from vflags; not used outside cgame.
 */
typedef struct
{
	int vflags;
	int vflagsNeg;
	int vflagsExt;
	int vflagsNegExt;
	int frameChecks;
	qboolean showInIntermission;
	qboolean showWhenDead;
	qboolean showWhenSpect;
} superhudElementVis_t;

typedef struct superhudElement_s
{
	superHUDConfigElement_t element;
	superhudConfig_t config;
	superhudElementVis_t vis;
	struct superhudElement_s* next;
}
superhudElement_t;

typedef struct
{
	int pm_type;
	int currentWeapon;
	int weaponVis;
	int gtHideMask;
	int keyHideMask;
	qboolean is_dead;
	qboolean is_intermission;
	qboolean is_team_game;
	qboolean is_spectator;
	qboolean is_scores;
	qboolean is_warmup;
	qboolean is_enterchat;
	qboolean is_showchat;
	qboolean is_printui;
	qboolean is_bestats;
	qboolean demo_playback;
} superhudFrameContext_t;

typedef struct superHUDConfigCommand_s
{
	const char* name;
	superhudConfigParseStatus_t (*parse)(configFileInfo_t* finfo, superhudConfig_t* config);
	struct superHUDConfigCommand_s* next;
} superHUDConfigCommand_t;

typedef struct
{
	const superHUDConfigCommand_t* item;
	superhudConfigParseStatus_t status;
} superhudConfigParseCommand_t;

typedef struct
{
	const superHUDConfigElement_t* item;
	superhudConfigParseStatus_t status;
	char name[MAX_QPATH];
} superhudConfigParseElement_t;

#define SHUD_ELEMENT_INIT(E, CFG)                     \
    do{                                                 \
        E = Z_Malloc(sizeof(*E));                         \
        OSP_MEMORY_CHECK(E);                              \
        memset(E, 0, sizeof(*E));                         \
        memcpy(&E->config, CFG, sizeof((E)->config)); \
    }while(0)

#define SHUD_WRITE_MATCH_TIME_STR(buf, buflen, msecIn) \
	do { \
		if (cg_drawTimer.integer == 0 && !CG_IS_LOCAL_CLIENT_SPECTATOR()) \
		{ \
			Q_strncpyz((buf), "XX:XX", (buflen)); \
		} \
		else \
		{ \
			int _mt_msec = (msecIn); \
			int _mt_sec; \
			int _mt_min; \
			int _mt_tens; \
			if (_mt_msec < 0) \
			{ \
				_mt_msec = -_mt_msec; \
			} \
			_mt_sec = _mt_msec / 1000; \
			_mt_min = _mt_sec / 60; \
			_mt_sec -= _mt_min * 60; \
			_mt_tens = _mt_sec / 10; \
			_mt_sec -= _mt_tens * 10; \
			Com_sprintf((buf), (buflen), "%i:%i%i", _mt_min, _mt_tens, _mt_sec); \
		} \
	} while (0)

void CG_SHUDParserInit(void);
const superHUDConfigElement_t* CG_SHUDFindConfigElementItem(const char* name);
const superHUDConfigCommand_t* CG_SHUDFindConfigCommandItem(const char* name);
superhudConfigParseStatus_t CG_SHUDParseVarElement(configFileInfo_t* finfo);
superhudConfigParseStatus_t CG_SHUDParseExecuteElement(configFileInfo_t* finfo, superhudConfig_t* config);

qboolean CG_SHUDFileInfoInit(configFileInfo_t* info, const char* fileContent);
void CG_SHUDFileInfoTeardown(configFileInfo_t* cfi);

qboolean CG_SHUDFileInfoGoToChar(configFileInfo_t* cfi, char to, qboolean next);
void CG_SHUDFileInfoSkipSpaces(configFileInfo_t* cfi);
qboolean CG_SHUDFileInfoSkipCommandEnd(configFileInfo_t* cfi);
const superhudConfigParseElement_t CG_SHUDFileInfoGetElementItem(configFileInfo_t* cfi);
const superhudConfigParseCommand_t CG_SHUDFileInfoGetCommandItem(configFileInfo_t* cfi);
qboolean CG_SHUDFileInfoSkipElementBody(configFileInfo_t* cfi);

void* CG_SHUDElementFPSCreate(const superhudConfig_t* config);
void CG_SHUDElementFPSRoutine(void* context);
void CG_SHUDElementFPSDestroy(void* context);

void* CG_SHUDElementSBHCCreate(const superhudConfig_t* config);
void CG_SHUDElementSBHCRoutine(void* context);
void CG_SHUDElementSBHCDestroy(void* context);

void* CG_SHUDElementSBHBCreate(const superhudConfig_t* config);
void CG_SHUDElementSBHBRoutine(void* context);
void CG_SHUDElementSBHBDestroy(void* context);

void* CG_SHUDElementSBHICreate(const superhudConfig_t* config);
void CG_SHUDElementSBHIRoutine(void* context);
void CG_SHUDElementSBHIDestroy(void* context);

void* CG_SHUDElementSBACCreate(const superhudConfig_t* config);
void CG_SHUDElementSBACRoutine(void* context);
void CG_SHUDElementSBACDestroy(void* context);

void* CG_SHUDElementSBABCreate(const superhudConfig_t* config);
void CG_SHUDElementSBABRoutine(void* context);
void CG_SHUDElementSBABDestroy(void* context);

void* CG_SHUDElementSBAICreate(const superhudConfig_t* config);
void CG_SHUDElementSBAIRoutine(void* context);
void CG_SHUDElementSBAIDestroy(void* context);

void* CG_SHUDElementSBAmBCreate(const superhudConfig_t* config);
void CG_SHUDElementSBAmBRoutine(void* context);
void CG_SHUDElementSBAmBDestroy(void* context);

void* CG_SHUDElementSBAmCCreate(const superhudConfig_t* config);
void CG_SHUDElementSBAmCRoutine(void* context);
void CG_SHUDElementSBAmCDestroy(void* context);

void* CG_SHUDElementSBAmICreate(const superhudConfig_t* config);
void CG_SHUDElementSBAmIRoutine(void* context);
void CG_SHUDElementSBAmIDestroy(void* context);

void* CG_SHUDElementTargetNameCreate(const superhudConfig_t* config);
void CG_SHUDElementTargetNameRoutine(void* context);
void CG_SHUDElementTargetNameDestroy(void* context);

void* CG_SHUDElementTargetStatusCreate(const superhudConfig_t* config);
void* CG_SHUDElementTargetStatusBarCreate(const superhudConfig_t* config);
void CG_SHUDElementTargetStatusRoutine(void* context);
void CG_SHUDElementTargetStatusDestroy(void* context);

void* CG_SHUDElementVMWCreate(const superhudConfig_t* config);
void CG_SHUDElementVMWRoutine(void* context);
void CG_SHUDElementVMWDestroy(void* context);

void* CG_SHUDElementAttackerNameCreate(const superhudConfig_t* config);
void CG_SHUDElementAttackerNameRoutine(void* context);
void CG_SHUDElementAttackerNameDestroy(void* context);

void* CG_SHUDElementAttackerIconCreate(const superhudConfig_t* config);
void CG_SHUDElementAttackerIconRoutine(void* context);
void CG_SHUDElementAttackerIconDestroy(void* context);

void* CG_SHUDElementFragMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementFragMessageRoutine(void* context);
void CG_SHUDElementFragMessageDestroy(void* context);

void* CG_SHUDElementKillConfirmCreate(const superhudConfig_t* config);
void CG_SHUDElementKillConfirmRoutine(void* context);
void CG_SHUDElementKillConfirmDestroy(void* context);

void* CG_SHUDElementCenterMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementCenterMessageRoutine(void* context);
void CG_SHUDElementCenterMessageDestroy(void* context);
qboolean CG_SHUDHasCenterMessage(void);

void* CG_SHUDElementRankMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementRankMessageRoutine(void* context);
void CG_SHUDElementRankMessageDestroy(void* context);

void* CG_SHUDElementNGPCreate(const superhudConfig_t* config);
void CG_SHUDElementNGPRoutine(void* context);
void CG_SHUDElementNGPDestroy(void* context);

void* CG_SHUDElementNGCreate(const superhudConfig_t* config);
void CG_SHUDElementNGRoutine(void* context);
void CG_SHUDElementNGDestroy(void* context);

void* CG_SHUDElementDecorCreate(const superhudConfig_t* config);
void CG_SHUDElementDecorRoutine(void* context);
void CG_SHUDElementDecorDestroy(void* context);

void* CG_SHUDElementPlayerSpeedCreate(const superhudConfig_t* config);
void CG_SHUDElementPlayerSpeedRoutine(void* context);
void CG_SHUDElementPlayerSpeedDestroy(void* context);

void* CG_SHUDElementLocalTimeCreate(const superhudConfig_t* config);
void* CG_SHUDElementLocalDateCreate(const superhudConfig_t* config);
void CG_SHUDElementLocalTimeRoutine(void* context);
void CG_SHUDElementLocalTimeDestroy(void* context);

void* CG_SHUDElementAmmoMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementAmmoMessageRoutine(void* context);
void CG_SHUDElementAmmoMessageDestroy(void* context);

void* CG_SHUDElementChat1Create(const superhudConfig_t* config);
void* CG_SHUDElementChat2Create(const superhudConfig_t* config);
void* CG_SHUDElementChat3Create(const superhudConfig_t* config);
void* CG_SHUDElementChat4Create(const superhudConfig_t* config);
void* CG_SHUDElementChat5Create(const superhudConfig_t* config);
void* CG_SHUDElementChat6Create(const superhudConfig_t* config);
void* CG_SHUDElementChat7Create(const superhudConfig_t* config);
void* CG_SHUDElementChat8Create(const superhudConfig_t* config);
void* CG_SHUDElementChat9Create(const superhudConfig_t* config);
void* CG_SHUDElementChat10Create(const superhudConfig_t* config);
void* CG_SHUDElementChat11Create(const superhudConfig_t* config);
void* CG_SHUDElementChat12Create(const superhudConfig_t* config);
void* CG_SHUDElementChat13Create(const superhudConfig_t* config);
void* CG_SHUDElementChat14Create(const superhudConfig_t* config);
void* CG_SHUDElementChat15Create(const superhudConfig_t* config);
void* CG_SHUDElementChat16Create(const superhudConfig_t* config);
void CG_SHUDElementChatRoutine(void* context);
void CG_SHUDElementChatDestroy(void* context);

void* CG_SHUDElementSpecMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementSpecMessageRoutine(void* context);
void CG_SHUDElementSpecMessageDestroy(void* context);

void* CG_SHUDElementRecordMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementRecordMessageRoutine(void* context);
void CG_SHUDElementRecordMessageDestroy(void* context);

void* CG_SHUDElementFollowMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementFollowMessageRoutine(void* context);
void CG_SHUDElementFollowMessageDestroy(void* context);

void* CG_SHUDElementRespawnMessageCreate(const superhudConfig_t* config);
void CG_SHUDElementRespawnMessageRoutine(void* context);
void CG_SHUDElementRespawnMessageDestroy(void* context);

void* CG_SHUDElementSpectatorsCreate(const superhudConfig_t* config);
void CG_SHUDElementSpectatorsRoutine(void* context);
void CG_SHUDElementSpectatorsDestroy(void* context);

void* CG_SHUDElementGameTimeCreate(const superhudConfig_t* config);
void CG_SHUDElementGameTimeRoutine(void* context);
void CG_SHUDElementGameTimeDestroy(void* context);

void* CG_SHUDElementTimelimitCreate(const superhudConfig_t* config);
void CG_SHUDElementTimelimitRoutine(void* context);
void CG_SHUDElementTimelimitDestroy(void* context);

void* CG_SHUDElementItemPickupCreate(const superhudConfig_t* config);
void CG_SHUDElementItemPickupRoutine(void* context);
void CG_SHUDElementItemPickupDestroy(void* context);

void* CG_SHUDElementItemPickupIconCreate(const superhudConfig_t* config);
void CG_SHUDElementItemPickupIconRoutine(void* context);
void CG_SHUDElementItemPickupIconDestroy(void* context);

void* CG_SHUDElementSpectatorItemSpawnsCreate(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemSpawnsRedCreate(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemSpawnsBlueCreate(const superhudConfig_t* config);
void CG_SHUDElementSpectatorItemSpawnsRoutine(void* context);
void CG_SHUDElementSpectatorItemSpawnsDestroy(void* context);

void* CG_SHUDElementSpectatorItemPickups1Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups2Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups3Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups4Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups5Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups6Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups7Create(const superhudConfig_t* config);
void* CG_SHUDElementSpectatorItemPickups8Create(const superhudConfig_t* config);
void CG_SHUDElementSpectatorItemPickupsRoutine(void* context);
void CG_SHUDElementSpectatorItemPickupsDestroy(void* context);

void* CG_SHUDElementFlagStatusNMECreate(const superhudConfig_t* config);
void* CG_SHUDElementFlagStatusOWNCreate(const superhudConfig_t* config);
void* CG_SHUDElementFlagStatusCarryCreate(const superhudConfig_t* config);
void CG_SHUDElementFlagStatusRoutine(void* context);
void CG_SHUDElementFlagStatusDestroy(void* context);

void* CG_SHUDElementPlayerNameCreate(const superhudConfig_t* config);
void CG_SHUDElementPlayerNameRoutine(void* context);
void CG_SHUDElementPlayerNameDestroy(void* context);

#define SUPERHUD_UPDATE_TIME 50

void* CG_SHUDElementPwTime1Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime2Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime3Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime4Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime5Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime6Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime7Create(const superhudConfig_t* config);
void* CG_SHUDElementPwTime8Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon1Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon2Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon3Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon4Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon5Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon6Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon7Create(const superhudConfig_t* config);
void* CG_SHUDElementPwIcon8Create(const superhudConfig_t* config);
void CG_SHUDElementPwRoutine(void* context);
void CG_SHUDElementPwDestroy(void* context);

void* CG_SHUDElementNameNMECreate(const superhudConfig_t* config);
void* CG_SHUDElementNameOWNCreate(const superhudConfig_t* config);
void CG_SHUDElementNameRoutine(void* context);
void CG_SHUDElementNameDestroy(void* context);

void* CG_SHUDElementScoreNMECreate(const superhudConfig_t* config);
void* CG_SHUDElementScoreOWNCreate(const superhudConfig_t* config);
void* CG_SHUDElementScoreMAXCreate(const superhudConfig_t* config);
void CG_SHUDElementScoreRoutine(void* context);
void CG_SHUDElementScoreDestroy(void* context);

void* CG_SHUDElementRewardIconCreate(const superhudConfig_t* config);
void* CG_SHUDElementRewardCountCreate(const superhudConfig_t* config);
void CG_SHUDElementRewardRoutine(void* context);
void CG_SHUDElementRewardDestroy(void* context);

void* CG_SHUDElementTeamCountOWNCreate(const superhudConfig_t* config);
void* CG_SHUDElementTeamCountNMECreate(const superhudConfig_t* config);
void CG_SHUDElementTeamCountRoutine(void* context);
void CG_SHUDElementTeamCountDestroy(void* context);

void* CG_SHUDElementTeam1Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam2Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam3Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam4Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam5Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam6Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam7Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam8Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam9Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam10Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam11Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam12Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam13Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam14Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam15Create(const superhudConfig_t* config);
void* CG_SHUDElementTeam16Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed1Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed2Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed3Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed4Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed5Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed6Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed7Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamRed8Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue1Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue2Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue3Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue4Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue5Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue6Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue7Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamBlue8Create(const superhudConfig_t* config);
void CG_SHUDElementTeamRoutine(void* context);
void CG_SHUDElementTeamDestroy(void* context);

void* CG_SHUDElementWeaponListCreate(const superhudConfig_t* config);
void CG_SHUDElementWeaponListRoutine(void* context);
void CG_SHUDElementWeaponListDestroy(void* context);

void* CG_SHUDElementObituaries1Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries2Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries3Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries4Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries5Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries6Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries7Create(const superhudConfig_t* config);
void* CG_SHUDElementObituaries8Create(const superhudConfig_t* config);
void CG_SHUDElementObituariesRoutine(void* context);
void CG_SHUDElementObituariesDestroy(void* context);

void* CG_SHUDElementTeamTask1Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask2Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask3Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask4Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask5Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask6Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask7Create(const superhudConfig_t* config);
void* CG_SHUDElementTeamTask8Create(const superhudConfig_t* config);
void CG_SHUDElementTeamTasksRoutine(void* context);
void CG_SHUDElementTeamTasksDestroy(void* context);


void* CG_SHUDElementTempAccTextCreate(const superhudConfig_t* config);
void* CG_SHUDElementTempAccIconCreate(const superhudConfig_t* config);
void CG_SHUDElementTempAccRoutine(void* context);
void CG_SHUDElementTempAccDestroy(void* context);

void* CG_SHUDElementWarmupInfoCreate(const superhudConfig_t* config);
void CG_SHUDElementWarmupInfoRoutine(void* context);
void CG_SHUDElementWarmupInfoDestroy(void* context);
void* CG_SHUDElementGameTypeCreate(const superhudConfig_t* config);
void CG_SHUDElementGameTypeRoutine(void* context);
void CG_SHUDElementGameTypeDestroy(void* context);

void* CG_SHUDElementLocationCreate(const superhudConfig_t* config);
void CG_SHUDElementLocationRoutine(void* context);
void CG_SHUDElementLocationDestroy(void* context);

void* CG_SHUDElementCreateCurrentWeapon(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateMG(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateSG(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateGL(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateRL(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateLG(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreateRG(const superhudConfig_t* config);
void* CG_SHUDElementWeaponStatsCreatePG(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateCurrentWeapon(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateMG(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateSG(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateGL(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateRL(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateLG(const superhudConfig_t* config);
void* CG_SHUDElementIconCreateRG(const superhudConfig_t* config);
void* CG_SHUDElementIconCreatePG(const superhudConfig_t* config);
void CG_SHUDElementWeaponStatsRoutine(void* context);
void CG_SHUDElementWeaponStatsDestroy(void* context);


void* CG_SHUDElementCreatePlayerStatsDG(const superhudConfig_t* config);
void* CG_SHUDElementCreatePlayerStatsDR(const superhudConfig_t* config);
void* CG_SHUDElementCreatePlayerStatsDamageRatio(const superhudConfig_t* config);
void* CG_SHUDElementCreatePlayerStatsDRIcon(const superhudConfig_t* config);
void* CG_SHUDElementCreatePlayerStatsDGIcon(const superhudConfig_t* config);
void CG_SHUDElementPlayerStatsRoutine(void* context);
void CG_SHUDElementPlayerStatsDestroy(void* context);

void* CG_SHUDElementGridCreate(const superhudConfig_t* config);
void CG_SHUDElementGridRoutine(void* context);
void CG_SHUDElementGridDestroy(void* context);

void* CG_SHUDElementRoundWarmupTextCreate(const superhudConfig_t* config);
void* CG_SHUDElementRoundWarmupTimeCreate(const superhudConfig_t* config);
void* CG_SHUDElementRoundWarmupFightCreate(const superhudConfig_t* config);
void CG_SHUDElementRoundWarmupRoutine(void* context);
void CG_SHUDElementRoundWarmupDestroy(void* context);

void* CG_SHUDElementLastStandingCreate(const superhudConfig_t* config);
void CG_SHUDElementLastStandingRoutine(void* context);
void CG_SHUDElementLastStandingDestroy(void* context);

void* CG_SHUDElementThawOwnCreate(const superhudConfig_t* config);
void* CG_SHUDElementThawFreezingCreate(const superhudConfig_t* config);
void CG_SHUDElementThawRoutine(void* context);
void CG_SHUDElementThawDestroy(void* context);

void* CG_SHUDElementThawOwnBarCreate(const superhudConfig_t* config);
void* CG_SHUDElementThawFreezingBarCreate(const superhudConfig_t* config);
void CG_SHUDElementThawBarRoutine(void* context);
void CG_SHUDElementThawBarDestroy(void* context);

/*
 * cg_superhud_util.c
 */
typedef union
{
	struct
	{
		float x;
		float y;
		float w;
		float h;
	} named;
	vec4_t arr;
} superhudCoord_t;

typedef struct
{
	superhudCoord_t coord;
	int flags;
	vec4_t color_origin;
	vec4_t color;
	vec4_t shadowColor;
	vec4_t background;
	vec4_t border;
	vec4_t borderColor;
	int width;
	int fontIndex;
	const char* text;
} superhudTextContext_t;

typedef struct
{
	superhudCoord_t coord;
	superhudCoord_t coordPicture;
	qhandle_t image;
	vec4_t color;
	vec4_t color_origin;
} superhudDrawContext_t;

typedef struct
{
	superhudDirection_t direction;
	float max; // maximum coord for bar
	float koeff; //multiplier
	vec4_t bar[2]; // coord of two bars
	vec4_t box; // aligned rect (640) for fill/border
	vec4_t color_top; // color of bar
	vec4_t color2_top; // color of top bar of doublebar
	vec4_t color_back; // color of background
	qboolean two_bars; // one or two bars
	qhandle_t image; // shader for bar image (divided into 100 parts)
	qhandle_t image2; // shader for second layer (divided into 100 parts)
} superhudBarContext_t;

#define OSPHUD_TEAMOVERLAY_STR_SIZE 128
typedef struct
{
	int powerupOffsetChar;
	int powerupLenChar;
	float powerupOffsetPix;
	float powerupLenPix;

	int nameOffsetChar;
	int nameLenChar;
	float nameOffsetPix;
	float nameLenPix;

	int healthAndArmorOffsetChar;
	int healthAndArmorLenChar;
	float healthAndArmorOffsetPix;
	float healthAndArmorLenPix;

	int weaponOffsetChar;
	int weaponLenChar;
	float weaponOffsetPix;
	float weaponLenPix;

	int locationOffsetChar;
	int locationLenChar;
	float locationOffsetPix;
	float locationLenPix;

	int overlayWidthChar;
	float overlayWidthPix;
} shudTeamOverlay_t;

void CG_SHUDBarMakeContext(const superhudConfig_t* in, superhudBarContext_t* out, float max);
void CG_SHUDTextMakeContext(const superhudConfig_t* in, superhudTextContext_t* out);
const char* CG_SHUDVaTextWithInt(superhudConfig_t* config, int value);
void CG_SHUDDrawStretchPic(superhudCoord_t coord, const superhudCoord_t coordPicture, const float* color, qhandle_t shader);
void CG_SHUDDrawMakeContext(const superhudConfig_t* cfg, superhudDrawContext_t* out);

void CG_SHUDTextPrint(const superhudConfig_t* cfg, superhudTextContext_t* pos);
void CG_SHUDTextPrintNew(const superhudConfig_t* cfg, superhudTextContext_t* pos, qboolean colorOverride);
void CG_SHUDDrawStretchPicCtx(const superhudConfig_t* cfg, superhudDrawContext_t* out);
void CG_SHUDBarPrint(const superhudConfig_t* cfg, superhudBarContext_t* ctx, float value);
qboolean CG_SHUDFill(const superhudConfig_t* cfg);
qboolean CG_SHUDFillFade(const superhudConfig_t* cfg, float fadeAlpha);
qboolean CG_SHUDDrawBorderFade(const superhudConfig_t* cfg, float fadeAlpha);
void CG_SHUDFillAndFrameForElement(const superhudConfig_t* cfg, const vec4_t color_origin, const vec4_t color);
float CG_SHUDFadeAlphaScale(const vec4_t color_origin, const vec4_t color);

team_t CG_SHUDGetOurActiveTeam(void);
qboolean CG_SHUDGetFadeColor(const vec4_t from_color, vec4_t out, const superhudConfig_t* cfg, int startTime);
void CG_SHUDFillWithColor(const superhudCoord_t* coord, const float* color);
void CG_SHUDElementCompileTeamOverlayConfig(int fontWidth, int style, shudTeamOverlay_t* configOut);
void CG_SHUDConfigPickBgColor(const superhudConfig_t* config, float* color, qboolean alphaOverride);
void CG_SHUDConfigPickColor_Init(const superhudConfig_t* config, vec4_t color);
void CG_SHUDConfigPickColor_Runtime(const superhudConfig_t* config, vec4_t color);


typedef struct
{
	char message[MAX_SAY_TEXT];
	int time;
	int senderClientNum;
} superhudChatEntry_t;

typedef struct
{
	int time;
	int attacker;
	int target;
	int attackerTeam;
	int targetTeam;
	int mod;
	qboolean unfrozen;
	int droppedPowerups;
	int droppedFlagTeam;
	struct
	{
		qboolean isInitialized;
		qhandle_t iconShader;
		vec4_t attackerColor;
		vec4_t targetColor;
		vec4_t enemyColor;
		char attackerName[MAX_QPATH];
		char targetName[MAX_QPATH];
		int maxVisibleChars;
		float baseX;
		float attackerWidth;
		float targetWidth;
		float droppedPuWidth;
		int droppedPuCount;
		qhandle_t droppedPuShaders[6];
		vec4_t droppedFlagColor;
		float spacing;
		int maxNameLenPix;
	} runtime;
} superhudObituariesEntry_t;

typedef struct
{
	float tempAccuracy;
} superhudTempAccEntry_t;

typedef struct
{
	int time;
	int sender;
	int taskType;
	int repeatCount;
	int gameTime; /* Game time when task was sent (msec from level start) */
	qboolean isPickup; /* True if this is a pickup mark (toClient=1) */
	struct
	{
		qboolean isInitialized;
		qhandle_t iconShader;
		qboolean useMarkColor; /* yes/no/medic/frozen: color captured at mark time */
		vec4_t iconColor;
		vec4_t senderColor;
		char senderName[MAX_QPATH];
		char eventName[MAX_QPATH];
		char timeString[16];
		int maxVisibleChars;
		float baseX;
		float timeWidth;
		float senderWidth;
		float eventNameWidth;
		float spacing;
		int maxNameLenPix;
	} runtime;
} superhudTeamTasksEntry_t;

typedef struct
{
	int time;
	int pickerClient;
	int group;
	int gameTime;
	struct
	{
		qboolean isInitialized;
		qhandle_t iconShader;
		vec4_t pickerColor;
		char pickerName[MAX_QPATH];
		char timeString[16];
		float baseX;
		float pickerWidth;
		float timeWidth;
		float spacing;
		int maxNameLenPix;
	} runtime;
} superhudSpectatorItemPickupEntry_t;

typedef struct
{
	int lastTrackedWeapon;
	float lastAccuracy;
	float kdratio;
	struct
	{
		float accuracy;
		int kills;
		int deaths;
		int hits;
		int shots;
		int pickUps;
		int drops;
	} stats[WP_NUM_WEAPONS];
} customStats_t;

#define SHUD_MAX_OBITUARIES_LINES 8
#define SHUD_MAX_TEAMTASKS_LINES 8
#define SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES 8
#define SHUD_MAX_CHAT_LINES 16
#define SHUD_MAX_POWERUPS 8

typedef struct
{
	struct
	{
		int time;
		char message[256];
	} fragmessage;
	struct
	{
		int time;
		char message[256];
	} killconfirm;
	struct
	{
		int time;
		char message[256];
	} rankmessage;
	struct
	{
		superhudChatEntry_t line[SHUD_MAX_CHAT_LINES];
		unsigned int index;
	} chat;
	struct
	{
		superhudObituariesEntry_t line[SHUD_MAX_OBITUARIES_LINES];
		unsigned int index;
	} obituaries;
	struct
	{
		superhudTeamTasksEntry_t line[SHUD_MAX_TEAMTASKS_LINES];
		unsigned int index;
		qboolean hasActive;
	} teamTasks;
	struct
	{
		superhudSpectatorItemPickupEntry_t line[SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES];
		unsigned int index;
		qboolean hasActive;
	} spectatorItemPickups;
	struct superhudPowerupsCache_t
	{
		struct superhudPowerupElement_t
		{
			int time;
			int powerup;
			qboolean isHoldable;
			int dyeTintItemNum;
		} element[SHUD_MAX_POWERUPS];
		int numberOfActive;
		int lastUpdateTime;
	} powerupsCache;
	struct
	{
		superhudTempAccEntry_t weapon[WP_NUM_WEAPONS];
	} tempAcc;
	customStats_t customStats;
	superhudFrameContext_t frame;
} superhudGlobalContext_t;

typedef enum
{
	SHUD_VAR_TYPE_INT,
	SHUD_VAR_TYPE_FLOAT,
	SHUD_VAR_TYPE_STRING,
	SHUD_VAR_TYPE_FLOAT4,
} superhudVarType_t;

typedef struct superhudVar_s
{
	char name[MAX_QPATH];
	superhudVarType_t type;
	union
	{
		int intValue;
		float floatValue;
		float float4[4];
		char stringValue[MAX_QPATH];
	} value;
	struct superhudVar_s* next;
} superhudVar_t;

superhudVar_t* CG_SHUDVarFind(const char* name);
void CG_SHUDVarSetInt(const char* name, int value);
void CG_SHUDVarSetFloat(const char* name, float value);
void CG_SHUDVarSetFloat4(const char* name, const float value[4]);
void CG_SHUDVarSetString(const char* name, const char* value);
void CG_SHUDVarClearAll(void);
qboolean CG_SHUDVarGetInt(const char* name, int* value);
qboolean CG_SHUDVarGetFloat(const char* name, float* value);
qboolean CG_SHUDVarGetString(const char* name, char* value, int maxlen);
qboolean CG_SHUDVarGetFloat4(const char* name, float value[4]);

extern superhudGlobalContext_t superhudGlobalContext;
void CG_SHUDAvailableElementsInit(void);
const superHUDConfigElement_t* CG_SHUDAvailableElementsGet(void);

int CG_SHUDGetAmmo(int wpi);
void CG_SHUDParseWeaponMaxValue(const char* valueStr, int* maxammo, int arraySize);

void CG_SHUDFillAndFrameForText(superhudConfig_t* cfg, superhudTextContext_t* ctx);
qboolean CG_SHUDDrawBorder(const superhudConfig_t* cfg);
void CG_SHUDConfigPickBorderColor(const superhudConfig_t* config, float* color, qboolean alphaOverride);
void CG_SHUDDrawBorderDirect(const superhudCoord_t* coord, const vec4_t border, const vec4_t borderColor);

#ifdef __cplusplus
}
#endif

#endif
