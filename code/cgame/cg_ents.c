/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

typedef struct timepos_s
{
	float time;
	vec3_t pos;
} timepos_t;

static vec_t VectorDistance(vec3_t a, vec3_t b)
{
	return (vec_t)sqrt((a[0] - b[0]) * (a[0] - b[0]) +
	                   (a[1] - b[1]) * (a[1] - b[1]) +
	                   (a[2] - b[2]) * (a[2] - b[2]));
}

static void CG_LinearInterpolation(timepos_t* positions, vec3_t out, float f)
{
	int i;
	for (i = 0; i < 3; ++i)
		out[i] = positions[0].pos[i] + (positions[1].pos[i] - positions[0].pos[i]) * f;
}

static void CG_LagrangeInterpolation(timepos_t* positions, int count, vec3_t out, float time)
{
	int i, j;
	vec_t coefficient;

	out[0] = out[1] = out[2] = 0.0f;
	for (i = 0; i < count; ++i)
	{
		coefficient = 1.0f;
		for (j = 0; j < count; ++j)
		{
			if (i == j)
				continue;
			coefficient *= (time - positions[j].time) / (positions[i].time - positions[j].time);
		}
		VectorMA(out, coefficient, positions[i].pos, out);
	}
}

static void CG_PolyBasePredictPosition(timepos_t* positions, vec3_t out, float f)
{
	const vec_t bias = 0.366f;   /* Chebyshev node */
	const vec_t threshold = 660.0f;
	const float time = positions[1].time + (positions[2].time - positions[1].time) * f;

	/* Teleportation/respawn invalidates oldest position */
	if (VectorDistance(positions[0].pos, positions[1].pos) >= threshold)
	{
		CG_LinearInterpolation(&positions[1], out, f);
		return;
	}
	positions[3].time = positions[2].time + (positions[2].time - positions[1].time) * bias;
	CG_LinearInterpolation(&positions[1], positions[3].pos, 1.0f + bias);
	CG_LagrangeInterpolation(positions, 4, out, time);
}

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
                            qhandle_t parentModel, char* tagName)
{
	int             i;
	orientation_t   lerped;

	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
	               1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
                                   qhandle_t parentModel, char* tagName)
{
	int             i;
	orientation_t   lerped;
	vec3_t          tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
	               1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply(entity->axis, lerped.axis, tempAxis);
	MatrixMultiply(tempAxis, ((refEntity_t*)parent)->axis, entity->axis);
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition(centity_t* cent)
{
	if (cent->currentState.solid == SOLID_BMODEL)
	{
		vec3_t  origin;
		float*   v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd(cent->lerpOrigin, v, origin);
		trap_S_UpdateEntityPosition(cent->currentState.number, origin);
	}
	else
	{
		trap_S_UpdateEntityPosition(cent->currentState.number, cent->lerpOrigin);
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects(centity_t* cent)
{

	// update sound origins
	CG_SetEntitySoundPosition(cent);

	// add loop sound
	if (cent->currentState.loopSound && !CG_IsMutedWorldSound(cent->currentState.loopSound))
	{
		if (cent->currentState.eType != ET_SPEAKER)
		{
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                       cgs.gameSounds[ cent->currentState.loopSound ]);
		}
		else if (s_ambient.integer)
		{
			trap_S_AddRealLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                           cgs.gameSounds[ cent->currentState.loopSound ]);
		}
	}


	// constant light glow
	if (cent->currentState.constantLight)
	{
		int     cl;
		int     i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = (cl >> 8) & 255;
		b = (cl >> 16) & 255;
		i = ((cl >> 24) & 255) * 4;
		trap_R_AddLightToScene(cent->lerpOrigin, i, r, g, b);
	}

}


/*
==================
CG_General
==================
*/
static void CG_General(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex)
	{
		return;
	}

	memset(&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum)
	{
		ent.renderfx |= RF_THIRD_PERSON;    // only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis(cent->lerpAngles, ent.axis);

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker(centity_t* cent)
{
	if (! cent->currentState.clientNum)      // FIXME: use something other than clientNum...
	{
		return;     // not auto triggering
	}

	if (cg.time < cent->miscTime)
	{
		return;
	}

	if (CG_IsMutedWorldSound(cent->currentState.eventParm))
	{
		cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
		return;
	}

	trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm]);

	//  ent->s.frame = ent->wait * 10;
	//  ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}
/*
==================
CG_Item
==================
*/
static void CG_AddSimpleItem(centity_t* cent)
{
	refEntity_t     ent;
	gitem_t*         item;
	int modelIndex = cent->currentState.modelindex;
	item = &bg_itemlist[modelIndex];

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	VectorCopy(cent->lerpOrigin, ent.origin);
	if (cg_simpleItems.integer == 2)
	{
		ent.radius = 14;
	}
	else if (cg_simpleItems.integer)
	{
		ent.radius = cg_simpleItemsRadius.integer;
		if (ent.radius > 24)
		{
			ent.radius = 24;
		}
		if (ent.radius < 8)
		{
			ent.radius = 8;
		}
		if (ent.radius > 14)
		{
			qboolean isAmmoOrWeapon = qfalse;
			float originalRadius = ent.radius;

			switch (item->giType)
			{
				case IT_AMMO:
				case IT_WEAPON:
					ent.radius *= 0.8f;
					isAmmoOrWeapon = qtrue;
					break;
				default:
					break;
			}

			if (isAmmoOrWeapon)
			{
				float centerLift = (originalRadius - 14) * 1.5f;
				ent.origin[2] += centerLift;
			}
			else
			{
				ent.origin[2] += (ent.radius - 14) * 1.5f;
			}
		}
	}

	ent.customShader = cg_items[modelIndex].icon;
	if (cg_items[modelIndex].customVisualMask & ITEMCV_ICON)
	{
		vec4_t t;

		CG_GetCustomDyeItemTintVec4(item, t);
		ent.shaderRGBA[0] = (byte)(t[0] * 255.0f);
		ent.shaderRGBA[1] = (byte)(t[1] * 255.0f);
		ent.shaderRGBA[2] = (byte)(t[2] * 255.0f);
		ent.shaderRGBA[3] = (byte)(t[3] * 255.0f);
	}
	else
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
	}

	// items bob up and down continuously
	if (cg_simpleItemsBob.integer)
	{
		qboolean shouldBob = qfalse;

		if (cg_simpleItemsBob.integer == 1)
		{
			// All simple items except armor shard
			shouldBob = qtrue;
		}
		else if (cg_simpleItemsBob.integer == 2)
		{
			// Only IT_ARMOR (except shard), IT_POWERUP, IT_HOLDABLE and MEGAHEALTH (item_health_large)
			if (item->giType == IT_ARMOR && Q_stricmp(item->classname, "item_armor_shard"))
			{
				shouldBob = qtrue;
			}
			else if (item->giType == IT_POWERUP || item->giType == IT_HOLDABLE)
			{
				shouldBob = qtrue;
			}
			else if (!Q_stricmp(item->classname, "item_health_mega"))
			{
				// MEGAHEALTH (item_health_large)
				shouldBob = qtrue;
			}
		}

		if (shouldBob)
		{
			// Use unified timer so all items bob in sync
			float scale = 0.005;
			ent.origin[2] += 4 + cos((cg.time + 1000) * scale) * 4;
		}
	}

	if (cg_simpleItems.integer == 2)
	{
		switch (item->giType)
		{
			case IT_ARMOR:
			case IT_WEAPON:
				ent.radius = 20;
				ent.origin[2] += 15;
				break;
			case IT_AMMO:
				break;
			case IT_HEALTH:
				ent.origin[2] += 10;
				break;
			case IT_HOLDABLE:
			case IT_POWERUP:
				ent.radius = 20;
				ent.origin[2] += 10;
				break;
			default:
				break;
		}
	}

	trap_R_AddRefEntityToScene(&ent);
}

/*
==================
CG_FillCustomItemWorldDyeShaderRGBA

Per-frame tint for world models registered with a bg_customItemVisuals world_model slot.
==================
*/
static qboolean CG_FillCustomItemWorldDyeShaderRGBA(int itemNum, int slot, byte rgba[4])
{
	vec4_t         t;
	unsigned int slotFlag;

	if (itemNum < 1 || itemNum >= bg_numItems || slot < 0 || slot >= MAX_ITEM_MODELS)
	{
		return qfalse;
	}
	slotFlag = ITEMCV_WORLDMODEL_SLOT(slot);
	if (!(cg_items[itemNum].customVisualMask & slotFlag))
	{
		return qfalse;
	}
	CG_GetCustomDyeItemTintVec4(&bg_itemlist[itemNum], t);
	rgba[0] = (byte)(t[0] * 255.0f);
	rgba[1] = (byte)(t[1] * 255.0f);
	rgba[2] = (byte)(t[2] * 255.0f);
	rgba[3] = (byte)(t[3] * 255.0f);
	return qtrue;
}

/*
==================
CG_ShadowItemGhostShader

Dedicated shadow pickup shaders (beShadowItem / beShadowItemFlag), alt-style envmap.
==================
*/
static qhandle_t CG_ShadowItemGhostShader(int gitype)
{
	if (gitype == IT_TEAM)
	{
		if (cgs.media.shadowItemFlagShader)
		{
			return cgs.media.shadowItemFlagShader;
		}
	}

	if (cgs.media.shadowItemShader)
	{
		return cgs.media.shadowItemShader;
	}

	if (cgs.media.altFullBrightShader)
	{
		return cgs.media.altFullBrightShader;
	}
	return 0;
}

/*
==================
CG_AddWorldItem3D

Shared 3D item draw (world pickups). Used by real ET_ITEM entities and shadow ghosts.
==================
*/
void CG_AddWorldItem3D(const vec3_t inOrigin, int modelindex, int entNumber, int miscTime, qboolean shadowGhost, int shadowGhostAlpha)
{
	refEntity_t     ent;
	vec3_t          origin;
	gitem_t*        item;
	int             itemNum;
	int             itemFx;
	int             gitype;
	float           frac;
	int             ghostAlpha;

	itemNum = modelindex;
	if (itemNum < 1 || itemNum >= bg_numItems)
	{
		return;
	}

	item = &bg_itemlist[itemNum];
	itemFx = cg_itemFx.integer;
	gitype = item->giType;

	if (shadowGhost && (!cg_items[itemNum].registered || !cg_items[itemNum].models[0]))
	{
		return;
	}

	ghostAlpha = shadowGhostAlpha;
	if (shadowGhost && ghostAlpha <= 0)
	{
		ghostAlpha = 77;
	}
	if (shadowGhost && ghostAlpha > 255)
	{
		ghostAlpha = 255;
	}

	VectorCopy(inOrigin, origin);
	memset(&ent, 0, sizeof(ent));
	frac = 1.0f;

	if (itemFx & 1)
	{
		float scale;
		scale = 0.005f + entNumber * 0.00001f;
		origin[2] += 4.0f + cos((cg.time + 1000) * scale) * 4.0f;
	}

	if (itemFx & 2)
	{
		if (gitype == IT_HEALTH)
		{
			AxisCopy(cg.autoAxisFast, ent.axis);
		}
		else
		{
			AxisCopy(cg.autoAxis, ent.axis);
		}
	}
	else
	{
		AxisClear(ent.axis);
		ent.axis[0][0] = 1;
		ent.axis[1][1] = 1;
		ent.axis[2][2] = 1;
	}

	if (gitype == IT_WEAPON)
	{
		weaponInfo_t* wi;
		wi = &cg_weapons[item->giTag];
		origin[0] -= wi->weaponMidpoint[0] * ent.axis[0][0] + wi->weaponMidpoint[1] * ent.axis[1][0] + wi->weaponMidpoint[2] * ent.axis[2][0];
		origin[1] -= wi->weaponMidpoint[0] * ent.axis[0][1] + wi->weaponMidpoint[1] * ent.axis[1][1] + wi->weaponMidpoint[2] * ent.axis[2][1];
		origin[2] -= wi->weaponMidpoint[0] * ent.axis[0][2] + wi->weaponMidpoint[1] * ent.axis[1][2] + wi->weaponMidpoint[2] * ent.axis[2][2];
		origin[2] += 8;
	}

	ent.hModel = cg_items[itemNum].models[0];
	VectorCopy(origin, ent.origin);
	VectorCopy(origin, ent.oldorigin);
	ent.nonNormalizedAxes = qfalse;

	if (itemFx & 4)
	{
		int msec;
		msec = cg.time - miscTime;
		frac = (msec >= 0 && msec < ITEM_SCALEUP_TIME) ? (float)msec / ITEM_SCALEUP_TIME : 1.0f;
		if (frac != 1.0f)
		{
			VectorScale(ent.axis[0], frac, ent.axis[0]);
			VectorScale(ent.axis[1], frac, ent.axis[1]);
			VectorScale(ent.axis[2], frac, ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;
		}
	}

	if (gitype == IT_WEAPON || gitype == IT_ARMOR)
	{
		ent.renderfx |= RF_MINLIGHT;
	}

	if (gitype == IT_WEAPON)
	{
		VectorScale(ent.axis[0], 1.5f, ent.axis[0]);
		VectorScale(ent.axis[1], 1.5f, ent.axis[1]);
		VectorScale(ent.axis[2], 1.5f, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
	}

	{
		qboolean usedBrightItem;
		qboolean brightStyleCached;
		qhandle_t brightShader;
		byte     brightRGBA[4] = { 0, 0, 0, 0 };

		usedBrightItem = qfalse;
		brightStyleCached = qfalse;

		if (shadowGhost)
		{
			ent.customShader = CG_ShadowItemGhostShader(gitype);
			ent.shaderRGBA[0] = 255;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 255;
			ent.shaderRGBA[3] = ghostAlpha;
			usedBrightItem = qtrue;
			brightStyleCached = qtrue;
			brightShader = ent.customShader;
			brightRGBA[0] = ent.shaderRGBA[0];
			brightRGBA[1] = ent.shaderRGBA[1];
			brightRGBA[2] = ent.shaderRGBA[2];
			brightRGBA[3] = ent.shaderRGBA[3];
		}
		else if (gitype != IT_WEAPON)
		{
			usedBrightItem = CG_ApplyBrightItem(&ent, itemNum, gitype);
		}
		else if (CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT) && (cg_drawBrightWeapons.integer & 8))
		{
			int w;

			w = item->giTag;
			if (w > WP_NONE && w < WP_NUM_WEAPONS)
			{
				ent.customShader = (cg_brightWeaponsShader.integer == 2) ?
				                   cgs.media.altFullBrightShader : cgs.media.fullBrightShader;
				CG_SetWeaponBrightColorWorld(&ent, w);
				usedBrightItem = qtrue;
			}
		}
		else if (item->giTag == WP_RAILGUN)
		{
			const float* color;

			color = (cg_railCustomChamber.integer & 2) ?
			        (const float*)cgs.osp.myColors.railRings : (const float*)colorWhite;
			ent.shaderRGBA[0] = 255 * color[0];
			ent.shaderRGBA[1] = 255 * color[1];
			ent.shaderRGBA[2] = 255 * color[2];
			ent.shaderRGBA[3] = 255;
		}

		if (!usedBrightItem)
		{
			if (CG_FillCustomItemWorldDyeShaderRGBA(itemNum, 0, ent.shaderRGBA))
			{
				/* custom dye */
			}
			else if (gitype != IT_WEAPON)
			{
				memset(ent.shaderRGBA, 0, sizeof(ent.shaderRGBA));
			}
		}
		else if (!shadowGhost)
		{
			brightStyleCached = qtrue;
			brightShader = ent.customShader;
			brightRGBA[0] = ent.shaderRGBA[0];
			brightRGBA[1] = ent.shaderRGBA[1];
			brightRGBA[2] = ent.shaderRGBA[2];
			brightRGBA[3] = ent.shaderRGBA[3];
		}

		trap_R_AddRefEntityToScene(&ent);

		if (gitype == IT_HEALTH || gitype == IT_POWERUP)
		{
			if ((ent.hModel = cg_items[itemNum].models[1]) != 0)
			{
				vec3_t spinAngles;

				VectorClear(spinAngles);
				if (gitype == IT_POWERUP)
				{
					ent.origin[2] += 12;
					spinAngles[1] = (cg.time & 1023) * 360.0f / -1024.0f;
				}
				AnglesToAxis(spinAngles, ent.axis);
				if (frac != 1.0f)
				{
					VectorScale(ent.axis[0], frac, ent.axis[0]);
					VectorScale(ent.axis[1], frac, ent.axis[1]);
					VectorScale(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				if (shadowGhost)
				{
					ent.customShader = brightShader;
					ent.shaderRGBA[0] = brightRGBA[0];
					ent.shaderRGBA[1] = brightRGBA[1];
					ent.shaderRGBA[2] = brightRGBA[2];
					ent.shaderRGBA[3] = brightRGBA[3];
				}
				else if (brightStyleCached)
				{
					ent.customShader = brightShader;
					ent.shaderRGBA[0] = brightRGBA[0];
					ent.shaderRGBA[1] = brightRGBA[1];
					ent.shaderRGBA[2] = brightRGBA[2];
					ent.shaderRGBA[3] = brightRGBA[3];
				}
				else if (!CG_ApplyBrightItem(&ent, itemNum, gitype))
				{
					if (!CG_FillCustomItemWorldDyeShaderRGBA(itemNum, 1, ent.shaderRGBA))
					{
						memset(ent.shaderRGBA, 0, sizeof(ent.shaderRGBA));
					}
				}
				trap_R_AddRefEntityToScene(&ent);
			}
		}
	}
}

/*
==================
CG_Item
==================
*/
static void CG_Item(centity_t* cent)
{
	entityState_t*  es;
	gitem_t*        item;
	int             itemNum;
	int             gitype;

	es = &cent->currentState;
	if (!es->modelindex || (es->eFlags & EF_NODRAW))
		return;
	if (cg.numItemEntityIndices < MAX_GENTITIES)
		cg.itemEntityIndices[cg.numItemEntityIndices++] = (int)(cent - cg_entities);
	if (es->modelindex >= bg_numItems)
		CG_Error("Bad item index %i on entity", es->modelindex);

	itemNum = es->modelindex;
	item = &bg_itemlist[itemNum];
	gitype = item->giType;

	if (cg_simpleItems.integer && gitype != IT_TEAM)
	{
		CG_AddSimpleItem(cent);
		return;
	}

	{
		int     mapPoint;
		int     phaseEnt;

		mapPoint = -1;
		if (!cg_simpleItems.integer && CG_ShadowItems_NumPoints() > 0 && !CG_OSPIsGameTypeCA(cgs.gametype))
		{
			if (cg_shadowItems.integer)
			{
				mapPoint = CG_ShadowItems_NoteLiveItem(cent);
			}
			else
			{
				mapPoint = CG_ShadowItems_MapPointForItemBob(cent);
			}
		}
		phaseEnt = (mapPoint >= 0) ? CG_ShadowItems_PhaseEntForPoint(mapPoint) : cent->currentState.number;
		CG_AddWorldItem3D(cent->lerpOrigin, itemNum, phaseEnt, cent->miscTime, qfalse, 0);
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;
	const weaponInfo_t*      weapon;
	qboolean useAltGrenades;
	qboolean useFullbright;
//	int  col;

	useAltGrenades = CG_BE_FEATURE_ENABLED(CG_BE_ALT_GRENADES);
	useFullbright = CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT);
	s1 = &cent->currentState;
	if (s1->weapon > WP_NUM_WEAPONS)
	{
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

	if (cent->currentState.weapon == WP_PLASMAGUN)
	{
		if (weapon->missileSound)
		{
			vec3_t  velocity;

			BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity);
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, velocity,
			                       weapon->missileSound);
		}
		CG_AddPlasmaMissileScene(cent->lerpOrigin, s1->otherEntityNum, 0, NULL, qtrue);
		return;
	}

	// add trails
	if (weapon->missileTrailFunc)
	{
		weapon->missileTrailFunc(cent, weapon);
	}
	// add dynamic light
	if (weapon->missileDlight)
	{
		float   dlR;
		float   dlG;
		float   dlB;

		dlR = weapon->missileDlightColor[0];
		dlG = weapon->missileDlightColor[1];
		dlB = weapon->missileDlightColor[2];
		if (s1->weapon == WP_PLASMAGUN)
		{
			vec4_t  plasmaColor;

			if (CG_PlasmaColorForOwner(s1->otherEntityNum, plasmaColor))
			{
				dlR = plasmaColor[0];
				dlG = plasmaColor[1];
				dlB = plasmaColor[2];
			}
		}
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
		                       dlR, dlG, dlB);
	}

	// add missile sound
	if (weapon->missileSound)
	{
		vec3_t  velocity;

		BG_EvaluateTrajectoryDelta(&cent->currentState.pos, cg.time, velocity);

		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound);
	}

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	if (cent->currentState.weapon == WP_GRENADE_LAUNCHER && cg_altGrenades.integer)
	{
		if (useAltGrenades && (cg_altGrenades.integer == 2 || cg_altGrenades.integer == 3))
		{
			int owner = s1->otherEntityNum;

			if (owner >= 0 && owner < MAX_CLIENTS)
			{
				const clientInfo_t* ci = &cgs.clientinfo[owner];

				qboolean isAlly = CG_OSPIsGameTypeCA(cgs.gametype) || !CG_CACHED_IS_ENEMY_CI(ci)
				                  || (cgs.gametype == GT_TOURNAMENT/*  && cgs.isOspServer */); //

				if (isAlly)
				{
					ent.shaderRGBA[0] = cg.beAltGrenadesColor[0] * 255;
					ent.shaderRGBA[1] = cg.beAltGrenadesColor[1] * 255;
					ent.shaderRGBA[2] = cg.beAltGrenadesColor[2] * 255;

					if (cg_altGrenades.integer == 2 && useFullbright && (cg_drawBrightWeapons.integer & 1 || cg_drawBrightWeapons.integer & 2) && CG_SelectBrightWeaponsShader())
					{
						ent.customShader = CG_SelectBrightWeaponsShader();
					}
					else
					{
						ent.customShader = cgs.media.grenadeCPMANoPicMipShaderNew;
					}
				}
				else
				{
					ent.shaderRGBA[0] = cg.beEnemyGrenadesColor[0] * 255;
					ent.shaderRGBA[1] = cg.beEnemyGrenadesColor[1] * 255;
					ent.shaderRGBA[2] = cg.beEnemyGrenadesColor[2] * 255;

					if (cg_altGrenades.integer == 2 && cg_drawBrightWeapons.integer & 4 && CG_SelectBrightWeaponsShader())
					{
						ent.customShader = CG_SelectBrightWeaponsShader();
					}
					else
					{
						ent.customShader = cgs.media.grenadeCPMANoPicMipShaderNew;
					}
				}

				ent.shaderRGBA[3] = 255;
			}
			else
			{
				ent.customShader = cgs.media.grenadeCPMANoPicMipShaderNew;
			}
		}
		else
		{
			if ((cg_nomip.integer & 8) && cgs.media.grenadeCPMANoPicMipShader)
			{
				ent.customShader = cgs.media.grenadeCPMANoPicMipShader;
			}
		}

		ent.hModel = cgs.media.grenadeCPMAModel;
	}




	// convert direction of travel into axis
	if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
	{
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if (s1->pos.trType != TR_STATIONARY)
	{
		RotateAroundDirection(ent.axis, cg.time / 4);
	}
	else
	{
		{
			RotateAroundDirection(ent.axis, s1->time);
		}
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups(&ent, s1, TEAM_FREE);
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
static void CG_Grapple(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;
	const weaponInfo_t*      weapon;

	s1 = &cent->currentState;
	if (s1->weapon > WP_NUM_WEAPONS)
	{
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

#if 0 // FIXME add grapple pull sound here..?
	// add missile sound
	if (weapon->missileSound)
	{
		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound);
	}
#endif

	// Will draw cable if needed
	CG_GrappleTrail(cent, weapon);

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if (VectorNormalize2(s1->pos.trDelta, ent.axis[0]) == 0)
	{
		ent.axis[0][2] = 1;
	}

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis(cent->lerpAngles, ent.axis);

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = (cg.time >> 6) & 1;

	// get the model, either as a bmodel or a modelindex
	if (s1->solid == SOLID_BMODEL)
	{
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	}
	else
	{
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if (s1->modelindex2)
	{
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}

}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(s1->pos.trBase, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal(centity_t* cent)
{
	refEntity_t         ent;
	entityState_t*       s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	ByteToDir(s1->eventParm, ent.axis[0]);
	PerpendicularVector(ent.axis[1], ent.axis[0]);

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract(vec3_origin, ent.axis[1], ent.axis[1]);

	CrossProduct(ent.axis[0], ent.axis[1], ent.axis[2]);
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;      // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360;  // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out)
{
	centity_t*   cent;
	vec3_t       oldOrigin, origin, deltaOrigin;
	vec3_t       oldAngles, angles, deltaAngles;

	if (moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL)
	{
		VectorCopy(in, out);
		VectorCopy(angles_in, angles_out);
		return;
	}

	cent = &cg_entities[moverNum];
	if (cent->currentState.eType != ET_MOVER)
	{
		VectorCopy(in, out);
		VectorCopy(angles_in, angles_out);
		return;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);
	BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
	BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

	VectorSubtract(origin, oldOrigin, deltaOrigin);
	VectorAdd(in, deltaOrigin, out);

	VectorSubtract(angles, oldAngles, deltaAngles);
	VectorAdd(angles_in, deltaAngles, angles_out);
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition(centity_t* cent)
{
	static trajectory_t currStates[MAX_CLIENTS];
	static trajectory_t prevStates[MAX_CLIENTS];
	static int          timeStates[MAX_CLIENTS];
	static int          prevTimes[MAX_CLIENTS];
	vec3_t       current, next;
	float        f;
	int          client;
	timepos_t    positions[5];

	if (cg.nextSnap == NULL)
		CG_Error("CG_InterpoateEntityPosition: cg.nextSnap == NULL");

	client = cent->currentState.clientNum;
	if (timeStates[client] != cg.snap->serverTime)
	{
		prevTimes[client] = timeStates[client];
		timeStates[client] = cg.snap->serverTime;
		prevStates[client] = currStates[client];
		currStates[client] = cent->currentState.pos;
	}

	f = cg.frameInterpolation + cg_entityOverpolate.value / (float)(cg.nextSnap->serverTime - cg.snap->serverTime);

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, positions[1].pos);
	BG_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, positions[2].pos);
	BG_EvaluateTrajectory(&prevStates[client], prevTimes[client], positions[0].pos);

	positions[0].time = (float)prevTimes[client];
	positions[1].time = (float)cg.snap->serverTime;
	positions[2].time = (float)cg.nextSnap->serverTime;
	positions[3].time = positions[2].time + (positions[2].time - positions[1].time);

	if (cg_enhancedInterpolation.integer)
	{
		CG_PolyBasePredictPosition(positions, current, f);
		VectorCopy(current, cent->lerpOrigin);
	}
	else
	{
		cent->lerpOrigin[0] = positions[1].pos[0] + f * (positions[2].pos[0] - positions[1].pos[0]);
		cent->lerpOrigin[1] = positions[1].pos[1] + f * (positions[2].pos[1] - positions[1].pos[1]);
		cent->lerpOrigin[2] = positions[1].pos[2] + f * (positions[2].pos[2] - positions[1].pos[2]);
	}

	BG_EvaluateTrajectory(&cent->currentState.apos, cg.snap->serverTime, current);
	BG_EvaluateTrajectory(&cent->nextState.apos, cg.nextSnap->serverTime, next);

	cent->lerpAngles[0] = LerpAngle(current[0], next[0], f);
	cent->lerpAngles[1] = LerpAngle(current[1], next[1], f);
	cent->lerpAngles[2] = LerpAngle(current[2], next[2], f);
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions(centity_t* cent)
{
	int timeshift = 0;

	// if this player does not want to see extrapolated players
	if (!cg_smoothClients.integer)
	{
		// make sure the clients use TR_INTERPOLATE
		if (cent->currentState.number < MAX_CLIENTS)
		{
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if (cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE)
	{
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if (cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
	        cent->currentState.number < MAX_CLIENTS)
	{
		CG_InterpolateEntityPosition(cent);
		return;
	}

	// if it's a missile but not a grappling hook
	if (cent->currentState.eType == ET_MISSILE && cent->currentState.weapon != WP_GRAPPLING_HOOK && cg_projectileNudge.integer && cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR && !cg.demoPlayback)
	{
		int serverTick = 0;
		if (cg.nextSnap && cg.snap)
		{
			serverTick = cg.nextSnap->serverTime - cg.snap->serverTime;
		}
		// if it's one of ours
		if ((cent->currentState.otherEntityNum) == cg.clientNum && (cg_projectileNudge.integer & 2))
		{
			// extrapolate one server frame's worth - this will correct for tiny
			// visual inconsistencies introduced by backward-reconciling all players
			// one server frame before running projectiles
			timeshift = serverTick;
		}
		// if it's not, and it's not a grenade launcher
		else if ((cent->currentState.weapon != WP_GRENADE_LAUNCHER) && (cg_projectileNudge.integer & 1))
		{
			// extrapolate based on cg_projectileNudge
			timeshift = cgs.osp.pingMs + serverTick;
		}
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time + timeshift, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time + timeshift, cent->lerpAngles);

	// if there's a time shift
	if (timeshift != 0)
	{
		trace_t tr;
		vec3_t lastOrigin;

		BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, lastOrigin);

		CG_Trace(&tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin, cent->currentState.number, MASK_SHOT);

		// don't let the projectile go through the floor
		if (tr.fraction < 1.0f)
		{
			cent->lerpOrigin[0] = lastOrigin[0] + tr.fraction * (cent->lerpOrigin[0] - lastOrigin[0]);
			cent->lerpOrigin[1] = lastOrigin[1] + tr.fraction * (cent->lerpOrigin[1] - lastOrigin[1]);
			cent->lerpOrigin[2] = lastOrigin[2] + tr.fraction * (cent->lerpOrigin[2] - lastOrigin[2]);
		}
	}

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if (cent != &cg.predictedPlayerEntity)
	{
		CG_AdjustPositionForMover(cent->lerpOrigin, cent->currentState.groundEntityNum,
		                          cg.snap->serverTime, cg.time, cent->lerpOrigin,
		                          cent->lerpAngles, cent->lerpAngles);
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase(centity_t* cent)
{
	refEntity_t model;
	if (cgs.gametype == GT_CTF)
	{
		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy(cent->lerpOrigin, model.lightingOrigin);
		VectorCopy(cent->lerpOrigin, model.origin);
		AnglesToAxis(cent->currentState.angles, model.axis);
		if (cent->currentState.modelindex == TEAM_RED)
		{
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if (cent->currentState.modelindex == TEAM_BLUE)
		{
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else
		{
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		trap_R_AddRefEntityToScene(&model);
	}
}

/*
===============
CG_AddCEntity

===============
*/
/*
================
CG_IsKillerGhostBody
Killer-ghost corpses: entityState weapon 0 with Quad and Invis bits only (server marker).
================
*/
qboolean CG_IsKillerGhostBody(const entityState_t* s)
{
	const int mask = (1 << PW_QUAD) | (1 << PW_INVIS);
	if (s->weapon != 0)
	{
		return qfalse;
	}
	if ((s->powerups & mask) != mask)
	{
		return qfalse;
	}
	return qtrue;
}

static void CG_AddCEntity(centity_t* cent)
{
	// event-only entities will have been dealt with already
	if (cent->currentState.eType >= ET_EVENTS)
	{
		return;
	}

	if (cent->currentState.eFlags & EF_DEAD && cg_deadBodyFilter.integer)
	{
		return;
	}

	if (cent->currentState.eType == ET_PLAYER && !cg_killerGhost.integer && CG_IsKillerGhostBody(&cent->currentState))
	{
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions(cent);

	// add automatic effects
	CG_EntityEffects(cent);

	switch (cent->currentState.eType)
	{
		default:
			CG_Error("Bad entity type: %i\n", cent->currentState.eType);
			break;
		case ET_INVISIBLE:
		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
			break;
		case ET_GENERAL:
			CG_General(cent);
			break;
		case ET_PLAYER:
			CG_Player(cent);
			break;
		case ET_ITEM:
			CG_Item(cent);
			break;
		case ET_MISSILE:
			CG_Missile(cent);
			break;
		case ET_MOVER:
			CG_Mover(cent);
			break;
		case ET_BEAM:
			CG_Beam(cent);
			break;
		case ET_PORTAL:
			CG_Portal(cent);
			break;
		case ET_SPEAKER:
			if (s_ambient.integer)
			{
				CG_Speaker(cent);
			}
			break;
		case ET_GRAPPLE:
			CG_Grapple(cent);
			break;
		case ET_TEAM:
			CG_TeamBase(cent);
			break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities(void)
{
	int                 num;
	int                 delta;
	centity_t*           cent;
	playerState_t*       ps;
	static int          lastFrameTime = 0;
	static int          lastSpectatorTarget = -1;
	int                 currentSpectatorTarget;

	/* Spectator target change: only when cg_spectPOV and new frame */
	if (cg_spectPOV.integer && cg.time != lastFrameTime)
	{
		lastFrameTime = cg.time;
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
			lastSpectatorTarget = -1;
		else
		{
			currentSpectatorTarget = (cg.snap->ps.pm_flags & PMF_FOLLOW &&
			                          cg.snap->ps.clientNum >= 0 &&
			                          cg.snap->ps.clientNum < MAX_CLIENTS)
			                         ? cg.snap->ps.clientNum : -1;
			if (currentSpectatorTarget != lastSpectatorTarget)
			{
				lastSpectatorTarget = currentSpectatorTarget;
				if (cgs.gametype >= GT_TEAM || cg_spectPOV.integer)
				{
					int index;

					for (index = 0; index < MAX_CLIENTS; ++index)
					{
						if (index != cg.clientNum && CG_ConfigString(CS_PLAYERS + index))
						{
							CG_NewClientInfo(index);
						}
					}
				}

			}
		}
	}

	/* frameInterpolation: one branch, delta at top */
	if (!cg.nextSnap)
		cg.frameInterpolation = 0;
	else
	{
		delta = cg.nextSnap->serverTime - cg.snap->serverTime;
		cg.frameInterpolation = (delta == 0) ? 0 : (float)(cg.time - cg.snap->serverTime) / delta;
	}

	/* auto-rotating items */
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0;
	cg.autoAngles[2] = 0;
	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;
	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	/* predicted player entity */
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState(ps, &cg.predictedPlayerEntity.currentState, qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);
	CG_CalcEntityLerpPositions(&cg_entities[ cg.snap->ps.clientNum ]);

	/* pre-add nextSnap entities that need no interpolation */
	if (cg.nextSnap)
		for (num = 0 ; num < cg.nextSnap->numEntities ; num++)
		{
			cent = &cg_entities[ cg.nextSnap->entities[ num ].number ];
			if (cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL)
				continue;
			CG_TransitionEntity(cent);
			cent->interpolate = qtrue;
			CG_AddCEntity(cent);
		}

	/* add snap entities (skip if already added from nextSnap) */
	for (num = 0 ; num < cg.snap->numEntities ; num++)
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if (cg.nextSnap && (cent->nextState.eType == ET_MISSILE || cent->nextState.eType == ET_GENERAL))
			continue;
		CG_AddCEntity(cent);
	}

	CG_AddMapSpawns();
	CG_AddMapTPExits();
	CG_AddShadowItems();
}
