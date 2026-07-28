#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctxTime;
	superhudTextContext_t ctxSender;
	superhudDrawContext_t ctxIcon;
	superhudTextContext_t ctxEventName;
	superhudGlobalContext_t* gctx;
	int index;
} shudElementTeamTasks_t;

static qhandle_t CG_SHUDTeamTaskGetTaskIcon(int taskType, qboolean isPickup);
static void CG_SHUDTeamTaskGetTaskName(int taskType, char* name, int maxlen, qboolean isPickup);

static void* CG_SHUDElementTeamTasksCreate(const superhudConfig_t* config, int line)
{
	shudElementTeamTasks_t* element;
	superhudConfig_t lcfg;

	SHUD_ELEMENT_INIT(element, config);
	element->gctx = &superhudGlobalContext;
	element->index = line;

	memcpy(&lcfg, config, sizeof(superhudConfig_t));

	if (!lcfg.alignV.isSet)
	{
		lcfg.alignV.value = SUPERHUD_ALIGNV_CENTER;
		lcfg.alignV.isSet = qtrue;
	}

	CG_SHUDTextMakeContext(&lcfg, &element->ctxTime);
	CG_SHUDTextMakeContext(&lcfg, &element->ctxSender);
	CG_SHUDDrawMakeContext(&lcfg, &element->ctxIcon);
	CG_SHUDTextMakeContext(&lcfg, &element->ctxEventName);

	return element;
}

void* CG_SHUDElementTeamTask1Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 1);
}

void* CG_SHUDElementTeamTask2Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 2);
}

void* CG_SHUDElementTeamTask3Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 3);
}

void* CG_SHUDElementTeamTask4Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 4);
}

void* CG_SHUDElementTeamTask5Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 5);
}

void* CG_SHUDElementTeamTask6Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 6);
}

void* CG_SHUDElementTeamTask7Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 7);
}

void* CG_SHUDElementTeamTask8Create(const superhudConfig_t* config)
{
	return CG_SHUDElementTeamTasksCreate(config, 8);
}

static void CG_SHUDElementTeamTasksUpdatePosition(shudElementTeamTasks_t* element, superhudTeamTasksEntry_t* entry)
{
	float senderWidth;
	float eventNameWidth;

	senderWidth = CG_OSPDrawStringNewTextWidth(entry->runtime.senderName,
	              element->config.fontsize.value[0],
	              element->config.fontsize.value[1],
	              element->ctxSender.flags,
	              element->ctxSender.fontIndex);
	if (senderWidth > (float)entry->runtime.maxNameLenPix)
	{
		senderWidth = (float)entry->runtime.maxNameLenPix;
	}

	eventNameWidth = CG_OSPDrawStringNewTextWidth(entry->runtime.eventName,
	                 element->config.fontsize.value[0],
	                 element->config.fontsize.value[1],
	                 element->ctxEventName.flags,
	                 element->ctxEventName.fontIndex);

	entry->runtime.senderWidth = senderWidth;
	entry->runtime.eventNameWidth = eventNameWidth;

	if (element->config.alignH.value == SUPERHUD_ALIGNH_LEFT)
	{
		entry->runtime.baseX = element->config.rect.value[0];
	}
	else if (element->config.alignH.value == SUPERHUD_ALIGNH_RIGHT)
	{
		float totalWidth;
		totalWidth = entry->runtime.spacing * 2 + senderWidth + element->ctxIcon.coord.named.w + eventNameWidth;
		entry->runtime.baseX = element->config.rect.value[0] - totalWidth;
	}
	else /* SUPERHUD_ALIGNH_CENTER */
	{
		float totalWidth;
		totalWidth = entry->runtime.spacing * 2 + senderWidth + element->ctxIcon.coord.named.w + eventNameWidth;
		entry->runtime.baseX = element->config.rect.value[0] - (totalWidth / 2);
	}
}

static void CG_SHUDElementTeamTasksInitializeRuntime(shudElementTeamTasks_t* element, superhudTeamTasksEntry_t* entry)
{
	clientInfo_t* ci;

	entry->runtime.maxVisibleChars = 13;
	entry->runtime.spacing = element->config.rect.value[3] / 4.0f;
	entry->runtime.maxNameLenPix = (element->config.rect.value[2] - (element->config.rect.value[3] + entry->runtime.spacing * 2)) / 2;

	/* Calculate game time when task was sent */
	SHUD_WRITE_MATCH_TIME_STR(entry->runtime.timeString, sizeof(entry->runtime.timeString), entry->gameTime);

	/* Get icon */
	entry->runtime.iconShader = CG_SHUDTeamTaskGetTaskIcon(entry->taskType, entry->isPickup);

	/* Get sender name */
	if (entry->sender >= 0 && entry->sender < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[entry->sender];
		if (ci->infoValid)
		{
			if (element->config.style.isSet && element->config.style.value == 1)
			{
				Q_strncpyz(entry->runtime.senderName, ci->name_clean, MAX_QPATH);
			}
			else
			{
				Q_strncpyz(entry->runtime.senderName, ci->name, MAX_QPATH);
			}
		}
		else
		{
			entry->runtime.senderName[0] = '\0';
		}
	}
	else
	{
		entry->runtime.senderName[0] = '\0';
	}

	/* Get event name */
	CG_SHUDTeamTaskGetTaskName(entry->taskType, entry->runtime.eventName, sizeof(entry->runtime.eventName), entry->isPickup);
	if (entry->repeatCount > 1)
	{
		char eventName[MAX_QPATH];

		Q_strncpyz(eventName, entry->runtime.eventName, sizeof(eventName));
		Com_sprintf(entry->runtime.eventName, sizeof(entry->runtime.eventName), "%s x%i", eventName, entry->repeatCount);
	}

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		Vector4Copy(colorWhite, entry->runtime.senderColor);
	}

	entry->runtime.isInitialized = qtrue;
}

static void CG_SHUDTeamTasksClearHasActiveIfEmpty(superhudGlobalContext_t* ctx)
{
	int i;

	if (!ctx->teamTasks.hasActive)
	{
		return;
	}

	for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
	{
		if (ctx->teamTasks.line[i].time != 0 &&
		        ctx->teamTasks.line[i].taskType != TEAMTASK_PLAYERDEAD)
		{
			return;
		}
	}

	ctx->teamTasks.hasActive = qfalse;
}

/*
 * Map HUD line (TeamTask1 = 1, ...) to the Nth newest still-visible event.
 * Skips expired and PLAYERDEAD entries so lines pack toward the first slot.
 */
static superhudTeamTasksEntry_t* CG_SHUDElementTeamTasksFindEntry(shudElementTeamTasks_t* element)
{
	superhudGlobalContext_t* ctx;
	int i;
	int checkIndex;
	int visibleSlot;
	superhudTeamTasksEntry_t* entry;

	ctx = element->gctx;
	visibleSlot = 0;

	for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
	{
		checkIndex = (ctx->teamTasks.index - 1 - i + SHUD_MAX_TEAMTASKS_LINES) % SHUD_MAX_TEAMTASKS_LINES;
		entry = &ctx->teamTasks.line[checkIndex];

		if (entry->time == 0)
		{
			continue;
		}

		if (entry->taskType == TEAMTASK_PLAYERDEAD)
		{
			entry->time = 0;
			CG_SHUDTeamTasksClearHasActiveIfEmpty(ctx);
			continue;
		}

		if (!CG_SHUDGetFadeColor(element->ctxTime.color_origin, element->ctxTime.color, &element->config, entry->time))
		{
			entry->time = 0;
			CG_SHUDTeamTasksClearHasActiveIfEmpty(ctx);
			continue;
		}

		++visibleSlot;
		if (visibleSlot == element->index)
		{
			return entry;
		}
	}

	return NULL;
}

void CG_SHUDElementTeamTasksRoutine(void* context)
{
	shudElementTeamTasks_t* element = (shudElementTeamTasks_t*)context;
	superhudTeamTasksEntry_t* entry;
	float currentX;
	float fadeScale;

	if (!element->gctx->teamTasks.hasActive)
	{
		return;
	}

	entry = CG_SHUDElementTeamTasksFindEntry(element);
	if (!entry)
	{
		return;
	}

	fadeScale = CG_SHUDFadeAlphaScale(element->ctxTime.color_origin, element->ctxTime.color);

	element->ctxIcon.coord.named.w = element->config.rect.value[3];
	element->ctxIcon.coord.named.h = element->config.rect.value[3];

	if (!entry->runtime.isInitialized)
	{
		CG_SHUDElementTeamTasksInitializeRuntime(element, entry);
	}

	CG_SHUDElementTeamTasksUpdatePosition(element, entry);

	currentX = entry->runtime.baseX;

	/* Calculate total width of the line for background and border */
	{
		float totalWidth;
		float bgX;
		float bgY;
		float bgW;
		float bgH;
		superhudCoord_t bgCoord;
		vec4_t bgColor;
		vec4_t borderColor;

		/* Calculate total width of the line */
		totalWidth = entry->runtime.spacing * 2 + entry->runtime.senderWidth + element->ctxIcon.coord.named.w + entry->runtime.eventNameWidth;

		/* Calculate background/border position and size */
		bgX = entry->runtime.baseX;
		bgY = element->config.rect.value[1];
		bgW = totalWidth;
		bgH = element->config.rect.value[3];

		/* Draw background fill if fill and bgcolor are set */
		if (element->config.fill.isSet && element->config.bgcolor.isSet)
		{
			float fadeAlpha;

			/* Set up coord for background */
			bgCoord.named.x = bgX;
			bgCoord.named.y = bgY;
			bgCoord.named.w = bgW;
			bgCoord.named.h = bgH;

			fadeAlpha = fadeScale;
			CG_SHUDConfigPickBgColor(&element->config, bgColor, qfalse);
			if (fadeAlpha != 1.0f)
			{
				bgColor[3] *= fadeAlpha;
			}

			/* Draw background */
			CG_SHUDFillWithColor(&bgCoord, bgColor);
		}

		/* Draw border if border and borderColor are set */
		if (element->config.border.isSet && element->config.borderColor.isSet)
		{
			float borderX;
			float borderY;
			float borderW;
			float borderH;
			float fadeAlpha;

			/* Use same coordinates as background */
			borderX = bgX;
			borderY = bgY;
			borderW = bgW;
			borderH = bgH;

			/* Adjust coordinates for 640x480 */
			CG_AdjustFrom640(&borderX, &borderY, &borderW, &borderH);

			fadeAlpha = fadeScale;
			CG_SHUDConfigPickBorderColor(&element->config, borderColor, qfalse);
			if (fadeAlpha != 1.0f)
			{
				borderColor[3] *= fadeAlpha;
			}

			/* Draw border (outer border) */
			CG_OSPDrawFrame(borderX, borderY, borderW, borderH,
			                (float*)element->config.border.value, borderColor, qfalse);
		}
	}

	/* Draw sender name */
	element->ctxSender.text = entry->runtime.senderName;
	element->ctxSender.coord.named.x = currentX;
	element->ctxSender.background[3] = 0;

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		Vector4Copy(entry->runtime.senderColor, element->ctxSender.color);
	}
	else
	{
		Vector4Copy(element->ctxSender.color_origin, element->ctxSender.color);
		CG_SHUDConfigPickColor_Runtime(&element->config, element->ctxSender.color);
	}

	if (fadeScale != 1.0f)
	{
		element->ctxSender.color[3] *= fadeScale;
	}

	element->ctxSender.width = entry->runtime.maxNameLenPix;
	CG_SHUDTextPrintNew(&element->config, &element->ctxSender, qfalse);
	currentX += entry->runtime.senderWidth + entry->runtime.spacing;

	/* Draw icon */
	if (entry->runtime.iconShader)
	{
		element->ctxIcon.image = entry->runtime.iconShader;
		element->ctxIcon.coord.named.x = currentX;
		element->ctxIcon.coord.named.w = element->ctxIcon.coord.named.h * 0.75f;
		Vector4Copy(element->ctxIcon.color_origin, element->ctxIcon.color);
		CG_SHUDConfigPickColor_Runtime(&element->config, element->ctxIcon.color);
		{
			qhandle_t markShader;
			vec4_t markColor;

			CG_GetTeamTaskMarkerShaderAndColor(entry->taskType, &markShader, markColor, qfalse, entry->sender, entry->isPickup, NULL);
			/* Snowflake (frozenFoeTag): FROZEN_ALLY, FROZEN_ENEMY, HERE_DEAD in freeze mode */
			if (markShader == cgs.media.frozenFoeTagShader)
			{
				element->ctxIcon.color[0] = markColor[0];
				element->ctxIcon.color[1] = markColor[1];
				element->ctxIcon.color[2] = markColor[2];
			}
			else if (entry->taskType == TEAMTASK_YES || entry->taskType == TEAMTASK_NO ||
			         entry->taskType == TEAMTASK_MEDIC)
			{
				element->ctxIcon.color[0] = markColor[0];
				element->ctxIcon.color[1] = markColor[1];
				element->ctxIcon.color[2] = markColor[2];
			}
		}
		if (fadeScale != 1.0f)
		{
			element->ctxIcon.color[3] *= fadeScale;
		}
		CG_SHUDDrawStretchPic(element->ctxIcon.coord, element->ctxIcon.coordPicture, element->ctxIcon.color, element->ctxIcon.image);
		currentX += element->ctxIcon.coord.named.w + entry->runtime.spacing;
	}

	/* Draw event name */
	element->ctxEventName.text = entry->runtime.eventName;
	element->ctxEventName.coord.named.x = currentX;
	Vector4Copy(element->ctxTime.color, element->ctxEventName.color);
	CG_SHUDTextPrintNew(&element->config, &element->ctxEventName, qfalse);
}


static qhandle_t CG_SHUDTeamTaskGetTaskIcon(int taskType, qboolean isPickup)
{
	qhandle_t shader;
	vec4_t color;

	CG_GetTeamTaskMarkerShaderAndColor(taskType, &shader, color, qfalse, -1, isPickup, NULL);
	return shader;
}

static void CG_SHUDTeamTaskGetTaskName(int taskType, char* name, int maxlen, qboolean isPickup)
{
	const char* taskName;

	/* Handle pickup marks with different names */
	if (isPickup)
	{
		switch (taskType)
		{
			case TEAMTASK_REDARMOR:
				taskName = "Picked up Red Armor";
				break;
			case TEAMTASK_MEGAHEALTH:
				taskName = "Picked up Mega Health";
				break;
			case TEAMTASK_YELLOWARMOR:
				taskName = "Picked up Yellow Armor";
				break;
			case TEAMTASK_QUADDAMAGE:
				taskName = "Picked up Quad Damage";
				break;
			case TEAMTASK_REGENERATION:
				taskName = "Picked up Regeneration";
				break;
			case TEAMTASK_BATTLESUIT:
				taskName = "Picked up Battle Suit";
				break;
			case TEAMTASK_HASTE:
				taskName = "Picked up Haste";
				break;
			case TEAMTASK_HEALTH:
				taskName = "Picked up Health";
				break;
			default:
				taskName = "Picked up";
				break;
		}
	}
	else
	{
		switch (taskType)
		{
			case TEAMTASK_OFFENSE:
				taskName = "Attack";
				break;
			case TEAMTASK_DEFENCE:
				taskName = "Defence";
				break;
			case TEAMTASK_WATCH:
				taskName = "Watching";
				break;
			case TEAMTASK_PLAYERDEAD:
				taskName = "Dead";
				break;
			case TEAMTASK_YES:
				taskName = "Affirmative";
				break;
			case TEAMTASK_NO:
				taskName = "Negative";
				break;
			case TEAMTASK_LOCATION:
				taskName = "On my way";
				break;
			case TEAMTASK_TARGET:
				taskName = "Enemy here";
				break;
			case TEAMTASK_REDARMOR:
				taskName = "Red Armor up";
				break;
			case TEAMTASK_MEGAHEALTH:
				taskName = "Mega Health up";
				break;
			case TEAMTASK_YELLOWARMOR:
				taskName = "Yellow Armor up";
				break;
			case TEAMTASK_QUADDAMAGE:
				taskName = "Quad Damage up";
				break;
			case TEAMTASK_REGENERATION:
				taskName = "Regeneration up";
				break;
			case TEAMTASK_BATTLESUIT:
				taskName = "Battle Suit up";
				break;
			case TEAMTASK_HASTE:
				taskName = "Haste up";
				break;
			case TEAMTASK_HEALTH:
				taskName = "Health up";
				break;
			case TEAMTASK_MEDIC:
				taskName = "Need health";
				break;
			case TEAMTASK_FROZEN_ALLY:
				taskName = "Frozen ally";
				break;
			case TEAMTASK_FROZEN_ENEMY:
				taskName = "Frozen enemy";
				break;
			case TEAMTASK_HERE_ALIVE:
				taskName = "Here";
				break;
			case TEAMTASK_HERE_DEAD:
				taskName = "Here (dead)";
				break;
			case TEAMTASK_HERE_FLAG:
				taskName = "I carry flag";
				break;
			case TEAMTASK_WARN_ALIVE:
				taskName = "Caution";
				break;
			case TEAMTASK_WARN_DEAD:
				taskName = "Caution (dead)";
				break;
			case TEAMTASK_REDFLAG:
				taskName = "Red Flag";
				break;
			case TEAMTASK_BLUEFLAG:
				taskName = "Blue Flag";
				break;
			case TEAMTASK_REDFLAG_CARRIER:
				taskName = "Red Flag carrier";
				break;
			case TEAMTASK_BLUEFLAG_CARRIER:
				taskName = "Blue Flag carrier";
				break;
			default:
				taskName = "Unknown";
				break;
		}
	}

	Q_strncpyz(name, taskName, maxlen);
}

void CG_SHUDElementTeamTasksDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
