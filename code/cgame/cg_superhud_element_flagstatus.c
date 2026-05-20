#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

enum flagType_t
{
	SHUDFLTYPE_OWN,
	SHUDFLTYPE_NME,
	SHUDFLTYPE_CARRY,
} flagType;

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t ctx;
	enum flagType_t flagType;
	qhandle_t carryCustomImage;
} shudElementFlagStatus_t;

static void* CG_SHUDElementFlagStatusCreate(const superhudConfig_t* config, enum flagType_t flagType)
{
	shudElementFlagStatus_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDDrawMakeContext(&element->config, &element->ctx);
	Vector4Copy(colorWhite, element->ctx.color);

	element->flagType = flagType;
	element->carryCustomImage = 0;

	if (flagType == SHUDFLTYPE_CARRY && config->image.isSet)
	{
		element->carryCustomImage = trap_R_RegisterShader(config->image.value);
	}

	return element;
}

void* CG_SHUDElementFlagStatusNMECreate(const superhudConfig_t* config)
{
	return CG_SHUDElementFlagStatusCreate(config, SHUDFLTYPE_NME);
}

void* CG_SHUDElementFlagStatusOWNCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementFlagStatusCreate(config, SHUDFLTYPE_OWN);
}

void* CG_SHUDElementFlagStatusCarryCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementFlagStatusCreate(config, SHUDFLTYPE_CARRY);
}

void CG_SHUDElementFlagStatusRoutine(void* context)
{
	shudElementFlagStatus_t* element = (shudElementFlagStatus_t*)context;
	team_t team;
	gitem_t* item;
	vec4_t tint;

	if (element->flagType == SHUDFLTYPE_CARRY)
	{
		if (cg.predictedPlayerState.powerups[PW_REDFLAG])
		{
			team = TEAM_RED;
		}
		else if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
		{
			team = TEAM_BLUE;
		}
		else if (cg.predictedPlayerState.powerups[PW_NEUTRALFLAG])
		{
			team = TEAM_FREE;
		}
		else
		{
			return;
		}

		if (element->carryCustomImage)
		{
			CG_SHUDFill(&element->config);
			CG_SHUDDrawBorder(&element->config);
			CG_SHUDDrawStretchPic(element->ctx.coord, element->ctx.coordPicture, element->ctx.color, element->carryCustomImage);
			return;
		}
	}
	else
	{
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}

	if (element->flagType == SHUDFLTYPE_NME)
	{
		switch (team)
		{
			case TEAM_RED:
				team = TEAM_BLUE;
				break;
			case TEAM_BLUE:
				team = TEAM_RED;
				break;
			case TEAM_FREE:
			case TEAM_SPECTATOR:
			case TEAM_4:
			case TEAM_5:
			case TEAM_6:
			case TEAM_7:
			case TEAM_NUM_TEAMS:
				return;
		}
	}

	if (team == TEAM_RED)
	{
		item = BG_FindItemForPowerup(PW_REDFLAG);
		element->ctx.image = cgs.media.redFlagShader[element->flagType == SHUDFLTYPE_CARRY ? 0 : cgs.redflag];
	}
	else if (team == TEAM_BLUE)
	{
		item = BG_FindItemForPowerup(PW_BLUEFLAG);
		element->ctx.image = cgs.media.blueFlagShader[element->flagType == SHUDFLTYPE_CARRY ? 0 : cgs.blueflag];
	}
	else if (team == TEAM_FREE)
	{
		item = BG_FindItemForPowerup(PW_NEUTRALFLAG);
		element->ctx.image = item ? cg_items[ITEM_INDEX(item)].icon : 0;
	}
	else
	{
		return;
	}

	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);

	if (item)
	{
		qhandle_t pic;

		pic = CG_DyeFlagResolveInverseIcon(ITEM_INDEX(item), element->ctx.image);
		if (CG_GetDyeFlagTintVec4(team, tint))
		{
			if (CG_ItemDyeFlagInverseHudPath(item))
			{
				Vector4Set(tint, 1.0f, 1.0f, 1.0f, element->ctx.color[3]);
			}
			else
			{
				tint[3] = element->ctx.color[3];
			}
			CG_SHUDDrawStretchPic(element->ctx.coord, element->ctx.coordPicture, tint, pic);
		}
		else
		{
			CG_SHUDDrawStretchPic(element->ctx.coord, element->ctx.coordPicture, element->ctx.color, pic);
		}
	}
}

void CG_SHUDElementFlagStatusDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

