#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef enum
{
	SHUD_ELEMENT_SCORE_OWN,
	SHUD_ELEMENT_SCORE_NME,
	SHUD_ELEMENT_SCORE_MAX,
} shudElementScoreType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t ctx;
	superhudDrawContext_t drawCtx;
	shudElementScoreType_t type;
	qboolean box; // image set: bg/border/image drawn over the whole rect
} shudElementScore;

static void* CG_SHUDElementScoreCreate(const superhudConfig_t* config, shudElementScoreType_t type)
{
	shudElementScore* element;

	SHUD_ELEMENT_INIT(element, config);

	//load defaults
	if (!element->config.color.isSet)
	{
		element->config.color.isSet = qtrue;
		element->config.color.value.type = SUPERHUD_COLOR_RGBA;
		Vector4Set(element->config.color.value.rgba, 1, 0.7, 0, 1);
	}

	if (!element->config.text.isSet)
	{
		element->config.text.isSet = qtrue;
		Q_strncpyz(element->config.text.value, "%i", sizeof(element->config.text.value));
	}

	CG_SHUDTextMakeContext(&element->config, &element->ctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

	element->type = type;

	if (type != SHUD_ELEMENT_SCORE_MAX && element->config.image.isSet && element->config.rect.isSet)
	{
		CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);
		element->drawCtx.image = trap_R_RegisterShader(element->config.image.value);
		if (!element->drawCtx.image)
		{
			CG_Printf("^1Score image %s is not found\n", element->config.image.value);
		}
		element->box = qtrue;
		// frame and background belong to the rect, not to the text
		Vector4Set(element->ctx.background, 0, 0, 0, 0);
		Vector4Set(element->ctx.border, 0, 0, 0, 0);
		Vector4Set(element->ctx.borderColor, 0, 0, 0, 0);
	}

	return element;
}

void* CG_SHUDElementScoreOWNCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementScoreCreate(config, SHUD_ELEMENT_SCORE_OWN);
}

void* CG_SHUDElementScoreNMECreate(const superhudConfig_t* config)
{
	return CG_SHUDElementScoreCreate(config, SHUD_ELEMENT_SCORE_NME);
}

void* CG_SHUDElementScoreMAXCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementScoreCreate(config, SHUD_ELEMENT_SCORE_MAX);
}

static qboolean CG_SHUDScoresGetMax(int* scores)
{
	if (cgs.gametype == GT_CTF)
	{
		*scores = cgs.capturelimit;
	}
	else
	{
		*scores = cgs.fraglimit;
	}

	return qtrue;
}

static qboolean CG_SHUDScoresGetOWN(int* scores)
{
	team_t team = CG_SHUDGetOurActiveTeam();

	switch (team)
	{
		case TEAM_FREE:
		{
			*scores = cgs.scores1;
			return *scores != SCORE_NOT_PRESENT;
		}
		case TEAM_SPECTATOR:
		case TEAM_RED:
			*scores = cgs.scores1;
			return *scores != SCORE_NOT_PRESENT;
		case TEAM_BLUE:
			*scores = cgs.scores2;
			return *scores != SCORE_NOT_PRESENT;
		case TEAM_4:
		case TEAM_5:
		case TEAM_6:
		case TEAM_7:
		case TEAM_NUM_TEAMS:
			break;
	}
	return qfalse;
}

static qboolean CG_SHUDScoresGetNME(int* scores)
{
	team_t team = CG_SHUDGetOurActiveTeam();

	switch (team)
	{
		case TEAM_FREE:
		{
			int playerScore = cg.snap->ps.persistant[PERS_SCORE];

			if (cgs.scores1 != playerScore)
			{
				cgs.scores2 = playerScore;
			}

			*scores = cgs.scores2;
			return *scores != SCORE_NOT_PRESENT;
		}
		case TEAM_SPECTATOR:
		case TEAM_RED:
			*scores = cgs.scores2;
			return *scores != SCORE_NOT_PRESENT;
		case TEAM_BLUE:
			*scores = cgs.scores1;
			return *scores != SCORE_NOT_PRESENT;
		case TEAM_4:
		case TEAM_5:
		case TEAM_6:
		case TEAM_7:
		case TEAM_NUM_TEAMS:
			break;
	}
	return qfalse;
}

/* qtrue when the cell shows our own score (team: OWN; FFA: the cell holding our score) */
static qboolean CG_SHUDScoresIsMine(shudElementScoreType_t type)
{
	team_t team = CG_SHUDGetOurActiveTeam();
	int playerScore;

	/* non-team gametypes: decide by the viewed player's score even when
	   our team reads as SPECTATOR (free spectator, spectPOV 0) */
	if (cgs.gametype >= GT_TEAM && team != TEAM_FREE)
		return type == SHUD_ELEMENT_SCORE_OWN;

	/* free-flying spectator: no cell is ours */
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
		return qfalse;

	playerScore = cg.snap->ps.persistant[PERS_SCORE];

	if (type == SHUD_ELEMENT_SCORE_OWN)
		return playerScore == cgs.scores1;
	if (type == SHUD_ELEMENT_SCORE_NME)
		return playerScore != cgs.scores1;
	return qfalse;
}

/* bg, image, border over the rect; colorX2 - mine, colorX - not mine (same as text color2) */
static void CG_SHUDElementScoreDrawBox(shudElementScore* element, qboolean mine)
{
	const superhudConfig_t* cfg = &element->config;
	vec4_t color;

	if (cfg->fill.isSet && cfg->bgcolor.isSet)
	{
		if (mine && cfg->bgcolor2.isSet)
			CG_SHUDConfigPickColorValue(&cfg->bgcolor2.value, color);
		else
			CG_SHUDConfigPickColorValue(&cfg->bgcolor.value, color);

		if (color[3] > 0.0f)
			CG_SHUDFillWithColor(&element->drawCtx.coord, color);
	}

	if (element->drawCtx.image)
	{
		if (mine && cfg->color2.isSet)
			CG_SHUDConfigPickColorValue(&cfg->color2.value, color);
		else
			CG_SHUDConfigPickColorValue(&cfg->color.value, color);

		if (color[3] > 0.0f)
			CG_SHUDDrawStretchPic(element->drawCtx.coord, element->drawCtx.coordPicture, color, element->drawCtx.image);
	}

	if (cfg->border.isSet && (cfg->borderColor.isSet || cfg->borderColor2.isSet))
	{
		if (mine && cfg->borderColor2.isSet)
			CG_SHUDConfigPickColorValue(&cfg->borderColor2.value, color);
		else if (cfg->borderColor.isSet)
			CG_SHUDConfigPickColorValue(&cfg->borderColor.value, color);
		else
			return;

		if (color[3] > 0.0f)
		{
			float x = element->drawCtx.coord.named.x;
			float y = element->drawCtx.coord.named.y;
			float w = element->drawCtx.coord.named.w;
			float h = element->drawCtx.coord.named.h;
			CG_AdjustFrom640(&x, &y, &w, &h);
			CG_OSPDrawFrame(x, y, w, h, (float*)cfg->border.value, color, qfalse);
		}
	}
}

void CG_SHUDElementScoreRoutine(void* context)
{
	shudElementScore* element = (shudElementScore*)context;
	int scores;
	qboolean result = qfalse;
	switch (element->type)
	{
		case SHUD_ELEMENT_SCORE_OWN:
			result = CG_SHUDScoresGetOWN(&scores);
			break;

		case SHUD_ELEMENT_SCORE_NME:
			result = CG_SHUDScoresGetNME(&scores);

			if (!result && SHUD_CHECK_SHOW_EMPTY(element) &&
			        CG_SHUDGetOurActiveTeam() == TEAM_FREE)
			{
				scores = 0;
				result = qtrue;
			}
			break;

		case SHUD_ELEMENT_SCORE_MAX:
			result = CG_SHUDScoresGetMax(&scores);
			break;
	}

	if (!result)
		return;

	if (element->box)
	{
		CG_SHUDElementScoreDrawBox(element, CG_SHUDScoresIsMine(element->type));
	}

	element->ctx.text = CG_SHUDVaTextWithInt(&element->config, scores);

	if (element->config.color2.isSet && CG_SHUDScoresIsMine(element->type))
	{
		Vector4Copy(element->config.color2.value.rgba, element->ctx.color);
		CG_SHUDTextPrintNew(&element->config, &element->ctx, qfalse);
	}
	else
	{
		CG_SHUDTextPrint(&element->config, &element->ctx);
	}
}


void CG_SHUDElementScoreDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
