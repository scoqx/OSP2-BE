#include "cg_local.h"
#include "cg_superhud_private.h"
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
	element->gctx = CG_SHUDGetContext();
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

	CG_FontSelect(element->ctxSender.fontIndex);
	senderWidth = CG_OSPDrawStringLenPix(entry->runtime.senderName, element->config.fontsize.value[0], element->ctxSender.flags, entry->runtime.maxNameLenPix);

	CG_FontSelect(element->ctxEventName.fontIndex);
	eventNameWidth = CG_OSPDrawStringLenPix(entry->runtime.eventName, element->config.fontsize.value[0], element->ctxEventName.flags, 0);

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
	int msec;
	int seconds;
	int mins;
	int tens;
	clientInfo_t* ci;

	entry->runtime.maxVisibleChars = 13;
	entry->runtime.spacing = element->config.rect.value[3] / 4.0f;
	entry->runtime.maxNameLenPix = (element->config.rect.value[2] - (element->config.rect.value[3] + entry->runtime.spacing * 2)) / 2;

	/* Calculate game time when task was sent */
	msec = entry->gameTime;
	if (msec < 0)
	{
		msec *= -1;
	}
	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;
	Com_sprintf(entry->runtime.timeString, sizeof(entry->runtime.timeString), "%i:%i%i", mins, tens, seconds);

	/* Get icon */
	entry->runtime.iconShader = CG_SHUDTeamTaskGetTaskIcon(entry->taskType, entry->isPickup);

	/* Get sender name */
	if (entry->sender >= 0 && entry->sender < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[entry->sender];
		if (ci->infoValid)
		{
			Q_strncpyz(entry->runtime.senderName, ci->name, MAX_QPATH);
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

	/* Set sender color - always white since we only display our own team */
	Vector4Copy(colorWhite, entry->runtime.senderColor);

	entry->runtime.isInitialized = qtrue;
}

void CG_SHUDElementTeamTasksRoutine(void* context)
{
	shudElementTeamTasks_t* element = (shudElementTeamTasks_t*)context;
	superhudTeamTasksEntry_t* entry;
	int index;
	float currentX;

	index = (element->gctx->teamTasks.index - element->index) % SHUD_MAX_TEAMTASKS_LINES;
	entry = &element->gctx->teamTasks.line[index];

	if (entry->time == 0 || !CG_SHUDGetFadeColor(element->ctxTime.color_origin, element->ctxTime.color, &element->config, entry->time))
	{
		entry->time = 0;
		return;
	}

	/* Ignore Dead messages */
	if (entry->taskType == TEAMTASK_PLAYERDEAD)
	{
		return;
	}

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
			/* Set up coord for background */
			bgCoord.named.x = bgX;
			bgCoord.named.y = bgY;
			bgCoord.named.w = bgW;
			bgCoord.named.h = bgH;

			/* Get background color with fade support */
			CG_SHUDConfigPickBgColor(&element->config, bgColor, qfalse);
			/* Apply fade to background color alpha (fade is already calculated in ctxTime.color) */
			bgColor[3] *= element->ctxTime.color[3];

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

			/* Use same coordinates as background */
			borderX = bgX;
			borderY = bgY;
			borderW = bgW;
			borderH = bgH;

			/* Adjust coordinates for 640x480 */
			CG_AdjustFrom640(&borderX, &borderY, &borderW, &borderH);

			/* Get border color with fade support */
			CG_SHUDConfigPickBorderColor(&element->config, borderColor, qfalse);
			/* Apply fade to border color alpha (fade is already calculated in ctxTime.color) */
			borderColor[3] *= element->ctxTime.color[3];

			/* Draw border (outer border) */
			CG_OSPDrawFrame(borderX, borderY, borderW, borderH,
			                (float*)element->config.border.value, borderColor, qfalse);
		}
	}

	/* Draw sender name */
	element->ctxSender.text = entry->runtime.senderName;
	element->ctxSender.coord.named.x = currentX;
	element->ctxSender.background[3] = 0;

	element->ctxSender.width = entry->runtime.maxNameLenPix;
	CG_SHUDTextPrintNew(&element->config, &element->ctxSender, qfalse);
	currentX += entry->runtime.senderWidth + entry->runtime.spacing;

	/* Draw icon */
	if (entry->runtime.iconShader)
	{
		element->ctxIcon.image = entry->runtime.iconShader;
		element->ctxIcon.coord.named.x = currentX;
		element->ctxIcon.coord.named.w = element->ctxIcon.coord.named.h * 0.75f;
		CG_SHUDDrawStretchPicCtx(&element->config, &element->ctxIcon);
		currentX += element->ctxIcon.coord.named.w + entry->runtime.spacing;
	}

	/* Draw event name */
	element->ctxEventName.text = entry->runtime.eventName;
	element->ctxEventName.coord.named.x = currentX;
	CG_SHUDTextPrintNew(&element->config, &element->ctxEventName, qfalse);
}


static qhandle_t CG_SHUDTeamTaskGetTaskIcon(int taskType, qboolean isPickup)
{
	qhandle_t shader;
	vec4_t color;

	CG_GetTeamTaskMarkerShaderAndColor(taskType, &shader, color, qfalse, -1, isPickup);
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
			case TEAMTASK_PATROL:
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
			case TEAMTASK_WARN_ALIVE:
				taskName = "Caution";
				break;
			case TEAMTASK_WARN_DEAD:
				taskName = "Caution (dead)";
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






