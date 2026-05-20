// Copyright (C) 1999-2000 Id Software, Inc.
//
// ui_players.c

#include "ui_local.h"


#define UI_TIMER_GESTURE        2300
#define UI_TIMER_JUMP           1000
#define UI_TIMER_LAND           130
#define UI_TIMER_WEAPON_SWITCH  300
#define UI_TIMER_ATTACK         500
#define UI_TIMER_MUZZLE_FLASH   20
#define UI_TIMER_WEAPON_DELAY   250

#define JUMP_HEIGHT             56

#define SWINGSPEED              0.3f

#define SPIN_SPEED              0.9f
#define COAST_TIME              1000


static int          dp_realtime;
static float        jumpHeight;
static qhandle_t    uiCandleFlameShader;
static qhandle_t    uiCandleFlameShaderBlue;


/*
===============
UI_PlayerInfo_SetWeapon
===============
*/
static void UI_PlayerInfo_SetWeapon(playerInfo_t* pi, weapon_t weaponNum)
{
	gitem_t*    item;
	char        path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
tryagain:
	pi->realWeapon = weaponNum;
	pi->weaponModel = 0;
	pi->barrelModel = 0;
	pi->flashModel = 0;

	if (weaponNum == WP_NONE)
	{
		return;
	}

	for (item = bg_itemlist + 1; item->classname ; item++)
	{
		if (item->giType != IT_WEAPON)
		{
			continue;
		}
		if (item->giTag == weaponNum)
		{
			break;
		}
	}

	if (item->classname)
	{
		pi->weaponModel = trap_R_RegisterModel(item->world_model[0]);
	}

	if (pi->weaponModel == 0)
	{
		if (weaponNum == WP_MACHINEGUN)
		{
			weaponNum = WP_NONE;
			goto tryagain;
		}
		weaponNum = WP_MACHINEGUN;
		goto tryagain;
	}

	if (weaponNum == WP_MACHINEGUN || weaponNum == WP_GAUNTLET || weaponNum == WP_BFG)
	{
		COM_StripExtension(item->world_model[0], path, sizeof(path));
		Q_strcat(path, sizeof(path), "_barrel.md3");
		pi->barrelModel = trap_R_RegisterModel(path);
	}

	COM_StripExtension(item->world_model[0], path, sizeof(path));
	Q_strcat(path, sizeof(path), "_flash.md3");
	pi->flashModel = trap_R_RegisterModel(path);

	switch (weaponNum)
	{
		case WP_GAUNTLET:
			MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
			break;

		case WP_MACHINEGUN:
			MAKERGB(pi->flashDlightColor, 1, 1, 0);
			break;

		case WP_SHOTGUN:
			MAKERGB(pi->flashDlightColor, 1, 1, 0);
			break;

		case WP_GRENADE_LAUNCHER:
			MAKERGB(pi->flashDlightColor, 1, 0.7f, 0.5f);
			break;

		case WP_ROCKET_LAUNCHER:
			MAKERGB(pi->flashDlightColor, 1, 0.75f, 0);
			break;

		case WP_LIGHTNING:
			MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
			break;

		case WP_RAILGUN:
			MAKERGB(pi->flashDlightColor, 1, 0.5f, 0);
			break;

		case WP_PLASMAGUN:
			MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
			break;

		case WP_BFG:
			MAKERGB(pi->flashDlightColor, 1, 0.7f, 1);
			break;

		case WP_GRAPPLING_HOOK:
			MAKERGB(pi->flashDlightColor, 0.6f, 0.6f, 1);
			break;

		default:
			MAKERGB(pi->flashDlightColor, 1, 1, 1);
			break;
	}
}


/*
===============
UI_ForceLegsAnim
===============
*/
static void UI_ForceLegsAnim(playerInfo_t* pi, int anim)
{
	pi->legsAnim = ((pi->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if (anim == LEGS_JUMP)
	{
		pi->legsAnimationTimer = UI_TIMER_JUMP;
	}
}


/*
===============
UI_SetLegsAnim
===============
*/
static void UI_SetLegsAnim(playerInfo_t* pi, int anim)
{
	if (pi->pendingLegsAnim)
	{
		anim = pi->pendingLegsAnim;
		pi->pendingLegsAnim = 0;
	}
	UI_ForceLegsAnim(pi, anim);
}


/*
===============
UI_ForceTorsoAnim
===============
*/
static void UI_ForceTorsoAnim(playerInfo_t* pi, int anim)
{
	pi->torsoAnim = ((pi->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;

	if (anim == TORSO_GESTURE)
	{
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;
	}

	if (anim == TORSO_ATTACK || anim == TORSO_ATTACK2)
	{
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
	}
}


/*
===============
UI_SetTorsoAnim
===============
*/
static void UI_SetTorsoAnim(playerInfo_t* pi, int anim)
{
	if (pi->pendingTorsoAnim)
	{
		anim = pi->pendingTorsoAnim;
		pi->pendingTorsoAnim = 0;
	}

	UI_ForceTorsoAnim(pi, anim);
}


/*
===============
UI_TorsoSequencing
===============
*/
static void UI_TorsoSequencing(playerInfo_t* pi)
{
	int     currentAnim;

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if (pi->weapon != pi->currentWeapon)
	{
		if (currentAnim != TORSO_DROP)
		{
			pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
			UI_ForceTorsoAnim(pi, TORSO_DROP);
		}
	}

	if (pi->torsoAnimationTimer > 0)
	{
		return;
	}

	if (currentAnim == TORSO_GESTURE)
	{
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if (currentAnim == TORSO_ATTACK || currentAnim == TORSO_ATTACK2)
	{
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}

	if (currentAnim == TORSO_DROP)
	{
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
		UI_ForceTorsoAnim(pi, TORSO_RAISE);
		return;
	}

	if (currentAnim == TORSO_RAISE)
	{
		UI_SetTorsoAnim(pi, TORSO_STAND);
		return;
	}
}


/*
===============
UI_LegsSequencing
===============
*/
static void UI_LegsSequencing(playerInfo_t* pi)
{
	int     currentAnim;

	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;

	if (pi->legsAnimationTimer > 0)
	{
		if (currentAnim == LEGS_JUMP)
		{
			jumpHeight = JUMP_HEIGHT * sin(M_PI * (UI_TIMER_JUMP - pi->legsAnimationTimer) / UI_TIMER_JUMP);
		}
		return;
	}

	if (currentAnim == LEGS_JUMP)
	{
		UI_ForceLegsAnim(pi, LEGS_LAND);
		pi->legsAnimationTimer = UI_TIMER_LAND;
		jumpHeight = 0;
		return;
	}

	if (currentAnim == LEGS_LAND)
	{
		UI_SetLegsAnim(pi, LEGS_IDLE);
		return;
	}
}


/*
======================
UI_PositionEntityOnTag
======================
*/
static void UI_PositionEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
                                   clipHandle_t parentModel, char* tagName)
{
	int             i;
	orientation_t   lerped;

	// lerp the tag
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
	                1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// cast away const because of compiler problems
	MatrixMultiply(lerped.axis, ((refEntity_t*)parent)->axis, entity->axis);
	entity->backlerp = parent->backlerp;
}


/*
======================
UI_PositionRotatedEntityOnTag
======================
*/
static void UI_PositionRotatedEntityOnTag(refEntity_t* entity, const refEntity_t* parent,
        clipHandle_t parentModel, char* tagName)
{
	int             i;
	orientation_t   lerped;
	vec3_t          tempAxis[3];

	// lerp the tag
	trap_CM_LerpTag(&lerped, parentModel, parent->oldframe, parent->frame,
	                1.0 - parent->backlerp, tagName);

	// FIXME: allow origin offsets along tag?
	VectorCopy(parent->origin, entity->origin);
	for (i = 0 ; i < 3 ; i++)
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// cast away const because of compiler problems
	MatrixMultiply(entity->axis, ((refEntity_t*)parent)->axis, tempAxis);
	MatrixMultiply(lerped.axis, tempAxis, entity->axis);
}


/*
===============
UI_SetLerpFrameAnimation
===============
*/
static void UI_SetLerpFrameAnimation(playerInfo_t* ci, lerpFrame_t* lf, int newAnimation)
{
	animation_t* anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_ANIMATIONS)
	{
		trap_Error(va("Bad animation number: %i", newAnimation));
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}


/*
===============
UI_RunLerpFrame
===============
*/
static void UI_RunLerpFrame(playerInfo_t* ci, lerpFrame_t* lf, int newAnimation)
{
	int         f;
	animation_t* anim;

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation)
	{
		UI_SetLerpFrameAnimation(ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (dp_realtime >= lf->frameTime)
	{
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if (dp_realtime < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		if (f >= anim->numFrames)
		{
			f -= anim->numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = dp_realtime;
			}
		}
		lf->frame = anim->firstFrame + f;
		if (dp_realtime > lf->frameTime)
		{
			lf->frameTime = dp_realtime;
		}
	}

	if (lf->frameTime > dp_realtime + 200)
	{
		lf->frameTime = dp_realtime;
	}

	if (lf->oldFrameTime > dp_realtime)
	{
		lf->oldFrameTime = dp_realtime;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - (float)(dp_realtime - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}


/*
===============
UI_PlayerAnimation
===============
*/
static void UI_PlayerAnimation(playerInfo_t* pi, int* legsOld, int* legs, float* legsBackLerp,
                               int* torsoOld, int* torso, float* torsoBackLerp)
{

	if (pi->freezeActive)
	{
		if (pi->legs.animation)
		{
			pi->legs.oldFrame = pi->legs.frame = pi->legs.animation->firstFrame;
		}
		else
		{
			pi->legs.oldFrame = pi->legs.frame = 0;
		}
		if (pi->torso.animation)
		{
			pi->torso.oldFrame = pi->torso.frame = pi->torso.animation->firstFrame;
		}
		else
		{
			pi->torso.oldFrame = pi->torso.frame = 0;
		}
		pi->legs.backlerp = 0.0f;
		pi->torso.backlerp = 0.0f;
		pi->legsAnimationTimer = 0;
		pi->torsoAnimationTimer = 0;
		jumpHeight = 0;
		*legsOld = pi->legs.oldFrame;
		*legs = pi->legs.frame;
		*legsBackLerp = 0.0f;
		*torsoOld = pi->torso.oldFrame;
		*torso = pi->torso.frame;
		*torsoBackLerp = 0.0f;
		return;
	}

	// legs animation
	pi->legsAnimationTimer -= uis.frametime;
	if (pi->legsAnimationTimer < 0)
	{
		pi->legsAnimationTimer = 0;
	}

	UI_LegsSequencing(pi);

	if (pi->legs.yawing && (pi->legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE)
	{
		UI_RunLerpFrame(pi, &pi->legs, LEGS_TURN);
	}
	else
	{
		UI_RunLerpFrame(pi, &pi->legs, pi->legsAnim);
	}
	*legsOld = pi->legs.oldFrame;
	*legs = pi->legs.frame;
	*legsBackLerp = pi->legs.backlerp;

	// torso animation
	pi->torsoAnimationTimer -= uis.frametime;
	if (pi->torsoAnimationTimer < 0)
	{
		pi->torsoAnimationTimer = 0;
	}

	UI_TorsoSequencing(pi);

	UI_RunLerpFrame(pi, &pi->torso, pi->torsoAnim);
	*torsoOld = pi->torso.oldFrame;
	*torso = pi->torso.frame;
	*torsoBackLerp = pi->torso.backlerp;
}


/*
==================
UI_SwingAngles
==================
*/
static void UI_SwingAngles(float destination, float swingTolerance, float clampTolerance,
                           float speed, float* angle, qboolean* swinging)
{
	float   swing;
	float   move;
	float   scale;

	if (!*swinging)
	{
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance)
		{
			*swinging = qtrue;
		}
	}

	if (!*swinging)
	{
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if (scale < swingTolerance * 0.5)
	{
		scale = 0.5;
	}
	else if (scale < swingTolerance)
	{
		scale = 1.0;
	}
	else
	{
		scale = 2.0;
	}

	// swing towards the destination angle
	if (swing >= 0)
	{
		move = uis.frametime * scale * speed;
		if (move >= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	else
	{
		move = uis.frametime * scale * -speed;
		if (move <= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance)
	{
		*angle = AngleMod(destination - (clampTolerance - 1));
	}
	else if (swing < -clampTolerance)
	{
		*angle = AngleMod(destination + (clampTolerance - 1));
	}
}


/*
======================
UI_MovedirAdjustment
======================
*/
static float UI_MovedirAdjustment(playerInfo_t* pi)
{
	vec3_t      relativeAngles;
	vec3_t      moveVector;

	VectorSubtract(pi->viewAngles, pi->moveAngles, relativeAngles);
	AngleVectors(relativeAngles, moveVector, NULL, NULL);
	if (Q_fabs(moveVector[0]) < 0.01)
	{
		moveVector[0] = 0.0;
	}
	if (Q_fabs(moveVector[1]) < 0.01)
	{
		moveVector[1] = 0.0;
	}

	if (moveVector[1] == 0 && moveVector[0] > 0)
	{
		return 0;
	}
	if (moveVector[1] < 0 && moveVector[0] > 0)
	{
		return 22;
	}
	if (moveVector[1] < 0 && moveVector[0] == 0)
	{
		return 45;
	}
	if (moveVector[1] < 0 && moveVector[0] < 0)
	{
		return -22;
	}
	if (moveVector[1] == 0 && moveVector[0] < 0)
	{
		return 0;
	}
	if (moveVector[1] > 0 && moveVector[0] < 0)
	{
		return 22;
	}
	if (moveVector[1] > 0 && moveVector[0] == 0)
	{
		return  -45;
	}

	return -22;
}


/*
===============
UI_PlayerAngles
===============
*/
static void UI_PlayerAngles(playerInfo_t* pi, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t      legsAngles, torsoAngles, headAngles;
	float       dest;
	float       adjust;

	VectorCopy(pi->viewAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ((pi->legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
	        || (pi->torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND)
	{
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;   // always center
		pi->torso.pitching = qtrue; // always center
		pi->legs.yawing = qtrue;    // always center
	}

	// adjust legs for movement dir
	adjust = UI_MovedirAdjustment(pi);
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;


	// torso
	UI_SwingAngles(torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yawAngle, &pi->torso.yawing);
	UI_SwingAngles(legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yawAngle, &pi->legs.yawing);

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180)
	{
		dest = (-360 + headAngles[PITCH]) * 0.75;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75;
	}
	UI_SwingAngles(dest, 15, 30, 0.1f, &pi->torso.pitchAngle, &pi->torso.pitching);
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}


/*
===============
UI_PlayerFloatSprite
===============
*/
static void UI_PlayerFloatSprite(playerInfo_t* pi, vec3_t origin, qhandle_t shader)
{
	refEntity_t     ent;

	memset(&ent, 0, sizeof(ent));
	VectorCopy(origin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene(&ent);
}


/*
======================
UI_MachinegunSpinAngle
======================
*/
float   UI_MachinegunSpinAngle(playerInfo_t* pi)
{
	int     delta;
	float   angle;
	float   speed;
	int     torsoAnim;

	delta = dp_realtime - pi->barrelTime;
	if (pi->barrelSpinning)
	{
		angle = pi->barrelAngle + delta * SPIN_SPEED;
	}
	else
	{
		if (delta > COAST_TIME)
		{
			delta = COAST_TIME;
		}

		speed = 0.5 * (SPIN_SPEED + (float)(COAST_TIME - delta) / COAST_TIME);
		angle = pi->barrelAngle + delta * speed;
	}

	torsoAnim = pi->torsoAnim  & ~ANIM_TOGGLEBIT;
	if (torsoAnim == TORSO_ATTACK2)
	{
		torsoAnim = TORSO_ATTACK;
	}
	if (pi->barrelSpinning == !(torsoAnim == TORSO_ATTACK))
	{
		pi->barrelTime = dp_realtime;
		pi->barrelAngle = AngleMod(angle);
		pi->barrelSpinning = !!(torsoAnim == TORSO_ATTACK);
	}

	return angle;
}

static unsigned char UI_ColorFloatToByte(float value)
{
	int channel;

	if (value <= 0.0f)
	{
		return 0;
	}
	if (value >= 1.0f)
	{
		return 255;
	}

	channel = (int)(value * 255.0f + 0.5f);
	if (channel < 0)
	{
		channel = 0;
	}
	if (channel > 255)
	{
		channel = 255;
	}
	return (unsigned char)channel;
}

static void UI_AddOutlineEntity(const refEntity_t* source, const playerInfo_t* pi)
{
	refEntity_t outline;

	if (!pi->useOutline || !pi->outlineShader)
	{
		return;
	}

	outline = *source;
	outline.customSkin = 0;
	outline.customShader = pi->outlineShader;
	outline.shaderRGBA[0] = UI_ColorFloatToByte(pi->outlineColor[0]);
	outline.shaderRGBA[1] = UI_ColorFloatToByte(pi->outlineColor[1]);
	outline.shaderRGBA[2] = UI_ColorFloatToByte(pi->outlineColor[2]);
	outline.shaderRGBA[3] = 0xff;

	trap_R_AddRefEntityToScene(&outline);
}

static void UI_AddFrozenOverlay(const refEntity_t* source, const playerInfo_t* pi)
{
	refEntity_t overlay;

	if (!pi->freezeActive || !pi->freezeShader)
	{
		return;
	}

	overlay = *source;
	overlay.customSkin = 0;
	overlay.customShader = pi->freezeShader;
	overlay.shaderRGBA[0] = UI_ColorFloatToByte(pi->freezeColor[0]);
	overlay.shaderRGBA[1] = UI_ColorFloatToByte(pi->freezeColor[1]);
	overlay.shaderRGBA[2] = UI_ColorFloatToByte(pi->freezeColor[2]);
	overlay.shaderRGBA[3] = UI_ColorFloatToByte(pi->freezeColor[3]);

	trap_R_AddRefEntityToScene(&overlay);
}

static void UI_DrawCandleAtTag(const refEntity_t* torso, qboolean blueFlame)
{
	polyVert_t verts[4];
	polyVert_t capVerts[16];
	refEntity_t anchor;
	refEntity_t flame;
	vec3_t candleOrigin;
	vec3_t p0Bottom;
	vec3_t p1Bottom;
	vec3_t p0Top;
	vec3_t p1Top;
	float radius;
	float height;
	float angle0;
	float angle1;
	float x0;
	float y0;
	float x1;
	float y1;
	float t;
	int i;
	int segments;

	if (!uiCandleFlameShader)
	{
		uiCandleFlameShader = trap_R_RegisterShaderNoMip("textures/sfx/flame1");
	}
	if (!uiCandleFlameShaderBlue)
	{
		uiCandleFlameShaderBlue = trap_R_RegisterShaderNoMip("textures/sfx/flameanim_dimmer");
	}

	memset(&anchor, 0, sizeof(anchor));
	UI_PositionEntityOnTag(&anchor, torso, torso->hModel, "tag_weapon");
	VectorCopy(anchor.origin, candleOrigin);
	candleOrigin[2] -= 7.0f; /* Lower candle by 12 units to fit hands. */

	radius = 1.2f;
	height = 10.0f;
	segments = 12;

	for (i = 0; i < segments; i++)
	{
		angle0 = ((float)i / (float)segments) * (2.0f * M_PI);
		angle1 = ((float)(i + 1) / (float)segments) * (2.0f * M_PI);
		x0 = cos(angle0) * radius;
		y0 = sin(angle0) * radius;
		x1 = cos(angle1) * radius;
		y1 = sin(angle1) * radius;
		t = (float)i / (float)segments;

		VectorSet(p0Bottom, candleOrigin[0] + x0, candleOrigin[1] + y0, candleOrigin[2]);
		VectorSet(p1Bottom, candleOrigin[0] + x1, candleOrigin[1] + y1, candleOrigin[2]);
		VectorSet(p0Top, candleOrigin[0] + x0, candleOrigin[1] + y0, candleOrigin[2] + height);
		VectorSet(p1Top, candleOrigin[0] + x1, candleOrigin[1] + y1, candleOrigin[2] + height);

		VectorCopy(p0Top, verts[0].xyz);
		verts[0].st[0] = t;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 237;
		verts[0].modulate[2] = 179;
		verts[0].modulate[3] = 255;

		VectorCopy(p0Bottom, verts[1].xyz);
		verts[1].st[0] = t;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 237;
		verts[1].modulate[2] = 179;
		verts[1].modulate[3] = 255;

		VectorCopy(p1Bottom, verts[2].xyz);
		verts[2].st[0] = t + 1.0f / segments;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 237;
		verts[2].modulate[2] = 179;
		verts[2].modulate[3] = 255;

		VectorCopy(p1Top, verts[3].xyz);
		verts[3].st[0] = t + 1.0f / segments;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 237;
		verts[3].modulate[2] = 179;
		verts[3].modulate[3] = 255;

		trap_R_AddPolyToScene(uis.whiteShader, 4, verts);
	}

	for (i = 0; i < segments; i++)
	{
		angle0 = ((float)i / (float)segments) * (2.0f * M_PI);
		x0 = cos(angle0) * radius;
		y0 = sin(angle0) * radius;

		capVerts[i].xyz[0] = candleOrigin[0] + x0;
		capVerts[i].xyz[1] = candleOrigin[1] + y0;
		capVerts[i].xyz[2] = candleOrigin[2] + height;
		capVerts[i].st[0] = 0.5f + x0 / (2.0f * radius);
		capVerts[i].st[1] = 0.5f + y0 / (2.0f * radius);
		capVerts[i].modulate[0] = 255;
		capVerts[i].modulate[1] = 245;
		capVerts[i].modulate[2] = 200;
		capVerts[i].modulate[3] = 255;
	}
	trap_R_AddPolyToScene(uis.whiteShader, segments, capVerts);

	for (i = 0; i < segments; i++)
	{
		angle0 = ((float)i / (float)segments) * (2.0f * M_PI);
		x0 = cos(angle0) * radius;
		y0 = sin(angle0) * radius;

		capVerts[i].xyz[0] = candleOrigin[0] + x0;
		capVerts[i].xyz[1] = candleOrigin[1] + y0;
		capVerts[i].xyz[2] = candleOrigin[2];
		capVerts[i].st[0] = 0.5f + x0 / (2.0f * radius);
		capVerts[i].st[1] = 0.5f + y0 / (2.0f * radius);
		capVerts[i].modulate[0] = 215;
		capVerts[i].modulate[1] = 172;
		capVerts[i].modulate[2] = 128;
		capVerts[i].modulate[3] = 255;
	}
	trap_R_AddPolyToScene(uis.whiteShader, segments, capVerts);

	memset(&flame, 0, sizeof(flame));
	VectorCopy(candleOrigin, flame.origin);
	flame.origin[2] += height + 3.0f;
	flame.reType = RT_SPRITE;
	flame.radius = 2.6f;
	flame.customShader = blueFlame ? uiCandleFlameShaderBlue : uiCandleFlameShader;
	flame.shaderRGBA[0] = 255;
	flame.shaderRGBA[1] = blueFlame ? 180 : 170;
	flame.shaderRGBA[2] = blueFlame ? 255 : 70;
	flame.shaderRGBA[3] = 230;

	trap_R_AddRefEntityToScene(&flame);
}


/*
===============
UI_DrawPlayer
===============
*/
void UI_DrawPlayer(float x, float y, float w, float h, playerInfo_t* pi, int time)
{
	refdef_t        refdef;
	refEntity_t     legs;
	refEntity_t     torso;
	refEntity_t     head;
	refEntity_t     gun;
	refEntity_t     barrel;
	refEntity_t     flash;
	vec3_t          origin;
	int             renderfx;
	vec3_t          mins = {-16, -16, -24};
	vec3_t          maxs = {16, 16, 32};
	float           len;
	float           xx;

	if (!pi->legsModel || !pi->torsoModel || !pi->headModel || !pi->animations[0].numFrames)
	{
		return;
	}

	dp_realtime = time;

	if (pi->pendingWeapon != WP_PENDING && dp_realtime > pi->weaponTimer)
	{
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = WP_PENDING;
		pi->weaponTimer = 0;
		if (pi->currentWeapon != pi->weapon)
		{
			trap_S_StartLocalSound(weaponChangeSound, CHAN_LOCAL);
		}
	}

	memset(&refdef, 0, sizeof(refdef));
	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	// calculate fov from virtual dimensions
	// so it will be resolution-independent
	refdef.fov_x = (int)(w / 640.0f * 90.0f);
	xx = w / tan(refdef.fov_x / 360 * M_PI);
	refdef.fov_y = atan2(h, xx);
	refdef.fov_y *= (360 / M_PI);

	UI_AdjustFrom640(&x, &y, &w, &h);

	y -= jumpHeight;

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	// calculate distance so the player nearly fills the box
	len = 0.7 * (maxs[2] - mins[2]);
	origin[0] = len / tan(DEG2RAD(refdef.fov_x) * 0.5);
	origin[1] = 0.5 * (mins[1] + maxs[1]);
	origin[2] = -0.5 * (mins[2] + maxs[2]);

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
	UI_PlayerAngles(pi, legs.axis, torso.axis, head.axis);

	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation(pi, &legs.oldframe, &legs.frame, &legs.backlerp,
	                   &torso.oldframe, &torso.frame, &torso.backlerp);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	//
	// add the legs
	//
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;
	if (pi->useCustomSkinColors)
	{
		legs.shaderRGBA[0] = UI_ColorFloatToByte(pi->customLegsColor[0]);
		legs.shaderRGBA[1] = UI_ColorFloatToByte(pi->customLegsColor[1]);
		legs.shaderRGBA[2] = UI_ColorFloatToByte(pi->customLegsColor[2]);
		legs.shaderRGBA[3] = 0xff;
	}
	else
	{
		memset(legs.shaderRGBA, 255, sizeof(legs.shaderRGBA));
	}
	if (pi->freezeActive)
	{
		legs.shaderRGBA[3] = UI_ColorFloatToByte(pi->freezeColor[3]);
	}

	VectorCopy(origin, legs.origin);

	VectorCopy(origin, legs.lightingOrigin);
	legs.renderfx = renderfx;
	VectorCopy(legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene(&legs);
	UI_AddOutlineEntity(&legs, pi);
	UI_AddFrozenOverlay(&legs, pi);

	if (!legs.hModel)
	{
		return;
	}

	//
	// add the torso
	//
	torso.hModel = pi->torsoModel;
	if (!torso.hModel)
	{
		return;
	}

	torso.customSkin = pi->torsoSkin;
	if (pi->useCustomSkinColors)
	{
		torso.shaderRGBA[0] = UI_ColorFloatToByte(pi->customTorsoColor[0]);
		torso.shaderRGBA[1] = UI_ColorFloatToByte(pi->customTorsoColor[1]);
		torso.shaderRGBA[2] = UI_ColorFloatToByte(pi->customTorsoColor[2]);
		torso.shaderRGBA[3] = 0xff;
	}
	else
	{
		memset(torso.shaderRGBA, 255, sizeof(torso.shaderRGBA));
	}
	if (pi->freezeActive)
	{
		torso.shaderRGBA[3] = UI_ColorFloatToByte(pi->freezeColor[3]);
	}

	VectorCopy(origin, torso.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&torso, &legs, pi->legsModel, "tag_torso");

	torso.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&torso);
	UI_AddOutlineEntity(&torso, pi);
	UI_AddFrozenOverlay(&torso, pi);

	//
	// add the head
	//
	head.hModel = pi->headModel;
	if (!head.hModel)
	{
		return;
	}
	head.customSkin = pi->headSkin;
	if (pi->useCustomSkinColors)
	{
		head.shaderRGBA[0] = UI_ColorFloatToByte(pi->customHeadColor[0]);
		head.shaderRGBA[1] = UI_ColorFloatToByte(pi->customHeadColor[1]);
		head.shaderRGBA[2] = UI_ColorFloatToByte(pi->customHeadColor[2]);
		head.shaderRGBA[3] = 0xff;
	}
	else
	{
		memset(head.shaderRGBA, 255, sizeof(head.shaderRGBA));
	}
	if (pi->freezeActive)
	{
		head.shaderRGBA[3] = UI_ColorFloatToByte(pi->freezeColor[3]);
	}

	VectorCopy(origin, head.lightingOrigin);

	UI_PositionRotatedEntityOnTag(&head, &torso, pi->torsoModel, "tag_head");

	head.renderfx = renderfx;

	trap_R_AddRefEntityToScene(&head);
	UI_AddOutlineEntity(&head, pi);
	UI_AddFrozenOverlay(&head, pi);

	if (!pi->freezeActive)
	{
		//
		// add the gun
		//
		if (pi->useCandleWeapon)
		{
			UI_DrawCandleAtTag(&torso, pi->useCustomWeaponColor);
		}
		else if (pi->currentWeapon != WP_NONE)
		{
			memset(&gun, 0, sizeof(gun));
			gun.hModel = pi->weaponModel;
			VectorCopy(origin, gun.lightingOrigin);
			UI_PositionEntityOnTag(&gun, &torso, pi->torsoModel, "tag_weapon");
			gun.renderfx = renderfx;
			// Set custom weapon color if specified
			if (pi->useCustomWeaponColor)
			{
				gun.shaderRGBA[0] = UI_ColorFloatToByte(pi->customWeaponColor[0]);
				gun.shaderRGBA[1] = UI_ColorFloatToByte(pi->customWeaponColor[1]);
				gun.shaderRGBA[2] = UI_ColorFloatToByte(pi->customWeaponColor[2]);
				gun.shaderRGBA[3] = UI_ColorFloatToByte(pi->customWeaponColor[3]);
			}
			trap_R_AddRefEntityToScene(&gun);
		}

		//
		// add the spinning barrel
		//
		if (!pi->useCandleWeapon && (pi->realWeapon == WP_MACHINEGUN || pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG))
		{
			vec3_t  angles;

			memset(&barrel, 0, sizeof(barrel));
			VectorCopy(origin, barrel.lightingOrigin);
			barrel.renderfx = renderfx;

			barrel.hModel = pi->barrelModel;
			angles[YAW] = 0;
			angles[PITCH] = 0;
			angles[ROLL] = UI_MachinegunSpinAngle(pi);
			if (pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG)
			{
				angles[PITCH] = angles[ROLL];
				angles[ROLL] = 0;
			}
			AnglesToAxis(angles, barrel.axis);

			UI_PositionRotatedEntityOnTag(&barrel, &gun, pi->weaponModel, "tag_barrel");

			trap_R_AddRefEntityToScene(&barrel);
		}

		//
		// add muzzle flash
		//
		if (!pi->useCandleWeapon && dp_realtime <= pi->muzzleFlashTime && pi->currentWeapon != WP_NONE)
		{
			if (pi->flashModel)
			{
				memset(&flash, 0, sizeof(flash));
				flash.hModel = pi->flashModel;
				VectorCopy(origin, flash.lightingOrigin);
				UI_PositionEntityOnTag(&flash, &gun, pi->weaponModel, "tag_flash");
				flash.renderfx = renderfx;
				trap_R_AddRefEntityToScene(&flash);
			}

			// make a dlight for the flash
			if (pi->flashDlightColor[0] || pi->flashDlightColor[1] || pi->flashDlightColor[2])
			{
				trap_R_AddLightToScene(flash.origin, 200 + (rand() & 31), pi->flashDlightColor[0],
				                       pi->flashDlightColor[1], pi->flashDlightColor[2]);
			}
		}
	}

	//
	// add the chat icon
	//
	if (pi->chat)
	{
		UI_PlayerFloatSprite(pi, origin, trap_R_RegisterShaderNoMip("sprites/balloon3"));
	}

	//
	// add an accent light
	//
	origin[0] -= 100;   // + = behind, - = in front
	origin[1] += 100;   // + = left, - = right
	origin[2] += 100;   // + = above, - = below
	trap_R_AddLightToScene(origin, 500, 1.0, 1.0, 1.0);

	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene(origin, 500, 1.0, 0.0, 0.0);

	trap_R_RenderScene(&refdef);
}


/*
==========================
UI_RegisterClientSkin
==========================
*/
static qboolean UI_RegisterClientSkin(playerInfo_t* pi, const char* modelName, const char* skinName)
{
	char        filename[MAX_QPATH];

	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower_%s.skin", modelName, skinName);
	pi->legsSkin = trap_R_RegisterSkin(filename);

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper_%s.skin", modelName, skinName);
	pi->torsoSkin = trap_R_RegisterSkin(filename);

	Com_sprintf(filename, sizeof(filename), "models/players/%s/head_%s.skin", modelName, skinName);
	pi->headSkin = trap_R_RegisterSkin(filename);

	if (!pi->legsSkin || !pi->torsoSkin || !pi->headSkin)
	{
		return qfalse;
	}

	return qtrue;
}


/*
======================
UI_ParseAnimationFile
======================
*/
static qboolean UI_ParseAnimationFile(const char* filename, animation_t* animations)
{
	char*        text_p, *prev;
	int         len;
	int         i;
	char*        token;
	float       fps;
	int         skip;
	char        text[20000];
	fileHandle_t    f;

	memset(animations, 0, sizeof(animation_t) * MAX_ANIMATIONS);

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0)
	{
		if (f != FS_INVALID_HANDLE)
		{
			trap_FS_FCloseFile(f);
		}
		return qfalse;
	}
	if (len >= (sizeof(text) - 1))
	{
		Com_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[ len ] = '\0';
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0;   // quite the compiler warning

	// read optional parameters
	while (1)
	{
		prev = text_p;  // so we can unget
		token = COM_Parse(&text_p);
		if (!token[0])
		{
			break;
		}
		if (!Q_stricmp(token, "footsteps"))
		{
			token = COM_Parse(&text_p);
			if (!token[0])
			{
				break;
			}
			continue;
		}
		else if (!Q_stricmp(token, "headoffset"))
		{
			for (i = 0 ; i < 3 ; i++)
			{
				token = COM_Parse(&text_p);
				if (!token[0])
				{
					break;
				}
			}
			continue;
		}
		else if (!Q_stricmp(token, "sex"))
		{
			token = COM_Parse(&text_p);
			if (!token[0])
			{
				break;
			}
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9')
		{
			text_p = prev;  // unget the token
			break;
		}

		Com_Printf("unknown token '%s' in %s\n", token, filename);
	}

	// read information for each frame
	for (i = 0 ; i < MAX_ANIMATIONS ; i++)
	{

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			if (i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE)
			{
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstFrame = atoi(token);
		// leg only frames are adjusted to not count the upper body only frames
		if (i == LEGS_WALKCR)
		{
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if (i >= LEGS_WALKCR)
		{
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			break;
		}
		animations[i].numFrames = atoi(token);

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			break;
		}
		animations[i].loopFrames = atoi(token);

		token = COM_Parse(&text_p);
		if (!token[0])
		{
			break;
		}
		fps = atof(token);
		if (fps == 0)
		{
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if (i != MAX_ANIMATIONS)
	{
		Com_Printf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}


/*
==========================
UI_RegisterClientModelname
==========================
*/
qboolean UI_RegisterClientModelname(playerInfo_t* pi, const char* modelSkinName)
{
	char        modelName[MAX_QPATH];
	char        skinName[MAX_QPATH];
	char        filename[MAX_QPATH];
	char*        slash;

	pi->torsoModel = 0;
	pi->headModel = 0;

	if (!modelSkinName[0])
	{
		return qfalse;
	}

	Q_strncpyz(modelName, modelSkinName, sizeof(modelName));

	slash = strchr(modelName, '/');
	if (!slash)
	{
		// modelName did not include a skin name
		Q_strncpyz(skinName, "default", sizeof(skinName));
	}
	else
	{
		Q_strncpyz(skinName, slash + 1, sizeof(skinName));
		// truncate modelName
		*slash = '\0';
	}

	// load cmodels before models so filecache works

	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower.md3", modelName);
	pi->legsModel = trap_R_RegisterModel(filename);
	if (!pi->legsModel)
	{
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper.md3", modelName);
	pi->torsoModel = trap_R_RegisterModel(filename);
	if (!pi->torsoModel)
	{
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", modelName);
	pi->headModel = trap_R_RegisterModel(filename);
	if (!pi->headModel)
	{
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}

	// if any skins failed to load, fall back to default
	if (!UI_RegisterClientSkin(pi, modelName, skinName))
	{
		if (!UI_RegisterClientSkin(pi, modelName, "default"))
		{
			Com_Printf("Failed to load skin file: %s : %s\n", modelName, skinName);
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelName);
	if (!UI_ParseAnimationFile(filename, pi->animations))
	{
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	return qtrue;
}


/*
===============
UI_PlayerInfo_SetModel
===============
*/
void UI_PlayerInfo_SetModel(playerInfo_t* pi, const char* model)
{
	memset(pi, 0, sizeof(*pi));
	UI_RegisterClientModelname(pi, model);
	pi->weapon = WP_MACHINEGUN;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = WP_PENDING;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon(pi, pi->weapon);
}


/*
===============
UI_PlayerInfo_SetInfo
===============
*/
void UI_PlayerInfo_SetInfo(playerInfo_t* pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNumber, qboolean chat)
{
	int         currentAnim;
	weapon_t    weaponNum;

	pi->chat = chat;

	// view angles
	VectorCopy(viewAngles, pi->viewAngles);

	// move angles
	VectorCopy(moveAngles, pi->moveAngles);

	if (pi->newModel)
	{
		pi->newModel = qfalse;

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
		pi->legs.yawAngle = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
		pi->torso.yawAngle = viewAngles[YAW];
		pi->torso.yawing = qfalse;

		if (weaponNumber != WP_PENDING)
		{
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = WP_PENDING;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon(pi, pi->weapon);
		}

		return;
	}

	// weapon
	if (weaponNumber == WP_PENDING)
	{
		pi->pendingWeapon = WP_PENDING;
		pi->weaponTimer = 0;
	}
	else if (weaponNumber != WP_NONE)
	{
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

	if (torsoAnim == BOTH_DEATH1 || legsAnim == BOTH_DEATH1)
	{
		torsoAnim = legsAnim = BOTH_DEATH1;
		pi->weapon = pi->currentWeapon = WP_NONE;
		UI_PlayerInfo_SetWeapon(pi, pi->weapon);

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);

		return;
	}

	// leg animation
	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	if (legsAnim != LEGS_JUMP && (currentAnim == LEGS_JUMP || currentAnim == LEGS_LAND))
	{
		pi->pendingLegsAnim = legsAnim;
	}
	else if (legsAnim != currentAnim)
	{
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim(pi, legsAnim);
	}

	// torso animation
	if (torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2)
	{
		if (weaponNum == WP_NONE || weaponNum == WP_GAUNTLET)
		{
			torsoAnim = TORSO_STAND2;
		}
		else
		{
			torsoAnim = TORSO_STAND;
		}
	}

	if (torsoAnim == TORSO_ATTACK || torsoAnim == TORSO_ATTACK2)
	{
		if (weaponNum == WP_NONE || weaponNum == WP_GAUNTLET)
		{
			torsoAnim = TORSO_ATTACK2;
		}
		else
		{
			torsoAnim = TORSO_ATTACK;
		}
		pi->muzzleFlashTime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		//FIXME play firing sound here
	}

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if (weaponNum != pi->currentWeapon || currentAnim == TORSO_RAISE || currentAnim == TORSO_DROP)
	{
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ((currentAnim == TORSO_GESTURE || currentAnim == TORSO_ATTACK) && (torsoAnim != currentAnim))
	{
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if (torsoAnim != currentAnim)
	{
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim(pi, torsoAnim);
	}
}
