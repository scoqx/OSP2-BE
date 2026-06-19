/*
===========================================================================
cg_maplocs.c

Map zones for tinfo -> world coords (cg_friendsWallhack bit 4).

  locs/<map>.qloc64  — QLZ1 + url-safe base64(QLM1), optional per-map override
  locs/locs.q3lb     — Q3LB bundle (Q3 locs bundle): many maps, raw QLM1 (see tools)

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_MAPLOC_NAME_LEN      64
#define CG_MAPLOC_QLZ64_MAX     (96 * 1024)
#define CG_MAPLOC_BUNDLE_PATH   "locs/locs.q3lb"
#define CG_MAPLOC_QLM_PAYLOAD_MAX (16 * 1024)
#define CG_MAPLOC_BUNDLE_MAX_MAPS 512

typedef struct
{
	int     id;
	int     color;
	vec3_t  origin;
	char    name[CG_MAPLOC_NAME_LEN];
} cg_mapLoc_t;

static cg_mapLoc_t cg_mapLocs[MAX_LOCATIONS];
static int cg_mapLocCount;
static char cg_mapLocsSource[MAX_QPATH];
static qboolean cg_mapLocOriginValid[MAX_LOCATIONS];
static vec3_t cg_mapLocOriginById[MAX_LOCATIONS];
static qboolean cg_mapLocServerNamesCached;
static qboolean cg_mapLocServerNamesPresent;

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

static void CG_MapLocs_Clear(void)
{
	cg_mapLocCount = 0;
	cg_mapLocsSource[0] = '\0';
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));
	CG_MapLocs_InvalidateServerLocationNamesCache();
}

static void CG_MapLocs_GetMapBaseName(char* out, int outSize)
{
	const char* map;
	const char* slash;
	const char* base;

	out[0] = '\0';
	map = cgs.mapname;
	if (!map || !map[0])
	{
		map = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "mapname");
	}
	if (!map || !map[0])
	{
		return;
	}

	base = map;
	slash = Q_strrchr(map, '/');
	if (slash && slash[1])
	{
		base = slash + 1;
	}

	Q_strncpyz(out, base, outSize);
	if (outSize > 4)
	{
		int len;

		len = strlen(out);
		if (len > 4 && !Q_stricmp(out + len - 4, ".bsp"))
		{
			out[len - 4] = '\0';
		}
	}
}

static int CG_MapLocs_B64Value(char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c - 'A';
	}
	if (c >= 'a' && c <= 'z')
	{
		return c - 'a' + 26;
	}
	if (c >= '0' && c <= '9')
	{
		return c - '0' + 52;
	}
	if (c == '-')
	{
		return 62;
	}
	if (c == '_')
	{
		return 63;
	}
	return -1;
}

static qboolean CG_MapLocs_MapNameMatchesBundle(const char* mapBase, const byte* name, int nameLen)
{
	char    entryName[MAX_QPATH];
	char    altName[MAX_QPATH];

	if (!mapBase || !mapBase[0] || !name || nameLen <= 0 || nameLen >= MAX_QPATH)
	{
		return qfalse;
	}

	memcpy(entryName, name, nameLen);
	entryName[nameLen] = '\0';

	if (!Q_stricmp(mapBase, entryName))
	{
		return qtrue;
	}
	if (!Q_stricmpn(mapBase, "map-", 4) && !Q_stricmp(mapBase + 4, entryName))
	{
		return qtrue;
	}
	Com_sprintf(altName, sizeof(altName), "map-%s", mapBase);
	if (!Q_stricmp(altName, entryName))
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean CG_MapLocs_LoadQLM1Binary(const byte* raw, int rawLen)
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

		if (nameLen <= 0 || pos + nameLen > rawLen)
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

static qboolean CG_MapLocs_DecodeQLZ1Base64(const char* b64, int b64Len, byte** out, int* outLen)
{
	byte* decoded;
	int   decodedLen;
	int   i;
	int   v[4];
	int   outPos;

	*out = NULL;
	*outLen = 0;

	if (b64Len <= 0)
	{
		return qfalse;
	}

	decoded = (byte*)Z_Malloc((b64Len * 3) / 4 + 4);
	if (!decoded)
	{
		return qfalse;
	}

	decodedLen = 0;
	outPos = 0;
	for (i = 0; i < b64Len && b64[i]; i++)
	{
		if (b64[i] == '=' || b64[i] == ' ' || b64[i] == '\t' || b64[i] == '\r' || b64[i] == '\n')
		{
			continue;
		}
		v[outPos & 3] = CG_MapLocs_B64Value(b64[i]);
		if (v[outPos & 3] < 0)
		{
			continue;
		}
		outPos++;
		if (outPos == 4)
		{
			decoded[decodedLen++] = (byte)((v[0] << 2) | (v[1] >> 4));
			decoded[decodedLen++] = (byte)((v[1] << 4) | (v[2] >> 2));
			decoded[decodedLen++] = (byte)((v[2] << 6) | v[3]);
			outPos = 0;
		}
	}
	if (outPos == 2)
	{
		decoded[decodedLen++] = (byte)((v[0] << 2) | (v[1] >> 4));
	}
	else if (outPos == 3)
	{
		decoded[decodedLen++] = (byte)((v[0] << 2) | (v[1] >> 4));
		decoded[decodedLen++] = (byte)((v[1] << 4) | (v[2] >> 2));
	}

	if (decodedLen < 4)
	{
		Z_Free(decoded);
		return qfalse;
	}

	*out = decoded;
	*outLen = decodedLen;
	return qtrue;
}

static qboolean CG_MapLocs_LoadQLoc64File(const char* path)
{
	fileHandle_t fh;
	int fileSize;
	char* fileContent;
	char* p;
	byte* decoded;
	int decodedLen;
	qboolean ok;

	fileSize = trap_FS_FOpenFile(path, &fh, FS_READ);
	if (fileSize <= 0 || !fh)
	{
		return qfalse;
	}
	if (fileSize > CG_MAPLOC_QLZ64_MAX)
	{
		trap_FS_FCloseFile(fh);
		return qfalse;
	}

	fileContent = (char*)Z_Malloc(fileSize + 1);
	if (!fileContent)
	{
		trap_FS_FCloseFile(fh);
		return qfalse;
	}

	trap_FS_Read(fileContent, fileSize, fh);
	trap_FS_FCloseFile(fh);
	fileContent[fileSize] = '\0';

	if (Q_strncmp(fileContent, "QLZ1 ", 5))
	{
		Z_Free(fileContent);
		return qfalse;
	}

	p = fileContent + 5;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	{
		p++;
	}

	if (!CG_MapLocs_DecodeQLZ1Base64(p, fileSize - (int)(p - fileContent), &decoded, &decodedLen))
	{
		Z_Free(fileContent);
		return qfalse;
	}

	Z_Free(fileContent);

	ok = CG_MapLocs_LoadQLM1Binary(decoded, decodedLen);
	Z_Free(decoded);
	if (!ok)
	{
		return qfalse;
	}

	Q_strncpyz(cg_mapLocsSource, path, sizeof(cg_mapLocsSource));
	return qtrue;
}

static int CG_MapLocs_ReadLE32(const byte* p)
{
	return (int)((unsigned)p[0] |
	             ((unsigned)p[1] << 8) |
	             ((unsigned)p[2] << 16) |
	             ((unsigned)p[3] << 24));
}

static void CG_MapLocs_SkipBytes(fileHandle_t fh, int bytes)
{
	byte    scratch[1024];
	int     chunk;

	while (bytes > 0)
	{
		chunk = bytes;
		if (chunk > (int)sizeof(scratch))
		{
			chunk = (int)sizeof(scratch);
		}
		trap_FS_Read(scratch, chunk, fh);
		bytes -= chunk;
	}
}

static qboolean CG_MapLocs_LoadFromBundle(const char* mapBase)
{
	fileHandle_t    fh;
	int             fileSize;
	byte            header[6];
	int             nMaps;
	int             i;
	byte            nameLen;
	byte            nameBuf[MAX_QPATH];
	byte            lenBuf[4];
	int             qlmLen;
	int             bundleOffset;
	byte*           payload;
	qboolean        ok;

	if (!mapBase || !mapBase[0])
	{
		return qfalse;
	}

	fileSize = trap_FS_FOpenFile(CG_MAPLOC_BUNDLE_PATH, &fh, FS_READ);
	if (fileSize <= 0 || !fh)
	{
		return qfalse;
	}

	trap_FS_Read(header, 6, fh);

	if (header[0] != 'Q' || header[1] != '3' || header[2] != 'L' || header[3] != 'B')
	{
		trap_FS_FCloseFile(fh);
		return qfalse;
	}

	nMaps = header[4] | (header[5] << 8);
	if (nMaps <= 0 || nMaps > CG_MAPLOC_BUNDLE_MAX_MAPS)
	{
		trap_FS_FCloseFile(fh);
		return qfalse;
	}

	bundleOffset = 6;

	for (i = 0; i < nMaps; i++)
	{
		if (bundleOffset + 1 > fileSize)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}
		trap_FS_Read(&nameLen, 1, fh);
		bundleOffset += 1;
		if (nameLen <= 0 || nameLen >= MAX_QPATH)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}
		if (bundleOffset + nameLen + 4 > fileSize)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}
		trap_FS_Read(nameBuf, nameLen, fh);
		trap_FS_Read(lenBuf, 4, fh);
		bundleOffset += nameLen + 4;

		qlmLen = CG_MapLocs_ReadLE32(lenBuf);
		if (qlmLen <= 0 || bundleOffset + qlmLen > fileSize)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}

		if (CG_MapLocs_MapNameMatchesBundle(mapBase, nameBuf, nameLen))
		{
			if (qlmLen > CG_MAPLOC_QLM_PAYLOAD_MAX)
			{
				trap_FS_FCloseFile(fh);
				return qfalse;
			}

			payload = (byte*)Z_Malloc(qlmLen);
			if (!payload)
			{
				trap_FS_FCloseFile(fh);
				return qfalse;
			}

			trap_FS_Read(payload, qlmLen, fh);
			bundleOffset += qlmLen;

			ok = CG_MapLocs_LoadQLM1Binary(payload, qlmLen);
			Z_Free(payload);
			trap_FS_FCloseFile(fh);

			if (ok)
			{
				Com_sprintf(cg_mapLocsSource, sizeof(cg_mapLocsSource),
				            "%s (%s)", CG_MAPLOC_BUNDLE_PATH, mapBase);
				return qtrue;
			}
			return qfalse;
		}

		CG_MapLocs_SkipBytes(fh, qlmLen);
		bundleOffset += qlmLen;
	}

	trap_FS_FCloseFile(fh);
	return qfalse;
}

qboolean CG_MapLocs_Load(void)
{
	char mapBase[MAX_QPATH];
	char path[MAX_QPATH];

	CG_MapLocs_Clear();
	CG_MapLocs_GetMapBaseName(mapBase, sizeof(mapBase));
	if (!mapBase[0])
	{
		return qfalse;
	}

	Com_sprintf(path, sizeof(path), "locs/%s.qloc64", mapBase);
	if (CG_MapLocs_LoadQLoc64File(path))
	{
		return qtrue;
	}

	if (CG_MapLocs_LoadFromBundle(mapBase))
	{
		return qtrue;
	}

	return qfalse;
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

void CG_MapLocs_f(void)
{
	char mapBase[MAX_QPATH];
	const char* csName;
	int i;
	int showId;
	qboolean foundOne;

	if (trap_Argc() >= 2)
	{
		showId = atoi(CG_Argv(1));
	}
	else
	{
		showId = 0;
	}

	CG_MapLocs_GetMapBaseName(mapBase, sizeof(mapBase));
	if (!mapBase[0])
	{
		CG_Printf("^1maplocs: no map name\n");
		return;
	}

	if (!CG_MapLocs_Load())
	{
		CG_Printf("^1maplocs: no data in ^3locs/%s.qloc64^7 or ^3%s^7\n",
		          mapBase, CG_MAPLOC_BUNDLE_PATH);
		return;
	}

	CG_Printf("^3maplocs ^7from ^2%s^7 - ^2%i^7 zones ([CS] = server tinfo names)\n",
	          cg_mapLocsSource,
	          cg_mapLocCount);

	foundOne = qfalse;
	for (i = 0; i < cg_mapLocCount; i++)
	{
		if (showId && cg_mapLocs[i].id != showId)
		{
			continue;
		}
		foundOne = qtrue;
		csName = CG_ConfigString(CS_LOCATIONS + cg_mapLocs[i].id);
		CG_Printf("  ^3%2d^7  ^5%.0f %.0f %.0f^7  %s%s^7\n",
		          cg_mapLocs[i].id,
		          cg_mapLocs[i].origin[0],
		          cg_mapLocs[i].origin[1],
		          cg_mapLocs[i].origin[2],
		          (csName && csName[0]) ? "^2[CS] " : "",
		          cg_mapLocs[i].name);
	}
	if (!foundOne && showId)
	{
		CG_Printf("^1maplocs: no zone with id %i\n", showId);
	}
}
