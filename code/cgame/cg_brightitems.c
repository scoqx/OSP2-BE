/*
===========================================================================
Bright 3D world items (cg_drawBrightItems). Edit itemBrightColorDefaults[] below.
Each row: classname, R, G, B, A (0.0-1.0).
===========================================================================
*/
#include "cg_local.h"
#include "../game/bg_local.h"
typedef struct
{
	const char* classname;
	float       r;
	float       g;
	float       b;
	float       a;
} itemBrightColorEntry_t;

/*
 * Per-item RGBA. Add or change rows; unmatched classnames stay white, opaque.
 */
static const itemBrightColorEntry_t itemBrightColorDefaults[] =
{
	/* armor */
	{ "item_armor_shard",      0.49f, 0.69, 0.28f, 1.00f },
	{ "item_armor_combat",     1.00f, 1.00f, 0.00f, 1.00f },
	{ "item_armor_body",       1.00f, 0.00f, 0.00f, 1.00f },
	{ "item_armor_jacket",     0.00f, 1.00f, 0.00f, 1.00f },
	/* health */
	{ "item_health_small",     0.00f, 1.00f, 0.00f, 1.00f },
	{ "item_health",           1.00f, 1.00f, 0.00f, 1.00f },
	{ "item_health_large",     1.00f, 0.50f, 0.00f, 1.00f },
	{ "item_health_mega",      0.15f, 0.6f, 0.95f, 1.00f },
	/* ammo (world weapons: cg_drawBrightWeapons bit 8) */
	{ "ammo_shells",             1.00f, 0.50f, 0.00f, 1.00f },
	{ "ammo_bullets",            1.00f, 1.00f, 0.00f, 1.00f },
	{ "ammo_grenades",           0.00f, 1.00f, 0.00f, 1.00f },
	{ "ammo_cells",              1.00f, 0.00f, 1.00f, 1.00f },
	{ "ammo_lightning",          1.00f, 1.00f, 1.00f, 1.00f },
	{ "ammo_rockets",            1.00f, 0.00f, 0.00f, 1.00f },
	{ "ammo_slugs",              0.00f, 1.00f, 1.00f, 1.00f },
	{ "ammo_bfg",                0.00f, 0.00f, 1.00f, 1.00f },
	/* holdables */
	{ "holdable_teleporter",     0.00f, 1.00f, 1.00f, 1.00f },
	{ "holdable_medkit",         0.00f, 1.00f, 0.50f, 1.00f },
	/* powerups (quad/enviro defaults match icons/dye/colors.cfg) */
	{ "item_quad",    CG_DEFAULT_QUAD_DAMAGE_R, CG_DEFAULT_QUAD_DAMAGE_G, CG_DEFAULT_QUAD_DAMAGE_B, 1.00f },
	{ "item_enviro",  CG_DEFAULT_BATTLE_SUIT_R, CG_DEFAULT_BATTLE_SUIT_G, CG_DEFAULT_BATTLE_SUIT_B, 1.00f },
	{ "item_haste",              1.00f, 1.00f, 0.00f, 1.00f },
	{ "item_invis",              0.60f, 0.60f, 1.00f, 1.00f },
	{ "item_regen",              1.00f, 0.00f, 1.00f, 1.00f },
	{ "item_flight",             0.50f, 0.80f, 1.00f, 1.00f },
	/* CTF (classic red/blue only if cg_oldFlags bit 1; else team colors) */
	{ "team_CTF_redflag",  CG_DEFAULT_FLAG_RED_R, CG_DEFAULT_FLAG_RED_G, CG_DEFAULT_FLAG_RED_B, 1.00f },
	{ "team_CTF_blueflag", CG_DEFAULT_FLAG_BLUE_R, CG_DEFAULT_FLAG_BLUE_G, CG_DEFAULT_FLAG_BLUE_B, 1.00f },
	{ NULL, 1.00f, 1.00f, 1.00f, 1.00f }
};

#define BRIGHT_CLASSNAME_HASH_SIZE 65536u
#define BRIGHT_DEFAULTS_HASH_MAX  32

static unsigned long brightItemNameHash[MAX_ITEMS];
static unsigned long brightKnownHashQuad;
static unsigned long brightKnownHashEnviro;
static unsigned long brightKnownHashRedFlag;
static unsigned long brightKnownHashBlueFlag;
static int           brightItemIdxQuad;
static int           brightItemIdxEnviro;
static int           brightItemIdxRedFlag;
static int           brightItemIdxBlueFlag;

static void CG_BrightItemsCacheClassnameData(void)
{
	int           i;
	const char*   cn;

	brightKnownHashQuad = Com_GenerateHashValue("item_quad", BRIGHT_CLASSNAME_HASH_SIZE);
	brightKnownHashEnviro = Com_GenerateHashValue("item_enviro", BRIGHT_CLASSNAME_HASH_SIZE);
	brightKnownHashRedFlag = Com_GenerateHashValue("team_CTF_redflag", BRIGHT_CLASSNAME_HASH_SIZE);
	brightKnownHashBlueFlag = Com_GenerateHashValue("team_CTF_blueflag", BRIGHT_CLASSNAME_HASH_SIZE);

	brightItemIdxQuad = -1;
	brightItemIdxEnviro = -1;
	brightItemIdxRedFlag = -1;
	brightItemIdxBlueFlag = -1;

	for (i = 1; i < bg_numItems; i++)
	{
		cn = bg_itemlist[i].classname;
		if (!cn)
		{
			brightItemNameHash[i] = 0;
			continue;
		}
		brightItemNameHash[i] = Com_GenerateHashValue(cn, BRIGHT_CLASSNAME_HASH_SIZE);

		if (brightItemIdxQuad < 0 && brightItemNameHash[i] == brightKnownHashQuad &&
		        !Q_stricmp(cn, "item_quad"))
		{
			brightItemIdxQuad = i;
		}
		else if (brightItemIdxEnviro < 0 && brightItemNameHash[i] == brightKnownHashEnviro &&
		         !Q_stricmp(cn, "item_enviro"))
		{
			brightItemIdxEnviro = i;
		}
		else if (brightItemIdxRedFlag < 0 && brightItemNameHash[i] == brightKnownHashRedFlag &&
		         !Q_stricmp(cn, "team_CTF_redflag"))
		{
			brightItemIdxRedFlag = i;
		}
		else if (brightItemIdxBlueFlag < 0 && brightItemNameHash[i] == brightKnownHashBlueFlag &&
		         !Q_stricmp(cn, "team_CTF_blueflag"))
		{
			brightItemIdxBlueFlag = i;
		}
	}
}

static int CG_BrightItemIndexByClassname(const char* classname)
{
	unsigned long h;
	int           i;
	const char*   cn;

	if (!classname || !classname[0])
	{
		return -1;
	}

	h = Com_GenerateHashValue(classname, BRIGHT_CLASSNAME_HASH_SIZE);
	for (i = 1; i < bg_numItems; i++)
	{
		if (brightItemNameHash[i] != h)
		{
			continue;
		}
		cn = bg_itemlist[i].classname;
		if (cn && !Q_stricmp(cn, classname))
		{
			return i;
		}
	}
	return -1;
}

static int CG_SetBrightItemColorByClassname(const char* classname, const float rgba[4])
{
	int idx;

	idx = CG_BrightItemIndexByClassname(classname);
	if (idx < 0 || !rgba)
	{
		return -1;
	}

	cgs.beBrightItemsColor[idx][0] = rgba[0];
	cgs.beBrightItemsColor[idx][1] = rgba[1];
	cgs.beBrightItemsColor[idx][2] = rgba[2];
	cgs.beBrightItemsColor[idx][3] = rgba[3];
	return idx;
}

qboolean CG_BrightItemBeinfoOverrideAllowed(const char* classname)
{
	unsigned long h;

	if (!classname || !classname[0])
	{
		return qfalse;
	}

	h = Com_GenerateHashValue(classname, BRIGHT_CLASSNAME_HASH_SIZE);
	if (h == brightKnownHashQuad && !Q_stricmp(classname, "item_quad"))
	{
		return cg.quadDamageCustomColor ? qfalse : qtrue;
	}
	if (h == brightKnownHashEnviro && !Q_stricmp(classname, "item_enviro"))
	{
		return cg.battleSuitCustomColor ? qfalse : qtrue;
	}
	if ((h == brightKnownHashRedFlag && !Q_stricmp(classname, "team_CTF_redflag")) ||
	        (h == brightKnownHashBlueFlag && !Q_stricmp(classname, "team_CTF_blueflag")))
	{
		return CG_FlagUsesClassicBrightColor();
	}

	return qtrue;
}

static weapon_t CG_BrightWeaponByClassname(const char* classname)
{
	unsigned long h;
	int           i;
	const char*   cn;

	if (!classname || !classname[0])
	{
		return WP_NONE;
	}

	h = Com_GenerateHashValue(classname, BRIGHT_CLASSNAME_HASH_SIZE);
	for (i = 1; i < bg_numItems; i++)
	{
		if (bg_itemlist[i].giType != IT_WEAPON)
		{
			continue;
		}
		if (brightItemNameHash[i] != h)
		{
			continue;
		}
		cn = bg_itemlist[i].classname;
		if (cn && !Q_stricmp(cn, classname))
		{
			return bg_itemlist[i].giTag;
		}
	}

	return WP_NONE;
}

static void CG_BeinfoBrightWeaponOverride(const char* classname, float r, float g, float b, float a)
{
	weapon_t weapon;

	weapon = CG_BrightWeaponByClassname(classname);
	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS)
	{
		return;
	}

	cgs.beBrightWeaponsColor[weapon][0] = r;
	cgs.beBrightWeaponsColor[weapon][1] = g;
	cgs.beBrightWeaponsColor[weapon][2] = b;
	cgs.beBrightWeaponsBeinfoOpaque[weapon] = a;
	cgs.beBrightWeaponsBeinfoSet[weapon] = qtrue;
}

static void CG_BeinfoBrightItemOverride(const char* classname, float r, float g, float b, float a)
{
	vec4_t color;
	int    idx;

	if (!CG_BrightItemBeinfoOverrideAllowed(classname))
	{
		return;
	}

	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
	idx = CG_SetBrightItemColorByClassname(classname, color);
	if (idx >= 0)
	{
		cgs.beBrightItemsBeinfoSet[idx] = qtrue;
	}
}

void CG_UpdateBrightItemsDynamicColors(void)
{
	vec4_t color;

	if (cg.quadDamageCustomColor)
	{
		color[0] = cg.beQuadDamageColor[0];
		color[1] = cg.beQuadDamageColor[1];
		color[2] = cg.beQuadDamageColor[2];
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("item_quad", color);
	}
	else if (brightItemIdxQuad < 0 || !cgs.beBrightItemsBeinfoSet[brightItemIdxQuad])
	{
		CG_GetDefaultQuadDamageColor(color);
		CG_SetBrightItemColorByClassname("item_quad", color);
	}

	if (cg.battleSuitCustomColor)
	{
		color[0] = cg.beBattleSuitColor[0];
		color[1] = cg.beBattleSuitColor[1];
		color[2] = cg.beBattleSuitColor[2];
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("item_enviro", color);
	}
	else if (brightItemIdxEnviro < 0 || !cgs.beBrightItemsBeinfoSet[brightItemIdxEnviro])
	{
		CG_GetDefaultBattleSuitColor(color);
		CG_SetBrightItemColorByClassname("item_enviro", color);
	}

	if (CG_FlagUsesClassicBrightColor())
	{
		color[0] = CG_DEFAULT_FLAG_RED_R;
		color[1] = CG_DEFAULT_FLAG_RED_G;
		color[2] = CG_DEFAULT_FLAG_RED_B;
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("team_CTF_redflag", color);

		color[0] = CG_DEFAULT_FLAG_BLUE_R;
		color[1] = CG_DEFAULT_FLAG_BLUE_G;
		color[2] = CG_DEFAULT_FLAG_BLUE_B;
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("team_CTF_blueflag", color);
	}
	else
	{
		color[0] = cgs.beRedTeamColor[0];
		color[1] = cgs.beRedTeamColor[1];
		color[2] = cgs.beRedTeamColor[2];
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("team_CTF_redflag", color);

		color[0] = cgs.beBlueTeamColor[0];
		color[1] = cgs.beBlueTeamColor[1];
		color[2] = cgs.beBlueTeamColor[2];
		color[3] = 1.0f;
		CG_SetBrightItemColorByClassname("team_CTF_blueflag", color);
	}
}

void CG_InitBrightItemsColors(void)
{
	int           i;
	int           j;
	int           numDefaults;
	vec4_t        white;
	unsigned long brightDefaultHash[BRIGHT_DEFAULTS_HASH_MAX];

	white[0] = 1.0f;
	white[1] = 1.0f;
	white[2] = 1.0f;
	white[3] = 1.0f;

	for (i = 0; i < MAX_ITEMS; i++)
	{
		Vector4Copy(white, cgs.beBrightItemsColor[i]);
		cgs.beBrightItemsBeinfoSet[i] = qfalse;
	}

	CG_BrightItemsCacheClassnameData();

	numDefaults = 0;
	for (j = 0; itemBrightColorDefaults[j].classname; j++)
	{
		if (numDefaults < BRIGHT_DEFAULTS_HASH_MAX)
		{
			brightDefaultHash[numDefaults] = Com_GenerateHashValue(
			        itemBrightColorDefaults[j].classname, BRIGHT_CLASSNAME_HASH_SIZE);
			numDefaults++;
		}
	}

	for (i = 1; i < bg_numItems; i++)
	{
		const char*   cn;
		unsigned long h;

		cn = bg_itemlist[i].classname;
		if (!cn)
		{
			continue;
		}
		h = brightItemNameHash[i];
		for (j = 0; j < numDefaults; j++)
		{
			if (h != brightDefaultHash[j])
			{
				continue;
			}
			if (!Q_stricmp(cn, itemBrightColorDefaults[j].classname))
			{
				cgs.beBrightItemsColor[i][0] = itemBrightColorDefaults[j].r;
				cgs.beBrightItemsColor[i][1] = itemBrightColorDefaults[j].g;
				cgs.beBrightItemsColor[i][2] = itemBrightColorDefaults[j].b;
				cgs.beBrightItemsColor[i][3] = itemBrightColorDefaults[j].a;
				break;
			}
		}
	}

	CG_UpdateBrightItemsDynamicColors();

}

void CG_BeinfoApplyBrightColor(const char* classname, float r, float g, float b, float a)
{
	if (!classname || !classname[0])
	{
		return;
	}

	if (classname[0] == 'w' && !Q_stricmpn(classname, "weapon_", 7))
	{
		CG_BeinfoBrightWeaponOverride(classname, r, g, b, a);
		return;
	}

	CG_BeinfoBrightItemOverride(classname, r, g, b, a);
}

qhandle_t CG_SelectBrightItemsShader(int gitype)
{
	if (gitype == IT_TEAM)
	{
		if (cg_drawBrightItems.integer == 2)
		{
			return cgs.media.flagBrightAltShader;
		}
		return cgs.media.flagBrightShader;
	}
	if (cg_drawBrightItems.integer == 2)
	{
		return cgs.media.altFullBrightShader;
	}
	return cgs.media.fullBrightShader;
}

qboolean CG_ApplyBrightItem(refEntity_t* ent, int itemNum, int gitype)
{
	const float* bc;
	float       op;
	int         drawBright;

	if (gitype == IT_WEAPON)
	{
		return qfalse;
	}
	drawBright = cg_drawBrightItems.integer;
	if (!drawBright || cg_simpleItems.integer)
	{
		return qfalse;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT))
	{
		return qfalse;
	}
	if (itemNum < 1 || itemNum >= MAX_ITEMS)
	{
		return qfalse;
	}

	bc = cgs.beBrightItemsColor[itemNum];
	if (cgs.beBrightItemsBeinfoSet[itemNum])
	{
		op = bc[3];
	}
	else
	{
		op = cg_brightItemsOpaque.value;
	}
	if (op < 0.0f)
	{
		op = 0.0f;
	}
	else if (op > 1.0f)
	{
		op = 1.0f;
	}

	ent->customShader = CG_SelectBrightItemsShader(gitype);
	ent->shaderRGBA[0] = (byte)(255 * bc[0]);
	ent->shaderRGBA[1] = (byte)(255 * bc[1]);
	ent->shaderRGBA[2] = (byte)(255 * bc[2]);
	ent->shaderRGBA[3] = (byte)(255 * op);
	return qtrue;
}
