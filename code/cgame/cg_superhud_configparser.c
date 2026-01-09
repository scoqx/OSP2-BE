#include "cg_superhud_private.h"
#include "cg_local.h"
#include "../qcommon/qcommon.h"


static superhudConfigParseStatus_t CG_SHUDConfigCommandParseColor(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseColor2(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAlighH(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAlighV(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAngles(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBgColor(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBgColor2(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseDirection(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseDoublebar(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFade(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFadeDelay(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFill(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFont(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFontSize(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImage(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImage2(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImageTC(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseItTeam(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseMargins(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseModel(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseMonospace(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseOffset(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseRect(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseText(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextAlign(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextOffset(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextStyle(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTime(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseVisFlags(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseHlColor(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseHlSize(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorder(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorderColor(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorderColor2(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseStyle(configFileInfo_t* finfo, superhudConfig_t* config);
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseShadowColor(configFileInfo_t* finfo, superhudConfig_t* config);

static superHUDConfigCommand_t superHUDConfigItemCommands[] =
{
	{ "alignh", CG_SHUDConfigCommandParseAlighH },
	{ "alignv", CG_SHUDConfigCommandParseAlighV },
	{ "angles", CG_SHUDConfigCommandParseAngles },
	{ "bgcolor", CG_SHUDConfigCommandParseBgColor },
	{ "bgcolor2", CG_SHUDConfigCommandParseBgColor2 },
	{ "hlcolor", CG_SHUDConfigCommandParseHlColor },
	{ "color", CG_SHUDConfigCommandParseColor },
	{ "color2", CG_SHUDConfigCommandParseColor2 },
	{ "direction", CG_SHUDConfigCommandParseDirection },
	{ "doublebar", CG_SHUDConfigCommandParseDoublebar },
	{ "fade", CG_SHUDConfigCommandParseFade },
	{ "fadedelay", CG_SHUDConfigCommandParseFadeDelay },
	{ "fill", CG_SHUDConfigCommandParseFill },
	{ "font", CG_SHUDConfigCommandParseFont },
	{ "fontsize", CG_SHUDConfigCommandParseFontSize },
	{ "image", CG_SHUDConfigCommandParseImage },
	{ "image2", CG_SHUDConfigCommandParseImage2 },
	{ "imagetc", CG_SHUDConfigCommandParseImageTC },
	{ "itteam", CG_SHUDConfigCommandParseItTeam },
	{ "margins", CG_SHUDConfigCommandParseMargins },
	{ "model", CG_SHUDConfigCommandParseModel },
	{ "monospace", CG_SHUDConfigCommandParseMonospace },
	{ "offset", CG_SHUDConfigCommandParseOffset },
	{ "rect", CG_SHUDConfigCommandParseRect },
	{ "shadowColor", CG_SHUDConfigCommandParseShadowColor },
	{ "text", CG_SHUDConfigCommandParseText },
	{ "textalign", CG_SHUDConfigCommandParseTextAlign },
	{ "textoffset", CG_SHUDConfigCommandParseTextOffset },
	{ "textstyle", CG_SHUDConfigCommandParseTextStyle },
	{ "time", CG_SHUDConfigCommandParseTime },
	{ "visflags", CG_SHUDConfigCommandParseVisFlags},
	{ "hlsize", CG_SHUDConfigCommandParseHlSize },
	{ "border", CG_SHUDConfigCommandParseBorder },
	{ "borderColor", CG_SHUDConfigCommandParseBorderColor },
	{ "borderColor2", CG_SHUDConfigCommandParseBorderColor2 },
	{ "style", CG_SHUDConfigCommandParseStyle },
	{ NULL, NULL, NULL },
};

#define CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(CCC) \
       ((CCC >= 'a' && CCC <= 'z') \
      || (CCC >= 'A' && CCC <= 'Z') \
      || (CCC >= '0' && CCC <= '9') \
      || (CCC == '_') \
      || (CCC == '!') \
       )

#define CG_SHUD_CONFIG_INFO_IS_TEXT_SPLITTER(CCC) \
       ( (CCC == '{') \
      || (CCC == '}') \
      || (CCC == ',') \
       )

#define CG_SHUD_CONFIG_INFO_IS_CHAR_END_OF_LINE(CCC) \
       ( (CCC == '#') \
      || (CCC == '\n') \
      || (CCC == 0) \
       )

#define CG_SHUD_CONFIG_INFO_GET_CHAR(CFI) CFI->last_line->line[CFI->pos]
#define CG_SHUD_CONFIG_INFO_NEXT_CHAR(CFI) ++CFI->pos
#define CG_SHUD_CONFIG_INFO_NEXT_LINE(CFI) CFI->last_line = CFI->last_line->next; CFI->pos = 0
#define CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(CFI) (!CFI->last_line)

#define SHUD_CONFIG_ITEMS_DICT_SIZE   256

static superhudElementDictMember_t* superHUDConfigItemElementsDict[SHUD_CONFIG_ITEMS_DICT_SIZE];
static superHUDConfigCommand_t* superHUDConfigItemCommandsDict[SHUD_CONFIG_ITEMS_DICT_SIZE];

const superHUDConfigElement_t* CG_SHUDFindConfigElementItem(const char* name)
{
	const superhudElementDictMember_t* targetEl = superHUDConfigItemElementsDict[Com_GenerateHashValue(name, SHUD_CONFIG_ITEMS_DICT_SIZE)];
	const superHUDConfigElement_t* target = targetEl ? targetEl->element : NULL;

	while (target && Q_stricmp(name, target->name))
	{
		targetEl = targetEl->next;
		target = targetEl ? targetEl->element : NULL;
	}
	return target;
}

const superHUDConfigCommand_t* CG_SHUDFindConfigCommandItem(const char* name)
{
	const superHUDConfigCommand_t* target = superHUDConfigItemCommandsDict[Com_GenerateHashValue(name, SHUD_CONFIG_ITEMS_DICT_SIZE)];

	while (target && Q_stricmp(name, target->name))
	{
		target = target->next;
	}
	return target;
}

/*
 * Skip spaces, comments, new lines, comma
 */
static superhudConfigParseStatus_t CG_SHUDConfigSkipSCN(configFileInfo_t* finfo)
{
	qboolean skip;
	qboolean skip_line;
	char c;

	do
	{
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

		skip = c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == ',';
		skip_line = c == '#' || c == 0;

		if (skip_line)
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(finfo);
			if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(finfo))
			{
				return SUPERHUD_CONFIG_END_OF_FILE;
			}
		}
		else if (skip)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
		}
	}
	while (skip || skip_line);

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDParseString(configFileInfo_t* finfo, char* value, int len)
{
	superhudConfigParseStatus_t status;
	int rc;
	char c;
	char* src;
	char* dst;
	int lenMax = 0;
	char varName[MAX_QPATH];
	int i;
	int startPos;

	status = CG_SHUDConfigSkipSCN(finfo);
	if (status != SUPERHUD_CONFIG_OK) return status;

	startPos = finfo->pos;
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it's a quoted string */
	if (c == '"')
	{
		/* Parse quoted string */
		dst = finfo->last_line->line + finfo->pos;

		do
		{
			c = *dst;
			if (c == 0 || c == '#' || c == '\n' || c == ';' || c == '}')
			{
				break;
			}
			++lenMax;
			++dst;
		}
		while (qtrue);

		if (!lenMax)
		{
			return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
		}

		dst = Z_Malloc(lenMax + 1);
		OSP_MEMORY_CHECK(dst);

		src = Z_Malloc(lenMax + 1);
		OSP_MEMORY_CHECK(src);

		memcpy(src, finfo->last_line->line + finfo->pos, lenMax);
		src[lenMax] = 0;

		rc = Q_sscanf(src, "\"%s\"", dst);

		if (rc <= 0)
		{
			rc = Q_sscanf(src, "%s", dst);
		}

		if (rc <= 0)
		{
			Z_Free(dst);
			Z_Free(src);
			return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
		}

		finfo->pos += lenMax;

		Q_strncpyz(value, dst, len);

		Z_Free(dst);
		Z_Free(src);

		return SUPERHUD_CONFIG_OK;
	}

	/* Not a quoted string, try to parse as variable name with $ prefix */
	finfo->pos = startPos;
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it starts with $ (variable prefix) */
	if (c != '$')
	{
		/* Not a variable, parse as regular string */
		goto parse_as_string;
	}

	/* Skip $ */
	CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it's a valid variable name (starts with letter or underscore) */
	if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	/* Read variable name */
	i = 0;
	while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c) && i < (MAX_QPATH - 1))
	{
		varName[i++] = c;
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
	}
	varName[i] = 0;

	/* Try to get variable value */
	if (CG_SHUDVarGetString(varName, value, len))
	{
		return SUPERHUD_CONFIG_OK;
	}

	/* Variable not found - use variable name as string value and warn */
	CG_Printf("^3SuperHUD: variable '$%s' not found, using as string value\n", varName);
	Q_strncpyz(value, varName, len);
	return SUPERHUD_CONFIG_OK;

parse_as_string:

	/* Parse as regular string */
	finfo->pos = startPos;
	dst = finfo->last_line->line + finfo->pos;
	lenMax = 0;

	do
	{
		c = *dst;
		if (c == 0 || c == '#' || c == '\n' || c == ';' || c == '}')
		{
			break;
		}
		++lenMax;
		++dst;
	}
	while (qtrue);

	if (!lenMax)
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	dst = Z_Malloc(lenMax + 1);
	OSP_MEMORY_CHECK(dst);

	src = Z_Malloc(lenMax + 1);
	OSP_MEMORY_CHECK(src);

	memcpy(src, finfo->last_line->line + finfo->pos, lenMax);
	src[lenMax] = 0;

	rc = Q_sscanf(src, "%s", dst);

	if (rc <= 0)
	{
		Z_Free(dst);
		Z_Free(src);
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	finfo->pos += lenMax;

	Q_strncpyz(value, dst, len);

	Z_Free(dst);
	Z_Free(src);

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDParseFloat(configFileInfo_t* finfo, float* value)
{
	superhudConfigParseStatus_t status;
	int rc;
	qboolean skip;
	char c;
	char varName[MAX_QPATH];
	int i;
	int startPos;
	float varValue;

	status = CG_SHUDConfigSkipSCN(finfo);
	if (status != SUPERHUD_CONFIG_OK) return status;

	startPos = finfo->pos;

	/* Try to parse as number first */
	rc = Q_sscanf(finfo->last_line->line + finfo->pos, "%f", value);

	if (rc == 1)
	{
		/* Successfully parsed as number */
		do
		{
			c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

			skip = (c >= '0' && c <= '9') || c == '.' || c == '-';

			if (skip)
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			}
		}
		while (skip);

		return SUPERHUD_CONFIG_OK;
	}

	/* Not a number, try to parse as variable name with $ prefix */
	finfo->pos = startPos;
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it starts with $ (variable prefix) */
	if (c != '$')
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	/* Skip $ */
	CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it's a valid variable name (starts with letter or underscore) */
	if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	/* Read variable name */
	i = 0;
	while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c) && i < (MAX_QPATH - 1))
	{
		varName[i++] = c;
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
	}
	varName[i] = 0;

	/* Try to get variable value */
	if (CG_SHUDVarGetFloat(varName, &varValue))
	{
		*value = varValue;
		return SUPERHUD_CONFIG_OK;
	}

	/* Variable not found or wrong type - try to parse as number from variable name */
	CG_Printf("^3SuperHUD: variable '$%s' not found or wrong type, trying to parse as number\n", varName);
	return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
}

static superhudConfigParseStatus_t CG_SHUDParseInt(configFileInfo_t* finfo, int* value)
{
	superhudConfigParseStatus_t status;
	int rc;
	qboolean skip;
	char c;
	char varName[MAX_QPATH];
	int i;
	int startPos;
	int varValue;

	status = CG_SHUDConfigSkipSCN(finfo);
	if (status != SUPERHUD_CONFIG_OK) return status;

	startPos = finfo->pos;

	/* Try to parse as number first */
	rc = Q_sscanf(finfo->last_line->line + finfo->pos, "%i", value);

	if (rc == 1)
	{
		/* Successfully parsed as number */
		do
		{
			c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

			skip = (c >= '0' && c <= '9') || c == '-';

			if (skip)
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			}
		}
		while (skip);

		return SUPERHUD_CONFIG_OK;
	}

	/* Not a number, try to parse as variable name with $ prefix */
	finfo->pos = startPos;
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it starts with $ (variable prefix) */
	if (c != '$')
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	/* Skip $ */
	CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
	c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

	/* Check if it's a valid variable name (starts with letter or underscore) */
	if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	/* Read variable name */
	i = 0;
	while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c) && i < (MAX_QPATH - 1))
	{
		varName[i++] = c;
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
	}
	varName[i] = 0;

	/* Try to get variable value */
	if (CG_SHUDVarGetInt(varName, &varValue))
	{
		*value = varValue;
		return SUPERHUD_CONFIG_OK;
	}

	/* Variable not found or wrong type - try to parse as number from variable name */
	CG_Printf("^3SuperHUD: variable '$%s' not found or wrong type, trying to parse as number\n", varName);
	return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
}

static superhudConfigParseStatus_t CG_SHUDParseVec4t(configFileInfo_t* finfo, vec4_t color)
{
	superhudConfigParseStatus_t status;

	status = CG_SHUDParseFloat(finfo, &color[0]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[1]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[2]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[3]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDParseVec3t(configFileInfo_t* finfo, vec3_t color)
{
	superhudConfigParseStatus_t status;

	status = CG_SHUDParseFloat(finfo, &color[0]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[1]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[2]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDParseVec2t(configFileInfo_t* finfo, vec2_t color)
{
	superhudConfigParseStatus_t status;

	status = CG_SHUDParseFloat(finfo, &color[0]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	status = CG_SHUDParseFloat(finfo, &color[1]);
	if (status != SUPERHUD_CONFIG_OK) return status;

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextStyle(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->textStyle.isSet = qfalse;
	status = CG_SHUDParseInt(finfo, &config->textStyle.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->textStyle.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTime(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->time.isSet = qfalse;
	status = CG_SHUDParseInt(finfo, &config->time.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->time.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

static qboolean CG_SHUDConfigCommandParseVisFlagsVariant(configFileInfo_t* finfo, const char* token, int mask, int* flags)
{
	qboolean isFound = qfalse;
	const char* str = finfo->last_line->line + finfo->pos;

	if (Q_stricmpn(str, token, strlen(token)) == 0)
	{
		*flags |= mask;
		isFound = qtrue;
		finfo->pos += strlen(token);
	}

	return isFound;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseVisFlags(configFileInfo_t* finfo, superhudConfig_t* config)
{
	qboolean skip;
	superhudConfigParseStatus_t status;
	int flagsFound = 0;
	char c;
	int flags = 0;

	config->visflags.isSet = qfalse;

	do
	{
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status != SUPERHUD_CONFIG_OK)
		{
			break;
		}

		c = *(finfo->last_line->line + finfo->pos);
		if (c == 0 || c == '#' || c == '\n' || c == ';' || c == '}')
		{
			break; // end of command
		}

		if (CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_IM_STR, SE_IM, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_TEAM_ONLY_STR, SE_TEAM_ONLY, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_SPECT_STR, SE_SPECT, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_DEAD_STR, SE_DEAD, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_DEMO_HIDE_STR, SE_DEMO_HIDE, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_SCORES_HIDE_STR, SE_SCORES_HIDE, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_KEY1_SHOW_STR, SE_KEY1_SHOW, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_KEY2_SHOW_STR, SE_KEY2_SHOW, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_KEY3_SHOW_STR, SE_KEY3_SHOW, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_KEY4_SHOW_STR, SE_KEY4_SHOW, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_FFA_STR, SE_GT_FFA, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_TDM_STR, SE_GT_TDM, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_CTF_STR, SE_GT_CTF, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_TOURNEY_STR, SE_GT_TOURNEY, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_FREEZETAG_STR, SE_GT_FREEZETAG, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_GT_CLANARENA_STR, SE_GT_CLANARENA, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_GAUNTLET_STR, SE_WEAPON_GAUNTLET, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_MACHINEGUN_STR, SE_WEAPON_MACHINEGUN, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_SHOTGUN_STR, SE_WEAPON_SHOTGUN, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_GRENADE_LAUNCHER_STR, SE_WEAPON_GRENADE_LAUNCHER, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_ROCKET_LAUNCHER_STR, SE_WEAPON_ROCKET_LAUNCHER, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_LIGHTNING_STR, SE_WEAPON_LIGHTNING, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_RAILGUN_STR, SE_WEAPON_RAILGUN, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_PLASMAGUN_STR, SE_WEAPON_PLASMAGUN, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_WEAPON_BFG_STR, SE_WEAPON_BFG, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_HEALTH_LOW_STR, SE_HEALTH_LOW, &flags) ||
		        CG_SHUDConfigCommandParseVisFlagsVariant(finfo, SE_SHOW_EMPTY_STR, SE_SHOW_EMPTY, &flags)
		   )
		{
			++flagsFound;
		}
		//skip word
		do
		{
			c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

			skip = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';

			if (skip)
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			}
		}
		while (skip);
	}
	while (qtrue);

	if (flagsFound > 0)
	{
		config->visflags.isSet = qtrue;
		config->visflags.value = flags;
	}

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse HLCOLOR r g b a
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseHlColor(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;

	config->hlcolor.isSet = qfalse;

	status = CG_SHUDParseVec4t(finfo, config->hlcolor.value);
	if (status != SUPERHUD_CONFIG_OK) return status;

	config->hlcolor.isSet = qtrue;

	return status;
}

/*
 * parse HLSIZE x y
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseHlSize(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->hlsize.isSet = qfalse;
	status = CG_SHUDParseInt(finfo, &config->hlsize.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->hlsize.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse TEXTOFFSET x y
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextOffset(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->textOffset.isSet = qfalse;
	status = CG_SHUDParseVec2t(finfo, config->textOffset.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->textOffset.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse TEXTALIGN L / C / R
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseTextAlign(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->textAlign.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 'l':
			config->textAlign.value = SUPERHUD_ALIGNH_LEFT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'c':
			config->textAlign.value = SUPERHUD_ALIGNH_CENTER;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'r':
			config->textAlign.value = SUPERHUD_ALIGNH_RIGHT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->textAlign.isSet = qtrue;
	return status;
}

/*
 * parse TEXT str
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseText(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->text.isSet = qfalse;
	status = CG_SHUDParseString(finfo, config->text.value, MAX_QPATH);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->text.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse RECT x y w h
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseRect(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->rect.isSet = qfalse;
	status = CG_SHUDParseVec4t(finfo, config->rect.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->rect.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse OFFSET x y z
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseOffset(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->offset.isSet = qfalse;
	status = CG_SHUDParseVec3t(finfo, config->offset.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->offset.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse MONOSPACE
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseMonospace(configFileInfo_t* finfo, superhudConfig_t* config)
{
	config->monospace.isSet = qtrue;
	(void) config;
	(void) finfo;
	return SUPERHUD_CONFIG_OK;
}

/*
 * parse MODEL str
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseModel(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->model.isSet = qfalse;
	status = CG_SHUDParseString(finfo, config->model.value, MAX_QPATH);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->model.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse MARGINS l t r b
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseMargins(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->margins.isSet = qfalse;
	status = CG_SHUDParseVec4t(finfo, config->margins.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->margins.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse ITTEAM B/R/N/O/E
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseItTeam(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->itTeam.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 'b':
			config->itTeam.value = SUPERHUD_ITTEAM_BLUE;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'r':
			config->itTeam.value = SUPERHUD_ITTEAM_RED;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'n':
			config->itTeam.value = SUPERHUD_ITTEAM_NEUTRAL;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'o':
			config->itTeam.value = SUPERHUD_ITTEAM_OWN;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->itTeam.value = SUPERHUD_ITTEAM_ENEMY;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->itTeam.isSet = qtrue;
	return status;
}

/*
 * parse IMAGETC l t r b
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImageTC(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->imagetc.isSet = qfalse;
	status = CG_SHUDParseVec4t(finfo, config->imagetc.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->imagetc.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse IMAGE str
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImage(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->image.isSet = qfalse;
	status = CG_SHUDParseString(finfo, config->image.value, MAX_QPATH);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->image.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse IMAGE2 str
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseImage2(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->image2.isSet = qfalse;
	status = CG_SHUDParseString(finfo, config->image2.value, MAX_QPATH);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->image2.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse FONTSIZE xsize [ysize]
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFontSize(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->fontsize.isSet = qfalse;

	status = CG_SHUDParseFloat(finfo, &config->fontsize.value[0]);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}

	status = CG_SHUDParseFloat(finfo, &config->fontsize.value[1]);
	if (status != SUPERHUD_CONFIG_OK)
	{
		config->fontsize.value[1] = config->fontsize.value[0];
	}

	config->fontsize.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse FONT str
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFont(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->font.isSet = qfalse;
	status = CG_SHUDParseString(finfo, config->font.value, MAX_QPATH);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->font.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse FILL
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFill(configFileInfo_t* finfo, superhudConfig_t* config)
{
	config->fill.isSet = qtrue;
	(void) config;
	(void) finfo;
	return SUPERHUD_CONFIG_OK;
}

/*
 * parse FADEDELAY t
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFadeDelay(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	float fadeDelay;

	config->fadedelay.isSet = qfalse;
	status = CG_SHUDParseFloat(finfo, &fadeDelay);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}
	config->fadedelay.isSet = qtrue;
	config->fadedelay.value = (int)(fadeDelay);

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse FADE [r g b a]
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseFade(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->fade.isSet = qtrue;
	status = CG_SHUDParseVec4t(finfo, config->fade.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		// it is ok if there are no colors
		Vector4Clear(config->fade.value);
	}

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse DOUBLEBAR
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseDoublebar(configFileInfo_t* finfo, superhudConfig_t* config)
{
	config->doublebar.isSet = qtrue;
	(void) config;
	(void) finfo;
	return SUPERHUD_CONFIG_OK;
}

/*
 * parse BGCOLOR r g b a
 */
// static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBgColor(configFileInfo_t* finfo, superhudConfig_t* config)
// {
//  superhudConfigParseStatus_t status;

//  config->bgcolor.isSet = qfalse;

//  status = CG_SHUDParseVec4t(finfo, config->bgcolor.value.rgba);
//  if (status != SUPERHUD_CONFIG_OK) return status;

//  config->bgcolor.isSet = qtrue;

//  return status;
// }

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBgColor(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->bgcolor.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);
	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->bgcolor.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->bgcolor.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->bgcolor.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->bgcolor.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->bgcolor.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->bgcolor.isSet = qtrue;
	return status;
}

/*
 * parse color2 command: rgba/t/e/I
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBgColor2(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->bgcolor2.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->bgcolor2.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->bgcolor2.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->bgcolor2.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->bgcolor2.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->bgcolor2.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->bgcolor2.isSet = qtrue;
	return status;
}

/*
 * parse ANGLES p y r [e]
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAngles(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	float e = .0f;

	config->angles.isSet = qfalse;
	//p y r
	status = CG_SHUDParseVec3t(finfo, config->angles.value);
	if (status != SUPERHUD_CONFIG_OK) return status;


	//[e]
	do
	{
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status != SUPERHUD_CONFIG_OK)  break;

		status = CG_SHUDParseFloat(finfo, &e);
		if (status != SUPERHUD_CONFIG_OK) break;
	}
	while (qfalse);

	config->angles.value[3] = e;
	config->angles.isSet = qtrue;

	return SUPERHUD_CONFIG_OK;
}

/*
 * parse horizontal align command: L/C/R
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAlighH(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->alignH.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 'l':
			config->alignH.value = SUPERHUD_ALIGNH_LEFT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'c':
			config->alignH.value = SUPERHUD_ALIGNH_CENTER;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'r':
			config->alignH.value = SUPERHUD_ALIGNH_RIGHT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->alignH.isSet = qtrue;
	return status;
}

/*
2024-07-11T04:46:02Z MORION_SWITCH daemon.warn phyd[895]: Port 8: Media changed to 10/100/1000BASE-T
 * parse vertical align command: T/C/B
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseAlighV(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->alignV.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->alignV.value = SUPERHUD_ALIGNV_TOP;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'c':
			config->alignV.value = SUPERHUD_ALIGNV_CENTER;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'b':
			config->alignV.value = SUPERHUD_ALIGNV_BOTTOM;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->alignV.isSet = qtrue;
	return status;
}

/*
 * parse direction L | R | T | B
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseDirection(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->direction.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 'l':
			config->direction.value = SUPERHUD_DIR_LEFT_TO_RIGHT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'r':
			config->direction.value = SUPERHUD_DIR_RIGHT_TO_LEFT;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 't':
			config->direction.value = SUPERHUD_DIR_TOP_TO_BOTTOM;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'b':
			config->direction.value = SUPERHUD_DIR_BOTTOM_TO_TOP;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->direction.isSet = qtrue;
	return status;

}

/*
 * parse color2 command: rgba/t/e/I
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseColor2(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->color2.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->color2.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->color2.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->color2.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->color2.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->color2.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->color2.isSet = qtrue;
	return status;
}

/*
 * parse color command: rgba/t/e/I
 */
static superhudConfigParseStatus_t CG_SHUDConfigCommandParseColor(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->color.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->color.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->color.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->color.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->color.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->color.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->color.isSet = qtrue;
	return status;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorder(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	config->border.isSet = qfalse;

	status = CG_SHUDParseVec4t(finfo, config->border.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}

	config->border.isSet = qtrue;
	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorderColor(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->borderColor.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->borderColor.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->borderColor.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->borderColor.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->borderColor.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->borderColor.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->borderColor.isSet = qtrue;
	return status;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseBorderColor2(configFileInfo_t* finfo, superhudConfig_t* config)
{
	char c;
	superhudConfigParseStatus_t status;

	config->borderColor2.isSet = qfalse;

	/* skip to value */
	status = CG_SHUDConfigSkipSCN(finfo);

	if (status != SUPERHUD_CONFIG_OK) return status;

	c = tolower(CG_SHUD_CONFIG_INFO_GET_CHAR(finfo));
	switch (c)
	{
		case 't':
			config->borderColor2.value.type = SUPERHUD_COLOR_T;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'e':
			config->borderColor2.value.type = SUPERHUD_COLOR_E;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		case 'i':
			config->borderColor2.value.type = SUPERHUD_COLOR_I;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			break;
		default:
			config->borderColor2.value.type = SUPERHUD_COLOR_RGBA;
			status = CG_SHUDParseVec4t(finfo, config->borderColor2.value.rgba);
			break;
	}

	if (status == SUPERHUD_CONFIG_OK) config->borderColor2.isSet = qtrue;
	return status;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseStyle(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;

	config->style.isSet = qfalse;

	// Парсим целочисленное значение для стиля
	status = CG_SHUDParseInt(finfo, &config->style.value);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}

	config->style.isSet = qtrue;
	return SUPERHUD_CONFIG_OK;
}

static superhudConfigParseStatus_t CG_SHUDConfigCommandParseShadowColor(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;

	config->shadowColor.isSet = qfalse;

	status = CG_SHUDParseVec4t(finfo, config->shadowColor.value);
	if (status != SUPERHUD_CONFIG_OK) return status;

	config->shadowColor.isSet = qtrue;

	return status;
}

/*
 * Split NULL-terminated config file to lines
 */
qboolean CG_SHUDFileInfoInit(configFileInfo_t* info, const char* fileContent)
{
	int line_number = 0;
	const char* startline = fileContent;
	const char* endline;
	const char* endoffile = fileContent + strlen(fileContent) + 1;

	memset(info, 0, sizeof(*info));

	while (startline < endoffile && *startline != 0)
	{
		endline = strchr(startline, '\n');
		if (endline == NULL)
		{
			endline = endoffile - 1;
		}

		if (!info->root)
		{
			info->root = (configFileLine_t*)Z_Malloc(sizeof(*info->root));
			OSP_MEMORY_CHECK(info->root);
			memset(info->root, 0, sizeof(*info->root));
			info->last_line = info->root;
		}
		else
		{
			info->last_line->next = (configFileLine_t*)Z_Malloc(sizeof(*info->root));
			OSP_MEMORY_CHECK(info->last_line->next);
			memset(info->last_line->next, 0, sizeof(*info->root));
			info->last_line = info->last_line->next;
		}

		info->last_line->line_number = ++line_number;
		info->last_line->size = endline - startline;
		info->last_line->line = Z_Malloc(info->last_line->size + 1);
		OSP_MEMORY_CHECK(info->last_line->line);
		memcpy(info->last_line->line, startline, info->last_line->size);
		info->last_line->line[info->last_line->size] = 0;
		if (info->last_line->line[info->last_line->size - 1] == 0x0d)
		{
			//skip /r
			info->last_line->line[info->last_line->size - 1] = 0;
		}

		startline = endline + 1;
	}
	if (info->root)
	{
		info->last_line = info->root;
	}

	return line_number > 0;
}

void CG_SHUDFileInfoTeardown(configFileInfo_t* cfi)
{
	configFileLine_t* to_delete;

	while (cfi->root)
	{
		to_delete = cfi->root;
		cfi->root = to_delete->next;

		if (to_delete->line)
		{
			Z_Free(to_delete->line);
		}
		Z_Free(to_delete);
	}
	cfi->last_line = NULL;
}

/* Поиск элемента
 *
 * пропускаем пробелы
 * пропускаем комментарии
 * пропускаем новую строку
 * берем токен
 * пропускаем пробелы
 * пропускаем комментарии
 * пропускаем новую строку
 * находим {
 *
 * ошибки:
 *
 * SUPERHUD_CONFIG_WRONG_ELEMENT_NAME
 * SUPERHUD_CONFIG_LOST_ELEMENT_BODY
 * SUPERHUD_CONFIG_END_OF_FILE
 *
 *
 */

const superhudConfigParseElement_t CG_SHUDFileInfoGetElementItem(configFileInfo_t* cfi)
{
	char elementName[MAX_QPATH];
	int i;
	char c;
	qboolean skip;
	qboolean skip_line;
	qboolean start_of_body;

	superhudConfigParseElement_t result = {NULL, SUPERHUD_CONFIG_OK, {0}};
	// find start of element name
	do
	{
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);

		skip = c == ' ' || c < 32;
		skip_line = c == '#' || c == 0;

		if (skip_line)
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(cfi);
			if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(cfi))
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
				result.status = SUPERHUD_CONFIG_END_OF_FILE;
				return result;
			}
		}
		else if (skip)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
	}
	while (skip || skip_line);

	// copy text to local buffer
	i = 0;
	while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c))
	{
		if (i < (MAX_QPATH - 1))
		{
			elementName[i++] = c;
		}
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);
	}
	elementName[i] = 0;
	result.item = CG_SHUDFindConfigElementItem(elementName);
	Q_strncpyz(result.name, elementName, sizeof(result.name));
	if (result.item)
	{
		result.status = SUPERHUD_CONFIG_OK;
	}
	else
	{
		result.status = SUPERHUD_CONFIG_WRONG_ELEMENT_NAME;
		return result;
	}

	// find start of element body
	do
	{
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);

		start_of_body = c == '{';
		skip = c == ' ' || c < 32;
		skip_line = c == '#' || c == 0;

		if (skip_line)
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(cfi);
			if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(cfi))
			{
				result.status = SUPERHUD_CONFIG_END_OF_FILE;
				return result;
			}
		}
		else if (skip)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
		else if (start_of_body)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
			break;
		}
		else
		{
			result.status = SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			return result;
		}
	}
	while (skip || skip_line);

	return result;
}

/* Поиск команды
 *
 * пропускаем пробелы
 * пропускаем комментарии
 * пропускаем новую строку
 * берем токен
 *
 * SUPERHUD_CONFIG_WRONG_COMMAND_NAME
 * SUPERHUD_CONFIG_LOST_ELEMENT_BODY
 * SUPERHUD_CONFIG_END_OF_FILE
 *
 *
 */

const superhudConfigParseCommand_t CG_SHUDFileInfoGetCommandItem(configFileInfo_t* cfi)
{
	char commandName[MAX_QPATH];
	int i;
	char c;
	qboolean skip;
	qboolean skip_line;
	qboolean end_of_element;
	superhudConfigParseCommand_t result = {NULL, SUPERHUD_CONFIG_OK};

	// find start of element name
	do
	{
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);

		skip = c == ' ' || c < 32 || c == ';';
		skip_line = c == '#' || c == 0;
		end_of_element = c == '}';

		if (skip_line)
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(cfi);
			if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(cfi))
			{
				result.status = SUPERHUD_CONFIG_END_OF_FILE;
				return result;
			}
		}
		else if (skip)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
		else if (end_of_element)
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
			result.status = SUPERHUD_CONFIG_END_OF_ELEMENT;
			return result;
		}
	}
	while (skip || skip_line);

	// copy text to local buffer
	i = 0;
	while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c))
	{
		if (i < (MAX_QPATH - 1))
		{
			commandName[i++] = c;
		}
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);
	}
	commandName[i] = 0;

	result.item = CG_SHUDFindConfigCommandItem(commandName);
	if (result.item)
	{
		result.status = SUPERHUD_CONFIG_OK;
	}
	else
	{
		result.status = SUPERHUD_CONFIG_WRONG_COMMAND_NAME;
	}

	return result;
}

qboolean CG_SHUDFileInfoSkipElementBody(configFileInfo_t* cfi)
{
	char c;
	int depth;
	qboolean at_body;

	/* move to first '{' */
	at_body = qfalse;
	while (!at_body)
	{
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);
		if (c == '{')
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
			at_body = qtrue;
			break;
		}
		if (c == 0)
		{
			if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(cfi))
			{
				return qfalse;
			}
			CG_SHUD_CONFIG_INFO_NEXT_LINE(cfi);
		}
		else
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
	}

	/* skip until matching closing brace */
	depth = 1;
	while (depth > 0)
	{
		if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(cfi))
		{
			return qfalse;
		}

		c = CG_SHUD_CONFIG_INFO_GET_CHAR(cfi);

		if (c == '{')
		{
			depth++;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
		else if (c == '}')
		{
			depth--;
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
		else if (c == 0)
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(cfi);
		}
		else
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(cfi);
		}
	}

	return qtrue;
}

/*
 * Инициализация парсера
 * No need to free allocated memory as object will be destroyed only when cgame.qvm die
 */
void CG_SHUDParserInit(void)
{
	int i = 0;
	static qboolean initialized = qfalse;
	const superHUDConfigElement_t* e;
	const superHUDConfigElement_t* root;


	if (initialized)
	{
		return;
	}

	CG_SHUDAvailableElementsInit();

	root = CG_SHUDAvailableElementsGet();

	e = root;

	while (e->name)
	{
		superhudElementDictMember_t** target = NULL;
		superhudElementDictMember_t* collision = NULL;
		unsigned long hash;

		hash = Com_GenerateHashValue(e->name, SHUD_CONFIG_ITEMS_DICT_SIZE);

		/* store in the hash table */
		target = &superHUDConfigItemElementsDict[hash];
		if (*target == NULL)
		{
			*target = Z_Malloc(sizeof(superhudElementDictMember_t));
			OSP_MEMORY_CHECK(*target);
			(*target)->element = e;
		}
		else
		{
			collision = *target;
			//collision
			while (collision->next)
			{
				collision = collision->next;
			}
			collision->next = Z_Malloc(sizeof(superhudElementDictMember_t));
			OSP_MEMORY_CHECK(collision->next);
			collision->next->element = e;
		}
		++i;
		e = root + i;
	}

	i = 0;
	while (superHUDConfigItemCommands[i].name)
	{
		superHUDConfigCommand_t** target = NULL;
		superHUDConfigCommand_t* collision = NULL;

		unsigned long hash = Com_GenerateHashValue(superHUDConfigItemCommands[i].name, SHUD_CONFIG_ITEMS_DICT_SIZE);

		/* store in the hash table */
		target = &superHUDConfigItemCommandsDict[hash];
		if (*target == NULL)
		{
			*target = &superHUDConfigItemCommands[i];
		}
		else
		{
			collision = *target;
			//collision
			while (collision->next)
			{
				collision = collision->next;
			}
			collision->next = &superHUDConfigItemCommands[i];
		}
		++i;
	}

	initialized = qtrue;
}

/*
 * Parse EXECUTE element: execute { command1; command2; }
 * Note: finfo->pos should already be after the opening brace '{'
 */
superhudConfigParseStatus_t CG_SHUDParseExecuteElement(configFileInfo_t* finfo, superhudConfig_t* config)
{
	superhudConfigParseStatus_t status;
	int len;
	qboolean done;
	char c;

	status = CG_SHUDConfigSkipSCN(finfo);
	if (status != SUPERHUD_CONFIG_OK)
	{
		return status;
	}

	len = 0;
	done = qfalse;

	config->execute.isSet = qfalse;
	config->execute.value[0] = 0;

	while (!done)
	{
		if (CG_SHUD_CONFIG_INFO_IS_END_OF_FILE(finfo))
		{
			return SUPERHUD_CONFIG_END_OF_FILE;
		}

		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

		if (c == '}')
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			done = qtrue;
			break;
		}

		if (c == '#')
		{
			CG_SHUD_CONFIG_INFO_NEXT_LINE(finfo);
			continue;
		}

		if (c == 0)
		{
			if (len > 0 && len < (SUPERHUD_EXECUTE_MAX_LEN - 1) && config->execute.value[len - 1] != ' ')
			{
				config->execute.value[len++] = ' ';
			}
			CG_SHUD_CONFIG_INFO_NEXT_LINE(finfo);
			continue;
		}

		if (len < (SUPERHUD_EXECUTE_MAX_LEN - 1))
		{
			if (c == '\n' || c == '\r' || c == '\t')
			{
				if (len > 0 && config->execute.value[len - 1] != ' ')
				{
					config->execute.value[len++] = ' ';
				}
			}
			else
			{
				config->execute.value[len++] = c;
			}
		}

		CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
	}

	config->execute.value[len] = 0;

	if (len == 0)
	{
		return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
	}

	config->execute.isSet = qtrue;
	return SUPERHUD_CONFIG_OK;
}

/*
 * Parse VAR element: VAR { x = 1; y = 10.0; z = "Lol"; }
 * Note: finfo->pos should already be after the opening brace '{'
 */
superhudConfigParseStatus_t CG_SHUDParseVarElement(configFileInfo_t* finfo)
{
	superhudConfigParseStatus_t status;
	char varName[MAX_QPATH];
	char c;
	int i;
	qboolean eoe = qfalse;
	qboolean isNumber;
	qboolean hasDecimal;
	float floatValues[4];
	int floatCount;
	char stringValue[MAX_QPATH];
	int startPos;
	int afterValuePos;

	/* Parse variables until closing brace */
	while (!eoe)
	{
		/* Skip spaces, newlines, comments */
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status == SUPERHUD_CONFIG_END_OF_FILE)
		{
			return SUPERHUD_CONFIG_END_OF_FILE;
		}

		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
		if (c == '}')
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			eoe = qtrue;
			break;
		}

		/* Read variable name */
		i = 0;
		while (CG_SHUD_CONFIG_INFO_IS_TEXT_CHARACTER(c) && c != '=')
		{
			if (i < (MAX_QPATH - 1))
			{
				varName[i++] = c;
			}
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
		}
		varName[i] = 0;

		if (i == 0)
		{
			return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
		}

		/* Skip spaces and check for = */
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status != SUPERHUD_CONFIG_OK)
		{
			return status;
		}

		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
		if (c != '=')
		{
			return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
		}
		CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);

		/* Skip spaces after = */
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status != SUPERHUD_CONFIG_OK)
		{
			return status;
		}
		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);

		/* Determine value type and parse */
		if (c == '"')
		{
			/* String type */
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			i = 0;
			c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
			while (c != '"' && c != 0 && c != '\n' && i < (MAX_QPATH - 1))
			{
				stringValue[i++] = c;
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
				c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
			}
			stringValue[i] = 0;

			if (c == '"')
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
			}

			CG_SHUDVarSetString(varName, stringValue);
		}
		else
		{
			/* Check if it's a number (int or float) */
			startPos = finfo->pos;
			isNumber = qfalse;
			hasDecimal = qfalse;
			floatCount = 0;

			if (c == '-' || c == '+')
			{
				CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
				c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
			}

			if (c >= '0' && c <= '9')
			{
				isNumber = qtrue;
				while ((c >= '0' && c <= '9') || c == '.')
				{
					if (c == '.')
					{
						hasDecimal = qtrue;
					}
					CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
					c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
				}
			}

			if (isNumber)
			{
				/* Restore position and parse numbers (support up to 4 floats) */
				finfo->pos = startPos;
				floatCount = 0;
				while (floatCount < 4)
				{
					status = CG_SHUDParseFloat(finfo, &floatValues[floatCount]);
					if (status != SUPERHUD_CONFIG_OK)
					{
						break;
					}
					floatCount++;

					afterValuePos = finfo->pos;
					status = CG_SHUDConfigSkipSCN(finfo);
					if (status != SUPERHUD_CONFIG_OK)
					{
						return status;
					}

					c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
					if (c == ';' || c == '}' || c == 0)
					{
						break;
					}
					if (c == '-' || c == '+' || (c >= '0' && c <= '9'))
					{
						continue;
					}

					/* Not a number start, rewind to before non-number */
					finfo->pos = afterValuePos;
					break;
				}

				if (floatCount == 4)
				{
					CG_SHUDVarSetFloat4(varName, floatValues);
				}
				else if (floatCount == 1 && !hasDecimal)
				{
					CG_SHUDVarSetInt(varName, (int)floatValues[0]);
				}
				else if (floatCount >= 1)
				{
					CG_SHUDVarSetFloat(varName, floatValues[0]);
				}
				else
				{
					return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
				}
			}
			else
			{
				return SUPERHUD_CONFIG_UNEXPECTED_CHARACTER;
			}
		}

		/* Skip ; or , */
		status = CG_SHUDConfigSkipSCN(finfo);
		if (status == SUPERHUD_CONFIG_END_OF_FILE)
		{
			return SUPERHUD_CONFIG_END_OF_FILE;
		}

		c = CG_SHUD_CONFIG_INFO_GET_CHAR(finfo);
		if (c == ';' || c == ',')
		{
			CG_SHUD_CONFIG_INFO_NEXT_CHAR(finfo);
		}
	}

	return SUPERHUD_CONFIG_OK;
}
