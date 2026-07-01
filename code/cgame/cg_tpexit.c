/*
===========================================================================
cg_tpexit.c

Teleporter exit markers (cg_drawTPexit). Data: QTX1 in mapdata Q3LB.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_TPEXIT_MAX_POINTS     64

typedef struct
{
	vec3_t  origin;
	float   yaw;
} cg_tpExitPoint_t;

static cg_tpExitPoint_t cg_tpExitPoints[CG_TPEXIT_MAX_POINTS];
static int cg_tpExitPointCount;

static void CG_MapTPExit_Clear(void)
{
	cg_tpExitPointCount = 0;
}

static int CG_MapTPExit_ReadLE16(const byte* p)
{
	return p[0] | (p[1] << 8);
}

static float CG_MapTPExit_ReadFloat(const byte* p)
{
	union
	{
		byte    b[4];
		float   f;
	} u;

	u.b[0] = p[0];
	u.b[1] = p[1];
	u.b[2] = p[2];
	u.b[3] = p[3];
	return u.f;
}

static qboolean CG_MapTPExit_LoadQTX1Binary(const byte* raw, int rawLen)
{
	int pos;
	int version;
	int numPoints;
	int i;
	cg_tpExitPoint_t* pt;

	if (!raw || rawLen < 8)
	{
		return qfalse;
	}
	if (raw[0] != 'Q' || raw[1] != 'T' || raw[2] != 'X' || raw[3] != '1')
	{
		return qfalse;
	}

	pos = 4;
	version = CG_MapTPExit_ReadLE16(raw + pos);
	pos += 2;
	if (version != 1)
	{
		return qfalse;
	}

	numPoints = CG_MapTPExit_ReadLE16(raw + pos);
	pos += 2;
	if (numPoints <= 0 || numPoints > CG_TPEXIT_MAX_POINTS)
	{
		return qfalse;
	}

	CG_MapTPExit_Clear();
	cg_tpExitPointCount = numPoints;

	for (i = 0; i < numPoints; i++)
	{
		if (pos + 14 > rawLen)
		{
			CG_MapTPExit_Clear();
			return qfalse;
		}

		pt = &cg_tpExitPoints[i];
		pt->origin[0] = CG_MapTPExit_ReadFloat(raw + pos);
		pos += 4;
		pt->origin[1] = CG_MapTPExit_ReadFloat(raw + pos);
		pos += 4;
		pt->origin[2] = CG_MapTPExit_ReadFloat(raw + pos);
		pos += 4;
		pt->yaw = (float)CG_MapTPExit_ReadLE16(raw + pos);
		pos += 2;
	}

	return qtrue;
}

void CG_MapTPExit_Load(void)
{
	const byte* qtx;
	int         qtxLen;

	CG_MapTPExit_Clear();
	if (!CG_MapBundle_Section("QTX1", &qtx, &qtxLen))
	{
		return;
	}
	(void)CG_MapTPExit_LoadQTX1Binary(qtx, qtxLen);
}

static void CG_MapTPExit_AddTeleportModel(const cg_tpExitPoint_t* pt)
{
	refEntity_t re;
	vec3_t      angles;
	byte        rgba[4];

	if (!cgs.media.teleportEffectModel || !cgs.media.teleporterStarsShader)
	{
		return;
	}

	memset(&re, 0, sizeof(re));
	re.reType = RT_MODEL;
	re.hModel = cgs.media.teleportEffectModel;
	re.customShader = cgs.media.teleporterStarsShader;
	/* shaderTime 0: tcMod scroll uses refdef.time (do not set to cg.time each frame) */
	re.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	rgba[0] = 255.0f;
	rgba[1] = 255.0f;
	rgba[2] = 255.0f;
	rgba[3] = 77;
	*(int*)re.shaderRGBA = *(int*)rgba;

	VectorCopy(pt->origin, re.origin);
	re.origin[2] -= 24.0f;
	VectorCopy(re.origin, re.lightingOrigin);
	VectorCopy(re.origin, re.oldorigin);

	angles[0] = 0.0f;
	angles[1] = pt->yaw;
	angles[2] = 0.0f;
	AnglesToAxis(angles, re.axis);

	trap_R_AddRefEntityToScene(&re);
}

void CG_AddMapTPExits(void)
{
	int i;

	if (!cg_drawTPexit.integer || !cg_tpExitPointCount)
	{
		return;
	}

	for (i = 0; i < cg_tpExitPointCount; i++)
	{
		CG_MapTPExit_AddTeleportModel(&cg_tpExitPoints[i]);
	}
}

int CG_MapTPExit_NumPoints(void)
{
	return cg_tpExitPointCount;
}

const char* CG_MapTPExit_DataSource(void)
{
	if (!cg_tpExitPointCount)
	{
		return NULL;
	}
	return CG_MapBundle_DataSource();
}

void CG_MapTPExit_PrintList(int maxList)
{
	int i;
	int n;

	if (!cg_tpExitPointCount)
	{
		CG_Printf("  ^1no tp exit points loaded^7\n");
		return;
	}

	n = cg_tpExitPointCount;
	if (maxList > 0 && maxList < n)
	{
		n = maxList;
	}

	CG_Printf("  ^3tp exits^7 (%i):\n", cg_tpExitPointCount);
	for (i = 0; i < n; i++)
	{
		const cg_tpExitPoint_t* pt;

		pt = &cg_tpExitPoints[i];
		CG_Printf("  [%i] yaw %.0f  origin %.0f %.0f %.0f\n",
		          i, pt->yaw,
		          pt->origin[0], pt->origin[1], pt->origin[2]);
	}
	if (n < cg_tpExitPointCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list tpexit <count>)\n", cg_tpExitPointCount - n);
	}
}
