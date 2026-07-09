/*
===========================================================================
cg_spawns.c

Map spawn markers (cg_drawSpawns bitmask). Data: QSP1 in mapdata Q3LB.
  1 models, 2 plasma, 4 neutral (white, no team tint).

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_SPAWN_MAX_POINTS     128
#define CG_SPAWN_MODE_COUNT     5

#define CG_SPAWN_ROLE_INTERMISSION 2
#define CG_SPAWN_ROLE_FLAG_RED  5
#define CG_SPAWN_ROLE_FLAG_BLUE 6

#define CG_SPAWN_TEAM_FREE        0
#define CG_SPAWN_TEAM_RED         1
#define CG_SPAWN_TEAM_BLUE        2
#define CG_SPAWN_TEAM_INTERMISSION 3

#define CG_SPAWN_MARK_RADIUS      24

#define CG_SPAWN_PLASMA_LAYER_SLOTS     2
#define CG_SPAWN_PLASMA_PERIOD_MS       1500
#define CG_SPAWN_PLASMA_LIFE_REFRESH_MS 1800
#define CG_SPAWN_PLASMA_LIFE_INITIAL_MS 3200

#define CG_DRAW_SPAWNS_MODELS           1
#define CG_DRAW_SPAWNS_PLASMA           2
#define CG_DRAW_SPAWNS_NO_TEAM_COLOR    4

typedef struct
{
	vec3_t  legsAxis[3];
	vec3_t  torsoAxis[3];
	vec3_t  headAxis[3];
	vec3_t  torsoOfs;
	vec3_t  headOfs;
} cg_spawnModelPose_t;

typedef struct
{
	vec3_t  origin;
	float   yaw;
	int     spawnflags;
	int     count;
	int     role;
	int     team;
	vec3_t  markCornerOfs[4];
	float   markST[4][2];
} cg_spawnPoint_t;

static cg_spawnPoint_t cg_spawnPoints[CG_SPAWN_MAX_POINTS];
static int cg_spawnPointCount;
static int cg_spawnModeIndices[CG_SPAWN_MODE_COUNT][CG_SPAWN_MAX_POINTS];
static int cg_spawnModeCount[CG_SPAWN_MODE_COUNT];
static cg_spawnModelPose_t      cg_spawnModelPose[CG_SPAWN_MAX_POINTS];
static qhandle_t                cg_spawnPoseLegsModel;
static qhandle_t                cg_spawnPoseTorsoModel;
static qhandle_t                cg_spawnPoseHeadModel;
static qboolean                 cg_spawnPoseValid;

static int      cg_spawnPlasmaLayerEnd[CG_SPAWN_MAX_POINTS][CG_SPAWN_PLASMA_LAYER_SLOTS];
static byte     cg_spawnPlasmaLayerToggle[CG_SPAWN_MAX_POINTS];
static int      cg_spawnPlasmaNextPulseTime;
static int      cg_spawnPlasmaPrevDrawMode;

static void CG_MapSpawns_ResetPlasmaPulse(void);

static void CG_MapSpawns_BuildMarkGeom(cg_spawnPoint_t* pt);

static qboolean CG_MapSpawns_IsEnabled(void)
{
	return (cg_drawSpawns.integer & (CG_DRAW_SPAWNS_MODELS | CG_DRAW_SPAWNS_PLASMA)) != 0;
}

static qboolean CG_MapSpawns_UsePlasma(void)
{
	return (cg_drawSpawns.integer & CG_DRAW_SPAWNS_PLASMA) != 0;
}

static qboolean CG_MapSpawns_UseModels(void)
{
	if (CG_MapSpawns_UsePlasma())
	{
		return qfalse;
	}
	return (cg_drawSpawns.integer & CG_DRAW_SPAWNS_MODELS) != 0;
}

static qboolean CG_MapSpawns_UseTeamTint(void)
{
	return (cg_drawSpawns.integer & CG_DRAW_SPAWNS_NO_TEAM_COLOR) == 0;
}

static void CG_MapSpawns_Clear(void)
{
	cg_spawnPointCount = 0;
	memset(cg_spawnModeCount, 0, sizeof(cg_spawnModeCount));
	cg_spawnPoseValid = qfalse;
	CG_MapSpawns_ResetPlasmaPulse();
}

static void CG_MapSpawns_ResetPlasmaPulse(void)
{
	memset(cg_spawnPlasmaLayerEnd, 0, sizeof(cg_spawnPlasmaLayerEnd));
	memset(cg_spawnPlasmaLayerToggle, 0, sizeof(cg_spawnPlasmaLayerToggle));
	cg_spawnPlasmaNextPulseTime = 0;
	cg_spawnPlasmaPrevDrawMode = 0;
}

static qboolean CG_MapSpawns_IsPlayableSpawnPoint(const cg_spawnPoint_t* pt)
{
	if (!pt)
	{
		return qfalse;
	}
	if (pt->role == CG_SPAWN_ROLE_FLAG_RED || pt->role == CG_SPAWN_ROLE_FLAG_BLUE
	        || pt->role == CG_SPAWN_ROLE_INTERMISSION)
	{
		return qfalse;
	}
	return qtrue;
}

static void CG_MapSpawns_AddPlasmaLayer(int pointIdx, int lifeMs)
{
	int slot;

	if (pointIdx < 0 || pointIdx >= CG_SPAWN_MAX_POINTS)
	{
		return;
	}

	slot = cg_spawnPlasmaLayerToggle[pointIdx] & 1;
	cg_spawnPlasmaLayerToggle[pointIdx]++;
	cg_spawnPlasmaLayerEnd[pointIdx][slot] = cg.time + lifeMs;
}

static void CG_MapSpawns_InitPlasmaPulse(int mode)
{
	int i;
	int idx;

	CG_MapSpawns_ResetPlasmaPulse();
	cg_spawnPlasmaNextPulseTime = cg.time + CG_SPAWN_PLASMA_PERIOD_MS;

	for (i = 0; i < cg_spawnModeCount[mode]; i++)
	{
		idx = cg_spawnModeIndices[mode][i];
		if (idx < 0 || idx >= cg_spawnPointCount)
		{
			continue;
		}
		if (!CG_MapSpawns_IsPlayableSpawnPoint(&cg_spawnPoints[idx]))
		{
			continue;
		}
		cg_spawnPlasmaLayerEnd[idx][0] = cg.time + CG_SPAWN_PLASMA_LIFE_INITIAL_MS;
		cg_spawnPlasmaLayerToggle[idx] = 1;
	}
}

static void CG_MapSpawns_TickPlasmaPulse(int mode)
{
	int i;
	int idx;

	if (cg.time < cg_spawnPlasmaNextPulseTime)
	{
		return;
	}

	cg_spawnPlasmaNextPulseTime = cg.time + CG_SPAWN_PLASMA_PERIOD_MS;

	for (i = 0; i < cg_spawnModeCount[mode]; i++)
	{
		idx = cg_spawnModeIndices[mode][i];
		if (idx < 0 || idx >= cg_spawnPointCount)
		{
			continue;
		}
		if (!CG_MapSpawns_IsPlayableSpawnPoint(&cg_spawnPoints[idx]))
		{
			continue;
		}
		CG_MapSpawns_AddPlasmaLayer(idx, CG_SPAWN_PLASMA_LIFE_REFRESH_MS);
	}
}

static int CG_MapSpawns_ReadLE16(const byte* p)
{
	return p[0] | (p[1] << 8);
}

static float CG_MapSpawns_ReadFloat(const byte* p)
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

qboolean CG_MapSpawns_LoadQSP1Binary(const byte* raw, int rawLen)
{
	int pos;
	int version;
	int numPoints;
	int i;
	int mode;
	int nSpawned;
	int idx;
	cg_spawnPoint_t* pt;

	if (!raw || rawLen < 8)
	{
		return qfalse;
	}
	if (raw[0] != 'Q' || raw[1] != 'S' || raw[2] != 'P' || raw[3] != '1')
	{
		return qfalse;
	}

	pos = 4;
	version = CG_MapSpawns_ReadLE16(raw + pos);
	pos += 2;
	if (version != 1)
	{
		return qfalse;
	}

	numPoints = CG_MapSpawns_ReadLE16(raw + pos);
	pos += 2;
	if (numPoints <= 0 || numPoints > CG_SPAWN_MAX_POINTS)
	{
		return qfalse;
	}

	CG_MapSpawns_Clear();
	cg_spawnPointCount = numPoints;

	for (i = 0; i < numPoints; i++)
	{
		if (pos + 18 > rawLen)
		{
			CG_MapSpawns_Clear();
			return qfalse;
		}

		pt = &cg_spawnPoints[i];
		pt->origin[0] = CG_MapSpawns_ReadFloat(raw + pos);
		pos += 4;
		pt->origin[1] = CG_MapSpawns_ReadFloat(raw + pos);
		pos += 4;
		pt->origin[2] = CG_MapSpawns_ReadFloat(raw + pos);
		pos += 4;
		pt->yaw = (float)CG_MapSpawns_ReadLE16(raw + pos);
		pos += 2;
		pt->spawnflags = raw[pos++];
		pt->count = (signed char)raw[pos++];
		pt->role = raw[pos++];
		pt->team = raw[pos++];
	}

	for (mode = 0; mode < CG_SPAWN_MODE_COUNT; mode++)
	{
		if (pos + 2 > rawLen)
		{
			CG_MapSpawns_Clear();
			return qfalse;
		}
		nSpawned = CG_MapSpawns_ReadLE16(raw + pos);
		pos += 2;
		if (nSpawned < 0 || nSpawned > CG_SPAWN_MAX_POINTS)
		{
			CG_MapSpawns_Clear();
			return qfalse;
		}
		if (pos + nSpawned * 2 > rawLen)
		{
			CG_MapSpawns_Clear();
			return qfalse;
		}
		cg_spawnModeCount[mode] = nSpawned;
		for (i = 0; i < nSpawned; i++)
		{
			idx = CG_MapSpawns_ReadLE16(raw + pos);
			pos += 2;
			if (idx < 0 || idx >= numPoints)
			{
				CG_MapSpawns_Clear();
				return qfalse;
			}
			cg_spawnModeIndices[mode][i] = idx;
		}
	}

	if (pos != rawLen)
	{
		/* trailing bytes tolerated */
	}

	for (i = 0; i < cg_spawnPointCount; i++)
	{
		CG_MapSpawns_BuildMarkGeom(&cg_spawnPoints[i]);
	}

	return qtrue;
}

qboolean CG_MapSpawns_GetFlagStandOrigin(qboolean redFlag, vec3_t out)
{
	int role;
	int i;

	if (!out)
	{
		return qfalse;
	}

	role = redFlag ? CG_SPAWN_ROLE_FLAG_RED : CG_SPAWN_ROLE_FLAG_BLUE;
	for (i = 0; i < cg_spawnPointCount; i++)
	{
		if (cg_spawnPoints[i].role == role)
		{
			VectorCopy(cg_spawnPoints[i].origin, out);
			return qtrue;
		}
	}
	return qfalse;
}

void CG_MapSpawns_Load(void)
{
	const byte* qsp;
	int         qspLen;

	CG_MapSpawns_Clear();
	if (!CG_MapBundle_Section("QSP1", &qsp, &qspLen))
	{
		return;
	}
	(void)CG_MapSpawns_LoadQSP1Binary(qsp, qspLen);
}

static qboolean CG_MapSpawns_Q3mskFreezeUseFFA(void)
{
	return cgs.q3msk && cgs.osp.gameTypeFreeze != 0;
}

static int CG_MapSpawns_ModeIndex(void)
{
	if (CG_MapSpawns_Q3mskFreezeUseFFA())
	{
		/* q3msk freeze runs as team GT but uses DM spawn points on the server */
		return 0;
	}

	switch (cgs.gametype)
	{
		case GT_TOURNAMENT:
			return 1;
		case GT_SINGLE_PLAYER:
			return 2;
		case GT_TEAM:
			return 3;
		case GT_CTF:
			return 4;
		default:
			return 0;
	}
}

static void CG_MapSpawns_GetPointColor(const cg_spawnPoint_t* pt, float* r, float* g, float* b)
{
	const float* teamColor;

	if (!CG_MapSpawns_UseTeamTint())
	{
		*r = 1.0f;
		*g = 1.0f;
		*b = 1.0f;
		return;
	}

	*r = 0.8f;
	*g = 0.8f;
	*b = 0.8f;

	if (cgs.gametype >= GT_TEAM && !CG_MapSpawns_Q3mskFreezeUseFFA())
	{
		if (pt->team == CG_SPAWN_TEAM_RED)
		{
			teamColor = CG_TeamColor(TEAM_RED);
			*r = teamColor[0];
			*g = teamColor[1];
			*b = teamColor[2];
		}
		else if (pt->team == CG_SPAWN_TEAM_BLUE)
		{
			teamColor = CG_TeamColor(TEAM_BLUE);
			*r = teamColor[0];
			*g = teamColor[1];
			*b = teamColor[2];
		}
	}
}

static void CG_MapSpawns_BuildMarkGeom(cg_spawnPoint_t* pt)
{
	vec3_t      origin;
	vec3_t      dir;
	vec3_t      axis[3];
	vec3_t      corner;
	vec3_t      delta;
	float       radius;
	float       texCoordScale;
	int         i;

	radius = (float)CG_SPAWN_MARK_RADIUS;
	VectorCopy(pt->origin, origin);
	origin[2] += MINS_Z;
	origin[2] += 0.25f;

	VectorSet(dir, 0, 0, 1);
	VectorNormalize2(dir, axis[0]);
	PerpendicularVector(axis[1], axis[0]);
	RotatePointAroundVector(axis[2], axis[0], axis[1], pt->yaw);
	CrossProduct(axis[0], axis[2], axis[1]);

	texCoordScale = 0.5f / radius;

	for (i = 0; i < 4; i++)
	{
		switch (i)
		{
			case 0:
				VectorMA(origin, -radius, axis[1], corner);
				VectorMA(corner, -radius, axis[2], corner);
				break;
			case 1:
				VectorMA(origin, radius, axis[1], corner);
				VectorMA(corner, -radius, axis[2], corner);
				break;
			case 2:
				VectorMA(origin, radius, axis[1], corner);
				VectorMA(corner, radius, axis[2], corner);
				break;
			default:
				VectorMA(origin, -radius, axis[1], corner);
				VectorMA(corner, radius, axis[2], corner);
				break;
		}

		VectorSubtract(corner, pt->origin, pt->markCornerOfs[i]);
		VectorSubtract(corner, origin, delta);
		pt->markST[i][0] = 0.5f + DotProduct(delta, axis[1]) * texCoordScale;
		pt->markST[i][1] = 0.5f + DotProduct(delta, axis[2]) * texCoordScale;
	}
}

static void CG_MapSpawns_AddGroundMark(const cg_spawnPoint_t* pt)
{
	qhandle_t   shader;
	polyVert_t  verts[4];
	float       r;
	float       g;
	float       b;
	int         i;

	shader = cgs.media.shadowMarkShaderNew[0];
	if (!shader)
	{
		shader = cgs.media.shadowMarkShader;
	}
	if (!shader)
	{
		return;
	}

	r = 0.5f;
	g = 0.5f;
	b = 0.5f;
	CG_MapSpawns_GetPointColor(pt, &r, &g, &b);
	r *= 0.5f;
	g *= 0.5f;
	b *= 0.5f;

	for (i = 0; i < 4; i++)
	{
		VectorAdd(pt->origin, pt->markCornerOfs[i], verts[i].xyz);
		verts[i].st[0] = pt->markST[i][0];
		verts[i].st[1] = pt->markST[i][1];
		verts[i].modulate[0] = (byte)(r * 255.0f);
		verts[i].modulate[1] = (byte)(g * 255.0f);
		verts[i].modulate[2] = (byte)(b * 255.0f);
		verts[i].modulate[3] = 255;
	}

	trap_R_AddPolyToScene(shader, 4, verts);
}

static void CG_MapSpawns_BuildModelPose(int pointIdx, const clientInfo_t* ci, const cg_spawnPoint_t* pt)
{
	refEntity_t     legs;
	refEntity_t     torso;
	refEntity_t     head;
	vec3_t          angles;
	const animation_t* anim;
	cg_spawnModelPose_t* pose;

	if (pointIdx < 0 || pointIdx >= CG_SPAWN_MAX_POINTS || !ci || !pt)
	{
		return;
	}

	pose = &cg_spawnModelPose[pointIdx];
	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	angles[0] = 0;
	angles[1] = pt->yaw;
	angles[2] = 0;
	{
		vec3_t  legsAngles;
		vec3_t  torsoAngles;
		vec3_t  headAngles;

		VectorCopy(angles, legsAngles);
		VectorCopy(angles, torsoAngles);
		VectorCopy(angles, headAngles);
		AnglesSubtract(headAngles, torsoAngles, headAngles);
		AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
		AnglesToAxis(legsAngles, legs.axis);
		AnglesToAxis(torsoAngles, torso.axis);
		AnglesToAxis(headAngles, head.axis);
	}

	legs.hModel = ci->legsModel;
	legs.origin[0] = 0;
	legs.origin[1] = 0;
	legs.origin[2] = 0;
	VectorCopy(legs.origin, legs.oldorigin);
	anim = &ci->animations[LEGS_IDLE];
	legs.frame = anim->firstFrame;
	legs.oldframe = anim->firstFrame;
	legs.backlerp = 0;

	torso.hModel = ci->torsoModel;
	anim = &ci->animations[TORSO_STAND];
	torso.frame = anim->firstFrame;
	torso.oldframe = anim->firstFrame;
	torso.backlerp = 0;
	CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");

	head.hModel = ci->headModel;
	head.frame = 0;
	head.oldframe = 0;
	head.backlerp = 0;
	CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");

	memcpy(pose->legsAxis, legs.axis, sizeof(pose->legsAxis));
	memcpy(pose->torsoAxis, torso.axis, sizeof(pose->torsoAxis));
	memcpy(pose->headAxis, head.axis, sizeof(pose->headAxis));
	VectorSubtract(torso.origin, legs.origin, pose->torsoOfs);
	VectorSubtract(head.origin, legs.origin, pose->headOfs);
}

static void CG_MapSpawns_EnsureModelPoses(const clientInfo_t* ci)
{
	int     i;

	if (!ci || !ci->legsModel || !ci->torsoModel || !ci->headModel)
	{
		return;
	}

	if (cg_spawnPoseValid
	        && cg_spawnPoseLegsModel == ci->legsModel
	        && cg_spawnPoseTorsoModel == ci->torsoModel
	        && cg_spawnPoseHeadModel == ci->headModel)
	{
		return;
	}

	for (i = 0; i < cg_spawnPointCount; i++)
	{
		CG_MapSpawns_BuildModelPose(i, ci, &cg_spawnPoints[i]);
	}

	cg_spawnPoseLegsModel = ci->legsModel;
	cg_spawnPoseTorsoModel = ci->torsoModel;
	cg_spawnPoseHeadModel = ci->headModel;
	cg_spawnPoseValid = qtrue;
}

static void CG_MapSpawns_AddModelAtPoint(int pointIdx, const clientInfo_t* ci, const cg_spawnPoint_t* pt)
{
	refEntity_t             legs;
	refEntity_t             torso;
	refEntity_t             head;
	const animation_t*      anim;
	const cg_spawnModelPose_t* pose;
	byte                    rgba[4];

	if (!ci->legsModel || !ci->torsoModel || !ci->headModel)
	{
		return;
	}
	if (pointIdx < 0 || pointIdx >= CG_SPAWN_MAX_POINTS)
	{
		return;
	}

	CG_MapSpawns_AddGroundMark(pt);
	pose = &cg_spawnModelPose[pointIdx];

	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;
	VectorCopy(pt->origin, legs.origin);
	VectorCopy(pt->origin, legs.lightingOrigin);
	VectorCopy(legs.origin, legs.oldorigin);
	memcpy(legs.axis, pose->legsAxis, sizeof(legs.axis));
	legs.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	anim = &ci->animations[LEGS_IDLE];
	legs.frame = anim->firstFrame;
	legs.oldframe = anim->firstFrame;
	legs.backlerp = 0;
	rgba[0] = 100;
	rgba[1] = 100;
	rgba[2] = 100;
	rgba[3] = 100;
	*(int*)legs.shaderRGBA = *(int*)rgba;
	legs.customShader = cgs.media.spawnModelShader;
	trap_R_AddRefEntityToScene(&legs);

	torso.hModel = ci->torsoModel;
	torso.customSkin = ci->torsoSkin;
	anim = &ci->animations[TORSO_STAND];
	torso.frame = anim->firstFrame;
	torso.oldframe = anim->firstFrame;
	torso.backlerp = 0;
	VectorCopy(pt->origin, torso.lightingOrigin);
	VectorAdd(pt->origin, pose->torsoOfs, torso.origin);
	VectorCopy(torso.origin, torso.oldorigin);
	memcpy(torso.axis, pose->torsoAxis, sizeof(torso.axis));
	torso.renderfx = legs.renderfx;
	*(int*)torso.shaderRGBA = *(int*)rgba;
	torso.customShader = cgs.media.spawnModelShader;
	trap_R_AddRefEntityToScene(&torso);

	head.hModel = ci->headModel;
	head.customSkin = ci->headSkin;
	head.frame = 0;
	head.oldframe = 0;
	head.backlerp = 0;
	VectorCopy(pt->origin, head.lightingOrigin);
	VectorAdd(pt->origin, pose->headOfs, head.origin);
	VectorCopy(head.origin, head.oldorigin);
	memcpy(head.axis, pose->headAxis, sizeof(head.axis));
	head.renderfx = legs.renderfx;
	*(int*)head.shaderRGBA = *(int*)rgba;
	head.customShader = cgs.media.spawnModelShader;
	trap_R_AddRefEntityToScene(&head);
}

static void CG_MapSpawns_DrawPlasmaAtPoint(int pointIdx, const cg_spawnPoint_t* pt)
{
	vec3_t origin;
	vec3_t teamRgb;
	int    layer;
	int    activeLayers;

	if (!cgs.media.spawnMarkPlasmaShader)
	{
		return;
	}

	VectorCopy(pt->origin, origin);
	origin[2] += 24.0f;
	CG_MapSpawns_GetPointColor(pt, &teamRgb[0], &teamRgb[1], &teamRgb[2]);

	activeLayers = 0;
	for (layer = 0; layer < CG_SPAWN_PLASMA_LAYER_SLOTS; layer++)
	{
		if (cg_spawnPlasmaLayerEnd[pointIdx][layer] > cg.time)
		{
			activeLayers++;
		}
	}

	for (layer = 0; layer < CG_SPAWN_PLASMA_LAYER_SLOTS; layer++)
	{
		if (cg_spawnPlasmaLayerEnd[pointIdx][layer] <= cg.time)
		{
			continue;
		}
		CG_AddPlasmaMissileScene(origin, cg.clientNum, cgs.media.spawnMarkPlasmaShader, teamRgb, qfalse);
	}

	for (layer = 0; layer < activeLayers; layer++)
	{
		CG_AddPlasmaMissileDlight(origin, cg.clientNum, cgs.media.spawnMarkPlasmaShader, teamRgb);
	}
}

void CG_AddMapSpawns(void)
{
	const clientInfo_t* ci = NULL;
	int                 mode;
	int                 i;
	int                 idx;
	const cg_spawnPoint_t* pt;

	if (!CG_MapSpawns_IsEnabled() || !cg_spawnPointCount)
	{
		return;
	}

	if (CG_MapSpawns_UseModels())
	{
		ci = &cgs.clientinfo[cg.clientNum];
		if (!ci->infoValid)
		{
			return;
		}
	}

	mode = CG_MapSpawns_ModeIndex();
	if (mode < 0 || mode >= CG_SPAWN_MODE_COUNT)
	{
		return;
	}

	if (CG_MapSpawns_UsePlasma())
	{
		if (!cg_spawnPlasmaPrevDrawMode)
		{
			CG_MapSpawns_InitPlasmaPulse(mode);
		}
		CG_MapSpawns_TickPlasmaPulse(mode);
	}
	else if (cg_spawnPlasmaPrevDrawMode)
	{
		CG_MapSpawns_ResetPlasmaPulse();
	}
	cg_spawnPlasmaPrevDrawMode = CG_MapSpawns_UsePlasma() ? 1 : 0;

	if (CG_MapSpawns_UseModels())
	{
		CG_MapSpawns_EnsureModelPoses(ci);
	}

	for (i = 0; i < cg_spawnModeCount[mode]; i++)
	{
		idx = cg_spawnModeIndices[mode][i];
		if (idx < 0 || idx >= cg_spawnPointCount)
		{
			continue;
		}
		pt = &cg_spawnPoints[idx];
		if (!CG_MapSpawns_IsPlayableSpawnPoint(pt))
		{
			continue;
		}
		if (CG_MapSpawns_UsePlasma())
		{
			CG_MapSpawns_DrawPlasmaAtPoint(idx, pt);
		}
		else if (CG_MapSpawns_UseModels())
		{
			CG_MapSpawns_AddModelAtPoint(idx, ci, pt);
		}
	}
}

int CG_MapSpawns_NumPoints(void)
{
	return cg_spawnPointCount;
}

int CG_MapSpawns_NumPointsForMode(int mode)
{
	if (mode < 0 || mode >= CG_SPAWN_MODE_COUNT)
	{
		return 0;
	}
	return cg_spawnModeCount[mode];
}

int CG_MapSpawns_ActiveModeIndex(void)
{
	return CG_MapSpawns_ModeIndex();
}

const char* CG_MapSpawns_DataSource(void)
{
	if (!cg_spawnPointCount)
	{
		return NULL;
	}
	return CG_MapBundle_DataSource();
}

void CG_MapSpawns_PrintList(int maxList)
{
	int i;
	int n;

	if (!cg_spawnPointCount)
	{
		CG_Printf("  ^1no spawn points loaded^7\n");
		return;
	}

	n = cg_spawnPointCount;
	if (maxList > 0 && maxList < n)
	{
		n = maxList;
	}

	CG_Printf("  ^3spawns^7 (%i):\n", cg_spawnPointCount);
	for (i = 0; i < n; i++)
	{
		const cg_spawnPoint_t* pt;

		pt = &cg_spawnPoints[i];
		CG_Printf("  [%i] role=%i team=%i yaw %.0f  origin %.0f %.0f %.0f\n",
		          i, pt->role, pt->team, pt->yaw,
		          pt->origin[0], pt->origin[1], pt->origin[2]);
	}
	if (n < cg_spawnPointCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list spawns <count>)\n", cg_spawnPointCount - n);
	}
}
