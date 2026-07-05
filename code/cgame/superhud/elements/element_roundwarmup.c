#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

/* style bitmask: bit0 = compact time (%.1f, no " sec"); bit1 = time color via ^1/^2/^3/^8 prefix */
#define SHUD_ROUNDWARMUP_STYLE_COMPACT     (1 << 0)
#define SHUD_ROUNDWARMUP_STYLE_TIME_COLOR  (1 << 1)
#define SHUD_ROUNDWARMUP_FIGHT_DEFAULT_MS  500

typedef enum
{
	SHUD_ROUNDWARMUP_TEXT,
	SHUD_ROUNDWARMUP_TIME,
	SHUD_ROUNDWARMUP_FIGHT,
} shudRoundWarmupType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	shudRoundWarmupType_t type;
	int lastRoundStartState;
	int fightStartTime;
	qboolean useImage;
} shudElementRoundWarmup_t;

static void* CG_SHUDElementRoundWarmupCreate(const superhudConfig_t* config, shudRoundWarmupType_t type)
{
	shudElementRoundWarmup_t* element;

	element = (shudElementRoundWarmup_t*)Z_Malloc(sizeof(*element));
	OSP_MEMORY_CHECK(element);
	memset(element, 0, sizeof(*element));

	memcpy(&element->config, config, sizeof(element->config));
	element->type = type;
	element->lastRoundStartState = cgs.roundStartState;
	element->fightStartTime = 0;

	if (type == SHUD_ROUNDWARMUP_TEXT || type == SHUD_ROUNDWARMUP_FIGHT)
	{
		if (config->image.isSet)
		{
			element->useImage = qtrue;
			CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
			element->drawCtx.image = trap_R_RegisterShader(element->config.image.value);
			if (!element->drawCtx.image)
			{
				CG_Printf("^1RoundWarmup image %s is not found\n", element->config.image.value);
			}
		}
	}

	if (type == SHUD_ROUNDWARMUP_TEXT)
	{
		if (!config->text.isSet && !element->useImage)
		{
			Q_strncpyz(element->config.text.value, "Round starts in:", sizeof(element->config.text.value));
		}
	}
	else if (type == SHUD_ROUNDWARMUP_TIME)
	{
		if (config->style.isSet && (config->style.value & SHUD_ROUNDWARMUP_STYLE_COMPACT))
		{
			Q_strncpyz(element->config.text.value, "%.1f", sizeof(element->config.text.value));
		}
		else
		{
			Q_strncpyz(element->config.text.value, "%.1f ^7sec", sizeof(element->config.text.value));
		}
	}
	else if (type == SHUD_ROUNDWARMUP_FIGHT)
	{
		if (!config->text.isSet && !element->useImage)
		{
			Q_strncpyz(element->config.text.value, "FIGHT!", sizeof(element->config.text.value));
		}
	}

	if (!element->useImage)
	{
		CG_SHUDTextMakeContext(&element->config, &element->ctx);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	}

	return element;
}

void* CG_SHUDElementRoundWarmupTextCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementRoundWarmupCreate(config, SHUD_ROUNDWARMUP_TEXT);
}

void* CG_SHUDElementRoundWarmupTimeCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementRoundWarmupCreate(config, SHUD_ROUNDWARMUP_TIME);
}

void* CG_SHUDElementRoundWarmupFightCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementRoundWarmupCreate(config, SHUD_ROUNDWARMUP_FIGHT);
}

static int CG_SHUDElementRoundWarmupFightDurationMs(const shudElementRoundWarmup_t* element)
{
	if (element->config.time.isSet)
	{
		return element->config.time.value;
	}
	return SHUD_ROUNDWARMUP_FIGHT_DEFAULT_MS;
}

static qboolean CG_SHUDElementRoundWarmupFightActive(const shudElementRoundWarmup_t* element)
{
	int duration;

	if (cgs.roundStartState != 1 || !element->fightStartTime)
	{
		return qfalse;
	}
	duration = CG_SHUDElementRoundWarmupFightDurationMs(element);
	return (cg.time - element->fightStartTime <= duration) ? qtrue : qfalse;
}

static qboolean CG_SHUDElementRoundWarmupVisible(shudElementRoundWarmup_t* element)
{
	if (element->type == SHUD_ROUNDWARMUP_TEXT)
	{
		if (cgs.roundStartState == 2)
		{
			int time = cg.snap->ps.powerups[PW_BATTLESUIT];
			if (time > cg.time)
			{
				return qtrue;
			}
		}
	}
	else if (element->type == SHUD_ROUNDWARMUP_TIME)
	{
		if (cgs.roundStartState == 2)
		{
			int time = cg.snap->ps.powerups[PW_BATTLESUIT];
			if (time > cg.time)
			{
				return qtrue;
			}
		}
	}
	else if (element->type == SHUD_ROUNDWARMUP_FIGHT)
	{
		if (CG_SHUDElementRoundWarmupFightActive(element))
		{
			return qtrue;
		}
	}

	return qfalse;
}

void CG_SHUDElementRoundWarmupRoutine(void* context)
{
	shudElementRoundWarmup_t* element = (shudElementRoundWarmup_t*)context;
	char text[256];
	char tmp[256];
	qboolean visible;

	element->ctx.text = NULL;

	if (element->lastRoundStartState != cgs.roundStartState)
	{
		if (element->lastRoundStartState == 2 && cgs.roundStartState == 1)
		{
			element->fightStartTime = cg.time;
		}
		else if (cgs.roundStartState != 1)
		{
			element->fightStartTime = 0;
		}
		element->lastRoundStartState = cgs.roundStartState;
	}

	if (element->type == SHUD_ROUNDWARMUP_TEXT)
	{
		if (cgs.roundStartState == 2)
		{
			int time = cg.snap->ps.powerups[PW_BATTLESUIT];
			if (time > cg.time)
			{
				if (!element->useImage)
				{
					element->ctx.text = element->config.text.value;
				}
			}
		}
	}
	else if (element->type == SHUD_ROUNDWARMUP_TIME)
	{
		if (cgs.roundStartState == 2)
		{
			int time = cg.snap->ps.powerups[PW_BATTLESUIT];
			if (time > cg.time)
			{
				float seconds = (time - cg.time) * 0.001f;
				const char* timeColor;

				Com_sprintf(tmp, sizeof(tmp), element->config.text.value, seconds);

				if (element->config.style.isSet && (element->config.style.value & SHUD_ROUNDWARMUP_STYLE_TIME_COLOR))
				{
					/* OSP/Q3 color codes in the string (per-draw vec4 tint does not replace inline ^n) */
					if (seconds <= 1.0f)
					{
						timeColor = "^2";
					}
					else if (seconds <= 2.0f)
					{
						timeColor = "^3";
					}
					else if (seconds <= 3.0f)
					{
						timeColor = "^8";
					}
					else
					{
						timeColor = "^1";
					}
					Com_sprintf(text, sizeof(text), "%s%s", timeColor, tmp);
				}
				else
				{
					Q_strncpyz(text, tmp, sizeof(text));
				}
				element->ctx.text = text;
			}
		}
	}
	else if (element->type == SHUD_ROUNDWARMUP_FIGHT)
	{
		if (CG_SHUDElementRoundWarmupFightActive(element))
		{
			if (!element->useImage)
			{
				element->ctx.text = element->config.text.value;
			}
		}
	}

	visible = CG_SHUDElementRoundWarmupVisible(element);
	if (!visible)
	{
		return;
	}

	if (element->useImage && element->drawCtx.image)
	{
		CG_SHUDFill(&element->config);
		CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		CG_SHUDDrawBorder(&element->config);
		return;
	}

	if (!element->ctx.text)
	{
		return;
	}

	CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementRoundWarmupDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
