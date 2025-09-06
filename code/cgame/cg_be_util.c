#include "cg_local.h"
#include "../qcommon/qcommon.h"



#define MAX_XSTATS1_BLOCKS 64
#define MAX_XSTATS1_STRLEN 2048

static int xstats1_count = 0;
static char *xstats1_blocks[MAX_XSTATS1_BLOCKS];

qboolean CG_BE_Timer(int msec)
{
	static int lastTime = 0;
	int currentTime = cg.time;

	int diff = currentTime - lastTime;
	if (diff < 0)
	{
		lastTime = currentTime;
		return qfalse;
	}
	if (diff >= msec)
	{
		lastTime = currentTime;
		return qtrue;
	}
	return qfalse;
}

void CG_UpdateBeFeatures(void)
{
	qboolean changed = qfalse;
	int beFlags = 0;
	clientInfo_t* ci = &cgs.clientinfo[cg.clientNum];

	if (!BE_ENABLED)
	{
		beFlags = 0;
		changed = qtrue;
	}
	else
	{
		if (cg_friendsWallhack.integer)
		{
			beFlags |= CG_BE_TEAM_FOE_WH;
			changed = qtrue;
		}
		if (cg_altGrenades.integer == 2)
		{
			beFlags |= CG_BE_ALT_GRENADES;
			changed = qtrue;
		}
		if (cg_drawOutline.integer)
		{
			beFlags |= CG_BE_OUTLINE;
			changed = qtrue;
		}
		if (cgs.customModelSound)
		{
			beFlags |= CG_BE_MODELSOUND;
			changed = qtrue;
		}
		if (cg_markTeam.integer >= 0)
		{
			beFlags |= CG_BE_MARK_TEAMMATE;
			changed = qtrue;
		}
		if (cg_teamIndicator.integer)
		{
			beFlags |= CG_BE_TEAM_INDICATOR;
			changed = qtrue;
		}
		if (cg_altShadow.integer)
		{
			beFlags |= CG_BE_ALT_SHADOW;
			changed = qtrue;
		}
		if (cg_altBlood.integer)
		{
			beFlags |= CG_BE_ALT_BLOOD;
			changed = qtrue;
		}
		if (cg_enemyLightningColor.integer || cg_enemyLightningColor.string)
		{
			beFlags |= CG_BE_ENEMYLIGHTNING;
			changed = qtrue;
		}
	}

	if (changed)
	{
		trap_Cvar_Set("be_features", va("%d", beFlags));
	}
}

void BE_PrintDisabledFeatures(qboolean request)
{
	int value = cgs.be.disableFeatures;
	if (value == 0)
	{
		if (request)
		{
		CG_Printf("   [^9OSP2-BE^7] All features enabled.\n");
		}
	}
	else if (value == 4095) // all bits set
	{
		CG_Printf("   [^9OSP2-BE^7] All features disabled.\n");
	}
	else
	{
		char buffer[1024] = "   [^9OSP2-BE^7] Disabled features: ";
		qboolean first = qtrue;
		if (value & CG_BE_TEAM_FOE_WH) { Q_strcat(buffer, sizeof(buffer), first ? "Team wh" : ", team wh"); first = qfalse; }
		if (value & CG_BE_MODELSOUND) { Q_strcat(buffer, sizeof(buffer), first ? "Model sound" : ", model sound"); first = qfalse; }
		if (value & CG_BE_ALT_GRENADES) { Q_strcat(buffer, sizeof(buffer), first ? "Alt grenades" : ", alt grenades"); first = qfalse; }
		if (value & CG_BE_ENEMYLIGHTNING) { Q_strcat(buffer, sizeof(buffer), first ? "Enemy lightning" : ", enemy lightning"); first = qfalse; }
		if (value & CG_BE_MARK_TEAMMATE) { Q_strcat(buffer, sizeof(buffer), first ? "Mark teammate" : ", mark teammate"); first = qfalse; }
		if (value & CG_BE_ALT_SHADOW) { Q_strcat(buffer, sizeof(buffer), first ? "Alt shadow" : ", alt shadow"); first = qfalse; }
		if (value & CG_BE_ALT_BLOOD) { Q_strcat(buffer, sizeof(buffer), first ? "Alt blood" : ", alt blood"); first = qfalse; }
		if (value & CG_BE_OUTLINE) { Q_strcat(buffer, sizeof(buffer), first ? "Outline" : ", outline"); first = qfalse; }
		if (value & CG_BE_TEAM_INDICATOR) { Q_strcat(buffer, sizeof(buffer), first ? "team indicator" : ", team indicator"); first = qfalse; }
		if (value & CG_BE_DAMAGEINFO) { Q_strcat(buffer, sizeof(buffer), first ? "Damage count" : ", damage count"); first = qfalse; }
		if (value & CG_BE_FULLBRIGHT) { Q_strcat(buffer, sizeof(buffer), first ? "Fullbright" : ", fullbright"); first = qfalse; }
		CG_Printf("%s\n", buffer);
	}
}

// Helper: split a string into arguments (space-separated)
static int CG_ParseArgs(const char *str, char **argv, int max_args) {
	int argc = 0;
	const char *p = str;
	while (*p && argc < max_args) {
		// Skip spaces
		while (*p == ' ') p++;
		if (!*p) break;
		argv[argc++] = (char *)p;
		// Find end of arg
		while (*p && *p != ' ') p++;
		if (*p) {
			*((char *)p) = '\0'; // Temporarily null-terminate
			p++;
		}
	}
	return argc;
}

// Argument getter type
typedef const char* (*CG_ArgvGetter)(int idx, void *user);

// Local argv getter for parsed xstats1 block
static const char* CG_LocalArgv(int idx, void *user) {
	char **argv = (char **)user;
	return argv[idx];
}

// Refactored to accept argument getter
void CG_BEParseXStatsToStatsAllEx(int *pIndex, CG_ArgvGetter argv_getter, void *user) {
	int i;
	int index = *pIndex;
	int wstats_condition;
	int client_id;
	int hits_value, atts_value, kills_value, deaths_value;
	int pickUps, drops;
	float accuracy;
	int armor_taken, health_taken, megahealth, green_armor, red_armor, yellow_armor;
	int damage_given, damage_rcvd;
	int totalKills, totalDeaths;
	newStatsInfo_t* ws;

	client_id = atoi(argv_getter(index++, user));
	wstats_condition = atoi(argv_getter(index++, user));

	CG_Printf("[XSTATS] client_id=%d wstats_condition=%d\n", client_id, wstats_condition);

	if (client_id < 0 || client_id >= MAX_CLIENTS)
	{
		*pIndex = index;
		return;
	}

	ws = &cgs.be.statsAll[client_id];

	for (i = 1; i < WP_NUM_WEAPONS; ++i)
	{
		if ((wstats_condition & (1 << i)) != 0)
		{
			hits_value = atoi(argv_getter(index++, user));
			atts_value = atoi(argv_getter(index++, user));
			kills_value = atoi(argv_getter(index++, user));
			deaths_value = atoi(argv_getter(index++, user));

			pickUps = atts_value >> cgs.osp.stats_shift;
			drops   = hits_value >> cgs.osp.stats_shift;

			CG_Printf("[XSTATS] W%d: hits=%d shots=%d kills=%d deaths=%d pickUps=%d drops=%d\n",
				i, hits_value & cgs.osp.stats_mask, atts_value & cgs.osp.stats_mask, kills_value, deaths_value, pickUps, drops);

			ws->stats[i].hits = hits_value & cgs.osp.stats_mask;
			ws->stats[i].shots = atts_value & cgs.osp.stats_mask;
			ws->stats[i].kills = kills_value;
			ws->stats[i].deaths = deaths_value;
			ws->stats[i].pickUps = pickUps;
			ws->stats[i].drops = drops;
			ws->stats[i].accuracy = (ws->stats[i].shots > 0) ? ((float)ws->stats[i].hits / ws->stats[i].shots) * 100.0f : 0.0f;
		}
		else
		{
			ws->stats[i].hits = 0;
			ws->stats[i].shots = 0;
			ws->stats[i].kills = 0;
			ws->stats[i].deaths = 0;
			ws->stats[i].pickUps = 0;
			ws->stats[i].drops = 0;
			ws->stats[i].accuracy = 0.0f;
		}
	}

	armor_taken   = atoi(argv_getter(index++, user));
	health_taken  = atoi(argv_getter(index++, user));
	damage_given  = atoi(argv_getter(index + 2, user));
	damage_rcvd   = atoi(argv_getter(index + 3, user));
	megahealth    = atoi(argv_getter(index + 4, user));
	green_armor   = atoi(argv_getter(index + 5, user));
	red_armor     = atoi(argv_getter(index + 6, user));
	yellow_armor  = atoi(argv_getter(index + 7, user));

	CG_Printf("[XSTATS] Armor=%d Health=%d DmgGiven=%d DmgRcvd=%d MH=%d GA=%d RA=%d YA=%d\n",
		armor_taken, health_taken, damage_given, damage_rcvd, megahealth, green_armor, red_armor, yellow_armor);

	ws->armor      = armor_taken;
	ws->health     = health_taken;
	ws->megahealth = megahealth;
	ws->ga         = green_armor;
	ws->ra         = red_armor;
	ws->ya         = yellow_armor;
	ws->dmgGiven   = damage_given;
	ws->dmgReceived= damage_rcvd;

	ws->damageRatio = (damage_given > 0 || damage_rcvd > 0) ?
	                  (float)damage_given / (damage_rcvd > 0 ? damage_rcvd : 1) : 0.0f;

	totalKills = 0;
	totalDeaths = 0;
	for (i = 1; i < WP_NUM_WEAPONS; ++i)
	{
		totalKills += ws->stats[i].kills;
		totalDeaths += ws->stats[i].deaths;
	}
	ws->kills = totalKills;
	ws->deaths = totalDeaths;
	ws->kdratio = (totalKills > 0 && totalDeaths == 0) ? (float)totalKills :
	              (totalDeaths > 0) ? (float)totalKills / totalDeaths : 0.0f;

	ws->efficiency = (totalKills + totalDeaths > 0) ?
	                 (100.0f * (float)totalKills / (totalKills + totalDeaths)) : 0.0f;
	if (ws->efficiency < 0.0f)
		ws->efficiency = 0.0f;

	ws->customStatsCalled = qfalse;

	CG_Printf("[XSTATS] FINAL: client_id=%d kills=%d deaths=%d kdr=%.2f eff=%.2f dmgGiven=%d dmgRcvd=%d ratio=%.2f\n",
		client_id, ws->kills, ws->deaths, ws->kdratio, ws->efficiency, ws->dmgGiven, ws->dmgReceived, ws->damageRatio);

	index += 8;
	*pIndex = index;
}

// Old wrapper for server command
void CG_BEParseXStatsToStatsAll(int *pIndex)
{
	CG_BEParseXStatsToStatsAllEx(pIndex, (CG_ArgvGetter)CG_Argv, NULL);
}

// Wrapper for multiple xstats blocks (server command)
qboolean CG_BEParseXStatsToStatsAll_Multi(int numBlocks)
{
	int i;
	int index = 1;
	for (i = 0; i < numBlocks; ++i)
	{
		CG_BEParseXStatsToStatsAll(&index);
	}
	return qtrue;
}

// Вызывается при получении xstats1
void CG_StoreXStats1(const char *xstats1)
{
	if (xstats1_count < MAX_XSTATS1_BLOCKS) {
		// Выделяем память через Z_Malloc
		char *buf = Z_Malloc(MAX_XSTATS1_STRLEN);
		OSP_MEMORY_CHECK(buf);
		Q_strncpyz(buf, xstats1, MAX_XSTATS1_STRLEN);
		xstats1_blocks[xstats1_count++] = buf;
	}
}

// После накопления всех xstats1, вызывайте эту функцию
void CG_ParseAllXStats1Blocks(void)
{
	int i;
	for (i = 0; i < xstats1_count; ++i) {
		int index = 1;
		char *argv[64];
		char *block = xstats1_blocks[i];
		int argc = CG_ParseArgs(block, argv, 64);
		CG_BEParseXStatsToStatsAllEx(&index, CG_LocalArgv, argv);
		Z_Free(block);
	}
	xstats1_count = 0;
}

// Получить количество накопленных xstats1-блоков
int CG_GetXStats1Count(void) {
	return xstats1_count;
}

// Получить client_id из xstats1 блока (первый аргумент после команды)
int CG_GetXStats1ClientId(const char *xstats1) {
	// Пропустить команду, взять первый аргумент
	const char *p = xstats1;
	while (*p && *p != ' ') p++; // skip command
	while (*p == ' ') p++;
	return atoi(p);
}

// Разобрать все накопленные xstats1 по client_id
void CG_ParseAllXStats1BlocksByClient(void)
{
	int i;
	for (i = 0; i < xstats1_count; ++i) {
		int index = 1;
		char *argv[64];
		char *block = xstats1_blocks[i];
		int argc = CG_ParseArgs(block, argv, 64);
		CG_BEParseXStatsToStatsAllEx(&index, CG_LocalArgv, argv);
		Z_Free(block);
	}
	xstats1_count = 0;
}
