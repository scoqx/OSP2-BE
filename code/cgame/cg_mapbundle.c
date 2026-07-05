/*
===========================================================================
cg_mapbundle.c

Unified map bundle: mapdata/<map>.q3lb overrides mapdata/mapdata.q3lb when present.
  Per-map file is tried first (local testing); then the main Q3LB pack.
  QSP1 spawns, QITM1 shadow pickups, QLM1 locations, QTX1 teleporter exits (QMAP v2).

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_MAPBUNDLE_PATH       "mapdata/mapdata.q3lb"
#define CG_MAPBUNDLE_PERMAP     "mapdata/%s.q3lb"
#define CG_MAPBUNDLE_PAYLOAD_MAX (64 * 1024)

static byte*    cg_mapBundlePayload;
static int      cg_mapBundlePayloadLen;
static char     cg_mapBundleMap[MAX_QPATH];
static char     cg_mapBundleSource[MAX_QPATH];

static int CG_MapBundle_ReadLE32(const byte* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static qboolean CG_MapBundle_MapNameMatches(const char* mapBase, const byte* name, int nameLen)
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

static void CG_MapBundle_ClearPayload(void)
{
	if (cg_mapBundlePayload)
	{
		Z_Free(cg_mapBundlePayload);
		cg_mapBundlePayload = NULL;
	}
	cg_mapBundlePayloadLen = 0;
	cg_mapBundleMap[0] = '\0';
	cg_mapBundleSource[0] = '\0';
}

void CG_MapData_GetMapBaseName(char* out, int outSize)
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

static qboolean CG_MapBundle_StorePayload(const byte* raw, int rawLen, const char* mapBase, const char* sourcePath)
{
	CG_MapBundle_ClearPayload();
	if (rawLen <= 0 || rawLen > CG_MAPBUNDLE_PAYLOAD_MAX)
	{
		return qfalse;
	}
	cg_mapBundlePayload = (byte*)Z_Malloc(rawLen);
	if (!cg_mapBundlePayload)
	{
		return qfalse;
	}
	memcpy(cg_mapBundlePayload, raw, rawLen);
	cg_mapBundlePayloadLen = rawLen;
	Q_strncpyz(cg_mapBundleMap, mapBase, sizeof(cg_mapBundleMap));
	if (sourcePath && sourcePath[0])
	{
		Q_strncpyz(cg_mapBundleSource, sourcePath, sizeof(cg_mapBundleSource));
	}
	return qtrue;
}

static qboolean CG_MapBundle_ParseHeader(
        const byte* raw,
        int rawLen,
        int* qspLen,
        int* qitmLen,
        int* qlmLen,
        int* qtxLen,
        int* headerSize)
{
	int     version;

	if (rawLen < 18 || raw[0] != 'Q' || raw[1] != 'M' || raw[2] != 'A' || raw[3] != 'P')
	{
		return qfalse;
	}

	version = raw[4] | (raw[5] << 8);
	if (version != 1 && version != 2)
	{
		return qfalse;
	}

	*qspLen = CG_MapBundle_ReadLE32(raw + 6);
	*qitmLen = CG_MapBundle_ReadLE32(raw + 10);
	*qlmLen = CG_MapBundle_ReadLE32(raw + 14);
	if (version >= 2)
	{
		if (rawLen < 22)
		{
			return qfalse;
		}
		*qtxLen = CG_MapBundle_ReadLE32(raw + 18);
		*headerSize = 22;
	}
	else
	{
		*qtxLen = 0;
		*headerSize = 18;
	}

	if (*qspLen < 0 || *qitmLen < 0 || *qlmLen < 0 || *qtxLen < 0)
	{
		return qfalse;
	}
	if (*headerSize + *qspLen + *qitmLen + *qlmLen + *qtxLen > rawLen)
	{
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_MapBundle_ParseQMAP(const byte* raw, int rawLen)
{
	int     qspLen;
	int     qitmLen;
	int     qlmLen;
	int     qtxLen;
	int     headerSize;

	return CG_MapBundle_ParseHeader(raw, rawLen, &qspLen, &qitmLen, &qlmLen, &qtxLen, &headerSize);
}

static qboolean CG_MapBundle_PayloadHasContent(const byte* raw, int rawLen)
{
	int     qspLen;
	int     qitmLen;
	int     qlmLen;
	int     qtxLen;
	int     headerSize;

	if (!CG_MapBundle_ParseHeader(raw, rawLen, &qspLen, &qitmLen, &qlmLen, &qtxLen, &headerSize))
	{
		return qfalse;
	}
	return (qspLen > 0 || qitmLen > 0 || qlmLen > 0 || qtxLen > 0);
}

static void CG_MapBundle_SkipBytes(fileHandle_t fh, int bytes)
{
	byte    scratch[256];
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

static qboolean CG_MapBundle_LoadFromBundleFile(const char* bundlePath, const char* mapBase)
{
	fileHandle_t    fh;
	int             fileSize;
	byte            header[6];
	int             nMaps;
	int             i;
	byte            nameLen;
	byte            nameBuf[MAX_QPATH];
	byte            lenBuf[4];
	int             blobLen;

	fileSize = trap_FS_FOpenFile(bundlePath, &fh, FS_READ);
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
	if (nMaps <= 0 || nMaps > CG_MAPDATA_Q3LB_MAX_MAPS)
	{
		trap_FS_FCloseFile(fh);
		return qfalse;
	}

	for (i = 0; i < nMaps; i++)
	{
		trap_FS_Read(&nameLen, 1, fh);
		if (nameLen <= 0 || nameLen >= MAX_QPATH)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}
		trap_FS_Read(nameBuf, nameLen, fh);
		trap_FS_Read(lenBuf, 4, fh);
		blobLen = CG_MapBundle_ReadLE32(lenBuf);
		if (blobLen <= 0 || blobLen > CG_MAPBUNDLE_PAYLOAD_MAX)
		{
			trap_FS_FCloseFile(fh);
			return qfalse;
		}

		if (CG_MapBundle_MapNameMatches(mapBase, nameBuf, nameLen))
		{
			byte*   payload;
			qboolean ok;

			payload = (byte*)Z_Malloc(blobLen);
			if (!payload)
			{
				trap_FS_FCloseFile(fh);
				return qfalse;
			}
			trap_FS_Read(payload, blobLen, fh);
			ok = CG_MapBundle_ParseQMAP(payload, blobLen)
			     && CG_MapBundle_StorePayload(payload, blobLen, mapBase, bundlePath);
			Z_Free(payload);
			trap_FS_FCloseFile(fh);
			return ok;
		}

		CG_MapBundle_SkipBytes(fh, blobLen);
	}

	trap_FS_FCloseFile(fh);
	return qfalse;
}

static qboolean CG_MapBundle_LoadPerMapQ3lb(const char* mapBase)
{
	char    path[MAX_QPATH];

	Com_sprintf(path, sizeof(path), CG_MAPBUNDLE_PERMAP, mapBase);
	if (CG_MapBundle_LoadFromBundleFile(path, mapBase)
	        && CG_MapBundle_PayloadHasContent(cg_mapBundlePayload, cg_mapBundlePayloadLen))
	{
		return qtrue;
	}

	CG_MapBundle_ClearPayload();
	return qfalse;
}

void CG_MapBundle_Load(void)
{
	char    mapBase[MAX_QPATH];

	CG_MapData_GetMapBaseName(mapBase, sizeof(mapBase));
	if (!mapBase[0])
	{
		CG_MapBundle_ClearPayload();
		return;
	}

	if (!Q_stricmp(cg_mapBundleMap, mapBase) && cg_mapBundlePayload)
	{
		return;
	}

	CG_MapBundle_ClearPayload();

	if (CG_MapBundle_LoadPerMapQ3lb(mapBase))
	{
		return;
	}

	CG_MapBundle_ClearPayload();
	if (CG_MapBundle_LoadFromBundleFile(CG_MAPBUNDLE_PATH, mapBase)
	        && CG_MapBundle_PayloadHasContent(cg_mapBundlePayload, cg_mapBundlePayloadLen))
	{
		return;
	}

	CG_MapBundle_ClearPayload();
}

qboolean CG_MapBundle_HasPayload(void)
{
	return cg_mapBundlePayload != NULL && cg_mapBundlePayloadLen > 0;
}

const byte* CG_MapBundle_Raw(int* outLen)
{
	if (outLen)
	{
		*outLen = cg_mapBundlePayloadLen;
	}
	return cg_mapBundlePayload;
}

qboolean CG_MapBundle_Section(const char* magic4, const byte** out, int* outLen)
{
	const byte* raw;
	int         rawLen;
	int         qspLen;
	int         qitmLen;
	int         qlmLen;
	int         qtxLen;
	int         headerSize;
	int         pos;

	*out = NULL;
	if (outLen)
	{
		*outLen = 0;
	}

	if (!cg_mapBundlePayload || cg_mapBundlePayloadLen < 18)
	{
		return qfalse;
	}

	raw = cg_mapBundlePayload;
	rawLen = cg_mapBundlePayloadLen;
	if (!CG_MapBundle_ParseHeader(raw, rawLen, &qspLen, &qitmLen, &qlmLen, &qtxLen, &headerSize))
	{
		return qfalse;
	}

	pos = headerSize;

	if (!Q_strncmp(magic4, "QSP1", 4))
	{
		if (qspLen <= 0 || pos + qspLen > rawLen)
		{
			return qfalse;
		}
		*out = raw + pos;
		if (outLen)
		{
			*outLen = qspLen;
		}
		return qtrue;
	}

	pos += qspLen;
	if (!Q_strncmp(magic4, "QITM", 4))
	{
		if (qitmLen <= 0 || pos + qitmLen > rawLen)
		{
			return qfalse;
		}
		*out = raw + pos;
		if (outLen)
		{
			*outLen = qitmLen;
		}
		return qtrue;
	}

	pos += qitmLen;
	if (!Q_strncmp(magic4, "QLM1", 4))
	{
		if (qlmLen <= 0 || pos + qlmLen > rawLen)
		{
			return qfalse;
		}
		*out = raw + pos;
		if (outLen)
		{
			*outLen = qlmLen;
		}
		return qtrue;
	}

	pos += qlmLen;
	if (!Q_strncmp(magic4, "QTX1", 4))
	{
		if (qtxLen <= 0 || pos + qtxLen > rawLen)
		{
			return qfalse;
		}
		*out = raw + pos;
		if (outLen)
		{
			*outLen = qtxLen;
		}
		return qtrue;
	}

	return qfalse;
}

/*
==================
CG_MapBundle_LoadAllMapData

Single entry: load mapdata Q3LB, then parse QMAP sections for each feature.
==================
*/
void CG_MapBundle_LoadAllMapData(void)
{
	CG_MapBundle_Load();
	CG_MapLocs_Load();
	CG_MapSpawns_Load();
	CG_ShadowItems_Load();
	CG_MapTPExit_Load();
	CG_WorldFlags_Reset();
}

qboolean CG_MapBundle_QuerySections(int* version, int* qspLen, int* qitmLen, int* qlmLen, int* qtxLen)
{
	const byte* raw;
	int         rawLen;
	int         hdrSize;
	int         ver;
	int         qsp;
	int         qitm;
	int         qlm;
	int         qtx;

	if (version)
	{
		*version = 0;
	}
	if (qspLen)
	{
		*qspLen = 0;
	}
	if (qitmLen)
	{
		*qitmLen = 0;
	}
	if (qlmLen)
	{
		*qlmLen = 0;
	}
	if (qtxLen)
	{
		*qtxLen = 0;
	}

	if (!cg_mapBundlePayload || cg_mapBundlePayloadLen < 18)
	{
		return qfalse;
	}

	raw = cg_mapBundlePayload;
	rawLen = cg_mapBundlePayloadLen;
	ver = raw[4] | (raw[5] << 8);
	if (!CG_MapBundle_ParseHeader(raw, rawLen, &qsp, &qitm, &qlm, &qtx, &hdrSize))
	{
		return qfalse;
	}

	if (version)
	{
		*version = ver;
	}
	if (qspLen)
	{
		*qspLen = qsp;
	}
	if (qitmLen)
	{
		*qitmLen = qitm;
	}
	if (qlmLen)
	{
		*qlmLen = qlm;
	}
	if (qtxLen)
	{
		*qtxLen = qtx;
	}
	return qtrue;
}

const char* CG_MapBundle_DataSource(void)
{
	return cg_mapBundleSource[0] ? cg_mapBundleSource : NULL;
}

static void CG_TestQ3lb_PrintFileSize(const char* path)
{
	fileHandle_t    fh;
	int             len;

	len = trap_FS_FOpenFile(path, &fh, FS_READ);
	if (len > 0 && fh)
	{
		trap_FS_FCloseFile(fh);
		CG_Printf("  ^2%6i^7  ^7%s^7\n", len, path);
	}
	else
	{
		CG_Printf("  ^1     -^7  ^7%s^7\n", path);
	}
}

static void CG_TestQ3lb_PrintModeCounts(const char* label, int (*countForMode)(int))
{
	static const char* const modeNames[5] =
	{
		"ffa", "tour", "single", "team", "ctf"
	};
	int mode;

	CG_Printf("    %s per mode:", label);
	for (mode = 0; mode < 5; mode++)
	{
		CG_Printf(" ^3%s^7=%i", modeNames[mode], countForMode(mode));
	}
	CG_Printf("\n");
}

void CG_TestQ3lb_f(void)
{
	char            mapBase[MAX_QPATH];
	char            path[MAX_QPATH];
	const char*     map;
	int             qmapVer;
	int             qspBytes;
	int             qitmBytes;
	int             qlmBytes;
	int             qtxBytes;
	int             rawLen;
	int             spawnMode;
	int             itemMode;
	static const char* const modeNames[5] =
	{
		"ffa", "tournament", "single", "team", "ctf"
	};

	map = cgs.mapname;
	if (!map || !map[0])
	{
		map = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "mapname");
	}

	CG_MapData_GetMapBaseName(mapBase, sizeof(mapBase));

	CG_MapBundle_LoadAllMapData();

	CG_Printf("^3testq3lb^7 — Q3LB / mapdata diagnostic\n");

	if (map && map[0])
	{
		CG_Printf("  map: ^2%s^7", map);
		if (mapBase[0] && Q_stricmp(map, mapBase))
		{
			CG_Printf("  (base ^2%s^7)", mapBase);
		}
		if (cgs.gametype >= 0 && cgs.gametype < 5)
		{
			CG_Printf("  gametype: ^2%i^7 (^3%s^7)", cgs.gametype, modeNames[cgs.gametype]);
		}
		else
		{
			CG_Printf("  gametype: ^2%i^7", cgs.gametype);
		}
		CG_Printf("\n");
	}
	else
	{
		CG_Printf("  ^1no map name^7\n");
	}

	CG_Printf("^5mapdata Q3LB^7 (size, ^1-^7 = missing):\n");
	CG_TestQ3lb_PrintFileSize(CG_MAPBUNDLE_PATH);
	if (mapBase[0])
	{
		Com_sprintf(path, sizeof(path), CG_MAPBUNDLE_PERMAP, mapBase);
		CG_TestQ3lb_PrintFileSize(path);
	}

	CG_Printf("^5QMAP payload^7 (current map):\n");
	if (CG_MapBundle_HasPayload() && CG_MapBundle_QuerySections(&qmapVer, &qspBytes, &qitmBytes, &qlmBytes, &qtxBytes))
	{
		const char* src;

		(void)CG_MapBundle_Raw(&rawLen);
		src = CG_MapBundle_DataSource();
		CG_Printf("  ^2loaded^7  %i bytes  QMAP v^2%i^7", rawLen, qmapVer);
		if (src)
		{
			CG_Printf("  from ^2%s^7", src);
		}
		CG_Printf("\n");
		CG_Printf("  sections: QSP1 ^2%i^7  QITM1 ^2%i^7  QLM1 ^2%i^7  QTX1 ^2%i^7 bytes\n",
		          qspBytes, qitmBytes, qlmBytes, qtxBytes);
	}
	else
	{
		CG_Printf("  ^1not loaded^7 (need entry in ^3%s^7 or ^3mapdata/<map>.q3lb^7)\n",
		          CG_MAPBUNDLE_PATH);
		qspBytes = 0;
		qitmBytes = 0;
		qlmBytes = 0;
		qtxBytes = 0;
	}

	CG_Printf("^5Parsed features^7:\n");

	CG_Printf("  spawns (QSP1): ^2%i^7 points", CG_MapSpawns_NumPoints());
	if (CG_MapSpawns_DataSource())
	{
		CG_Printf("  src ^2%s^7", CG_MapSpawns_DataSource());
	}
	else if (qspBytes <= 0)
	{
		CG_Printf("  ^1(no QSP1)^7");
	}
	CG_Printf("\n");
	if (CG_MapSpawns_NumPoints() > 0)
	{
		spawnMode = CG_MapSpawns_ActiveModeIndex();
		if (spawnMode >= 0 && spawnMode < 5)
		{
			CG_Printf("    active mode ^3%s^7: ^2%i^7 playable spawns\n",
			          modeNames[spawnMode], CG_MapSpawns_NumPointsForMode(spawnMode));
		}
		CG_TestQ3lb_PrintModeCounts("all", CG_MapSpawns_NumPointsForMode);
	}

	CG_Printf("  shadow items (QITM1): ^2%i^7 points", CG_ShadowItems_NumPoints());
	if (qitmBytes <= 0)
	{
		CG_Printf("  ^1(no QITM1 in QMAP)^7");
	}
	CG_Printf("\n");
	if (CG_ShadowItems_NumPoints() > 0)
	{
		itemMode = CG_ShadowItems_ActiveModeIndex();
		if (itemMode >= 0 && itemMode < 5)
		{
			CG_Printf("    active mode ^3%s^7: ^2%i^7 points\n",
			          modeNames[itemMode], CG_ShadowItems_NumPointsForMode(itemMode));
		}
		CG_TestQ3lb_PrintModeCounts("all", CG_ShadowItems_NumPointsForMode);
		CG_Printf("    cvars: cg_shadowItems=^3%i^7 mask=^3%i^7 serverSI3=^3%i^7 cg_simpleItems=^3%i^7\n",
		          cg_shadowItems.integer, CG_ShadowItems_DebugEffectiveMask(),
		          CG_ShadowItems_UsesServerLayout(), cg_simpleItems.integer);
	}

	CG_Printf("  locations (QLM1): ^2%i^7 zones", CG_MapLocs_NumZones());
	if (CG_MapLocs_DataSource())
	{
		CG_Printf("  src ^2%s^7", CG_MapLocs_DataSource());
	}
	else if (qlmBytes <= 0 && !CG_MapLocs_NumZones())
	{
		CG_Printf("  ^1(no QLM1)^7");
	}
	if (CG_MapLocs_NumZones() > 0)
	{
		CG_Printf("  server tinfo names: ^2%s^7",
		          CG_MapLocs_ServerHasLocationNames() ? "yes" : "no");
	}
	CG_Printf("\n");

	CG_Printf("  tp exits (QTX1): ^2%i^7 points", CG_MapTPExit_NumPoints());
	if (CG_MapTPExit_DataSource())
	{
		CG_Printf("  src ^2%s^7", CG_MapTPExit_DataSource());
	}
	else if (qtxBytes <= 0 && !CG_MapTPExit_NumPoints())
	{
		CG_Printf("  ^1(no QTX1)^7");
	}
	CG_Printf("\n");

	CG_Printf("  draw cvars: cg_drawSpawns=^3%i^7  cg_drawTPexit=^3%i^7  cg_worldFlags=^3%i^7\n",
	          cg_drawSpawns.integer, cg_drawTPexit.integer, cg_worldFlags.integer);

	if (trap_Argc() >= 2 && !Q_stricmp(CG_Argv(1), "list"))
	{
		int         maxList;
		const char* what;

		maxList = 0;
		what = "items";
		if (trap_Argc() >= 3)
		{
			if (!Q_stricmp(CG_Argv(2), "spawns")
			        || !Q_stricmp(CG_Argv(2), "locs")
			        || !Q_stricmp(CG_Argv(2), "tpexit")
			        || !Q_stricmp(CG_Argv(2), "items"))
			{
				what = CG_Argv(2);
				if (trap_Argc() >= 4)
				{
					maxList = atoi(CG_Argv(3));
				}
			}
			else
			{
				maxList = atoi(CG_Argv(2));
			}
		}

		if (!Q_stricmp(what, "spawns"))
		{
			CG_MapSpawns_PrintList(maxList);
		}
		else if (!Q_stricmp(what, "locs"))
		{
			CG_MapLocs_PrintList(maxList);
		}
		else if (!Q_stricmp(what, "tpexit"))
		{
			CG_MapTPExit_PrintList(maxList);
		}
		else
		{
			CG_ShadowItems_PrintList(maxList);
		}
	}
	else
	{
		CG_Printf("  ^7list: ^3testq3lb list [items|spawns|locs|tpexit] [count]^7\n");
	}
}
