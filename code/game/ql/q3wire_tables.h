/*
===========================================================================
Quake Live -> Quake III / OSP2-BE value tables.

Pure data, no engine dependencies, so the same file can later be included
by OSP2-BE (QVM-safe C). Each table is indexed by the QL value and gives the
Q3/OSP2-BE value, or Q3W_DROP when Q3 has no equivalent.

Numbering sources: ioquakelive code/game/bg_public.h + bg_misc.c (QL) and
osp2-be code/game/bg_public.h + bg_misc.c (Q3 / OSP2-BE).
===========================================================================
*/
#ifndef Q3WIRE_TABLES_H
#define Q3WIRE_TABLES_H

#define Q3W_DROP (-1)

/* every includer uses only some tables: keep gcc quiet about the rest (q3lcc has no attributes) */
#if defined(__GNUC__)
#define Q3W_TABLE static const int __attribute__((unused))
#else
#define Q3W_TABLE static const int
#endif

/* ------------------------------------------------------------------------
   entity_event_t (QL EV_NUM_ETYPES = 100)
   ------------------------------------------------------------------------ */
#define Q3W_NUM_QL_EVENTS 100

#define Q3W_EV_NONE 0
#define Q3W_EV_FOOTSTEP 1
#define Q3W_EV_ITEM_PICKUP 19
#define Q3W_EV_GLOBAL_ITEM_PICKUP 20
#define Q3W_EV_GLOBAL_TEAM_SOUND 47
#define Q3W_EV_OBITUARY 60
#define Q3W_EV_DAMAGEPLUM 83 /* OSP2-BE */

/* QL event numbers that need their parameters translated too */
#define QLW_EV_ITEM_PICKUP 15
#define QLW_EV_GLOBAL_ITEM_PICKUP 16
#define QLW_EV_GLOBAL_TEAM_SOUND 44
#define QLW_EV_OBITUARY 58
#define QLW_EV_DAMAGEPLUM 96

Q3W_TABLE q3w_event[] = {
    0,         /* 0  NONE */
    1,         /* 1  FOOTSTEP */
    2,         /* 2  FOOTSTEP_METAL */
    3,         /* 3  FOOTSPLASH */
    4,         /* 4  FOOTWADE */
    5,         /* 5  SWIM */
    10,        /* 6  FALL_SHORT */
    11,        /* 7  FALL_MEDIUM */
    12,        /* 8  FALL_FAR */
    13,        /* 9  JUMP_PAD */
    14,        /* 10 JUMP */
    15,        /* 11 WATER_TOUCH */
    16,        /* 12 WATER_LEAVE */
    17,        /* 13 WATER_UNDER */
    18,        /* 14 WATER_CLEAR */
    19,        /* 15 ITEM_PICKUP */
    20,        /* 16 GLOBAL_ITEM_PICKUP */
    21,        /* 17 NOAMMO */
    22,        /* 18 CHANGE_WEAPON */
    Q3W_DROP,  /* 19 DROP_WEAPON */
    23,        /* 20 FIRE_WEAPON */
    24,        /* 21 USE_ITEM0 */
    25,        /* 22 USE_ITEM1 */
    26,        /* 23 USE_ITEM2 */
    27,        /* 24 USE_ITEM3 */
    28,        /* 25 USE_ITEM4 */
    29,        /* 26 USE_ITEM5 */
    30,        /* 27 USE_ITEM6 */
    31,        /* 28 USE_ITEM7 */
    32,        /* 29 USE_ITEM8 */
    33,        /* 30 USE_ITEM9 */
    34,        /* 31 USE_ITEM10 */
    35,        /* 32 USE_ITEM11 */
    36,        /* 33 USE_ITEM12 */
    37,        /* 34 USE_ITEM13 */
    38,        /* 35 USE_ITEM14 */
    39,        /* 36 USE_ITEM15 */
    40,        /* 37 ITEM_RESPAWN */
    41,        /* 38 ITEM_POP */
    42,        /* 39 PLAYER_TELEPORT_IN */
    43,        /* 40 PLAYER_TELEPORT_OUT */
    44,        /* 41 GRENADE_BOUNCE */
    45,        /* 42 GENERAL_SOUND */
    46,        /* 43 GLOBAL_SOUND */
    47,        /* 44 GLOBAL_TEAM_SOUND */
    48,        /* 45 BULLET_HIT_FLESH */
    49,        /* 46 BULLET_HIT_WALL */
    50,        /* 47 MISSILE_HIT */
    51,        /* 48 MISSILE_MISS */
    52,        /* 49 MISSILE_MISS_METAL */
    53,        /* 50 RAILTRAIL */
    54,        /* 51 SHOTGUN */
    55,        /* 52 BULLET */
    56,        /* 53 PAIN */
    57,        /* 54 DEATH1 */
    58,        /* 55 DEATH2 */
    59,        /* 56 DEATH3 */
    57,        /* 57 DROWN -> DEATH1 (Q3 cgame picks the drown sound in water) */
    60,        /* 58 OBITUARY */
    61,        /* 59 POWERUP_QUAD */
    62,        /* 60 POWERUP_BATTLESUIT */
    63,        /* 61 POWERUP_REGEN */
    Q3W_DROP,  /* 62 POWERUP_ARMORREGEN */
    64,        /* 63 GIB_PLAYER */
    65,        /* 64 SCOREPLUM */
    66,        /* 65 PROXIMITY_MINE_STICK */
    67,        /* 66 PROXIMITY_MINE_TRIGGER */
    68,        /* 67 KAMIKAZE */
    69,        /* 68 OBELISKEXPLODE */
    70,        /* 69 OBELISKPAIN */
    71,        /* 70 INVUL_IMPACT */
    72,        /* 71 JUICED */
    73,        /* 72 LIGHTNINGBOLT */
    74,        /* 73 DEBUG_LINE */
    76,        /* 74 TAUNT */
    77,        /* 75 TAUNT_YES */
    78,        /* 76 TAUNT_NO */
    79,        /* 77 TAUNT_FOLLOWME */
    80,        /* 78 TAUNT_GETFLAG */
    81,        /* 79 TAUNT_GUARDBASE */
    82,        /* 80 TAUNT_PATROL */
    1,         /* 81 FOOTSTEP_SNOW -> FOOTSTEP */
    1,         /* 82 FOOTSTEP_WOOD -> FOOTSTEP */
    Q3W_DROP,  /* 83 ITEM_PICKUP_SPEC */
    Q3W_DROP,  /* 84 OVERTIME */
    Q3W_DROP,  /* 85 GAMEOVER */
    51,        /* 86 MISSILE_MISS_DMGTHROUGH -> MISSILE_MISS */
    Q3W_DROP,  /* 87 THAW_PLAYER */
    Q3W_DROP,  /* 88 THAW_TICK */
    Q3W_DROP,  /* 89 SHOTGUN_KILL */
    Q3W_DROP,  /* 90 POI */
    Q3W_DROP,  /* 91 DEBUG_HITBOX */
    Q3W_DROP,  /* 92 LIGHTNING_DISCHARGE */
    Q3W_DROP,  /* 93 RACE_START */
    Q3W_DROP,  /* 94 RACE_CHECKPOINT */
    Q3W_DROP,  /* 95 RACE_FINISH */
    83,        /* 96 DAMAGEPLUM -> OSP2-BE EV_DAMAGEPLUM */
    Q3W_DROP,  /* 97 AWARD */
    Q3W_DROP,  /* 98 INFECTED */
    Q3W_DROP,  /* 99 NEW_HIGH_SCORE */
};

/* ------------------------------------------------------------------------
   bg_itemlist index (QL has 58 entries incl. the NULL one, OSP2-BE 37).
   0 means "not present" to the Q3 cgame (CG_Item / pickup ignore it).
   ------------------------------------------------------------------------ */
#define Q3W_NUM_QL_ITEMS 58
#define Q3W_NUM_Q3_ITEMS 37

Q3W_TABLE q3w_item[] = {
    0,  /* 0  NULL */
    1,  /* 1  item_armor_shard */
    2,  /* 2  item_armor_combat */
    3,  /* 3  item_armor_body */
    36, /* 4  item_armor_jacket (OSP2-BE keeps it at the end) */
    4,  /* 5  item_health_small */
    5,  /* 6  item_health */
    6,  /* 7  item_health_large */
    7,  /* 8  item_health_mega */
    8,  /* 9  weapon_gauntlet */
    9,  /* 10 weapon_shotgun */
    10, /* 11 weapon_machinegun */
    11, /* 12 weapon_grenadelauncher */
    12, /* 13 weapon_rocketlauncher */
    13, /* 14 weapon_lightning */
    14, /* 15 weapon_railgun */
    15, /* 16 weapon_plasmagun */
    16, /* 17 weapon_bfg */
    17, /* 18 weapon_grapplinghook */
    18, /* 19 ammo_shells */
    19, /* 20 ammo_bullets */
    20, /* 21 ammo_grenades */
    21, /* 22 ammo_cells */
    22, /* 23 ammo_lightning */
    23, /* 24 ammo_rockets */
    24, /* 25 ammo_slugs */
    25, /* 26 ammo_bfg */
    26, /* 27 holdable_teleporter */
    27, /* 28 holdable_medkit */
    28, /* 29 item_quad */
    29, /* 30 item_enviro */
    30, /* 31 item_haste */
    31, /* 32 item_invis */
    32, /* 33 item_regen */
    33, /* 34 item_flight */
    34, /* 35 team_CTF_redflag */
    35, /* 36 team_CTF_blueflag */
    0,  /* 37 holdable_kamikaze */
    0,  /* 38 holdable_portal */
    0,  /* 39 holdable_invulnerability */
    0,  /* 40 ammo_nails */
    0,  /* 41 ammo_mines */
    0,  /* 42 ammo_belt */
    0,  /* 43 item_scout */
    0,  /* 44 item_guard */
    0,  /* 45 item_doubler */
    0,  /* 46 item_armorregen */
    0,  /* 47 team_CTF_neutralflag */
    0,  /* 48 weapon_nailgun */
    0,  /* 49 weapon_prox_launcher */
    0,  /* 50 weapon_chaingun */
    0,  /* 51 item_spawnarmor */
    10, /* 52 weapon_hmg -> weapon_machinegun */
    19, /* 53 ammo_hmg -> ammo_bullets */
    0,  /* 54 ammo_pack */
    0,  /* 55 item_key_silver */
    0,  /* 56 item_key_gold */
    0,  /* 57 item_key_master */
};

/* ------------------------------------------------------------------------
   weapon_t: identical 0..13, QL adds WP_HMG = 14
   ------------------------------------------------------------------------ */
#define QLW_WP_MACHINEGUN 2
#define QLW_WP_HMG 14

/* ------------------------------------------------------------------------
   powerup_t (QL puts the flags first). Index = QL PW_*, value = Q3 PW_*.
   ------------------------------------------------------------------------ */
#define Q3W_NUM_POWERUPS 16

Q3W_TABLE q3w_powerup[] = {
    0,        /* 0  NONE */
    7,        /* 1  REDFLAG */
    8,        /* 2  BLUEFLAG */
    9,        /* 3  NEUTRALFLAG */
    1,        /* 4  QUAD */
    2,        /* 5  BATTLESUIT */
    3,        /* 6  HASTE */
    5,        /* 7  REGEN */
    4,        /* 8  INVIS */
    6,        /* 9  FLIGHT */
    14,       /* 10 INVULNERABILITY */
    10,       /* 11 SCOUT */
    11,       /* 12 GUARD */
    12,       /* 13 DOUBLER */
    13,       /* 14 AMMOREGEN (QL armor regen rune) */
    Q3W_DROP, /* 15 FREEZE */
};

/* ------------------------------------------------------------------------
   statIndex_t. Index = QL STAT_*, value = Q3/OSP2-BE STAT_*.
   OSP2-BE slots 4 and 7..15 (dead yaw, OSP phys, BE auth...) get nothing.
   ------------------------------------------------------------------------ */
#define Q3W_NUM_STATS 16
#define QLW_STAT_HOLDABLE_ITEM 1
#define QLW_STAT_WEAPONS 3

Q3W_TABLE q3w_stat[] = {
    0,        /* 0  HEALTH */
    1,        /* 1  HOLDABLE_ITEM (item index, translated) */
    Q3W_DROP, /* 2  RUNE */
    2,        /* 3  WEAPONS (bitmask, translated) */
    3,        /* 4  ARMOR */
    Q3W_DROP, /* 5  BSKILL */
    5,        /* 6  CLIENTS_READY */
    6,        /* 7  MAX_HEALTH */
    Q3W_DROP, /* 8  SPINUP */
    Q3W_DROP, /* 9  FLIGHT_THRUST */
    Q3W_DROP, /* 10 CUR_FLIGHT_FUEL */
    Q3W_DROP, /* 11 MAX_FLIGHT_FUEL */
    Q3W_DROP, /* 12 FLIGHT_REFUEL */
    Q3W_DROP, /* 13 JUMPTIME */
    Q3W_DROP, /* 14 ARMORTYPE */
    Q3W_DROP, /* 15 KEY */
};

/* ------------------------------------------------------------------------
   persEnum_t: QL moved ATTACKEE_ARMOR to the end.
   ------------------------------------------------------------------------ */
#define Q3W_NUM_PERSISTANT 16

Q3W_TABLE q3w_pers[] = {
    0,        /* 0  SCORE */
    1,        /* 1  HITS */
    2,        /* 2  RANK */
    3,        /* 3  TEAM */
    4,        /* 4  SPAWN_COUNT */
    5,        /* 5  PLAYEREVENTS */
    6,        /* 6  ATTACKER */
    8,        /* 7  KILLED */
    9,        /* 8  IMPRESSIVE_COUNT */
    10,       /* 9  EXCELLENT_COUNT */
    11,       /* 10 DEFEND_COUNT */
    12,       /* 11 ASSIST_COUNT */
    13,       /* 12 GAUNTLET_FRAG_COUNT */
    14,       /* 13 CAPTURES */
    7,        /* 14 ATTACKEE_ARMOR */
    Q3W_DROP, /* 15 */
};

/* ------------------------------------------------------------------------
   meansOfDeath_t: identical 0..22, QL-only ones mapped to the closest one.
   ------------------------------------------------------------------------ */
#define Q3W_NUM_QL_MODS 34

Q3W_TABLE q3w_mod[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, /* UNKNOWN .. LIGHTNING */
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,     /* BFG .. TRIGGER_HURT */
    0,  /* 23 NAIL */
    3,  /* 24 CHAINGUN -> MACHINEGUN */
    5,  /* 25 PROXIMITY_MINE -> GRENADE_SPLASH */
    0,  /* 26 KAMIKAZE */
    0,  /* 27 JUICED */
    23, /* 28 GRAPPLE */
    20, /* 29 SWITCH_TEAMS -> SUICIDE */
    0,  /* 30 THAW */
    11, /* 31 LIGHTNING_DISCHARGE -> LIGHTNING */
    3,  /* 32 HMG -> MACHINEGUN */
    10, /* 33 RAILGUN_HEADSHOT -> RAILGUN */
};

/* ------------------------------------------------------------------------
   global_team_sound_t: identical 0..13 (up to GTS_KAMIKAZE)
   ------------------------------------------------------------------------ */
#define Q3W_NUM_GTS 14

/* ------------------------------------------------------------------------
   pm_flags: QL bit -> Q3 bit (0 = no Q3 equivalent)
   ------------------------------------------------------------------------ */
Q3W_TABLE q3w_pmflag[][2] = {
    {0x0001, 0x0001}, /* DUCKED */
    {0x0002, 0x0002}, /* JUMP_HELD */
    {0x0008, 0x0008}, /* BACKWARDS_JUMP */
    {0x0010, 0x0010}, /* BACKWARDS_RUN */
    {0x0020, 0x0020}, /* TIME_LAND */
    {0x0040, 0x0040}, /* TIME_KNOCKBACK */
    {0x0080, 0x0100}, /* TIME_WATERJUMP */
    {0x0200, 0x0200}, /* RESPAWNED */
    {0x0400, 0x0400}, /* USE_ITEM_HELD */
    {0x0800, 0x0800}, /* GRAPPLE_PULL */
    {0x1000, 0x1000}, /* FOLLOW */
    {0x2000, 0x2000}, /* SCOREBOARD */
    {0x4000, 0x4000}, /* INVULEXPAND */
    /* QL movement state for OSP2-BE's QL pmove (bg_pmove_ql.h): free Q3 bits */
    {0x20000, 0x0080},  /* DOUBLE_JUMPED -> QLPMF_DOUBLE_JUMPED */
    {0x100000, 0x8000}, /* CROUCH_SLIDE -> QLPMF_CROUCH_SLIDE */
};
#define Q3W_NUM_PMFLAGS (sizeof(q3w_pmflag) / sizeof(q3w_pmflag[0]))

/* ------------------------------------------------------------------------
   QL-only playerState fields packed for OSP2-BE's QL pmove (bg_pmove_ql.h)
   ------------------------------------------------------------------------ */
#define Q3W_PMF_DOUBLE_JUMPED_STATE 0x0004 /* ps->doubleJumped */
#define Q3W_STAT_JUMPAGE 12                /* OSP2-BE STAT_TIME_LASTJUMP: commandTime - jumpTime */
#define Q3W_STAT_CROUCHSLIDE 7             /* OSP2-BE STAT_OSP_PHYS: crouchSlideTime */
#define Q3W_AGE_MAX 30000

/* QL weapons a Q3 client has no slot for (OSP2-BE is built without MISSIONPACK) */
#define QLW_WP_NAILGUN 11
#define QLW_WP_PROX_LAUNCHER 12
#define QLW_WP_CHAINGUN 13

/* ------------------------------------------------------------------------
   g_gametype: QL -> Q3/OSP (QL-only modes show as FFA)
   ------------------------------------------------------------------------ */
#define Q3W_NUM_QL_GAMETYPES 13

Q3W_TABLE q3w_gametype[] = {
    0, /* 0  FFA */
    1, /* 1  DUEL -> TOURNAMENT */
    0, /* 2  RACE */
    3, /* 3  TEAM */
    5, /* 4  CA */
    4, /* 5  CTF */
    0, /* 6  1FCTF */
    0, /* 7  OBELISK */
    0, /* 8  HARVESTER */
    0, /* 9  FREEZE */
    0, /* 10 DOMINATION */
    0, /* 11 AD */
    0, /* 12 RR */
};

#endif /* Q3WIRE_TABLES_H */
