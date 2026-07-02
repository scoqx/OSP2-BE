/*
===========================================================================
cg_maplocs.c

Map zones for tinfo -> world coords (cg_friendsWallhack bit 4).
Data: QLM1 section in mapdata Q3LB (mapdata/mapdata.q3lb or mapdata/<map>.q3lb).

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_MAPLOC_NAME_LEN      64

typedef struct
{
	int     id;
	int     color;
	vec3_t  origin;
	char    name[CG_MAPLOC_NAME_LEN];
} cg_mapLoc_t;

static cg_mapLoc_t cg_mapLocs[MAX_LOCATIONS];
static int cg_mapLocCount;
static qboolean cg_mapLocOriginValid[MAX_LOCATIONS];
static vec3_t cg_mapLocOriginById[MAX_LOCATIONS];
static qboolean cg_mapLocServerNamesCached;
static qboolean cg_mapLocServerNamesPresent;

// i hate this map
static const char cg_q3shw14LocCfg[] =
	"2338 617 -63 ^5MEGA\n"
	"685 -808 56 ^3YA\n"
	"2176 -826 -67 ^7long\n"
	"98 -66 -67 ^1LAVA\n"
	"1101 1520 528 ^1RED\n"
	"1251 484 492 EYE\n"
	"1258 401 492 EYE\n"
	"1976 186 492 ^5BIG JUMP\n"
	"924 -745 408 ^72st floor\n"
	"1015 -1650 212 exit TP\n"
	"-267 343 -263 ^1TP LAVA\n"
	"1758 -291 -3 diagonal\n"
	"1293 301 -71 1st floor\n"
	"973 871 188 snipe\n"
	"1889 1260 188 ^5MEGA snipe\n"
	"1228 1260 188 snipe\n"
	"1372 -1227 -55 ^850HP\n"
	"1129 -390 -55 1st floor\n"
	"44 -705 -67 ^3YA LOWER\n"
	"145 -1054 -67 ^3YA LOWER\n"
	"153 -432 -67 ^1LAVA\n"
	"620 460 492 behind EYE\n"
	"1369 -303 408 2st floor\n"
	"1264 -1190 308 exit TP\n"
	"719 -1116 323 exit TP\n"
	"1037 -992 56 ^3YA\n"
	"1412 -601 -55 ^850HP/^3YA\n"
	"1412 -601 -55 ^850HP^7/^3YA\n"
	"2224 -156 -67 long\n"
	"1047 1085 492 ^1RED\n"
	"955 891 492 EYE\n"
	"1288 928 492 EYE\n"
	"1975 629 820 hidden bones\n";

static void CG_MapLocs_InvalidateServerLocationNamesCache(void)
{
	cg_mapLocServerNamesCached = qfalse;
}

static void CG_MapLocs_RefreshServerLocationNamesCache(void)
{
	int i;
	const char* p;

	cg_mapLocServerNamesPresent = qfalse;
	for (i = 1; i < MAX_LOCATIONS; i++)
	{
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && p[0])
		{
			cg_mapLocServerNamesPresent = qtrue;
			break;
		}
	}
	cg_mapLocServerNamesCached = qtrue;
}

void CG_MapLocs_NotifyLocationConfigStringChanged(void)
{
	CG_MapLocs_InvalidateServerLocationNamesCache();
}

static qboolean CG_MapLocs_LoadCfgText(const char* fileContent)
{
	const char* ptr;
	char        line[1024];
	int         matches;
	const char* name;
	int         id;

	if (!fileContent || !fileContent[0])
	{
		return qfalse;
	}

	cg_mapLocCount = 0;
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));

	ptr = fileContent;
	do
	{
		ptr = CG_LoadLine(ptr, line, sizeof(line));
		if (!line[0] || line[0] == '/')
		{
			continue;
		}

		id = cg_mapLocCount + 1;
		if (id <= 0 || id >= MAX_LOCATIONS || cg_mapLocCount >= MAX_LOCATIONS)
		{
			break;
		}

		matches = Q_sscanf(line, "%f %f %f %64s",
		                   &cg_mapLocs[cg_mapLocCount].origin[0],
		                   &cg_mapLocs[cg_mapLocCount].origin[1],
		                   &cg_mapLocs[cg_mapLocCount].origin[2],
		                   cg_mapLocs[cg_mapLocCount].name);
		name = strstr(line, cg_mapLocs[cg_mapLocCount].name);
		if (name)
		{
			Q_strncpyz(cg_mapLocs[cg_mapLocCount].name, name, CG_MAPLOC_NAME_LEN);
		}
		if (matches != 4)
		{
			continue;
		}

		cg_mapLocs[cg_mapLocCount].id = id;
		cg_mapLocs[cg_mapLocCount].color = 0;
		cg_mapLocOriginById[id][0] = cg_mapLocs[cg_mapLocCount].origin[0];
		cg_mapLocOriginById[id][1] = cg_mapLocs[cg_mapLocCount].origin[1];
		cg_mapLocOriginById[id][2] = cg_mapLocs[cg_mapLocCount].origin[2];
		cg_mapLocOriginValid[id] = qtrue;
		cg_mapLocCount++;
	}
	while (*ptr);

	return (cg_mapLocCount > 0) ? qtrue : qfalse;
}

static qboolean CG_MapLocs_LoadEmbeddedForMap(void)
{
	char mapBase[MAX_QPATH];

	CG_MapData_GetMapBaseName(mapBase, sizeof(mapBase));
	if (!mapBase[0])
	{
		return qfalse;
	}
	if (Q_stricmp(mapBase, "q3shw14"))
	{
		return qfalse;
	}
	return CG_MapLocs_LoadCfgText(cg_q3shw14LocCfg);
}

static void CG_MapLocs_Clear(void)
{
	cg_mapLocCount = 0;
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));
	CG_MapLocs_InvalidateServerLocationNamesCache();
}

qboolean CG_MapLocs_LoadQLM1Binary(const byte* raw, int rawLen)
{
	int pos;
	int mapLen;
	int count;
	int i;
	int id;
	float ox;
	float oy;
	float oz;
	int color;
	int nameLen;
	int copyLen;

	if (rawLen < 6 || raw[0] != 'Q' || raw[1] != 'L' || raw[2] != 'M' || raw[3] != '1')
	{
		return qfalse;
	}

	pos = 4;
	mapLen = raw[pos++];
	if (mapLen <= 0 || pos + mapLen > rawLen)
	{
		return qfalse;
	}
	pos += mapLen;

	if (pos >= rawLen)
	{
		return qfalse;
	}
	count = raw[pos++];
	if (count <= 0)
	{
		return qfalse;
	}

	cg_mapLocCount = 0;
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));

	for (i = 0; i < count; i++)
	{
		if (pos + 15 > rawLen)
		{
			break;
		}

		id = raw[pos++];
		{
			int bits;

			memcpy(&bits, raw + pos, 4);
			ox = *(float*)&bits;
			pos += 4;
			memcpy(&bits, raw + pos, 4);
			oy = *(float*)&bits;
			pos += 4;
			memcpy(&bits, raw + pos, 4);
			oz = *(float*)&bits;
			pos += 4;
		}
		color = raw[pos++];
		nameLen = raw[pos++];

		if (pos + nameLen > rawLen)
		{
			break;
		}

		if (id <= 0 || id >= MAX_LOCATIONS || cg_mapLocCount >= MAX_LOCATIONS)
		{
			pos += nameLen;
			continue;
		}

		cg_mapLocs[cg_mapLocCount].id = id;
		cg_mapLocs[cg_mapLocCount].color = color;
		cg_mapLocs[cg_mapLocCount].origin[0] = ox;
		cg_mapLocs[cg_mapLocCount].origin[1] = oy;
		cg_mapLocs[cg_mapLocCount].origin[2] = oz;
		copyLen = nameLen;
		if (copyLen >= CG_MAPLOC_NAME_LEN)
		{
			copyLen = CG_MAPLOC_NAME_LEN - 1;
		}
		memcpy(cg_mapLocs[cg_mapLocCount].name, raw + pos, copyLen);
		cg_mapLocs[cg_mapLocCount].name[copyLen] = '\0';
		pos += nameLen;
		cg_mapLocOriginById[id][0] = ox;
		cg_mapLocOriginById[id][1] = oy;
		cg_mapLocOriginById[id][2] = oz;
		cg_mapLocOriginValid[id] = qtrue;
		cg_mapLocCount++;
	}

	return (cg_mapLocCount > 0) ? qtrue : qfalse;
}

qboolean CG_MapLocs_Load(void)
{
	const byte* qlm;
	int         qlmLen;

	CG_MapLocs_Clear();
	if (CG_MapLocs_LoadEmbeddedForMap())
	{
		return qtrue;
	}
	CG_MapLocs_Clear();
	if (!CG_MapBundle_Section("QLM1", &qlm, &qlmLen))
	{
		return qfalse;
	}
	return CG_MapLocs_LoadQLM1Binary(qlm, qlmLen);
}

qboolean CG_MapLocs_GetById(int id, vec3_t origin, char* name, int nameSize)
{
	int i;

	if (id <= 0 || id >= MAX_LOCATIONS)
	{
		return qfalse;
	}

	if (origin && cg_mapLocOriginValid[id])
	{
		VectorCopy(cg_mapLocOriginById[id], origin);
		if (!name || nameSize <= 0)
		{
			return qtrue;
		}
	}

	for (i = 0; i < cg_mapLocCount; i++)
	{
		if (cg_mapLocs[i].id == id)
		{
			if (origin)
			{
				VectorCopy(cg_mapLocs[i].origin, origin);
			}
			if (name && nameSize > 0)
			{
				Q_strncpyz(name, cg_mapLocs[i].name, nameSize);
			}
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_MapLocs_Count(void)
{
	return (cg_mapLocCount > 0) ? qtrue : qfalse;
}

qboolean CG_MapLocs_ServerHasLocationNames(void)
{
	if (!cg_mapLocServerNamesCached)
	{
		CG_MapLocs_RefreshServerLocationNamesCache();
	}
	return cg_mapLocServerNamesPresent;
}

int CG_MapLocs_NumZones(void)
{
	return cg_mapLocCount;
}

const char* CG_MapLocs_DataSource(void)
{
	if (!cg_mapLocCount)
	{
		return NULL;
	}
	return CG_MapBundle_DataSource();
}

void CG_MapLocs_PrintList(int maxList)
{
	int i;
	int n;

	if (!cg_mapLocCount)
	{
		CG_Printf("  ^1no location zones loaded^7\n");
		return;
	}

	n = cg_mapLocCount;
	if (maxList > 0 && maxList < n)
	{
		n = maxList;
	}

	CG_Printf("  ^3locations^7 (%i):\n", cg_mapLocCount);
	for (i = 0; i < n; i++)
	{
		const char* csName;

		csName = CG_ConfigString(CS_LOCATIONS + cg_mapLocs[i].id);
		CG_Printf("  id ^3%2d^7  ^5%.0f %.0f %.0f^7  %s%s^7\n",
		          cg_mapLocs[i].id,
		          cg_mapLocs[i].origin[0],
		          cg_mapLocs[i].origin[1],
		          cg_mapLocs[i].origin[2],
		          (csName && csName[0]) ? "^2[CS] " : "",
		          cg_mapLocs[i].name);
	}
	if (n < cg_mapLocCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list locs <count>)\n", cg_mapLocCount - n);
	}
}
