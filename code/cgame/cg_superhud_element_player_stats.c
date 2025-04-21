#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef enum
{
	SHUD_ELEMENT_PLAYER_STATS_DG,
	SHUD_ELEMENT_PLAYER_STATS_DR,
	SHUD_ELEMENT_PLAYER_STATS_DG_ICON,
	SHUD_ELEMENT_PLAYER_STATS_DR_ICON,
} shudElementPlayerStatsType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t textCtx;
	superhudDrawContext_t drawCtx;
	shudElementPlayerStatsType_t type;
} shudElementPlayerStats_t;

void* CG_SHUDElementPlayerStatsCreate(const superhudConfig_t* config, shudElementPlayerStatsType_t type)
{
	shudElementPlayerStats_t* element;
	SHUD_ELEMENT_INIT(element, config);
	element->type = type;
	if (type == SHUD_ELEMENT_PLAYER_STATS_DG || type == SHUD_ELEMENT_PLAYER_STATS_DR)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
	}
	else
	{
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
	}
	return element;
}

void* CG_SHUDElementCreatePlayerStatsDG(const superhudConfig_t* config)
{
	return CG_SHUDElementPlayerStatsCreate(config, SHUD_ELEMENT_PLAYER_STATS_DG);
}
void* CG_SHUDElementCreatePlayerStatsDR(const superhudConfig_t* config)
{
	return CG_SHUDElementPlayerStatsCreate(config, SHUD_ELEMENT_PLAYER_STATS_DR);
}
void* CG_SHUDElementCreatePlayerStatsDGIcon(const superhudConfig_t* config)
{
	return CG_SHUDElementPlayerStatsCreate(config, SHUD_ELEMENT_PLAYER_STATS_DG_ICON);
}
void* CG_SHUDElementCreatePlayerStatsDRIcon(const superhudConfig_t* config)
{
	return CG_SHUDElementPlayerStatsCreate(config, SHUD_ELEMENT_PLAYER_STATS_DR_ICON);
}

void CG_SHUDElementPlayerStatsRoutine(void* context)
{
	shudElementPlayerStats_t* element = (shudElementPlayerStats_t*)context;

	int dmgGiven = statsInfo[OSP_STATS_DMG_GIVEN];
	int dmgReceived = statsInfo[OSP_STATS_DMG_RCVD];
	char buffer[64];
	static int lastRequestTime = 0;
	static int lastClientNum = -1;
	int currentClientNum = cg.snap ? cg.snap->ps.clientNum : -1;
	qboolean clientChanged = (currentClientNum != lastClientNum);

	if (CG_OSPIsStatsHidden(qtrue, qtrue))
		return;
	// Update the stats if the player has taken damage or given damage
	if ((cg.time - cg.damageTime <= 10 || cg.time - cgs.osp.lastHitTime <= 100 || clientChanged))
	{
		CG_SHUDRequestStatsInfo();
		lastClientNum = currentClientNum;
	}

	switch (element->type)
	{
		case SHUD_ELEMENT_PLAYER_STATS_DG:
			if (dmgGiven <= 0 && !(element->config.visflags.isSet && (element->config.visflags.value & SE_SHOW_EMPTY))) return;
			Com_sprintf(buffer, sizeof(buffer), "^7%d", dmgGiven);
			element->textCtx.text = buffer;
			CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
			CG_SHUDTextPrint(&element->config, &element->textCtx);
			return;

		case SHUD_ELEMENT_PLAYER_STATS_DR:
			if (dmgGiven <= 0 && !(element->config.visflags.isSet && (element->config.visflags.value & SE_SHOW_EMPTY))) return;
			Com_sprintf(buffer, sizeof(buffer), "^7%d", dmgReceived);
			element->textCtx.text = buffer;
			CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
			CG_SHUDTextPrint(&element->config, &element->textCtx);
			return;

		case SHUD_ELEMENT_PLAYER_STATS_DG_ICON:
			if (dmgGiven <= 0 && !(element->config.visflags.isSet && (element->config.visflags.value & SE_SHOW_EMPTY))) return;
			element->drawCtx.image = cgs.media.arrowUp;
			CG_SHUDDrawBorder(&element->config);
			CG_SHUDFill(&element->config);
			CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
			return;

		case SHUD_ELEMENT_PLAYER_STATS_DR_ICON:
			if (dmgGiven <= 0 && !(element->config.visflags.isSet && (element->config.visflags.value & SE_SHOW_EMPTY))) return;
			element->drawCtx.image = cgs.media.arrowDown;
			CG_SHUDDrawBorder(&element->config);
			CG_SHUDFill(&element->config);
			CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
			return;

		default:
			return;
	}
}

void CG_SHUDElementPlayerStatsDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
