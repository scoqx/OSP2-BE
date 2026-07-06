#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

enum shudPWType
{
	SHUDPWTYPE_TIME,
	SHUDPWTYPE_ICON,
};

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t textCtx;
	superhudDrawContext_t drawCtx;
	enum shudPWType pwType;
	int pwIndex;
} shudElementPowerupContext;

static void* CG_SHUDElementPwCreate(const superhudConfig_t* config, enum shudPWType pwType, int pwIndex)
{
	shudElementPowerupContext* element;

	SHUD_ELEMENT_INIT(element, config);

	element->pwType = pwType;
	element->pwIndex = pwIndex;

	if (pwType == SHUDPWTYPE_TIME)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
		CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
	}
	else
	{
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
	}

	return element;
}

void* CG_SHUDElementPwTime1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 1);
}

void* CG_SHUDElementPwTime2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 2);
}

void* CG_SHUDElementPwTime3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 3);
}

void* CG_SHUDElementPwTime4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 4);
}

void* CG_SHUDElementPwTime5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 5);
}

void* CG_SHUDElementPwTime6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 6);
}

void* CG_SHUDElementPwTime7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 7);
}

void* CG_SHUDElementPwTime8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_TIME, 8);
}

void* CG_SHUDElementPwIcon1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 1);
}

void* CG_SHUDElementPwIcon2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 2);
}

void* CG_SHUDElementPwIcon3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 3);
}

void* CG_SHUDElementPwIcon4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 4);
}

void* CG_SHUDElementPwIcon5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 5);
}

void* CG_SHUDElementPwIcon6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 6);
}

void* CG_SHUDElementPwIcon7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 7);
}

void* CG_SHUDElementPwIcon8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementPwCreate(config, SHUDPWTYPE_ICON, 8);
}

static void CG_SHUDElementPwUpdateState(struct superhudPowerupsCache_t* pw)
{
	int     i, j, k;
	playerState_t*   ps;
	int     t;
	gitem_t* item;

	ps = &cg.snap->ps;

	if (ps->stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// sort the list by time remaining
	pw->numberOfActive = 0;
	for (i = 0 ; i < MAX_POWERUPS ; i++)
	{
		if (!ps->powerups[ i ])
		{
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if (t < 0 || t > 999000)
		{
			continue;
		}

		// insert with sort by time
		for (j = 0 ; j < pw->numberOfActive ; j++)
		{
			if (pw->element[j].time >= t)
			{
				for (k = pw->numberOfActive - 1 ; k >= j ; k--)
				{
					pw->element[k + 1].powerup = pw->element[k].powerup;
					pw->element[k + 1].time = pw->element[k].time;
					pw->element[k + 1].isHoldable = pw->element[k].isHoldable;
					pw->element[k + 1].dyeTintItemNum = pw->element[k].dyeTintItemNum;
				}
				break;
			}
		}
		pw->element[j].powerup = i;
		pw->element[j].time = t;
		pw->element[j].isHoldable = qfalse;
		pw->element[j].dyeTintItemNum = 0;
		++pw->numberOfActive;
	}

	{
		int hi = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
		if (hi)
		{
			pw->element[pw->numberOfActive].powerup = hi;
			pw->element[pw->numberOfActive].time = SUPERHUD_UPDATE_TIME * 2;
			pw->element[pw->numberOfActive].isHoldable = qtrue;
			pw->element[pw->numberOfActive].dyeTintItemNum = 0;
			++pw->numberOfActive;
		}
	}

	for (i = 0 ; i < pw->numberOfActive ; i++)
	{
		// need to fix
		if (pw->element[i].isHoldable)
		{
			int hi;

			hi = pw->element[i].powerup;
			CG_RegisterItemVisuals(hi);
			pw->element[i].powerup = cg_items[hi].icon;
			pw->element[i].dyeTintItemNum = hi;
			pw->element[i].time /= 1000;
		}
		else
		{
			switch (pw->element[i].powerup)
			{
				case PW_QUAD:
					item = BG_FindItemForPowerup(PW_QUAD);
					if (item)
					{
						int in = (int)(item - bg_itemlist);
						CG_RegisterItemVisuals(in);
						pw->element[i].powerup = cg_items[in].icon;
						pw->element[i].dyeTintItemNum = in;
					}
					else
					{
						pw->element[i].powerup = cgs.media.quadDamageIcon;
						pw->element[i].dyeTintItemNum = 0;
					}
					break;
				case PW_BATTLESUIT:
					item = BG_FindItemForPowerup(PW_BATTLESUIT);
					if (item)
					{
						int in = (int)(item - bg_itemlist);
						CG_RegisterItemVisuals(in);
						pw->element[i].powerup = cg_items[in].icon;
						pw->element[i].dyeTintItemNum = in;
					}
					else
					{
						pw->element[i].powerup = cgs.media.battleSuitIcon;
						pw->element[i].dyeTintItemNum = 0;
					}
					break;
				case PW_HASTE:
					pw->element[i].powerup = cgs.media.hasteIcon;
					pw->element[i].dyeTintItemNum = 0;
					break;
				case PW_INVIS:
					pw->element[i].powerup = cgs.media.invisIcon;
					pw->element[i].dyeTintItemNum = 0;
					break;
				case PW_REGEN:
					pw->element[i].powerup = cgs.media.regenIcon;
					pw->element[i].dyeTintItemNum = 0;
					break;
				case PW_FLIGHT:
					pw->element[i].powerup = cgs.media.flightIcon;
					pw->element[i].dyeTintItemNum = 0;
					break;
				case PW_REDFLAG:
				case PW_BLUEFLAG:
					item = BG_FindItemForPowerup(pw->element[i].powerup);
					if (item)
					{
						int in = (int)(item - bg_itemlist);
						CG_RegisterItemVisuals(in);
						pw->element[i].powerup = cg_items[in].icon;
						pw->element[i].dyeTintItemNum = in;
					}
					break;
				default:
					pw->element[i].powerup = 0;
					pw->element[i].dyeTintItemNum = 0;
					break;
			}
			pw->element[i].time = pw->element[i].time / 1000;
		}
	}
}

void CG_SHUDElementPwRoutine(void* context)
{
	shudElementPowerupContext* element = (shudElementPowerupContext*)context;
	struct superhudPowerupsCache_t* pw = &superhudGlobalContext.powerupsCache;
	struct superhudPowerupElement_t* pwElement;

	if (cg.time - pw->lastUpdateTime > SUPERHUD_UPDATE_TIME)
	{
		CG_SHUDElementPwUpdateState(pw);
		pw->lastUpdateTime = cg.time;
	}

	if (!pw->numberOfActive || element->pwIndex > pw->numberOfActive)
	{
		//nothing to do
		return;
	}

	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);

	pwElement = &pw->element[element->pwIndex - 1];

	if (element->pwType == SHUDPWTYPE_TIME)
	{
		if (!pwElement->isHoldable)
		{
			element->textCtx.text = va("%d", pwElement->time);
			CG_SHUDTextPrint(&element->config, &element->textCtx);
		}
	}
	else
	{
		vec4_t col;

		element->drawCtx.image = CG_DyeFlagResolveInverseIcon(pwElement->dyeTintItemNum, pwElement->powerup);
		Vector4Copy(element->drawCtx.color, col);
		CG_SHUDConfigPickColor_Runtime(&element->config, col);
		if (pwElement->dyeTintItemNum >= 1 && pwElement->dyeTintItemNum < bg_numItems &&
		        (cg_items[pwElement->dyeTintItemNum].customVisualMask & ITEMCV_ICON))
		{
			int pwOwnerClient;

			pwOwnerClient = cg.snap->ps.clientNum;
			CG_ModulateVec4ForCustomItemDyeIconForClient(pwElement->dyeTintItemNum, pwOwnerClient, col);
		}
		CG_SHUDDrawStretchPic(element->drawCtx.coord, element->drawCtx.coordPicture, col, element->drawCtx.image);
	}
}

void CG_SHUDElementPwDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
