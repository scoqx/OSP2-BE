#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

/* style bitmask: 1 = ally-only (hide auto-thaw), 2 = overlay doublebar */
#define SHUD_THAW_STYLE_ALLY_ONLY  1
#define SHUD_THAW_STYLE_OVERLAY    2

/* Catch-up rate (~1/e in ~1/rate seconds). No exp() in QVM — clamp linear factor. */
#define SHUD_THAW_SMOOTH_RATE  12.0f
#define SHUD_THAW_SMOOTH_STALE_MS  50

typedef enum
{
	SHUD_THAW_OWN,       /* own thaw progress % */
	SHUD_THAW_FREEZING,  /* thawing-other progress % */
} shudThawType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	shudThawType_t type;
	qboolean useImage;
} shudElementThaw_t;

typedef struct
{
	superhudConfig_t config;
	superhudBarContext_t ctx;
	shudThawType_t type;
} shudElementThawBar_t;

/* Shared display for text + bar of the same type (update once per cg.time). */
static float s_thawDisplay[2];
static qboolean s_thawHad[2];
static int s_thawSmoothTime[2];

/*
 * thaw_own: server packs OWN only for frozen players (alive always 0).
 * Chase-cam ps is the target's — detect local ice via PM_FREEZE / isFrozenEnt /
 * PMF_FOLLOW (team player), since OWN can be 0% at freeze start.
 */
static qboolean CG_SHUDThawOwnVisible(int v)
{
	if (!BE_CUSTOM_ACTIVE(v))
	{
		return qfalse;
	}
	if (cg.snap->ps.pm_type == PM_FREEZE)
	{
		return qtrue;
	}
	if (cgs.clientinfo[cg.clientNum].isFrozenEnt)
	{
		return qtrue;
	}
	/* Frozen teammate in chase cam: STAT keeps our OWN (+ watched FREEZING). */
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW)
			&& cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_SHUDThawOwnAllyGate(const superhudConfig_t* config, int v)
{
	if (config->style.isSet
			&& (config->style.value & SHUD_THAW_STYLE_ALLY_ONLY)
			&& !BE_CUSTOM_ALLY_THAWING(v))
	{
		return qfalse;
	}
	return qtrue;
}

/*
 * Smooth server int pct toward a shared float. First caller this frame updates;
 * later callers (text then bar) read the same value.
 */
static float CG_SHUDThawSmoothPct(shudThawType_t type, int targetPct)
{
	int idx;
	float target;
	float t;

	idx = (int)type;
	if (idx < 0 || idx > 1)
	{
		return (float)targetPct;
	}

	target = (float)targetPct;

	if (s_thawSmoothTime[idx] != cg.time)
	{
		/* Gap without draws → snap on next show */
		if (!s_thawHad[idx]
				|| (cg.time - s_thawSmoothTime[idx]) > SHUD_THAW_SMOOTH_STALE_MS
				|| target < s_thawDisplay[idx] - 0.5f)
		{
			s_thawDisplay[idx] = target;
		}
		else
		{
			t = SHUD_THAW_SMOOTH_RATE * (float)cg.frametime * 0.001f;
			if (t > 1.0f)
			{
				t = 1.0f;
			}
			if (t < 0.0f)
			{
				t = 0.0f;
			}
			s_thawDisplay[idx] += (target - s_thawDisplay[idx]) * t;
		}
		s_thawHad[idx] = qtrue;
		s_thawSmoothTime[idx] = cg.time;
	}

	return s_thawDisplay[idx];
}

static void* CG_SHUDElementThawCreate(const superhudConfig_t* config, shudThawType_t type)
{
	shudElementThaw_t* element;

	element = (shudElementThaw_t*)Z_Malloc(sizeof(*element));
	OSP_MEMORY_CHECK(element);
	memset(element, 0, sizeof(*element));

	memcpy(&element->config, config, sizeof(element->config));
	element->type = type;

	if (config->image.isSet)
	{
		element->useImage = qtrue;
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
		element->drawCtx.image = trap_R_RegisterShader(element->config.image.value);
		if (!element->drawCtx.image)
		{
			CG_Printf("^1Thaw image %s is not found\n", element->config.image.value);
		}
	}

	if (!element->useImage)
	{
		CG_SHUDTextMakeContext(&element->config, &element->ctx);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx);
	}

	return element;
}

void* CG_SHUDElementThawOwnCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementThawCreate(config, SHUD_THAW_OWN);
}

void* CG_SHUDElementThawFreezingCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementThawCreate(config, SHUD_THAW_FREEZING);
}

static void* CG_SHUDElementThawBarCreate(const superhudConfig_t* config, shudThawType_t type)
{
	shudElementThawBar_t* element;

	SHUD_ELEMENT_INIT(element, config);
	element->type = type;
	CG_SHUDBarMakeContext(&element->config, &element->ctx, 100);

	return element;
}

void* CG_SHUDElementThawOwnBarCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementThawBarCreate(config, SHUD_THAW_OWN);
}

void* CG_SHUDElementThawFreezingBarCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementThawBarCreate(config, SHUD_THAW_FREEZING);
}

static void CG_SHUDElementThawDraw(shudElementThaw_t* element, int pct)
{
	/* Image: 1+ % only, no text (thaw_own + thaw_freezing) */
	if (element->useImage)
	{
		if (pct >= 1 && element->drawCtx.image)
		{
			CG_SHUDFill(&element->config);
			CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
			CG_SHUDDrawBorder(&element->config);
		}
		return;
	}

	/* Text: optional prefix via text "Thaw " → "Thaw 45%" */
	if (element->config.text.isSet)
	{
		element->ctx.text = va("%s%d%%", element->config.text.value, pct);
	}
	else
	{
		element->ctx.text = va("%d%%", pct);
	}

	CG_SHUDTextPrint(&element->config, &element->ctx);
}

static void CG_SHUDElementThawBarApplyColors(shudElementThawBar_t* element)
{
	if (element->config.color.isSet)
	{
		Vector4Copy(element->config.color.value.rgba, element->ctx.color_top);
	}

	if (!element->config.color2.isSet)
	{
		Vector4Copy(element->ctx.color_top, element->ctx.color2_top);
		element->ctx.color2_top[3] *= 3;
	}
	else
	{
		Vector4Copy(element->config.color2.value.rgba, element->ctx.color2_top);
	}
}

static int CG_SHUDThawTextPct(float displayPct, int targetPct)
{
	int textPct;

	textPct = (int)(displayPct + 0.5f);
	if (textPct > targetPct)
	{
		textPct = targetPct;
	}
	if (textPct < 0)
	{
		textPct = 0;
	}
	return textPct;
}

void CG_SHUDElementThawRoutine(void* context)
{
	shudElementThaw_t* element = (shudElementThaw_t*)context;
	int v;
	int targetPct;
	float displayPct;

	element->ctx.text = NULL;

	/* STAT_BE_CUSTOM thaw pack — only on OSP2-BE servers */
	if (!cgs.beSupportedServer)
	{
		return;
	}
	if (!CG_OSPIsGameTypeFreeze() || !cg.snap || cg.intermissionStarted)
	{
		return;
	}

	v = cg.snap->ps.stats[STAT_BE_CUSTOM];

	if (element->type == SHUD_THAW_OWN)
	{
		if (!CG_SHUDThawOwnVisible(v))
		{
			return;
		}
		if (!CG_SHUDThawOwnAllyGate(&element->config, v))
		{
			return;
		}
		targetPct = BE_CUSTOM_OWN_THAW(v);
	}
	else
	{
		targetPct = BE_CUSTOM_THAWING(v);
		if (targetPct <= 0)
		{
			return;
		}
	}

	displayPct = CG_SHUDThawSmoothPct(element->type, targetPct);
	CG_SHUDElementThawDraw(element, CG_SHUDThawTextPct(displayPct, targetPct));
}

void CG_SHUDElementThawBarRoutine(void* context)
{
	shudElementThawBar_t* element = (shudElementThawBar_t*)context;
	int v;
	int targetPct;
	float displayPct;

	/* STAT_BE_CUSTOM thaw pack — only on OSP2-BE servers */
	if (!cgs.beSupportedServer)
	{
		return;
	}
	if (!CG_OSPIsGameTypeFreeze() || !cg.snap || cg.intermissionStarted)
	{
		return;
	}

	v = cg.snap->ps.stats[STAT_BE_CUSTOM];

	if (element->type == SHUD_THAW_OWN)
	{
		if (!CG_SHUDThawOwnVisible(v))
		{
			return;
		}
		if (!CG_SHUDThawOwnAllyGate(&element->config, v))
		{
			return;
		}
		targetPct = BE_CUSTOM_OWN_THAW(v);
	}
	else
	{
		targetPct = BE_CUSTOM_THAWING(v);
		if (targetPct <= 0 && !SHUD_CHECK_SHOW_EMPTY(element))
		{
			return;
		}
	}

	displayPct = CG_SHUDThawSmoothPct(element->type, targetPct);
	CG_SHUDElementThawBarApplyColors(element);
	CG_SHUDBarPrint(&element->config, &element->ctx, displayPct);
}

void CG_SHUDElementThawDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

void CG_SHUDElementThawBarDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
