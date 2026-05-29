#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_MEDAL_ANIM_SLOTS         46
#define CG_MEDAL_TIMELINE_SLOTS     141
#define CG_MEDAL_LAST_FRAME_INDEX   47

static const char* cg_medalCfgPaths[CG_MEDAL_TYPE_COUNT] =
{
	"icons/medals/impressive.cfg",
	"icons/medals/excellent.cfg",
	"icons/medals/gauntlet.cfg",
	"icons/medals/defend.cfg",
	"icons/medals/assist.cfg",
	"icons/medals/capture.cfg"
};

static void CG_MedalDefaultImagePath(const char* cfgPath, char* out, int outSize)
{
	const char* p;
	const char* base;
	char baseName[MAX_QPATH];

	base = cfgPath;
	for (p = cfgPath; *p; p++)
	{
		if (*p == '/' || *p == '\\')
		{
			base = p + 1;
		}
	}

	Q_strncpyz(baseName, base, sizeof(baseName));
	COM_StripExtension(baseName, baseName);
	Com_sprintf(out, outSize, "icons/medals/%s.tga", baseName);
}

static void CG_MedalImagePathFromToken(const char* token, char* out, int outSize)
{
	const char* p;
	qboolean hasSlash;

	if (!token[0])
	{
		out[0] = '\0';
		return;
	}

	hasSlash = qfalse;
	for (p = token; *p; p++)
	{
		if (*p == '/' || *p == '\\')
		{
			hasSlash = qtrue;
			break;
		}
	}

	if (hasSlash)
	{
		Q_strncpyz(out, token, outSize);
	}
	else
	{
		Com_sprintf(out, outSize, "icons/medals/%s", token);
	}
}

static qboolean CG_LoadMedalAtlas(int type, const char* cfgPath)
{
	char buf[8000];
	fileHandle_t f;
	char* text;
	char* token;
	char imagePath[MAX_QPATH];
	float atlasW;
	float atlasH;
	float invW;
	float invH;
	int len;
	int frameIndex;
	float x0;
	float y0;
	float x1;
	float y1;
	cgMedalAtlas_t* atlas;

	atlas = &cgs.media.medalAtlas[type];
	memset(atlas, 0, sizeof(*atlas));

	CG_MedalDefaultImagePath(cfgPath, imagePath, sizeof(imagePath));
	atlasW = 1024.0f;
	atlasH = 1024.0f;

	memset(buf, 0, sizeof(buf));
	len = trap_FS_FOpenFile(cfgPath, &f, FS_READ);
	if (f == FS_INVALID_HANDLE)
	{
		CG_Printf(S_COLOR_YELLOW "CG_LoadMedalAtlas: missing %s\n", cfgPath);
		return qfalse;
	}

	if (len >= (int)sizeof(buf))
	{
		len = sizeof(buf) - 1;
	}

	trap_FS_Read(buf, len, f);
	trap_FS_FCloseFile(f);
	buf[len] = '\0';

	text = buf;
	COM_BeginParseSession(cfgPath);

	while (1)
	{
		token = COM_ParseExt(&text, qtrue);
		if (!token[0])
		{
			break;
		}

		if (!strcmp(token, "img"))
		{
			token = COM_ParseExt(&text, qfalse);
			CG_MedalImagePathFromToken(token, imagePath, sizeof(imagePath));
			SkipRestOfLine(&text);
			continue;
		}

		if (!strcmp(token, "fnt"))
		{
			token = COM_ParseExt(&text, qfalse);
			if (token[0])
			{
				atlasW = (float)atof(token);
			}
			token = COM_ParseExt(&text, qfalse);
			if (token[0])
			{
				atlasH = (float)atof(token);
			}
			SkipRestOfLine(&text);
			continue;
		}

		frameIndex = atoi(token);
		if (frameIndex < 0 || frameIndex >= CG_MEDAL_FRAME_COUNT)
		{
			SkipRestOfLine(&text);
			continue;
		}

		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			break;
		}
		x0 = (float)atof(token);

		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			break;
		}
		y0 = (float)atof(token);

		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			break;
		}
		x1 = (float)atof(token);

		token = COM_ParseExt(&text, qfalse);
		if (!token[0])
		{
			break;
		}
		y1 = (float)atof(token);

		if (atlasW <= 0.0f)
		{
			atlasW = 1024.0f;
		}
		if (atlasH <= 0.0f)
		{
			atlasH = 1024.0f;
		}
		invW = 1.0f / atlasW;
		invH = 1.0f / atlasH;

		atlas->frameTc[frameIndex][0] = x0 * invW;
		atlas->frameTc[frameIndex][1] = y0 * invH;
		atlas->frameTc[frameIndex][2] = x1 * invW;
		atlas->frameTc[frameIndex][3] = y1 * invH;

		SkipRestOfLine(&text);
	}

	atlas->shader = trap_R_RegisterShaderNoMip(imagePath);
	if (!atlas->shader)
	{
		CG_Printf(S_COLOR_YELLOW "CG_LoadMedalAtlas: failed to load %s\n", imagePath);
		return qfalse;
	}

	return qtrue;
}

void CG_RegisterMedalAnimationShaders(void)
{
	int type;

	if (cg_oldMedals.integer)
	{
		return;
	}

	for (type = 0; type < CG_MEDAL_TYPE_COUNT; type++)
	{
		CG_LoadMedalAtlas(type, cg_medalCfgPaths[type]);
	}
}

static int CG_MedalTypeFromShader(qhandle_t medalShader)
{
	if (medalShader == cgs.media.medalImpressive)
	{
		return 0;
	}
	if (medalShader == cgs.media.medalExcellent)
	{
		return 1;
	}
	if (medalShader == cgs.media.medalGauntlet)
	{
		return 2;
	}
	if (medalShader == cgs.media.medalDefend)
	{
		return 3;
	}
	if (medalShader == cgs.media.medalAssist)
	{
		return 4;
	}
	if (medalShader == cgs.media.medalCapture)
	{
		return 5;
	}
	return -1;
}

static int CG_MedalFrameIndex(int startTime, int durationMsec)
{
	int elapsed;
	int slot;

	if (startTime <= 0)
	{
		return 0;
	}

	if (durationMsec <= 0)
	{
		durationMsec = REWARD_TIME;
	}

	elapsed = cg.time - startTime;
	if (elapsed < 0)
	{
		elapsed = 0;
	}

	slot = (elapsed * CG_MEDAL_TIMELINE_SLOTS) / durationMsec;
	if (slot >= CG_MEDAL_TIMELINE_SLOTS)
	{
		slot = CG_MEDAL_TIMELINE_SLOTS - 1;
	}

	if (slot >= CG_MEDAL_ANIM_SLOTS)
	{
		return CG_MEDAL_LAST_FRAME_INDEX;
	}

	return slot;
}

void CG_MedalResolveFrame(qhandle_t medalShader, int startTime, int durationMsec, qhandle_t* outShader, vec4_t outTc)
{
	int type;
	int frameIndex;
	cgMedalAtlas_t* atlas;

	type = CG_MedalTypeFromShader(medalShader);
	if (type < 0)
	{
		*outShader = medalShader;
		Vector4Set(outTc, 0.0f, 0.0f, 1.0f, 1.0f);
		return;
	}

	atlas = &cgs.media.medalAtlas[type];
	if (!atlas->shader)
	{
		*outShader = medalShader;
		Vector4Set(outTc, 0.0f, 0.0f, 1.0f, 1.0f);
		return;
	}

	frameIndex = CG_MedalFrameIndex(startTime, durationMsec);
	*outShader = atlas->shader;
	Vector4Copy(atlas->frameTc[frameIndex], outTc);
}

void CG_DrawMedalRewardPic(float x, float y, float w, float h, qhandle_t medalShader, int startTime, int durationMsec)
{
	qhandle_t shader;
	vec4_t tc;

	if (cg_oldMedals.integer)
	{
		CG_DrawPic(x, y, w, h, medalShader);
		return;
	}

	CG_MedalResolveFrame(medalShader, startTime, durationMsec, &shader, tc);
	trap_R_DrawStretchPic(x, y, w, h, tc[0], tc[1], tc[2], tc[3], shader);
}
