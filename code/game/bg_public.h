/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#ifndef BG_PUBLIC_H
#define BG_PUBLIC_H

#include "../qcommon/q_shared.h"

#ifdef __cplusplus
extern "C" {
#endif
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define GAME_VERSION        "baseq3-1"

#define DEFAULT_GRAVITY     800
#define GIB_HEALTH          -40
#define ARMOR_PROTECTION    0.66

#define MAX_ITEMS           256

#define RANK_TIED_FLAG      0x4000

#define DEFAULT_SHOTGUN_SPREAD  700
#define DEFAULT_SHOTGUN_COUNT   11

#define ITEM_RADIUS         15      // item sizes are needed for client side pickup detection

#define LIGHTNING_RANGE     768

#define SCORE_NOT_PRESENT   -9999   // for the CS_SCORES[12] when only one player is present

#define VOTE_TIME           30000   // 30 seconds before vote times out

#define MINS_Z              -24
#define DEFAULT_VIEWHEIGHT  26
#define CROUCH_VIEWHEIGHT   12
#define DEAD_VIEWHEIGHT     -16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define CS_MUSIC                2
#define CS_MESSAGE              3       // from the map worldspawn's message field
#define CS_MOTD                 4       // g_motd string for server message of the day
#define CS_WARMUP               5       // server time when the match will be restarted
#define CS_SCORES1              6
#define CS_SCORES2              7
#define CS_VOTE_TIME            8
#define CS_VOTE_STRING          9
#define CS_VOTE_YES             10
#define CS_VOTE_NO              11

#define CS_TEAMVOTE_TIME        12
#define CS_TEAMVOTE_STRING      14
#define CS_TEAMVOTE_YES         16
#define CS_TEAMVOTE_NO          18
#define CS_REDPLAYERALIVE       19      // FPS Classsico

#define CS_GAME_VERSION         20
#define CS_LEVEL_START_TIME     21      // so the timer only shows the current level
#define CS_INTERMISSION         22      // when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS           23      // string indicating flag status in CTF
#define CS_SHADERSTATE          24
#define CS_BOTINFO              25

#define CS_ITEMS                27      // string of 0's and 1's that tell which items are present

#define CS_BLUEPLAYERALIVE      29      // FPS Classsico
#define CS_MODELS               32
#define CS_SOUNDS               (CS_MODELS+MAX_MODELS) //32+256 = 288 (289?)
#define CS_PLAYERS              (CS_SOUNDS+MAX_SOUNDS) // 288 + 256 = 544
#define CS_LOCATIONS            (CS_PLAYERS+MAX_CLIENTS) // 544 + 64 = 608
#define CS_PARTICLES            (CS_LOCATIONS+MAX_LOCATIONS)  // 608 + MAX_LOCATIONS


#define CS_OSP_SERVER_MODE                        806
#define CS_OSP_CUSTOM_CLIENT                      807

#define CS_OSP_ALLOW_PMOVE                        809
#define CS_OSP_MAXPACKETS_MIN                     810
#define CS_OSP_MAXPACKETS_MAX                     811
#define CS_OSP_TIMENUDGE_MIN                      812
#define CS_OSP_TIMENUDGE_MAX                      813
#define CS_OSP_VERSION_STR                        814
#define CS_OSP_CLAN_BASE_TEAM_DM                  815
#define CS_OSP_MOTD0                              816
#define CS_OSP_MOTD1                              817
#define CS_OSP_MOTD2                              818
#define CS_OSP_MOTD3                              819
#define CS_OSP_MOTD4                              820
#define CS_OSP_MOTD5                              821
#define CS_OSP_MOTD6                              822
#define CS_OSP_MOTD7                              823
#define CS_OSP_CUSTOM_GFX                         824
#define CS_OSP_DECALS                             840
#define CS_OSP_AUTH                               872
#define CS_OSP_FREEZE_GAME_TYPE                   873
#define CS_OSP_CUSTOM_CLIENT2                     874
#define CS_OSP2BE_SUPPORTED                     887
#define CS_OSP2BE_DISABLED_FEATURES             888
#define CS_OSP2BE_TOURNEY_RESTRICTIONS          889
#define CS_OSP2BE_ITEM_CATALOG0                890
#define CS_OSP2BE_ITEM_CATALOG_COUNT           20
#define CS_OSP2BE_ITEM_CATALOG_LAST            (CS_OSP2BE_ITEM_CATALOG0 + CS_OSP2BE_ITEM_CATALOG_COUNT - 1)
/* BE-only: "1 <prestep> <rlSpeed> <pgSpeed> <bfgSpeed>" when client missile predict is on */
#define CS_OSP2BE_MISSILE_PREDICT              910
/* CS 911 player damage hitbox (shared slot):
 *   Ruthless/Devotion: "1 <wide> <hgh>" — body cylinder only (head removed; legacy mode 2 = same)
 *   Metarena/OSDF:     single-token "<scale>" (>0 = cylinder AABB*scale; "0"/empty = AABB cube) */
#define CS_OSP2BE_HITBOX                       911
#define CS_OSP2BE_MOVEMENT_LOCK                912  /* FPS lock: 0=off, >0=q3msk-style velocity quantize */

/* Client predict physics branch (cgs.pmoveMode); game code uses pmode_q3msk_lock */
#define PMODE_OSP                              0
#define PMODE_BASEQ3A                          1
#define PMODE_OSDF                             2

/*
 * Item catalog (OSP2BE): CS 890..909 concatenated (20 KiB max).
 * Leading <mode> — one hex nibble (0-9a-f), bitmask:
 *   1=shadow catalog, 2=PUI pickups, 4=spawns, 8=tp exits.
 * Wire: <seg>|<seg>|...
 *   item/PUI: <abbrev>[/<abbrev>...];<x>,<y>,<z>[,<side>]
 *   spawn:    sp;<x>,<y>,<z>,<yaw>,<sf>,<count>,<role>,<team>
 *   tp exit:  tde;<x>,<y>,<z>,<yaw>
 *   x,y,z deci-units (origin * 10); side 0 neutral 1 red 2 blue (CTF, server classify)
 *   PUI segments in CTF include side; shadow-only segments omit side
 *   team pad: puiAbbrevs;x,y,z and/or shadowAbbrevs;x,y,z (master origin)
 *   trackId = 0..63 left-to-right among PUI-mappable abbrevs only
 *   servercmd  pui <cmd> <readyTime>
 *   next unused: PUI_ID_INVALID (64)
 */
typedef enum
{
	PUI_MH,
	PUI_RA,
	PUI_YA,
	PUI_QD,
	PUI_BS,
	PUI_REG,
	PUI_INV,
	PUI_HST,
	PUI_FLG,
	PUI_TYPE_COUNT,
	PUI_ID_INVALID = 64
} puiType_t;

#define PUI_ID( type, side, slot ) \
    ( ( ( (side) & 3 ) << 8 ) | ( ( (slot) & 0xF ) << 4 ) | ( (type) & 0xF ) )
#define PUI_CMD( picker, pickId, nextId ) \
    ( ( ( (picker) & (MAX_CLIENTS - 1) ) << 20 ) | ( ( (pickId) & 0x3FF ) << 10 ) | ( (nextId) & 0x3FF ) )

int BG_PuiIdType(int itemId);
int BG_PuiIdSide(int itemId);
int BG_PuiIdSlot(int itemId);
void BG_PuiUnpackCmd(int cmd, int* picker, int* pickId, int* nextId);
char BG_ItemCatalogModeChar(int mode);
int BG_ItemCatalogParseMode(char c);

#define XQ3E_ALLOW_FEATURES                     1000

#define CS_MAX                  (XQ3E_ALLOW_FEATURES)


#if (CS_LOCATIONS + MAX_LOCATIONS) > CS_OSP_SERVER_MODE
#error location config strings overlap CS_OSP_SERVER_MODE
#endif

// #if (CS_MAX) > MAX_CONFIGSTRINGS
// #error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
// #endif

typedef enum
{
	GT_FFA,             // free for all
	GT_TOURNAMENT,      // one on one tournament
	GT_SINGLE_PLAYER,   // single player ffa

	//-- team games go after this --

	GT_TEAM,            // team deathmatch
	GT_CTF,             // capture the flag
	GT_CA,
	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
	PM_NORMAL,      // can accelerate and turn
	PM_NOCLIP,      // noclip movement
	PM_SPECTATOR,   // still run into walls
	PM_DEAD,        // no acceleration or turning, but free falling
	PM_FREEZE,      // stuck in place with no control
	PM_INTERMISSION,    // no movement or status bar
	PM_SPINTERMISSION   // no movement or status bar
} pmtype_t;

typedef enum
{
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define PMF_DUCKED          1
#define PMF_JUMP_HELD       2
#define PMF_RAMPSLIDE       4       // OSDF rampslide active
#define PMF_BACKWARDS_JUMP  8       // go into backwards land
#define PMF_BACKWARDS_RUN   16      // coast down to backwards run
#define PMF_TIME_LAND       32      // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK  64      // pm_time is an air-accelerate only time
#define PMF_TIME_WATERJUMP  256     // pm_time is waterjump
#define PMF_RESPAWNED       512     // clear after attack and jump buttons come up
#define PMF_USE_ITEM_HELD   1024
#define PMF_GRAPPLE_PULL    2048    // pull towards grapple location
#define PMF_FOLLOW          4096    // spectate following another player
#define PMF_SCOREBOARD      8192    // spectate as a scoreboard
#define PMF_INVULEXPAND     16384   // invulnerability sphere set to full size

#define PMF_ALL_TIMES   (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define MAXTOUCH    32
typedef struct pmove_s
{
	// state (in / out)
	playerState_t*   ps;

	// command (in)
	usercmd_t   cmd;
	int         tracemask;          // collide against these types of surfaces
	int         debugLevel;         // if set, diagnostic output will be printed
	qboolean    noFootsteps;        // if the game is setup for no footsteps by the server
	qboolean    gauntletHit;        // true if a gauntlet attack would actually hit something

	int         framecount;

	// results (out)
	int         numtouch;
	int         touchents[MAXTOUCH];

	vec3_t      mins, maxs;         // bounding box size

	int         watertype;
	int         waterlevel;

	float       xyspeed;

	// for fixed msec Pmove
	int         pmove_fixed;
	int         pmove_msec;
	int         pmove_float;
	int         pmove_accurate;
	int         pmove_lock_fps;     /* CS 912: 0=off, >0=target FPS for movement lock */

	/* movement_t selector; client sets from CS 806 (OSP_SERVER_MODE_*) */
	int         pmove_movement;
	int         pmove_autohop;      /* predict-only; QL forces 1 locally */
	int         pmove_ratflags;     /* g_altFlags from serverinfo */

	// callbacks to test the world
	// these will be different functions during game and cgame
	void (*trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
	int (*pointcontents)(const vec3_t point, int passEntityNum);


	//::OSDF modded
	int movetype;           // Physics type selection from cvar. Latch protected
	float overbounce_scale; // Amount of overbounce scale to apply. Default
	// OVERCLIP = 1.001f
	pmoveData_t pmd;        // Stores extracted pmove data needed outside of pmove
	//::OSDF end
} pmove_t;

/* Internal pmove selectors; public g_movement cvar maps to CS 806 on server */
typedef enum
{
	MOVEMENT_VQ3 = 0,
	MOVEMENT_CPM_DEFRAG = 1,
	MOVEMENT_CQ3 = 2,
	MOVEMENT_CPM_CPMA = 3,
	MOVEMENT_RM = 4
} movement_t;

#define MOVEMENT_PUBLIC_MAX	MOVEMENT_CQ3

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles(playerState_t* ps, const usercmd_t* cmd);
void PmoveSingle(pmove_t* pmove);
void Pmove(pmove_t* pmove);
void PmoveEx(pmove_t* pmove, void (*single)(pmove_t* pmove));

//===================================================================================


typedef enum
{
	ARMOR_GREEN = 0,
	ARMOR_YELLOW = 1,
	ARMOR_RED = 2,
} armorType_t;

// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum
{
	STAT_HEALTH = 0,
	STAT_HOLDABLE_ITEM = 1,
	STAT_WEAPONS = 2,                   // 16 bit fields
	STAT_ARMOR = 3,
	STAT_DEAD_YAW = 4,                  // look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY = 5,             // bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH = 6,                // health / armor limit, changable by handicap
	STAT_OSP_PHYS = 7,
	STAT_RAIL_DELAY = 8,                /* UnFreeZe phys DJ timer when uf_rampjump (STAT_UNFREEZE_PHYS) */
	STAT_ARMOR_TYPE = 9,                //aromor: red/green/yellow
	STAT_OSP_10 = 10,
	STAT_WEAPON_DELAY = 11,

	// OSDF: pack = crouchslide low16 | holdboost high16 (same wire index as STAT_OSP_10)
	STAT_OSDF_PACK = 10,
	STAT_TIME_LASTJUMP = 12,
	STAT_EXTFLAGS = 13,		/* EXTFL_* */
	STAT_BE_AUTH = 14,		/* OSP2-BE hidden connect challenge (owner PS only) */
	/* g_freeze (ruthless): own progress% + thawing% — see BE_CUSTOM_* */
	STAT_BE_CUSTOM = 15
} statIndex_t;

/* stats[STAT_EXTFLAGS] — q3msk overbounce: WalkMove uses AirMove while set */
#define EXTFL_SLIDING 1
#define EXTFL_OVERBOUNCE 2

/* UnFreeZe cgame uses ps->stats[8] for rampjump doublejump window (same index as STAT_RAIL_DELAY). */
#define STAT_UNFREEZE_PHYS	STAT_RAIL_DELAY

#define STAT_JUMP_HOLDBOOST	STAT_BE_AUTH	/* legacy name; unused by OSDF */

/* Quiet usercmd bits for BE auth response (no pmove side effects). */
#define BUTTON_BE_AUTH0		(1 << 14)	/* +button14 = response bit 0 */
#define BUTTON_BE_AUTH1		(1 << 15)	/* +button15 = response bit 1 */

#define BE_AUTH_SECRET		0xBEA0717u
#define BE_AUTH_NUM_BITS	8
#define BE_AUTH_TIMEOUT_MS	5000

/* Mix(challenge) -> 8-bit response; shared by game + cgame. */
unsigned	BG_BE_AuthResponse(unsigned challenge);

#define OSDF_CROUCH_MASK 0xFFFF
#define OSDF_BOOST_SHIFT 16

/*
 * STAT_BE_CUSTOM (stats[15]):
 *
 * Freeze (BE_CUSTOM_FLAG / bit15):
 *   bit15 = active, bit7 = ally thawing me,
 *   bits0-6 = own thaw 0-100 (frozen only; alive = 0),
 *   bits8-14 = thawing-other 0-100
 * Alive: OWN=0, FREEZING=who I thaw. Frozen: OWN=mine;
 * FREEZING=watched player's thawing while following.
 *
 * Non-freeze death (bit15 always 0):
 *   bits0-6 = canTenths 0..127 until attack/use respawn
 *   bits7-14 = forceTenths 0..255 until forceRespawn
 * Mutual exclusion with freeze thaw pack.
 */
#define BE_CUSTOM_FLAG			0x8000
#define BE_CUSTOM_ALLY			0x0080
#define BE_CUSTOM_U16(v)		((unsigned)(v) & 0xFFFFu)
#define BE_CUSTOM_ACTIVE(v)		(BE_CUSTOM_U16(v) & BE_CUSTOM_FLAG)
#define BE_CUSTOM_OWN_THAW(v)		(BE_CUSTOM_ACTIVE(v) ? (int)(BE_CUSTOM_U16(v) & 0x7Fu) : 0)
#define BE_CUSTOM_ALLY_THAWING(v)	(BE_CUSTOM_ACTIVE(v) && (BE_CUSTOM_U16(v) & BE_CUSTOM_ALLY))
#define BE_CUSTOM_THAWING(v)		(BE_CUSTOM_ACTIVE(v) ? (int)((BE_CUSTOM_U16(v) >> 8) & 0x7Fu) : 0)
#define BE_CUSTOM_PACK(own, thaw, ally)	(BE_CUSTOM_FLAG \
	| ((own) & 0x7F) \
	| ((ally) ? BE_CUSTOM_ALLY : 0) \
	| (((thaw) & 0x7F) << 8))

#define RESPAWN_TIMER_CAN(v)		(BE_CUSTOM_ACTIVE(v) ? 0 : (int)(BE_CUSTOM_U16(v) & 0x7Fu))
#define RESPAWN_TIMER_FORCE(v)		(BE_CUSTOM_ACTIVE(v) ? 0 : (int)((BE_CUSTOM_U16(v) >> 7) & 0xFFu))
#define RESPAWN_TIMER_PACK(can, force)	((((force) & 0xFF) << 7) | ((can) & 0x7F))


// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum
{
	PERS_SCORE,                     // !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,                      // total points damage inflicted so damage beeps can sound on change
	PERS_RANK,                      // player rank or team rank
	PERS_TEAM,                      // player team
	PERS_SPAWN_COUNT,               // incremented every respawn
	PERS_PLAYEREVENTS,              // 16 bits that can be flipped for events
	PERS_ATTACKER,                  // clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR,            // health/armor of last person we attacked
	PERS_KILLED,                    // count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,          // two railgun hits in a row
	PERS_EXCELLENT_COUNT,           // two successive kills in a short amount of time
	PERS_DEFEND_COUNT,              // defend awards
	PERS_ASSIST_COUNT,              // assist awards
	PERS_GAUNTLET_FRAG_COUNT,       // kills with the guantlet
	PERS_CAPTURES                   // captures
} persEnum_t;


// entityState_t->eFlags
#define EF_DEAD             0x00000001      // don't draw a foe marker over players with EF_DEAD
#define EF_TICKING          0x00000002      /* prox (unused) / g_passPlayers overlap ghost */
#define EF_PASSPLAYER       EF_TICKING      /* alias: networked bit1 */
#define EF_TELEPORT_BIT     0x00000004      // toggled every time the origin abruptly changes
#define EF_AWARD_EXCELLENT  0x00000008      // draw an excellent sprite
#define EF_PLAYER_EVENT     0x00000010
#define EF_BOUNCE           0x00000010      // for missiles
#define EF_BOUNCE_HALF      0x00000020      // for missiles
#define EF_AWARD_GAUNTLET   0x00000040      // draw a gauntlet sprite
#define EF_NODRAW           0x00000080      // may have an event, but no model (unspawned items)
#define EF_FIRING           0x00000100      // for lightning gun
#define EF_KAMIKAZE         0x00000200
#define EF_MOVER_STOP       0x00000400      // will push otherwise
#define EF_AWARD_CAP        0x00000800      // draw the capture sprite
#define EF_TALK             0x00001000      // draw a talk balloon
#define EF_CONNECTION       0x00002000      // draw a connection trouble sprite
#define EF_VOTED            0x00004000      // already cast a vote
#define EF_AWARD_IMPRESSIVE 0x00008000      // draw an impressive sprite
#define EF_AWARD_DEFEND     0x00010000      // draw a defend sprite
#define EF_AWARD_ASSIST     0x00020000      // draw a assist sprite
#define EF_AWARD_DENIED     0x00040000      // denied
#define EF_TEAMVOTED        0x00080000      // already cast a team vote

// NOTE: may not have more than 16
typedef enum
{
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	PW_AMMOREGEN,
	PW_INVULNERABILITY,

	PW_NUM_POWERUPS

} powerup_t;

typedef enum
{
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,

	HI_NUM_HOLDABLE
} holdable_t;


typedef enum
{
	WP_NONE,

	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
#ifdef MISSIONPACK
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,
#endif

	WP_NUM_WEAPONS,
	WP_PENDING = WP_NUM_WEAPONS,
	WP_MAX_WEAPONS = 16
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define PLAYEREVENT_DENIEDREWARD        0x0001
#define PLAYEREVENT_GAUNTLETREWARD      0x0002
#define PLAYEREVENT_HOLYSHIT            0x0004
#define PLAYEREVENT_KC_CONFIRM          0x0008  /* kill-confirm: enemy token pickup */
#define PLAYEREVENT_KC_RETURN           0x0010  /* kill-confirm: ally token return */

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define EV_EVENT_BIT1       0x00000100
#define EV_EVENT_BIT2       0x00000200
#define EV_EVENT_BITS       (EV_EVENT_BIT1|EV_EVENT_BIT2)

#define EVENT_VALID_MSEC    300

typedef enum
{
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP_PAD,            // boing sound at origin, jump sound on player

	EV_JUMP,
	EV_WATER_TOUCH, // foot touches
	EV_WATER_LEAVE, // foot leaves
	EV_WATER_UNDER, // head touches
	EV_WATER_CLEAR, // head leaves

	EV_ITEM_PICKUP,         // normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,  // powerup / team sounds are broadcast to everyone

	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,      // eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,        // no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,              // otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,

	EV_GIB_PLAYER,          // gib a previously living player
	EV_SCOREPLUM,           // score plum

//#ifdef MISSIONPACK
	EV_PROXIMITY_MINE_STICK,
	EV_PROXIMITY_MINE_TRIGGER,
	EV_KAMIKAZE,            // kamikaze explodes
	EV_OBELISKEXPLODE,      // obelisk explodes
	EV_OBELISKPAIN,         // obelisk is in pain
	EV_INVUL_IMPACT,        // invulnerability sphere impact
	EV_JUICED,              // invulnerability juiced effect
	EV_LIGHTNINGBOLT,       // lightning bolt bounced of invulnerability sphere
//#endif

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL,
	EV_DAMAGEPLUM,

} entity_event_t;


typedef enum
{
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;

// animations
typedef enum
{
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	LEGS_BACKCR,
	LEGS_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,

	MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s
{
	int     firstFrame;
	int     numFrames;
	int     loopFrames;         // 0 to numFrames
	int     frameLerp;          // msec between frames
	int     initialLerp;        // msec to get to first frame
	int     reversed;           // true if animation is reversed
	int     flipflop;           // true if animation should flipflop back to base
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT      128


typedef enum
{
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,
	TEAM_4,
	TEAM_5,
	TEAM_6,
	TEAM_7,
	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME       1000

// How many players on the overlay
#define TEAM_MAXOVERLAY     32

//team task
// TemplateID range: 0-31 (5 bits, max 32 values)
typedef enum
{
	TEAMTASK_NONE,              // 0 LOL?
	TEAMTASK_OFFENSE,           // 1
	TEAMTASK_DEFENCE,           // 2
	TEAMTASK_WATCH,            // 3
	TEAMTASK_PLAYERDEAD,        // 4
	TEAMTASK_YES,               // 5
	TEAMTASK_NO,                // 6
	TEAMTASK_LOCATION,          // 7
	TEAMTASK_TARGET,            // 8
	TEAMTASK_REDARMOR,          // 9
	TEAMTASK_MEGAHEALTH,        // 10
	TEAMTASK_MEDIC,             // 11
	TEAMTASK_FROZEN_ALLY,       // 12
	TEAMTASK_FROZEN_ENEMY,      // 13
	TEAMTASK_HERE_ALIVE,        // 14
	TEAMTASK_HERE_DEAD,         // 15
	TEAMTASK_WARN_ALIVE,        // 16
	TEAMTASK_WARN_DEAD,         // 17
	TEAMTASK_QUADDAMAGE,        // 18
	TEAMTASK_REGENERATION,      // 19
	TEAMTASK_BATTLESUIT,        // 20
	TEAMTASK_HASTE,             // 21
	TEAMTASK_YELLOWARMOR,       // 22
	TEAMTASK_HEALTH,            // 23
	TEAMTASK_HERE_FLAG,         // 24
	TEAMTASK_REDFLAG,           // 25
	TEAMTASK_BLUEFLAG,          // 26
	TEAMTASK_REDFLAG_CARRIER,   // 27
	TEAMTASK_BLUEFLAG_CARRIER   // 28
	// Free IDs: 29, 30, 31
} teamtask_t;

// means of death
typedef enum
{
	MOD_UNKNOWN,
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_PLASMA,
	MOD_PLASMA_SPLASH,
	MOD_RAILGUN,
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_GRAPPLE,
	// MechFrenzy
	MOD_LANDMINE,
	MOD_LANDMINE_SPLASH,
	MOD_TURRET,
	MOD_TURRET_SPLASH,
	MOD_TURRET_EXPLOSION,
	MOD_DISPENSER_EXPLOSION
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum
{
	IT_BAD,
	IT_WEAPON,              // EFX: rotate + upscale + minlight
	IT_AMMO,                // EFX: rotate
	IT_ARMOR,               // EFX: rotate + minlight
	IT_HEALTH,              // EFX: static external sphere + rotating internal
	IT_POWERUP,             // instant on, timer based
	// EFX: rotate + external ring that rotates
	IT_HOLDABLE,            // single use, holdable item
	// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s
{
	char*        classname; // spawning name
	char*        pickup_sound;
	char*        world_model[MAX_ITEM_MODELS];

	char*        icon;
	char*        pickup_name;   // for printing on pickup

	int         quantity;       // for ammo how much, or duration of powerup
	itemType_t  giType;         // IT_* flags

	int         giTag;

	char*        precaches;     // string of all models and images this item will use
	char*        sounds;        // string of all sounds this item will use
} gitem_t;

// Optional world model overrides by item classname (cgame registers these on top of bg_itemlist).
// NULL world_model[n] keeps the nth model from bg_itemlist.
typedef struct
{
	const char* classname;
	const char* world_model[MAX_ITEM_MODELS];
	const char* icon; /* optional; NULL = bg_itemlist icon */
} bg_customItemVisuals_t;

extern const bg_customItemVisuals_t bg_customItemVisuals[];

const bg_customItemVisuals_t* BG_CustomItemVisualsFind(const char* classname);

// included in both the game dll and the client
extern  gitem_t bg_itemlist[];
extern  int     bg_numItems;

gitem_t* BG_FindItem(const char* pickupName);
gitem_t* BG_FindItemForWeapon(weapon_t weapon);
gitem_t* BG_FindItemForPowerup(powerup_t pw);
gitem_t* BG_FindItemForHoldable(holdable_t pw);
#define ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean    BG_CanItemBeGrabbed(int gametype, const entityState_t* ent, const playerState_t* ps, qboolean disableArmorCheck);

int         BG_PuiTypeFromItem(gitem_t* item);

// g_dmflags->integer flags
#define DF_NO_FALLING               8
#define DF_FIXED_FOV                16
#define DF_NO_FOOTSTEPS             32
#define DF_NO_ITEM_RESET            64
#define DF_NO_TEAM_RESET            128
#define DF_INSTANT_WEAPON_CHANGE    256
#define DF_NO_BUNNY                 512
#define DF_NO_EMPTY_USE_ITEM_SOUND  1024


// content masks
#define MASK_ALL                (-1)
#define MASK_SOLID              (CONTENTS_SOLID)
#define MASK_PLAYERSOLID        (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define MASK_DEADSOLID          (CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define MASK_WATER              (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE             (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT               (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum
{
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,             // grapple hooked on wall
	ET_TEAM,

	ET_EVENTS               // any of the EV_* events can be added freestanding
	// by setting eType to ET_EVENTS + eventNum
	// this avoids having to set eFlags and eventNum
} entityType_t;

// KillConfirm sentinel tag.
// The server stamps this reserved value into entityState_t::otherEntityNum on
// both the ET_ITEM kill-confirm token and its ET_GENERAL helper (ring) entity.
// Clients identify them by ( otherEntityNum == KILLCONFIRM_MARKER ) plus eType,
// reading the team from generic1. Model-path independent; a real
// item_armor_shard has otherEntityNum == 0, so it never collides.
// otherEntityNum is a 10-bit entity-number field (0..MAX_GENTITIES-1); 666 fits
// and stays below ENTITYNUM_WORLD, so it cannot clash with engine-reserved ids.
#define KILLCONFIRM_MARKER 666

/* Team proxy sentinel (entityState_t::otherEntityNum2). Owner in otherEntityNum.
 * Lean out-of-PVS ally position for OSP2-BE teamIndicator / friends WH. */
#define TEAM_PROXY_MARKER 667

/* g_altFlags->integer — autoparsed from various ratmod cvars (bit-compatible with devotion/ruthless) */
#define RAT_EASYPICKUP          (1 << 0)
#define RAT_POWERUPGLOWS        (1 << 1)
#define RAT_SCREENSHAKE         (1 << 2)
#define RAT_PREDICTMISSILES     (1 << 3)
#define RAT_FASTSWITCH          (1 << 4)
#define RAT_FASTWEAPONS         (1 << 5)
#define RAT_RAMPJUMP            (1 << 7)
#define RAT_ALLOWFORCEDMODELS   (1 << 8)
#define RAT_FRIENDSWALLHACK     (1 << 9)
#define RAT_SPECSHOWZOOM        (1 << 10)
#define RAT_BRIGHTSHELL         (1 << 11)
#define RAT_NEWSHOTGUN          (1 << 12)
#define RAT_ADDITIVEJUMP        (1 << 13)
#define RAT_NOTIMENUDGE         (1 << 14)
#define RAT_FLAGINDICATOR       (1 << 15)
#define RAT_REGULARFOOTSTEPS    (1 << 16)
#define RAT_NOINVISWALLS        (1 << 17)
#define RAT_NOBOBUP             (1 << 18)
#define RAT_FASTSWIM            (1 << 19)
#define RAT_SWINGGRAPPLE        (1 << 20)
#define RAT_BRIGHTOUTLINE       (1 << 21)
#define RAT_BRIGHTMODEL         (1 << 22)
#define RAT_SMOOTHSTAIRS        (1 << 23)
#define RAT_NOOVERBOUNCE        (1 << 24)
#define RAT_FREEZETAG           (1 << 25)
#define RAT_CROUCHSLIDE         (1 << 26)
#define RAT_SLIDEMODE           (1 << 27)
#define RAT_FORCETAUNTS         (1 << 28)
#define RAT_PASSENEMIES         (1 << 29)   /* g_passPlayers & 1 */
#define RAT_PASSALLIES          (1 << 30)   /* g_passPlayers & 2 */

/* calculated by modulus.c for appropriate dividers */
#define TMOD_004  4272943
#define TMOD_075  2292106
#define TMOD_1000 5730265
#define TMOD_2000 5730265



void    BG_EvaluateTrajectory(const trajectory_t* tr, int atTime, vec3_t result);
void    BG_EvaluateTrajectoryDelta(const trajectory_t* tr, int atTime, vec3_t result);

void    BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t* ps);

void    BG_TouchJumpPad(playerState_t* ps, entityState_t* jumppad);

void    BG_PlayerStateToEntityState(playerState_t* ps, entityState_t* s, qboolean snap);
void    BG_PlayerStateToEntityStateExtraPolate(playerState_t* ps, entityState_t* s, int time, qboolean snap);

qboolean    BG_PlayerTouchesItem(playerState_t* ps, entityState_t* item, int atTime);

//
// bg_auth.c
//
void G_OSPAuthInit(void);
void G_OSPAuthUpdateName(char* str);
void G_OSPAuthGetSecret(char* str);


#define ARENAS_PER_TIER     4
#define MAX_ARENAS          1024
#define MAX_ARENAS_TEXT     8192

#define MAX_BOTS            1024
#define MAX_BOTS_TEXT       8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME        0
#define KAMI_SHOCKWAVEFADE_STARTTIME    1500
#define KAMI_SHOCKWAVE_ENDTIME          2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME          250
#define KAMI_IMPLODE_STARTTIME          2000
#define KAMI_IMPLODE_ENDTIME            2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME       2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME   2500
#define KAMI_SHOCKWAVE2_ENDTIME         3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS      88
#define KAMI_BOOMSPHEREMODEL_RADIUS     72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS        1320
#define KAMI_BOOMSPHERE_MAXRADIUS       720
#define KAMI_SHOCKWAVE2_MAXRADIUS       704

#ifdef __cplusplus
}
#endif

#endif
