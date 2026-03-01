#include "cg_local.h"
#include "../qcommon/qcommon.h"


struct crosshairColors_s
{
	const char* name;
	float* color;
};

const struct crosshairColors_s crosshairColors[13] =
{
	{"black",   colorBlack},
	{"red",     colorRed},
	{"yellow",  colorYellow},
	{"blue",    colorBlue},
	{"green",   colorGreen},
	{"cyan",    colorCyan},
	{"magenta", colorMagenta},
	{"white",   colorWhite},
	{"grey",    colorDkGrey},
	{"orange",  colorOrange},
	{"pink",    colorPink},
	{NULL,      NULL},
};

const char* weaponNames[10] = { "None", "Gauntlet", "MachineGun", "Shotgun", "G.Launcher", "R.Launcher", "LightningGun", "Railgun", "Plasmagun", "BFG" };
const char* weaponShortNames[10] = { "", "GAUNTLET", "MG", "SG", "GL", "RL", "LG", "RG", "PG", "BFG" };

// very unsafe
void CG_OSPNormalizeText(char* src, int size, char* dst)
{
	int i = 0;

	while (*src != 0)
	{
		if (*src == '^')
		{
			src += 2;
			i += 2;
		}
		if (*src == 0x0a)
		{
			++src;
			++i;
		}
		else
		{
			*(dst++) = *(src++);
			++i;
		}
		if (i >= size)
		{
			*(dst) = 0;
		}
	}
}

/*
 * returns string size without special symbols like ^x123456, ^N
 * BUG: possible string overflow
 */
#define CHAR_IS_COLOR_CODE(C)  ( ((C >= '0') && (C <='9')) || ((C >= 'a') && (C <='f')) || ((C >= 'A') && (C <='F')))
int CG_OSPGetNormalizedStringSize(const char* str)
{
	int i = 0;
	while (str && *str)
	{
		if (*str == '^' && *(str + 1) != 0 &&  *(str + 1) != '^')
		{
			if (*(str + 1) == 'x' || *(str + 1) == 'X')
			{
				/* Ok, we have found ^x , check that next 6 charactes is color code */

				if (CHAR_IS_COLOR_CODE(*(str + 2)) ||
				        CHAR_IS_COLOR_CODE(*(str + 3)) ||
				        CHAR_IS_COLOR_CODE(*(str + 4)) ||
				        CHAR_IS_COLOR_CODE(*(str + 5)) ||
				        CHAR_IS_COLOR_CODE(*(str + 6)) ||
				        CHAR_IS_COLOR_CODE(*(str + 7)))
				{
					/* found eight characters code like ^xAABBCC */
					str += 8;
					continue;
				}
			}
			/* found two characters code like ^B */
			str += 2;
		}
		else
		{
			/* no special code */
			++i;
			++str;
		}
	}
	return i;
}



void CG_OSPColorFromNumber(int number, float* vector)
{
	float red;
	float green;
	float blue;

	if (number >= 10 && number < 34)
	{
		float tmp = ((360.0 * (float)number) - 3600.0) / 24.0;
		const float sixty_degree_rad = 60.0 * M_PI / 180.0;
		if (tmp <= 120.0)
		{
			tmp = M_PI * tmp / 180.0;
			blue = 0.0;
			red = (cos(tmp) / (cos(sixty_degree_rad - tmp)) + 1.0) / 3.0;
			green = 1.0 - (red + blue);
		}
		else if (tmp <= 240.0)
		{
			tmp = M_PI * (tmp - 120.0) / 180.0;
			red = 0.0;
			green = (cos(tmp) / (cos(sixty_degree_rad - tmp)) + 1.0) / 3.0;
			blue = 1.0 - (red + green);
		}
		else
		{
			tmp = M_PI * (tmp - 240.0) / 180.0;
			green = 0.0;
			blue = ((cos(tmp)) / (cos(sixty_degree_rad - tmp))  + 1.0) / 3.0;
			red = 1.0 - (green + blue);
		}

		VectorSet(vector, red, green, blue);

		return;
	}

	switch (number)
	{
		case 0:
			VectorSet(vector, 1.0, 1.0, 1.0);
			break;
		case 1:
			VectorSet(vector, 1.0, 0.2, 0.2);
			break;
		case 2:
			VectorSet(vector, 0.2, 1.0, 0.2);
			break;
		case 3:
			VectorSet(vector, 1.0, 1.0, 0.1);
			break;
		case 4:
			VectorSet(vector, 0.2, 0.2, 1.0);
			break;
		case 5:
			VectorSet(vector, 0.1, 1.0, 1.0);
			break;
		case 6:
			VectorSet(vector, 1.0, 0.1, 1.0);
			break;
		case 7:
			VectorSet(vector, 1.0, 0.59, 0.12);
			break;
		case 8:
			VectorSet(vector, 0.65, 0.2, 0.65);
			break;
		case 9:
			VectorSet(vector, 0.6, 0.6, 1.0);
			break;
		default:
			VectorSet(vector, 1.0, 1.0, 1.0);
	}
}

void CG_OSPColorFromChar(char c, float* i)
{
	int number;

	if (c >= '0' && c <= '9')
	{
		number = c - '0';
	}
	else
	{
		number = tolower(c) - 'a' + 10;
	}

	CG_OSPColorFromNumber(number, i);

}

void CG_OSPColorsFromString(const char* s, float* a, float* b, float* c, float* d)
{
	int i = 0;
	char str[4] = { '2', '2', '2', '2'}; // default green

	while (s && *s && i < 4)
	{
		str[i++] = *s++;
	}

	if (a) CG_OSPColorFromChar(str[0], a);
	if (b) CG_OSPColorFromChar(str[1], b);
	if (c) CG_OSPColorFromChar(str[2], c);
	if (d) CG_OSPColorFromChar(str[3], d);
}

void CG_OSPNormalizeNameCopy(char* from, char* to, unsigned int size)
{
	int i = 0;
	while (*from != 0 && i < (size - 1))
	{
		if (from[0] == '^' &&
		        (from[1] == 'x' || from[1] == 'X') &&
		        from[2] && from[3] && from[4] && from[5] && from[6] && from[7])
		{
			from += 8;
		}
		else
		{
			*(to++) = *(from++);
			++i;
		}
	}
	*to = 0;
}

void CG_DynamicMemReport(void)
{
	zone_stats_t stats;
	memset(&stats, 0, sizeof(stats));
	Z_Stats(&stats);

	// Com_Printf("zoneSegments   = %d \n", stats.zoneSegments);
	// Com_Printf("zoneBlocks     = %d \n", stats.zoneBlocks);
	// Com_Printf("zoneBytes      = %d \n", stats.zoneBytes);
	// Com_Printf("freeBytes      = %d \n", stats.freeBytes);
	// Com_Printf("freeBlocks     = %d \n", stats.freeBlocks);
	// Com_Printf("freeSmallest   = %d \n", stats.freeSmallest);
	// Com_Printf("freeLargest    = %d \n", stats.freeLargest);
}
/*
   *  Copy line to out with max size outSize
   *  return pointer to next line
   *
   */
const char* CG_LoadLine(const char* ptr, char* out, int outSize)
{
	const char* result;
	int size;
	if (!ptr)
	{
		return ptr;
	}

	result = strchr(ptr, 0x0d);
	if (!result)
	{
		result = strchr(ptr, 0x0a);
		if (!result)
		{
			result = strchr(ptr, 0);
		}
	}
	if (!result)
	{
		return result;
	}
	size = result - ptr;

	if (size >= outSize)
	{
		size = outSize - 1;
	}
	memcpy(out, ptr, size);
	out[size] = 0;

	while (*result == 0x0a || *result == 0x0d)
	{
		++result;
	}

	return result;
}

static char CG_CharFromHex4bit(char in)
{
	if (in >= '0' && in <= '9')
	{
		return in - '0';
	}
	else if (in >= 'a' && in <= 'f')
	{
		return 10 + (in - 'a');
	}
	return 0;
}

static float CG_FloatColorFromHex4bit(char in)
{
	return (1.0f / 15.0f) * CG_CharFromHex4bit(in);
}

static float CG_FloatColorFromHex8bit(char inHi, char inLow)
{
	int result;

	result = CG_CharFromHex4bit(inHi) * 16 + CG_CharFromHex4bit(inLow);

	return (1.0f / 255.0f) * (float)(result);
}

static void CG_ParseColorStr12Bit(const char* str, vec4_t out)
{
	char hexstr[4] = {'0', '0', '0', 0};
	const int len = strlen(str);

	Q_strncpyz(&hexstr[3 - len], str, len + 1);


	out[0] = CG_FloatColorFromHex4bit(hexstr[0]);
	out[1] = CG_FloatColorFromHex4bit(hexstr[1]);
	out[2] = CG_FloatColorFromHex4bit(hexstr[2]);
	out[3] = 1.0f;
}

static void CG_ParseColorStr24Bit(const char* str, vec4_t out)
{
	char hexstr[7] = {'0', '0', '0', '0', '0', '0', 0};
	const int len = strlen(str);

	Q_strncpyz(&hexstr[6 - len], str, len + 1);


	out[0] = CG_FloatColorFromHex8bit(hexstr[0], hexstr[1]);
	out[1] = CG_FloatColorFromHex8bit(hexstr[2], hexstr[3]);
	out[2] = CG_FloatColorFromHex8bit(hexstr[4], hexstr[5]);
	out[3] = 1.0f;
}

// static void CG_ParseColorStr32Bit(const char* str, vec4_t out)
// {
//  char hexstr[9] = {'0','0','0','0','0','0','0','0', 0};
//  const int len = strlen(str);

//  Q_strncpyz(&hexstr[8 - len], str, len + 1);

//  out[3] = CG_FloatColorFromHex8bit(hexstr[0], hexstr[1]); // A
//  out[0] = CG_FloatColorFromHex8bit(hexstr[2], hexstr[3]); // R
//  out[1] = CG_FloatColorFromHex8bit(hexstr[4], hexstr[5]); // G
//  out[2] = CG_FloatColorFromHex8bit(hexstr[6], hexstr[7]); // B
// }

qboolean CG_ParseColorStr(const char* str, vec4_t out)
{
	char in[MAX_QPATH];
	int len;
	const char* ptr;
	const struct crosshairColors_s* colorsArray;
	int i;

	if (!str) return qfalse;

	// prepare string:
	// to lower case, get len,
	len = strlen(str);
	if (!len || len > 8) return qfalse;

	// Handle T (team) and E (enemy) color codes - check BEFORE tolower (uppercase only)
	// lowercase 'e' and 't' are Quake 3 color codes, so we only accept uppercase
	if (len == 1 && (str[0] == 'T' || str[0] == 'E'))
	{
		team_t myTeam;
		char originalChar;

		// Use tracked team from cgs.beMyTeam (updated every frame)
		myTeam = cgs.beMyTeam;
		originalChar = str[0];

		// Return team or enemy color based on my team
		if (originalChar == 'T')
		{
			// Team color - color of my team
			if (myTeam == TEAM_RED)
			{
				Vector4Copy(cgs.beRedTeamColor, out);
			}
			else if (myTeam == TEAM_BLUE)
			{
				Vector4Copy(cgs.beBlueTeamColor, out);
			}
			else
			{
				Vector4Copy(cgs.beRedTeamColor, out);
			}
		}
		else
		{
			// Enemy color - color of enemy team
			if (myTeam == TEAM_RED)
			{
				Vector4Copy(cgs.beBlueTeamColor, out);
			}
			else if (myTeam == TEAM_BLUE)
			{
				Vector4Copy(cgs.beRedTeamColor, out);
			}
			else
			{
				Vector4Copy(cgs.beBlueTeamColor, out);
			}
		}
		return qtrue;
	}

	for (i = 0; i < 8 && *str; ++i, ++str)
	{
		in[i] = tolower(*str);
	}
	in[i] = 0;

	// if len == 1, it and 0..9 - one letter color
	if (len == 1 && in[0] >= '0' && in[0] <= '9')
	{
		//color code 0-9
		VectorCopy(g_color_table[in[0] & 0x0f], out);
		out[3] = 1.0f;
		return qtrue;
	}

	//check, is it color string
	//
	if (in[0] >= 'a' && in[0] <= 'z')
	{
		colorsArray = &crosshairColors[0];
		while (colorsArray->name)
		{
			if (Q_stricmp(in, colorsArray->name) == 0)
			{
				break;
			}
			++colorsArray;
		}
		if (colorsArray->name)
		{
			//found color
			Vector4Copy(colorsArray->color, out);
			return qtrue;
		}

	}

	//remove prefix if have it
	ptr = &in[0];
	if (*ptr == '#')
	{
		++ptr;
	}
	else if (*ptr == 'x')
	{
		++ptr;
	}
	else if (ptr[0] == '0' && ptr[1] == 'x')
	{
		ptr += 2;
	}

	//check is characters allowed
	for (i = 0; ptr[i]; ++i)
	{
		if (!((ptr[i] >= '0' && ptr[i] <= '9') || (ptr[i] >= 'a' && ptr[i] <= 'f')))
		{
			//unexpected character
			return qfalse;
		}
	}

	len = strlen(ptr);

	if (len == 0)
	{
		VectorSet(out, 0, 0, 0);
		return qtrue;
	}
	//next len = 1-3 - 24 bit hex
	else if (len >= 1 && len <= 3)
	{
		CG_ParseColorStr12Bit(ptr, &out[0]);
		return qtrue;
	}
	// len 4-6 - 32 bit hex
	else if (len >= 4 && len <= 6)
	{
		CG_ParseColorStr24Bit(ptr, out);
		return qtrue;
	}
	// else if (len == 8)
	// {
	//  CG_ParseColorStr32Bit(ptr, out);
	//  return qtrue;
	// }


	return qfalse;
}

/*
=================
CG_UpdateMyTeam

Команда для цветов T/E = команда игрока, за которым мы смотрим в snapshot.
Играем за собой — это наша команда; наблюдаем за кем-то — команда того, за кем смотрим.
=================
*/
/* Cache for early-out when POV/team unchanged (QVM: avoid per-frame work when nothing changed) */
static int s_lastViewClientNum = -1;
static team_t s_lastViewedTeam = TEAM_FREE;
static team_t s_lastRealTeam = TEAM_FREE;

void CG_UpdateMyTeam(void)
{
	team_t viewedTeam;   /* Team of who we see (for E/T: always this; free spec = RED) */
	team_t realTeam;     /* For cg_myTeam 1/2: only viewed team when spectPOV 1 or we're in a team */
	team_t oldBeMyTeam;
	int viewClientNum;

	if (!cg.snap || cg.snap->ps.clientNum < 0 || cg.snap->ps.clientNum >= MAX_CLIENTS)
	{
		cgs.beMyTeam = TEAM_RED;
		s_lastViewClientNum = -1;
		return;
	}

	viewClientNum = cg.snap->ps.clientNum;
	if (CG_OSPIsGameTypeCA(cgs.gametype))
		viewedTeam = cgs.clientinfo[viewClientNum].rt;
	else
		viewedTeam = cgs.clientinfo[viewClientNum].team;

	if (viewedTeam == TEAM_SPECTATOR)
		viewedTeam = TEAM_RED;  /* Free spec: red = "our" team for E/T */

	/* cg_myTeam 1/2: only apply when spectPOV 1 (or we're in a team). spectPOV 0 = "look at ourselves", no swap */
	realTeam = viewedTeam;
	if (cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS)
	{
		const team_t ourTeam = CG_OSPIsGameTypeCA(cgs.gametype)
			? cgs.clientinfo[cg.clientNum].rt
			: cgs.clientinfo[cg.clientNum].team;
		if (ourTeam == TEAM_SPECTATOR && !cg_spectPOV.integer)
			realTeam = TEAM_SPECTATOR;
	}

	/* Early-out: same view client and teams as last frame — skip UpdateVisualTeamColors and ReloadColors path */
	if (viewClientNum == s_lastViewClientNum && viewedTeam == s_lastViewedTeam && realTeam == s_lastRealTeam)
	{
		cgs.beMyTeam = viewedTeam;
		return;
	}
	s_lastViewClientNum = viewClientNum;
	s_lastViewedTeam = viewedTeam;
	s_lastRealTeam = realTeam;

	oldBeMyTeam = cgs.beMyTeam;
	cgs.beMyTeam = viewedTeam;

	/* Always refresh be/scoreboard from base when in team game */
	if (cgs.gametype >= GT_TEAM)
		CG_UpdateVisualTeamColors(realTeam);

	if (viewedTeam != oldBeMyTeam)
		CG_ReloadColors();
}

/* Cache for CG_UpdateVisualTeamColors: skip recompute when realTeam and cg_myTeam unchanged */
static team_t s_lastVisualRealTeam = TEAM_FREE;
static int s_lastVisualCgMyTeam = -1;

void CG_InvalidateVisualTeamColorsCache(void)
{
	s_lastVisualRealTeam = TEAM_FREE;
	s_lastVisualCgMyTeam = -1;
	s_lastViewClientNum = -1;  /* force CG_UpdateMyTeam full run next frame */
}

/*
=================
CG_UpdateVisualTeamColors

Пересчитывает эффективные командные цвета (cgs.beRedTeamColor / cgs.beBlueTeamColor)
из базовых цветов и текущих настроек cg_myTeam. В неккомандных режимах и для
наблюдателя цвета остаются базовыми.

realTeam - реальная команда игрока (TEAM_RED, TEAM_BLUE, или другая)
=================
*/
void CG_UpdateVisualTeamColors(team_t realTeam)
{
	/* Non-team gametypes: use base colors as-is */
	if (cgs.gametype < GT_TEAM)
	{
		Vector4Copy(cgs.baseRedTeamColor,  cgs.beRedTeamColor);
		Vector4Copy(cgs.baseBlueTeamColor, cgs.beBlueTeamColor);
		return;
	}

	/* Spectator or unknown team: keep base colors */
	if (realTeam != TEAM_RED && realTeam != TEAM_BLUE)
	{
		Vector4Copy(cgs.baseRedTeamColor,  cgs.beRedTeamColor);
		Vector4Copy(cgs.baseBlueTeamColor, cgs.beBlueTeamColor);
		return;
	}

	/* Skip if same inputs as last time (avoids per-frame work when nothing changed) */
	if (realTeam == s_lastVisualRealTeam && cg_myTeam.integer == s_lastVisualCgMyTeam)
		return;
	s_lastVisualRealTeam = realTeam;
	s_lastVisualCgMyTeam = cg_myTeam.integer;

	/* Apply cg_myTeam mapping */
	switch (cg_myTeam.integer)
	{
	default:
	case 0:
		/* Default behavior: no remapping */
		Vector4Copy(cgs.baseRedTeamColor,  cgs.beRedTeamColor);
		Vector4Copy(cgs.baseBlueTeamColor, cgs.beBlueTeamColor);
		break;

	case 1: /* Force "my team" to use red color scheme */
		if (realTeam == TEAM_RED)
		{
			Vector4Copy(cgs.baseRedTeamColor,  cgs.beRedTeamColor);
			Vector4Copy(cgs.baseBlueTeamColor, cgs.beBlueTeamColor);
		}
		else /* realTeam == TEAM_BLUE */
		{
			/* Swap red/blue effective colors */
			Vector4Copy(cgs.baseBlueTeamColor, cgs.beRedTeamColor);
			Vector4Copy(cgs.baseRedTeamColor,  cgs.beBlueTeamColor);
		}
		break;

	case 2: /* Force "my team" to use blue color scheme */
		if (realTeam == TEAM_BLUE)
		{
			Vector4Copy(cgs.baseRedTeamColor,  cgs.beRedTeamColor);
			Vector4Copy(cgs.baseBlueTeamColor, cgs.beBlueTeamColor);
		}
		else /* realTeam == TEAM_RED */
		{
			/* Swap red/blue effective colors */
			Vector4Copy(cgs.baseBlueTeamColor, cgs.beRedTeamColor);
			Vector4Copy(cgs.baseRedTeamColor,  cgs.beBlueTeamColor);
		}
		break;
	}
	
	/* Also update scoreboard base colors (they follow beRedTeamColor/beBlueTeamColor) */
	Vector4Copy(cgs.beRedTeamColor, scoreboard_rtColor);
	Vector4Copy(cgs.beBlueTeamColor, scoreboard_btColor);
}

/*
=================
CG_ReloadColors

Проверяет все cvar с цветами и обновляет те, которые содержат 'E' или 'T'
для динамического обновления цветов при смене команды
=================
*/
void CG_ReloadColors(void)
{
	static const char* colorCvars[] =
	{
		"ch_crosshairColor",
		"ch_crosshairActionColor",
		"ch_crosshairActionColorLow",
		"ch_crosshairActionColorMid",
		"ch_crosshairActionColorHigh",
		"ch_crosshairDecorColor",
		"ch_crosshairDecorActionColor",
		"ch_crosshairDecorActionColorLow",
		"ch_crosshairDecorActionColorMid",
		"ch_crosshairDecorActionColorHigh",
		"cg_hitBoxColor",
		"cg_enemyOutlineColor",
		"cg_teamOutlineColor",
		"cg_altBloodColor",
		"cg_altShadowColor",
		"cg_altGrenadesColor",
		"cg_enemyGrenadesColor",
		"cg_healthColor",
		"cg_healthLowColor",
		"cg_healthMidColor",
		"cg_redTeamColor",
		"cg_blueTeamColor",
		"cg_markTeamColor",
		"cg_scoreboardRtColors",
		"cg_scoreboardBtColors",
		"cg_scoreboardSpecColor",
		"cg_bestats_bgColor",
		"cg_teamTaskThemeColor",
		"cg_teamIndicatorColor",
		"cg_teamIndicatorBgColor",
		"cg_brightWeaponsColorGauntlet",
		"cg_brightWeaponsColorMG",
		"cg_brightWeaponsColorSG",
		"cg_brightWeaponsColorGL",
		"cg_brightWeaponsColorRL",
		"cg_brightWeaponsColorLG",
		"cg_brightWeaponsColorRG",
		"cg_brightWeaponsColorPG",
		"cg_brightWeaponsColorBFG",
		NULL
	};

	const char** cvarName;
	char cvarValue[MAX_CVAR_VALUE_STRING];
	int i;
	qboolean needsUpdate;

	cvarName = &colorCvars[0];
	while (*cvarName)
	{
		// Get cvar value
		trap_Cvar_VariableStringBuffer(*cvarName, cvarValue, sizeof(cvarValue));

		// Check if value contains 'E' or 'T' (uppercase only, lowercase 'e' and 't' are color codes)
		needsUpdate = qfalse;
		for (i = 0; cvarValue[i]; ++i)
		{
			if (cvarValue[i] == 'E' || cvarValue[i] == 'T')
			{
				needsUpdate = qtrue;
				break;
			}
		}

		if (needsUpdate)
		{
			CG_CvarTouch(*cvarName);
		}

		++cvarName;
	}
}





