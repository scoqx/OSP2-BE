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
	if (cent->currentState.loopSound)
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
	ent.radius = 14;
	ent.customShader = cg_items[modelIndex].icon;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

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
CG_Item
==================
*/
static void CG_Item(centity_t* cent)
{
	refEntity_t     ent;
	entityState_t*   es;
	gitem_t*         item;
	int             msec;
	float           frac;
	float           scale;
	weaponInfo_t*    wi;

	es = &cent->currentState;
	if (es->modelindex >= bg_numItems)
	{
		CG_Error("Bad item index %i on entity", es->modelindex);
	}

	// if set to invisible, skip
	if (!es->modelindex || (es->eFlags & EF_NODRAW))
	{
		return;
	}

	item = &bg_itemlist[ es->modelindex ];
	if (cg_simpleItems.integer && item->giType != IT_TEAM)
	{
		CG_AddSimpleItem(cent);
		return;
	}

	// items bob up and down continuously
	if (cg_itemFx.integer & 1)
	{
		scale = 0.005 + cent->currentState.number * 0.00001;
		cent->lerpOrigin[2] += 4 + cos((cg.time + 1000) * scale) * 4;
	}

	memset(&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if (cg_itemFx.integer & 2)
	{
		if (item->giType == IT_HEALTH)
		{
			VectorCopy(cg.autoAnglesFast, cent->lerpAngles);
			AxisCopy(cg.autoAxisFast, ent.axis);
		}
		else
		{
			VectorCopy(cg.autoAngles, cent->lerpAngles);
			AxisCopy(cg.autoAxis, ent.axis);
		}
	}
	else
	{
		VectorClear(cent->lerpAngles);
		AxisClear(ent.axis);
		ent.axis[0][0] = 1;
		ent.axis[1][1] = 1;
		ent.axis[2][2] = 1;
	}



	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if (item->giType == IT_WEAPON)
	{
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -=
		    wi->weaponMidpoint[0] * ent.axis[0][0] +
		    wi->weaponMidpoint[1] * ent.axis[1][0] +
		    wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -=
		    wi->weaponMidpoint[0] * ent.axis[0][1] +
		    wi->weaponMidpoint[1] * ent.axis[1][1] +
		    wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -=
		    wi->weaponMidpoint[0] * ent.axis[0][2] +
		    wi->weaponMidpoint[1] * ent.axis[1][2] +
		    wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;   // an extra height boost
	}

	ent.hModel = cg_items[es->modelindex].models[0];

	VectorCopy(cent->lerpOrigin, ent.origin);
	VectorCopy(cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	if (cg_itemFx.integer & 4)
	{
		msec = cg.time - cent->miscTime;
		if (msec >= 0 && msec < ITEM_SCALEUP_TIME)
		{
			frac = (float)msec / ITEM_SCALEUP_TIME;
			VectorScale(ent.axis[0], frac, ent.axis[0]);
			VectorScale(ent.axis[1], frac, ent.axis[1]);
			VectorScale(ent.axis[2], frac, ent.axis[2]);
			ent.nonNormalizedAxes = qtrue;
		}
		else
		{
			frac = 1.0;
		}
	}
	else
	{
		frac = 1.0;
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
	if ((item->giType == IT_WEAPON) ||
	        (item->giType == IT_ARMOR))
	{
		ent.renderfx |= RF_MINLIGHT;
	}

	// increase the size of the weapons when they are presented as items
	if (item->giType == IT_WEAPON)
	{
		VectorScale(ent.axis[0], 1.5, ent.axis[0]);
		VectorScale(ent.axis[1], 1.5, ent.axis[1]);
		VectorScale(ent.axis[2], 1.5, ent.axis[2]);
		ent.nonNormalizedAxes = qtrue;
		if (cg_drawBrightWeapons.integer & 8)
		{
			ent.customShader = cgs.media.firstPersonGun;
			CG_SetWeaponBrightColorWorld(&ent, item->giTag);
		}
	}

	// set railgun color to client's rail rings color
	if (item->giType == IT_WEAPON && item->giTag == WP_RAILGUN && !(cg_drawBrightWeapons.integer & 8))
	{
		if (cg_railCustomChamber.integer == 2)
		{
			clientInfo_t* ci = &cgs.clientinfo[cg.clientNum];
			ent.shaderRGBA[0] = 255 * ci->colors.railRings[0];
			ent.shaderRGBA[1] = 255 * ci->colors.railRings[1];
			ent.shaderRGBA[2] = 255 * ci->colors.railRings[2];
			ent.shaderRGBA[3] = 255;
		}
		else
		{
			ent.shaderRGBA[0] = 255;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 255;
			ent.shaderRGBA[3] = 255;
		}
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// accompanying rings / spheres for powerups
	if (!cg_simpleItems.integer)
	{
		vec3_t spinAngles;

		VectorClear(spinAngles);

		if (item->giType == IT_HEALTH || item->giType == IT_POWERUP)
		{
			if ((ent.hModel = cg_items[es->modelindex].models[1]) != 0)
			{
				if (item->giType == IT_POWERUP)
				{
					ent.origin[2] += 12;
					spinAngles[1] = (cg.time & 1023) * 360 / -1024.0f;
				}
				AnglesToAxis(spinAngles, ent.axis);

				// scale up if respawning
				if (frac != 1.0)
				{
					VectorScale(ent.axis[0], frac, ent.axis[0]);
					VectorScale(ent.axis[1], frac, ent.axis[1]);
					VectorScale(ent.axis[2], frac, ent.axis[2]);
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene(&ent);
			}
		}
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
//	int  col;

	s1 = &cent->currentState;
	if (s1->weapon > WP_NUM_WEAPONS)
	{
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy(s1->angles, cent->lerpAngles);

	// add trails
	if (weapon->missileTrailFunc)
	{
		weapon->missileTrailFunc(cent, weapon);
	}
	/*
	    if ( cent->currentState.modelindex == TEAM_RED ) {
	        col = 1;
	    }
	    else if ( cent->currentState.modelindex == TEAM_BLUE ) {
	        col = 2;
	    }
	    else {
	        col = 0;
	    }

	    // add dynamic light
	    if ( weapon->missileDlight ) {
	        trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
	            weapon->missileDlightColor[col][0], weapon->missileDlightColor[col][1], weapon->missileDlightColor[col][2] );
	    }
	*/
	// add dynamic light
	if (weapon->missileDlight)
	{
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
		                       weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2]);
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

	if (cent->currentState.weapon == WP_PLASMAGUN)
	{
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;

		if (cg_altPlasma.integer == 2)
		{
			ent.customShader = cgs.media.plasmaOldBallShader;
		}
		else if (cg_oldPlasma.integer || !cg_altPlasma.integer || !(cgs.osp.custom_client & OSP_CUSTOM_CLIENT_ALT_WEAPON_FLAG))
		{
			if ((cg_nomip.integer & 2) == 0)
			{
				ent.customShader = cgs.media.plasmaBallShader;
			}
			else
			{
				ent.customShader = cgs.media.plasmaBallNoPicMipShader;
			}
		}
		else
		{
			if ((cg_nomip.integer & 2) == 0)
			{
				ent.customShader = cgs.media.plasmaNewBallShader;
			}
			else
			{
				ent.customShader = cgs.media.plasmaNewBallNoPicMipShader;
			}
		}

		trap_R_AddRefEntityToScene(&ent);
		return;
	}


	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	if (cent->currentState.weapon == WP_GRENADE_LAUNCHER && cg_altGrenades.integer)
	{
		if (cg_altGrenades.integer == 2)
		{
			int owner = s1->otherEntityNum;

			if (owner >= 0 && owner < MAX_CLIENTS)
			{
				const clientInfo_t* ci = &cgs.clientinfo[owner];

				if (CG_IsEnemy(ci))
				{
					ent.shaderRGBA[0] = cgs.be.enemyGrenadesColor[0] * 255;
					ent.shaderRGBA[1] = cgs.be.enemyGrenadesColor[1] * 255;
					ent.shaderRGBA[2] = cgs.be.enemyGrenadesColor[2] * 255;

					if (cg_drawBrightWeapons.integer & 4 && cgs.media.firstPersonGun)
					{
						ent.customShader = cgs.media.firstPersonGun;
					}
					else
					{
						ent.customShader = cgs.media.grenadeCPMANoPicMipShaderNew;
					}
				}
				else
				{
					ent.shaderRGBA[0] = cgs.be.altGrenadesColor[0] * 255;
					ent.shaderRGBA[1] = cgs.be.altGrenadesColor[1] * 255;
					ent.shaderRGBA[2] = cgs.be.altGrenadesColor[2] * 255;

					if ((cg_drawBrightWeapons.integer & 1 || cg_drawBrightWeapons.integer & 2) && cgs.media.firstPersonGun)
					{
						ent.customShader = cgs.media.firstPersonGun;
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
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out)
{
	centity_t*   cent;
	vec3_t  oldOrigin, origin, deltaOrigin;
	vec3_t  oldAngles, angles;

	if (moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL)
	{
		VectorCopy(in, out);
		return;
	}

	cent = &cg_entities[ moverNum ];
	if (cent->currentState.eType != ET_MOVER)
	{
		VectorCopy(in, out);
		return;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);

	BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
	BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

	VectorSubtract(origin, oldOrigin, deltaOrigin);

	VectorAdd(in, deltaOrigin, out);

	// FIXME: origin change when on a rotating object
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition(centity_t* cent)
{
	vec3_t      current, next;
	float       f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if (cg.nextSnap == NULL)
	{
		CG_Error("CG_InterpoateEntityPosition: cg.nextSnap == NULL");
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory(&cent->currentState.pos, cg.snap->serverTime, current);
	BG_EvaluateTrajectory(&cent->nextState.pos, cg.nextSnap->serverTime, next);

	cent->lerpOrigin[0] = current[0] + f * (next[0] - current[0]);
	cent->lerpOrigin[1] = current[1] + f * (next[1] - current[1]);
	cent->lerpOrigin[2] = current[2] + f * (next[2] - current[2]);

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
		                          cg.snap->serverTime, cg.time, cent->lerpOrigin);
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
	centity_t*           cent;
	playerState_t*       ps;

	// set cg.frameInterpolation
	if (cg.nextSnap)
	{
		int     delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if (delta == 0)
		{
			cg.frameInterpolation = 0;
		}
		else
		{
			cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / delta;
		}
	}
	else
	{
		cg.frameInterpolation = 0;  // actually, it should never be used, because
		// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = (cg.time & 2047) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = (cg.time & 1023) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis(cg.autoAngles, cg.autoAxis);
	AnglesToAxis(cg.autoAnglesFast, cg.autoAxisFast);

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState(ps, &cg.predictedPlayerEntity.currentState, qfalse);
	CG_AddCEntity(&cg.predictedPlayerEntity);

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions(&cg_entities[ cg.snap->ps.clientNum ]);

	if (cg.nextSnap)
	{
		// pre-add some of the entities sent over by the server
		// we have data for them and they don't need to interpolate
		for (num = 0 ; num < cg.nextSnap->numEntities ; num++)
		{
			cent = &cg_entities[ cg.nextSnap->entities[ num ].number ];
			if (cent->nextState.eType == ET_MISSILE || cent->nextState.eType == ET_GENERAL)
			{
				// transition it immediately and add it
				CG_TransitionEntity(cent);
				cent->interpolate = qtrue;
				CG_AddCEntity(cent);
			}
		}
	}

	// add each entity sent over by the server
	for (num = 0 ; num < cg.snap->numEntities ; num++)
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if (!cg.nextSnap || (cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL))
		{
			CG_AddCEntity(cent);
		}
	}
}

