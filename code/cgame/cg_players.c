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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"
#include "../qcommon/l_crc.h"
#include "tr_types.h"

#define SHADOW_DISTANCE 128  /* base from ground to player for shadow trace */

char*    cg_customSoundNames[MAX_CUSTOM_SOUNDS] =
{
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};

static void CG_UpdateOtherClientsInfo(void);
void CG_UpdateAllClientsInfo(void);
static void CG_ForceNewClientInfo(clientInfo_t* old, clientInfo_t* new);

//unknown variable
int global_viewlistFirstOption = 0;
/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound(int clientNum, const char* soundName)
{
	clientInfo_t* ci;
	int         i;
	int myteam;
	clientInfo_t* myself;

	{
		const playerState_t* ps = &cg.snap->ps;

		if (ps->pm_flags & PMF_FOLLOW && ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		{
			myself = &cgs.clientinfo[ps->clientNum];
			myteam = myself->team;
		}
		else
		{
			myteam = ps->persistant[PERS_TEAM];
			myself = &cgs.clientinfo[cg.clientNum];
		}
	}

	if (soundName[0] != '*')
	{
		return trap_S_RegisterSound(soundName, qfalse);
	}

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for (i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++)
	{
		if (!strcmp(soundName, cg_customSoundNames[i]))
		{
			if (ci == myself && cgs.mySounds[i])
			{
				return cgs.mySounds[i];
			}
			else if ((myteam != TEAM_FREE && ci->team == myteam && ci != myself) && cgs.teamSounds[i])
			{
				return cgs.teamSounds[i];
			}
			else if (((ci->team != myteam) || (myteam == TEAM_FREE && ci != myself)) && cgs.enemySounds[i])
			{
				return cgs.enemySounds[i];
			}
			return ci->sounds[i];
		}
	}

	CG_Error("Unknown custom sound: %s", soundName);
	return 0;
}

/*
 * Footsteps type to use for a client: from cg_mySound/cg_teamSound/cg_enemySound model if set, else from ci->footsteps.
 */
footstep_t CG_EffectiveFootsteps(int clientNum)
{
	clientInfo_t* ci;
	int myteam;
	clientInfo_t* myself;

	if (!cgs.customModelSound || !cg.snap)
		goto use_model;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		clientNum = 0;
	ci = &cgs.clientinfo[clientNum];
	{
		const playerState_t* ps = &cg.snap->ps;
		if (ps->pm_flags & PMF_FOLLOW && ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
		{
			myself = &cgs.clientinfo[ps->clientNum];
			myteam = myself->team;
		}
		else
		{
			myteam = ps->persistant[PERS_TEAM];
			myself = &cgs.clientinfo[cg.clientNum];
		}
	}
	if (ci == myself && cg_mySound.string[0])
	{
		if (cgs.myFootsteps < FOOTSTEP_TOTAL)
			return cgs.myFootsteps;
		goto use_model;
	}
	if (myteam != TEAM_FREE && ci->team == myteam && ci != myself && cg_teamSound.string[0])
	{
		if (cgs.teamFootsteps < FOOTSTEP_TOTAL)
			return cgs.teamFootsteps;
		goto use_model;
	}
	if ((ci->team != myteam || (myteam == TEAM_FREE && ci != myself)) && cg_enemySound.string[0])
	{
		if (cgs.enemyFootsteps < FOOTSTEP_TOTAL)
			return cgs.enemyFootsteps;
		goto use_model;
	}
use_model:
	ci = &cgs.clientinfo[clientNum];
	return ci->footsteps;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean CG_ParseAnimationFile(const char* filename, clientInfo_t* ci)
{
	char*        text_p, *prev;
	int         len;
	int         i;
	char*        token;
	float       fps;
	int         skip;
	char        text[20000];
	fileHandle_t    f;
	animation_t* animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0)
	{
		return qfalse;
	}
	if (len >= sizeof(text) - 1)
	{
		CG_Printf("File %s too long\n", filename);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0;   // quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear(ci->headOffset);
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while (1)
	{
		prev = text_p;  // so we can unget
		token = COM_Parse(&text_p);
		if (!token)
		{
			break;
		}
		if (!Q_stricmp(token, "footsteps"))
		{
			token = COM_Parse(&text_p);
			if (!token)
			{
				break;
			}
			if (!Q_stricmp(token, "default") || !Q_stricmp(token, "normal"))
			{
				ci->footsteps = FOOTSTEP_NORMAL;
			}
			else if (!Q_stricmp(token, "boot"))
			{
				ci->footsteps = FOOTSTEP_BOOT;
			}
			else if (!Q_stricmp(token, "flesh"))
			{
				ci->footsteps = FOOTSTEP_FLESH;
			}
			else if (!Q_stricmp(token, "mech"))
			{
				ci->footsteps = FOOTSTEP_MECH;
			}
			else if (!Q_stricmp(token, "energy"))
			{
				ci->footsteps = FOOTSTEP_ENERGY;
			}
			else
			{
				CG_Printf("Bad footsteps parm in %s: %s\n", filename, token);
			}
			continue;
		}
		else if (!Q_stricmp(token, "headoffset"))
		{
			for (i = 0 ; i < 3 ; i++)
			{
				token = COM_Parse(&text_p);
				if (!token)
				{
					break;
				}
				ci->headOffset[i] = atof(token);
			}
			continue;
		}
		else if (!Q_stricmp(token, "sex"))
		{
			token = COM_Parse(&text_p);
			if (!token)
			{
				break;
			}
			if (token[0] == 'f' || token[0] == 'F')
			{
				ci->gender = GENDER_FEMALE;
			}
			else if (token[0] == 'n' || token[0] == 'N')
			{
				ci->gender = GENDER_NEUTER;
			}
			else
			{
				ci->gender = GENDER_MALE;
			}
			continue;
		}
		else if (!Q_stricmp(token, "fixedlegs"))
		{
			ci->fixedlegs = qtrue;
			continue;
		}
		else if (!Q_stricmp(token, "fixedtorso"))
		{
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9')
		{
			text_p = prev;  // unget the token
			break;
		}
		Com_Printf("unknown token '%s' is %s\n", token, filename);
	}

	// read information for each frame
	for (i = 0 ; i < MAX_ANIMATIONS ; i++)
	{

		token = COM_Parse(&text_p);
		if (!*token)
		{
			if (i >= TORSO_GETFLAG)
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
		if (i >= LEGS_WALKCR && i < TORSO_GETFLAG)
		{
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse(&text_p);
		if (!*token)
		{
			break;
		}
		animations[i].numFrames = atoi(token);

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0)
		{
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse(&text_p);
		if (!*token)
		{
			break;
		}
		animations[i].loopFrames = atoi(token);

		token = COM_Parse(&text_p);
		if (!*token)
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
		CG_Printf("Error parsing animation file: %s", filename);
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean CG_RegisterClientSkin(clientInfo_t* ci, const char* teamName, const char* modelName, const char* skinName, const char* headModelName, const char* headSkinName)
{
	char filename[MAX_QPATH];

	Com_sprintf(filename, MAX_QPATH, "models/players/%s/%slower_%s.skin", modelName, teamName, skinName);
	ci->legsSkin = trap_R_RegisterSkin(filename);
	if (!ci->legsSkin)
	{
		Com_Printf("Leg skin load failure: %s\n", filename);
	}

	Com_sprintf(filename, MAX_QPATH, "models/players/%s/%supper_%s.skin", modelName, teamName, skinName);
	ci->torsoSkin = trap_R_RegisterSkin(filename);
	if (ci->torsoSkin == 0)
	{
		Com_Printf("Torso skin load failure: %s\n", filename);
	}

	if (headModelName[0] == '*')
	{
		Com_sprintf(filename, MAX_QPATH, "models/players/heads/%s/head_%s.skin", &headModelName[1], headSkinName);
	}
	else
	{
		Com_sprintf(filename, MAX_QPATH, "models/players/%s/%shead_%s.skin", headModelName, teamName, headSkinName);
	}
	ci->headSkin = trap_R_RegisterSkin(filename);
	if (ci->headSkin == 0)
	{
		if ((char) * (headModelName) != '*')
		{
			Com_sprintf(filename, MAX_QPATH, "models/players/heads/%s/head_%s.skin", headModelName, headSkinName);
			ci->headSkin = trap_R_RegisterSkin(filename);
		}
	}
	if (ci->headSkin == 0)
	{
		Com_Printf("Head skin load failure: %s\n", filename);
	}

	if (!ci->legsSkin || !ci->torsoSkin || !ci->headSkin)
	{
		return qfalse;
	}
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname(clientInfo_t* ci, const char* modelName, const char* skinName, const char* headModelName, const char* headSkinName, const char* teamName)
{
	char    filename[MAX_QPATH * 2];
	const char*      headName;
	char vertex_light_str[4];
	int vertex_light_num;
	qboolean failed_flag = qfalse;

	if (headModelName[0] == '\0')
	{
		headName = modelName;
	}
	else
	{
		headName = headModelName;
	}
	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower.md3", modelName);
	ci->legsModel = trap_R_RegisterModel(filename);
	if (!ci->legsModel)
	{
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower.md3", modelName);
		ci->legsModel = trap_R_RegisterModel(filename);
		if (!ci->legsModel)
		{
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper.md3", modelName);
	ci->torsoModel = trap_R_RegisterModel(filename);
	if (!ci->torsoModel)
	{
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper.md3", modelName);
		ci->torsoModel = trap_R_RegisterModel(filename);
		if (!ci->torsoModel)
		{
			Com_Printf("Failed to load model file %s\n", filename);
			return qfalse;
		}
	}


	if (headName[0] == '*')
	{
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1]);
	}
	else
	{
		Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headName);
	}
	ci->headModel = trap_R_RegisterModel(filename);
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if (!ci->headModel && headName[0] != '*')
	{
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", headModelName, headModelName);
		ci->headModel = trap_R_RegisterModel(filename);
	}
	if (!ci->headModel)
	{
		Com_Printf("Failed to load model file %s\n", filename);
		return qfalse;
	}


	trap_Cvar_VariableStringBuffer("r_vertexLight", vertex_light_str, 4);
	vertex_light_num = atoi(vertex_light_str);
	if (vertex_light_num)
	{
		trap_Cvar_Set("r_vertexLight", "0");
	}

	// if any skins failed to load, return failure
	if (!CG_RegisterClientSkin(ci, teamName, modelName, skinName, headName, headSkinName))
	{
		failed_flag = qtrue;
		if (teamName && *teamName)
		{
			Com_Printf("Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelName, skinName, headName, headSkinName);
		}
		else
		{
			Com_Printf("Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headName, headSkinName);
		}
	}

	if (vertex_light_num)
	{
		trap_Cvar_Set("r_vertexLight", "1");
	}

	if (failed_flag)
	{
		return qfalse;
	}

	// load the animations
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelName);
	if (!CG_ParseAnimationFile(filename, ci))
	{
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}
	if (headName[0] == '*')
	{
		Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/icon_%s.tga", &headName[1], headSkinName);
	}
	else
	{
		Com_sprintf(filename, sizeof(filename), "models/players/%s/icon_%s.tga", headName, headSkinName);
	}

	ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	if (!ci->modelIcon)
	{
		if (headName[0] != '*')
		{
			// here
			Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/icon_%s.tga", headName, headSkinName);
			ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
		}
	}
	if (!ci->modelIcon)
	{
		Com_Printf("Failed to load icon file: %s\n", filename);
		return qfalse;
	}

	return qtrue;
}

/*
==========================
CG_RegisterOriginalHeadModel
==========================
*/
static void CG_RegisterOriginalHeadModel(clientInfo_t* ci)
{
	char filename[MAX_QPATH];
	char* skinPtr;
	char headModelName[MAX_QPATH];
	char headSkinName[MAX_QPATH];

	ci->originalHeadModelHandle = 0;
	ci->originalHeadSkinHandle = 0;
	ci->originalModelIcon = 0;

	if (ci->originalHeadModel[0] == '\0')
		return;

	/* Parse model/skin from hmodel string */
	Q_strncpyz(headModelName, ci->originalHeadModel, sizeof(headModelName));
	skinPtr = strchr(headModelName, '/');
	if (skinPtr)
	{
		*skinPtr = '\0';
		Q_strncpyz(headSkinName, skinPtr + 1, sizeof(headSkinName));
	}
	else
		Q_strncpyz(headSkinName, "default", sizeof(headSkinName));

	/* Register head model */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headModelName);
	ci->originalHeadModelHandle = trap_R_RegisterModel(filename);

	if (ci->originalHeadModelHandle == 0)
	{
		Q_strncpyz(headModelName, "sarge", sizeof(headModelName));
		Q_strncpyz(headSkinName, "default", sizeof(headSkinName));
		Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headModelName);
		ci->originalHeadModelHandle = trap_R_RegisterModel(filename);
	}

	/* Team color skin when default skin and team game */
	if (Q_stricmp(headSkinName, "default") == 0 && cgs.gametype >= GT_TEAM)
	{
		if (ci->team == TEAM_BLUE)
			Q_strncpyz(headSkinName, cg_swapSkins.integer ? "red" : "blue", sizeof(headSkinName));
		else if (ci->team == TEAM_RED)
			Q_strncpyz(headSkinName, cg_swapSkins.integer ? "blue" : "red", sizeof(headSkinName));
	}

	Com_sprintf(filename, sizeof(filename), "models/players/%s/head_%s.skin", headModelName, headSkinName);
	ci->originalHeadSkinHandle = trap_R_RegisterSkin(filename);
	Com_sprintf(filename, sizeof(filename), "models/players/%s/icon_%s.tga", headModelName, headSkinName);
	ci->originalModelIcon = trap_R_RegisterShaderNoMip(filename);
}

/*
 * Parse footsteps type from a sound model's animation.cfg (for cg_mySound / cg_teamSound / cg_enemySound).
 * Returns FOOTSTEP_TOTAL if file not found or "footsteps" not present (caller should use model footsteps).
 */
static footstep_t CG_GetFootstepsForSoundModel(const char* modelName)
{
	char filename[MAX_QPATH];
	char text[4096];
	char* text_p;
	char* token;
	int len;
	fileHandle_t f;

	if (!modelName || !modelName[0])
		return FOOTSTEP_TOTAL;
	Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelName);
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0 || len >= (int)sizeof(text) - 1)
		return FOOTSTEP_TOTAL;
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);
	text_p = text;
	while (1)
	{
		token = COM_Parse(&text_p);
		if (!token || !token[0])
			break;
		if (!Q_stricmp(token, "footsteps"))
		{
			token = COM_Parse(&text_p);
			if (!token || !token[0])
				return FOOTSTEP_TOTAL;
			if (!Q_stricmp(token, "default") || !Q_stricmp(token, "normal"))
				return FOOTSTEP_NORMAL;
			if (!Q_stricmp(token, "boot"))
				return FOOTSTEP_BOOT;
			if (!Q_stricmp(token, "flesh"))
				return FOOTSTEP_FLESH;
			if (!Q_stricmp(token, "mech"))
				return FOOTSTEP_MECH;
			if (!Q_stricmp(token, "energy"))
				return FOOTSTEP_ENERGY;
			return FOOTSTEP_NORMAL;
		}
	}
	return FOOTSTEP_TOTAL;
}

void CG_LoadForcedSounds(void)
{
	char mySoundModel[MAX_QPATH];
	char teamSoundModel[MAX_QPATH];
	char enemySoundModel[MAX_QPATH];
	char* s;
	int i;

	cgs.customModelSound = qfalse;

	trap_Cvar_VariableStringBuffer("cg_mySound", mySoundModel, sizeof(mySoundModel));
	trap_Cvar_VariableStringBuffer("cg_teamSound", teamSoundModel, sizeof(teamSoundModel));
	trap_Cvar_VariableStringBuffer("cg_enemySound", enemySoundModel, sizeof(enemySoundModel));

	if (mySoundModel[0] || teamSoundModel[0] || enemySoundModel[0])
	{
		cgs.customModelSound = qtrue;
	}

	for (i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++)
	{
		s = cg_customSoundNames[i];
		if (!s)
		{
			break;
		}

		cgs.mySounds[i] = 0;
		cgs.teamSounds[i] = 0;
		cgs.enemySounds[i] = 0;

		if (mySoundModel[0])
		{
			cgs.mySounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", mySoundModel, s + 1), qfalse);
		}
		if (teamSoundModel[0])
		{
			cgs.teamSounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", teamSoundModel, s + 1), qfalse);
		}
		if (enemySoundModel[0])
		{
			cgs.enemySounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", enemySoundModel, s + 1), qfalse);
		}
	}

	if (mySoundModel[0])
		cgs.myFootsteps = CG_GetFootstepsForSoundModel(mySoundModel);
	else
		cgs.myFootsteps = FOOTSTEP_TOTAL;
	if (teamSoundModel[0])
		cgs.teamFootsteps = CG_GetFootstepsForSoundModel(teamSoundModel);
	else
		cgs.teamFootsteps = FOOTSTEP_TOTAL;
	if (enemySoundModel[0])
		cgs.enemyFootsteps = CG_GetFootstepsForSoundModel(enemySoundModel);
	else
		cgs.enemyFootsteps = FOOTSTEP_TOTAL;
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
===================
*/
static void CG_LoadClientInfo(clientInfo_t* ci)
{
	const char*  dir, *fallback;
	int         i, modelloaded;
	const char*  s;
	int         clientNum;
	char        teamname[MAX_QPATH];

	teamname[0] = 0;

	modelloaded = qtrue;
	if (!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname))
	{
		if (cg_buildScript.integer)
		{
			CG_Error("CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname);
		}

		// fall back to default team name
		if (cgs.gametype >= GT_TEAM)
		{
			if (!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->skinName, teamname))
			{
				const char* skin = ci->rt == TEAM_RED ? "red" : "blue";
				if (!CG_RegisterClientModelname(ci, "sarge", skin, "sarge", skin, teamname))
				{
					CG_Error("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, skin);
					modelloaded = qfalse;
				}
			}
		}
		else
		{
			if (!CG_RegisterClientModelname(ci, ci->modelName, "default", ci->headModelName, "default", teamname))
			{
				if (!CG_RegisterClientModelname(ci, "sarge", "default", "sarge", "default", teamname))
				{
					CG_Error("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, "default");
					modelloaded = qfalse;
				}
			}
		}
	}

	ci->newAnims = qfalse;
	if (ci->torsoModel)
	{
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if (trap_R_LerpTag(&tag, ci->torsoModel, 0, 0, 1, "tag_flag"))
		{
			ci->newAnims = qtrue;
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for (i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++)
	{
		s = cg_customSoundNames[i];
		if (!s)
		{
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded)
		{
			ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), qfalse);
		}
		if (!ci->sounds[i])
		{
			ci->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), qfalse);
		}
	}

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	// Main player is at cg_entities[clientNum]; freeze body at cg_entities[ci->frozenEntity].
	clientNum = ci - cgs.clientinfo;
	{
		centity_t* cent;

		cent = &cg_entities[clientNum];
		if (cent->currentState.clientNum == clientNum && cent->currentState.eType == ET_PLAYER)
		{
			CG_ResetPlayerEntity(cent);
		}
		if ((cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF) && cgs.osp.gameTypeFreeze
		        && ci->frozenEntity >= 0 && ci->frozenEntity < MAX_GENTITIES)
		{
			cent = &cg_entities[ci->frozenEntity];
			if (cent->currentState.eType == ET_PLAYER && cent->currentState.otherEntityNum == clientNum)
			{
				CG_ResetPlayerEntity(cent);
			}
		}
	}
}

void CG_ClientInfoUpdateTeamTask(const clientInfo_t* newInfo, int clientNum)
{
	clientInfo_t* ci;
	int qx;
	int qy;
	int qz;
	int templateID;
	int toClient;
	vec3_t coord;

	if (cgs.gametype < GT_TEAM || !cgs.teamMarkers.teamTaskEnabled)
		return;

	ci = &cgs.clientinfo[clientNum];
	if (!newInfo || !ci->infoValid || !cg.nextSnap ||
	        CG_IS_LOCAL_CLIENT_SPECTATOR() || CG_IS_ENEMY_SIMPLE(newInfo) || CG_TeamTaskFilterIsNameMuted(ci->name_original))
		return;


	/* Skip if teamTask hasn't changed */
	if (newInfo->teamTask == ci->teamTask)
	{
		return;
	}

	/* Process only new non-zero teamTask values */
	/* Simply react to what we received - process the new teamTask */
	if (newInfo->teamTask == 0)
	{
		/* Zero value - clear event flag, nothing to process */
		ci->teamTaskEvent = 0;
		return;
	}

	/* New non-zero value - process it */
	ci->teamTaskEvent = newInfo->teamTask; /* Set flag for other functions to check */

	/* Process event immediately - create the mark */
	/* clientNum is the sender's client ID */
	CG_TeamTaskUnpack(newInfo->teamTask, clientNum, &toClient, &templateID, &qx, &qy, &qz);

	/* Security check: reject if unpack failed (templateID == 0 means invalid/forged data) */
	if (templateID == 0)
	{
		/* Invalid or forged data - reject to prevent bypassing protection */
		return;
	}

	/* Mark that player supports team task only if unpack was successful (templateID != 0) */
	ci->supportsTeamTask = qtrue;

	// coord[0] = Dequantize(qx, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MIN : -3000, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MAX : 3000, 7);
	// coord[1] = Dequantize(qy, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MIN : -3000, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MAX : 3000, 7);
	// coord[2] = Dequantize(qz, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MIN : -3000, cg_teamTaskVersion.integer == 1 ? TEAMTASK_COORD_MAX : 3000, 7);

	coord[0] = Dequantize(qx, -3000, 3000, 7);
	coord[1] = Dequantize(qy, -3000, 3000, 7);
	coord[2] = Dequantize(qz, -3000, 3000, 7);

	switch (templateID)
	{
		case TEAMTASK_YES:
		case TEAMTASK_NO:
		case TEAMTASK_MEDIC:
		case TEAMTASK_HERE_ALIVE:
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
			return;
		case TEAMTASK_PLAYERDEAD:
		case TEAMTASK_HERE_DEAD:
		case TEAMTASK_WARN_DEAD:
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
			return;
		case TEAMTASK_TARGET:
		{
			/* For TARGET mark, try to find enemy player at the received position */
			/* If found, use enemy's current position with +24 height, otherwise use received coordinates */
			int i;
			centity_t* cent;
			vec3_t enemyPos;
			qboolean foundEnemy;
			float searchRadius;
			float dist;

			foundEnemy = qfalse;
			searchRadius = 128.0f; /* Search radius for finding enemy near received coordinates */

			/* Try to find enemy player near received coordinates (players at 0..MAX_CLIENTS-1) */
			for (i = 0; i < MAX_CLIENTS; i++)
			{
				cent = &cg_entities[i];
				if (cent->currentState.eType == ET_PLAYER && cent->currentValid)
				{
					/* Check if this is an enemy */
					if (cent->currentState.clientNum >= 0 && cent->currentState.clientNum < MAX_CLIENTS)
					{
						clientInfo_t* targetCi;
						targetCi = &cgs.clientinfo[cent->currentState.clientNum];
						if (targetCi->infoValid && CG_IS_ENEMY_SIMPLE(targetCi))
						{
							/* Check if enemy is near received coordinates */
							dist = Distance(cent->lerpOrigin, coord);
							if (dist <= searchRadius)
							{
								/* Found enemy - use their position with +24 height */
								VectorCopy(cent->lerpOrigin, enemyPos);
								enemyPos[2] += 24.0f;
								CG_TeamTaskMark(enemyPos, templateID, clientNum, toClient);
								foundEnemy = qtrue;
								break;
							}
						}
					}
				}
			}

			if (!foundEnemy)
			{
				CG_TeamTaskMark(coord, templateID, clientNum, toClient);
			}
		}
		return;
		case TEAMTASK_FROZEN_ALLY:
		case TEAMTASK_FROZEN_ENEMY:
		{
			/* Use frozen entity cache (set/clear in CG_Player when we process each cent) */
			int i;
			centity_t* cent;
			clientInfo_t* frozenCi;
			vec3_t frozenPos;
			qboolean foundFrozen;
			float searchRadius;
			float dist;
			qboolean isEnemy;
			qboolean matchesType;

			foundFrozen = qfalse;
			searchRadius = 128.0f;

			for (i = 0; i < MAX_CLIENTS; i++)
			{
				if (!cgs.clientinfo[i].isFrozenEnt)
					continue;
				cent = &cg_entities[cgs.clientinfo[i].frozenEntity];
				if (!cent->currentValid)
					continue;
				frozenCi = &cgs.clientinfo[i];
				if (!frozenCi->infoValid)
					continue;
				isEnemy = CG_IS_ENEMY_SIMPLE(frozenCi);
				matchesType = (templateID == TEAMTASK_FROZEN_ALLY) ? !isEnemy : isEnemy;
				if (!matchesType)
					continue;
				dist = Distance(cent->lerpOrigin, coord);
				if (dist > searchRadius)
					continue;
				VectorCopy(cent->lerpOrigin, frozenPos);
				frozenPos[2] += 24.0f;
				CG_TeamTaskMark(frozenPos, templateID, clientNum, toClient);
				foundFrozen = qtrue;
				break;
			}

			if (!foundFrozen)
				CG_TeamTaskMark(coord, templateID, clientNum, toClient);
		}
		return;
		case TEAMTASK_REDFLAG:
		case TEAMTASK_BLUEFLAG:
			if (cgs.gametype != GT_CTF)
			{
				goto do_aim_point;
			}
		case TEAMTASK_REDARMOR:
		case TEAMTASK_MEGAHEALTH:
		case TEAMTASK_QUADDAMAGE:
		case TEAMTASK_REGENERATION:
		case TEAMTASK_BATTLESUIT:
		case TEAMTASK_HASTE:
		case TEAMTASK_YELLOWARMOR:
		case TEAMTASK_HEALTH:
		{
			/* Find item entity near coord; use lerpOrigin + 4 for mark. */
			/* When itemEntityIndices filled: iterate only items, pick smallest ent index in range (same as full-scan order). */
			int i;
			int j;
			int entIndex;
			int bestEntIndex; /* among matches in range, the one with smallest entity index */
			centity_t* cent;
			gitem_t* item;
			int itemIndex;
			vec3_t itemPos;
			qboolean foundItem;
			float searchRadius;
			float dist;
			const char* targetClassname;
			qboolean matches;

			foundItem = qfalse;
			searchRadius = 128.0f;
			bestEntIndex = -1;

			switch (templateID)
			{
				case TEAMTASK_REDARMOR:
					targetClassname = "item_armor_body";
					break;
				case TEAMTASK_MEGAHEALTH:
					targetClassname = "item_health_mega";
					break;
				case TEAMTASK_QUADDAMAGE:
					targetClassname = "item_quad";
					break;
				case TEAMTASK_REGENERATION:
					targetClassname = "item_regen";
					break;
				case TEAMTASK_BATTLESUIT:
					targetClassname = "item_enviro";
					break;
				case TEAMTASK_HASTE:
					targetClassname = "item_haste";
					break;
				case TEAMTASK_YELLOWARMOR:
					targetClassname = "item_armor_combat";
					break;
				case TEAMTASK_REDFLAG:
					targetClassname = "team_CTF_redflag";
					break;
				case TEAMTASK_BLUEFLAG:
					targetClassname = "team_CTF_blueflag";
					break;
				default:
					targetClassname = NULL;
					break; /* TEAMTASK_HEALTH: both item_health */
			}

			if (cg.numItemEntityIndices > 0)
			{
				/* Iterate only item entities; pick match with smallest entity index (same as full-scan order). */
				for (j = 0; j < cg.numItemEntityIndices; j++)
				{
					entIndex = cg.itemEntityIndices[j];
					cent = &cg_entities[entIndex];
					if (cent->currentState.eType != ET_ITEM)
						continue;
					itemIndex = cent->currentState.modelindex;
					if (itemIndex < 1 || itemIndex >= bg_numItems)
						continue;
					item = &bg_itemlist[itemIndex];
					if (!item->classname)
						continue;
					if (templateID == TEAMTASK_HEALTH)
						matches = (!Q_stricmp(item->classname, "item_health") || !Q_stricmp(item->classname, "item_health_large"));
					else
						matches = !Q_stricmp(item->classname, targetClassname);
					if (!matches)
						continue;
					dist = Distance(cent->lerpOrigin, coord);
					if (dist > searchRadius)
						continue;
					if (bestEntIndex < 0 || entIndex < bestEntIndex)
						bestEntIndex = entIndex;
				}
				if (bestEntIndex >= 0)
				{
					cent = &cg_entities[bestEntIndex];
					VectorCopy(cent->lerpOrigin, itemPos);
					itemPos[2] += 4;
					CG_TeamTaskMark(itemPos, templateID, clientNum, toClient);
					foundItem = qtrue;
				}
			}
			else
			{
				/* Fallback: itemEntityIndices not filled yet; full scan. */
				for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
				{
					cent = &cg_entities[i];
					if (cent->currentState.eType == ET_ITEM)
					{
						itemIndex = cent->currentState.modelindex;
						if (itemIndex >= 1 && itemIndex < bg_numItems)
						{
							item = &bg_itemlist[itemIndex];
							if (item->classname)
							{
								if (templateID == TEAMTASK_HEALTH)
								{
									if (!Q_stricmp(item->classname, "item_health") ||
									        !Q_stricmp(item->classname, "item_health_large"))
									{
										dist = Distance(cent->lerpOrigin, coord);
										if (dist <= searchRadius)
										{
											VectorCopy(cent->lerpOrigin, itemPos);
											itemPos[2] += 4;
											CG_TeamTaskMark(itemPos, templateID, clientNum, toClient);
											foundItem = qtrue;
											break;
										}
									}
								}
								else if (!Q_stricmp(item->classname, targetClassname))
								{
									dist = Distance(cent->lerpOrigin, coord);
									if (dist <= searchRadius)
									{
										VectorCopy(cent->lerpOrigin, itemPos);
										itemPos[2] += 4;
										CG_TeamTaskMark(itemPos, templateID, clientNum, toClient);
										foundItem = qtrue;
										break;
									}
								}
							}
						}
					}
				}
			}

			if (!foundItem)
			{
				CG_TeamTaskMark(coord, templateID, clientNum, toClient);
			}
			return;
		}
		case TEAMTASK_REDFLAG_CARRIER:
		case TEAMTASK_BLUEFLAG_CARRIER:
			if (cgs.gametype != GT_CTF)
			{
				goto do_aim_point;
			}
			{
				int i;
				centity_t* cent;
				vec3_t carrierPos;
				qboolean foundCarrier;
				float searchRadius;
				float dist;
				int powerupFlag;

				foundCarrier = qfalse;
				searchRadius = 256.0f;
				powerupFlag = (templateID == TEAMTASK_REDFLAG_CARRIER) ? PW_REDFLAG : PW_BLUEFLAG;

				for (i = 0; i < MAX_CLIENTS; i++)
				{
					cent = &cg_entities[i];
					if (cent->currentState.eType == ET_PLAYER && cent->currentValid &&
					        (cent->currentState.powerups & (1 << powerupFlag)))
					{
						dist = Distance(cent->lerpOrigin, coord);
						if (dist <= searchRadius)
						{
							VectorCopy(cent->lerpOrigin, carrierPos);
							carrierPos[2] += 24.0f;
							CG_TeamTaskMark(carrierPos, templateID, clientNum, toClient);
							foundCarrier = qtrue;
							break;
						}
					}
				}
				if (!foundCarrier)
				{
					CG_TeamTaskMark(coord, templateID, clientNum, toClient);
				}
			}
			return;
		default:
do_aim_point:
			{
				vec3_t aimPoint;
				vec3_t start, end;
				trace_t trace;
				centity_t* cent;

				/* Inline: aim point only when needed; use cg_entities[clientNum] directly (we know client) */
				if (clientNum == cg.clientNum)
				{
					if (cg.predictedPlayerState.pm_type == PM_DEAD ||
					        cg.predictedPlayerState.pm_type == PM_SPINTERMISSION)
					{
						CG_TeamTaskMark(coord, templateID, clientNum, toClient);
						break;
					}
					VectorCopy(cg.refdef.vieworg, start);
					VectorMA(start, 131072, cg.refdef.viewaxis[0], end);
					CG_Trace(&trace, start, vec3_origin, vec3_origin, end, clientNum, CONTENTS_SOLID);
					VectorCopy(trace.endpos, aimPoint);
				}
				else if (clientNum >= 0 && clientNum < MAX_CLIENTS)
				{
					cent = &cg_entities[clientNum];
					if (cent->currentState.eType != ET_PLAYER || cent->currentState.clientNum != clientNum ||
					        (cent->currentState.eFlags & EF_DEAD) || !cent->currentValid)
					{
						CG_TeamTaskMark(coord, templateID, clientNum, toClient);
						break;
					}
					{
						vec3_t axis[3];

						AnglesToAxis(cent->currentState.apos.trBase, axis);
						VectorCopy(cent->currentState.pos.trBase, start);
						start[2] += DEFAULT_VIEWHEIGHT;
						VectorMA(start, 131072, axis[0], end);
						CG_Trace(&trace, start, vec3_origin, vec3_origin, end, clientNum, CONTENTS_SOLID);
						VectorCopy(trace.endpos, aimPoint);
					}
				}
				else
				{
					CG_TeamTaskMark(coord, templateID, clientNum, toClient);
					break;
				}
				CG_TeamTaskMark(aimPoint, templateID, clientNum, toClient);
			}
			break;
	}
}

static void CG_ForceNewClientInfo(clientInfo_t* old, clientInfo_t* new)
{
	CG_LoadClientInfo(new);

	new->infoValid = qtrue;
	memcpy(old, new, sizeof(clientInfo_t));
}


/*
 * Check if skin exists for given model
 */
static qboolean CG_SkinExists(const char* modelName, const char* skinName)
{
	char filename[MAX_QPATH];
	qhandle_t testSkin;

	if (!modelName || !modelName[0] || !skinName || !skinName[0])
	{
		return qfalse;
	}

	/* Check lower skin */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/lower_%s.skin", modelName, skinName);
	testSkin = trap_R_RegisterSkin(filename);
	if (!testSkin)
	{
		/* Try characters folder */
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower_%s.skin", modelName, skinName);
		testSkin = trap_R_RegisterSkin(filename);
		if (!testSkin)
		{
			return qfalse;
		}
	}

	/* Check upper skin */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/upper_%s.skin", modelName, skinName);
	testSkin = trap_R_RegisterSkin(filename);
	if (!testSkin)
	{
		/* Try characters folder */
		Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper_%s.skin", modelName, skinName);
		testSkin = trap_R_RegisterSkin(filename);
		if (!testSkin)
		{
			return qfalse;
		}
	}

	return qtrue;
}

static void CG_UpdateModelFromString(char* modelName, char* skinName, const char* resultModelString, qboolean isOurClient, team_t team)
{
	const char* nameModel;
	const char* nameSkin = NULL;
	qboolean isPmSkin = qfalse;
	qboolean isFbSkin = qfalse;
	char* ptr;

	char tmpStr[MAX_QPATH];
	Q_strncpyz(tmpStr, resultModelString, MAX_QPATH);

	nameModel = tmpStr;

	ptr = strchr(tmpStr, '/');
	if (ptr && ptr[1])
	{
		*ptr = 0;
		nameSkin = ++ptr;
	}

	isPmSkin = (nameSkin && (Q_stricmp(nameSkin, "pm") == 0)) ? qtrue : qfalse;

	if (nameSkin && (Q_stricmp(nameSkin, "fb") == 0))
	{
		if ((cgs.beDisableFeatures & CG_BE_FULLBRIGHT) || !cgs.beEnabled)
		{
			nameSkin = "pm";
			isPmSkin = qtrue;
		}
		else
			isFbSkin = qtrue;
	}

	if (isOurClient)
	{
		// our player only pm or default
		if (qfalse && !isPmSkin && !isFbSkin)
		{
			nameSkin = "default";
		}
	}
	else if (cgs.gametype >= GT_TEAM)
	{
		// in team games users able to set pm skin only
		if (!nameSkin || (!isPmSkin && !isFbSkin))
		{
			if (team == TEAM_BLUE)
			{
				nameSkin = cg_swapSkins.integer ? "red" : "blue";
			}
			else if (team == TEAM_RED)
			{
				nameSkin = cg_swapSkins.integer ? "blue" : "red";
			}
			else
			{
				nameSkin = "default";
			}
		}
	}
	else if (!(isPmSkin || isFbSkin))
	{
		/* In non-team modes, check if the custom skin exists before falling back to default */
		if (nameSkin && CG_SkinExists(nameModel, nameSkin))
		{
			/* Skin exists, use it */
		}
		else
		{
			/* Skin doesn't exist, use default */
			nameSkin = "default";
		}
	}

	Q_strncpyz(modelName, nameModel, MAX_QPATH);
	Q_strncpyz(skinName, nameSkin ? nameSkin : "default", MAX_QPATH);
}

static void CG_ClientInfoUpdateModel(clientInfo_t* ci, qboolean isOurClient, qboolean isTeamGame, const char* config, int clientNum)
{
	char modelName[MAX_QPATH];
	char headModelName[MAX_QPATH];
	const char* resultModelString = "keel/pm";
	const char* resultHModelString = "keel/pm";
	const char* cfgModelString = Info_ValueForKey(config, "model");
	const char* cfgHModelString = Info_ValueForKey(config, "hmodel");

	// сначала решим какую строку модели использовать
	if (isOurClient)
	{
		trap_Cvar_VariableStringBuffer("model", modelName, sizeof(modelName));
		resultModelString = modelName;
		trap_Cvar_VariableStringBuffer("headmodel", headModelName, sizeof(headModelName));
		resultHModelString = headModelName;
	}
	else
	{
		const char* forceModelString = cg_forceModel.integer ? cfgModelString : NULL;
		const char* enemyModelString = NULL;
		const qboolean useOriginal = cg_spectOrigModel.integer && CG_IsFollowing() && cg.snap->ps.clientNum == clientNum;

		if (cg_enemyModel.string[0])
		{
			enemyModelString = cg_enemyModel.string;
		}

		if (!isTeamGame)
		{
			// FFA or 1vs1
			qboolean isFollowedPlayer = qfalse;

			/* BE: Enhanced spectator perspective logic for non-team games */
			if (cg_spectPOV.integer && cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR &&
			        cg.snap->ps.pm_flags & PMF_FOLLOW &&
			        cg.snap->ps.clientNum >= 0 && cg.snap->ps.clientNum < MAX_CLIENTS &&
			        cg.snap->ps.clientNum == clientNum)
			{
				isFollowedPlayer = qtrue;
			}

			if (isFollowedPlayer)
			{
				/* Use our own model for the followed player */
				trap_Cvar_VariableStringBuffer("model", modelName, sizeof(modelName));
				trap_Cvar_VariableStringBuffer("headmodel", headModelName, sizeof(headModelName));

				resultModelString = modelName;
				resultHModelString = headModelName;
			}
			else if (useOriginal)
			{
				resultModelString = cfgModelString;
				resultHModelString = cfgHModelString;
			}
			else if (enemyModelString)
			{
				resultModelString = enemyModelString;
				resultHModelString = enemyModelString;
			}
			else if (forceModelString)
			{
				resultModelString = forceModelString;
				resultHModelString = forceModelString;
			}
			else
			{
				resultModelString = cfgModelString;
				resultHModelString = cfgHModelString;
			}
		}
		else
		{
			const char* teamModelString = cg_teamModel.string[0] ? cg_teamModel.string : NULL;
			qboolean isTeamMate;

			/* BE: Enhanced spectator perspective logic */
			if (cg_spectPOV.integer)
			{
				team_t ourPerspectiveTeam;

				if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
				{
					if (cg.snap->ps.pm_flags & PMF_FOLLOW && cg.snap->ps.clientNum >= 0 && cg.snap->ps.clientNum < MAX_CLIENTS)
					{
						ourPerspectiveTeam = cgs.clientinfo[cg.snap->ps.clientNum].rt;
					}
					else
					{
						ourPerspectiveTeam = TEAM_SPECTATOR;
					}
				}
				else
				{
					ourPerspectiveTeam = cgs.clientinfo[cg.clientNum].rt;
				}

				if (ourPerspectiveTeam == TEAM_SPECTATOR)
				{
					/* Not following anyone - use default: red is teammate */
					isTeamMate = (ci->rt == TEAM_RED);
				}
				else
				{
					isTeamMate = (ourPerspectiveTeam == ci->rt);
				}
			}
			else
			{
				/* Original PBE logic */
				qboolean isOurTeam = cgs.clientinfo[cg.clientNum].rt == ci->rt;
				qboolean isRedTeamWhileSpec = cgs.clientinfo[cg.clientNum].rt == TEAM_SPECTATOR && ci->rt == TEAM_RED;
				isTeamMate = isOurTeam || isRedTeamWhileSpec;
			}

			if (isTeamMate)
			{
				if (useOriginal)
				{
					resultModelString = cfgModelString;
					resultHModelString = cfgHModelString;
				}
				else if (teamModelString)
				{
					resultModelString = teamModelString;
					resultHModelString = teamModelString;
				}
				else if (forceModelString)
				{
					resultModelString = forceModelString;
					resultHModelString = forceModelString;
				}
				else
				{
					resultModelString = cfgModelString;
					resultHModelString = cfgHModelString;
				}
			}
			else
			{
				if (useOriginal)
				{
					resultModelString = cfgModelString;
					resultHModelString = cfgHModelString;
				}
				else if (enemyModelString)
				{
					resultModelString = enemyModelString;
					resultHModelString = enemyModelString;
				}
				else if (forceModelString)
				{
					resultModelString = forceModelString;
					resultHModelString = forceModelString;
				}
				else
				{
					resultModelString = cfgModelString;
					resultHModelString = cfgHModelString;
				}
			}
		}
	}
	CG_UpdateModelFromString(&ci->modelName[0], &ci->skinName[0], resultModelString, isOurClient, ci->rt);
	CG_UpdateModelFromString(&ci->headModelName[0], &ci->headSkinName[0], resultHModelString, isOurClient, ci->rt);
	ci->isPmSkin = (Q_stricmp(ci->skinName, "pm") == 0) ? qtrue : qfalse;
	ci->isFbSkin = (Q_stricmp(ci->skinName, "fb") == 0) ? qtrue : qfalse;
	CG_UpdateBeFeatures();
}

/*
 * Check is name invisible
 */
static qboolean CG_PlayerNameIsInvisible(clientInfo_t* ci)
{
	int i;
	int len = strlen(ci->name_clean);
	if (len == 0) return qtrue;
	for (i = 0; i < len; ++i)
	{
		if (ci->name_clean[i] != ' ')
		{
			return qfalse;
		}
	}
	return qtrue;
}

#define NUM_TO_HEX_CHAR(III) (((III&0x0f) < 10) ? ('0' + (III&0x0f)) : ('a' + ((III&0x0f) - 10)))
static void CG_PlayerXIDCalc(const char* cmd, clientInfo_t* ci)
{
	const char* data_c1;
	const char* data_c2;
	const char* data_c3;
	const char* data_c4;
	char* skin;
	unsigned short sign = 0;
	char tmp[MAX_QPATH];

	CRC_Init(&sign);

	data_c1 = Info_ValueForKey(cmd, "c1");
	CRC_ContinueProcessString(&sign, data_c1, strlen(data_c1));

	data_c2 = Info_ValueForKey(cmd, "c2");
	CRC_ContinueProcessString(&sign, data_c2, strlen(data_c2));

	data_c3 = Info_ValueForKey(cmd, "model");
	Q_strncpyz(tmp, data_c3, MAX_QPATH);

	skin = strchr(tmp, '/');
	if (skin)
	{
		*skin = '\0';
	}

	CRC_ContinueProcessString(&sign, tmp, strlen(tmp));

	data_c4 = Info_ValueForKey(cmd, "hmodel");
	Q_strncpyz(tmp, data_c4, MAX_QPATH);

	skin = strchr(tmp, '/');
	if (skin)
	{
		*skin = '\0';
	}

	CRC_ContinueProcessString(&sign, tmp, strlen(tmp));

	ci->xid = sign;

	ci->xidStr[0] = NUM_TO_HEX_CHAR((ci->xid >> 12) & 0x0f);
	ci->xidStr[1] = NUM_TO_HEX_CHAR((ci->xid >> 8) & 0x0f);
	ci->xidStr[2] = NUM_TO_HEX_CHAR((ci->xid >> 4) & 0x0f);
	ci->xidStr[3] = NUM_TO_HEX_CHAR(ci->xid & 0x0f);
	ci->xidStr[4] = 0;
}

/*
===================
CG_PlayerNameBuildFinal

OSP2-style: 0 = no ID/XID, 1 = always, 2 = only if duplicate/noname/invisible.
===================
*/
static void CG_PlayerNameBuildFinal(clientInfo_t* ci, int clientNum)
{
	int showID;
	int showXID;
	qboolean is_exists;
	char originalClean[MAX_QPATH];
	int p;
	char otherClean[MAX_QPATH];
	clientInfo_t* other;
	qboolean useID;
	qboolean useXID;

	showID = cg_playersID.integer;
	showXID = cg_playersXID.integer;

	if (showID == 0 && showXID == 0)
	{
		Q_strncpyz(ci->name, ci->name_original, sizeof(ci->name));
		return;
	}

	is_exists = qfalse;
	if (showID == 2 || showXID == 2)
	{
		Q_strncpyz(originalClean, ci->name_original, sizeof(originalClean));
		CG_RemoveChatEscapeCharAll(originalClean);
		for (p = 0; p < MAX_CLIENTS; ++p)
		{
			if (p == clientNum || !cgs.clientinfo[p].infoValid || cgs.clientinfo[p].nameIsInvisible)
				continue;
			Q_strncpyz(otherClean, cgs.clientinfo[p].name_original, sizeof(otherClean));
			CG_RemoveChatEscapeCharAll(otherClean);
			if (strcmp(originalClean, otherClean) != 0)
				continue;
			is_exists = qtrue;
			/* Add ID/XID to other client if not yet (like OSP2) */
			other = &cgs.clientinfo[p];
			if (strcmp(other->name_original, other->name) == 0)
			{
				useID = (showID != 0);
				useXID = (showXID != 0);
				if (useID && useXID)
					Com_sprintf(other->name, sizeof(other->name), other->nameIsInvisible ?
					            "[%d]^8%s^7:\'%s^7\'" : "[%d]^8%s^7:%s",
					            p, other->xidStr, other->nameIsInvisible ? other->name_codes : other->name_original);
				else if (useXID)
					Com_sprintf(other->name, sizeof(other->name), other->nameIsInvisible ?
					            "^8%s^7:\'%s^7\'" : "^8%s^7:%s",
					            other->xidStr, other->nameIsInvisible ? other->name_codes : other->name_original);
				else if (useID)
					Com_sprintf(other->name, sizeof(other->name), other->nameIsInvisible ?
					            "[%d]\'%s\'" : "[%d]%s",
					            p, other->nameIsInvisible ? other->name_codes : other->name_original);
			}
		}
	}

	useID = (showID == 1) || (showID == 2 && (is_exists || (strcmp(ci->name_original, "^1NONAME") == 0) || ci->nameIsInvisible));
	useXID = (showXID == 1) || (showXID == 2 && (is_exists || (strcmp(ci->name_original, "^1NONAME") == 0) || ci->nameIsInvisible));

	if (ci->nameIsInvisible)
	{
		if (useID && useXID)
			Com_sprintf(ci->name, sizeof(ci->name), "[%d]^8%s^7:\'%s^7\'", clientNum, ci->xidStr, ci->name_codes);
		else if (useXID)
			Com_sprintf(ci->name, sizeof(ci->name), "^8%s^7:\'%s^7\'", ci->xidStr, ci->name_codes);
		else if (useID)
			Com_sprintf(ci->name, sizeof(ci->name), "[%d]\'%s\'", clientNum, ci->name_codes);
		else
			Q_strncpyz(ci->name, ci->name_codes, sizeof(ci->name));
	}
	else if (useID && useXID)
		Com_sprintf(ci->name, sizeof(ci->name), "[%d]^8%s^7:%s", clientNum, ci->xidStr, ci->name_original);
	else if (useXID)
		Com_sprintf(ci->name, sizeof(ci->name), "^8%s^7:%s", ci->xidStr, ci->name_original);
	else if (useID)
		Com_sprintf(ci->name, sizeof(ci->name), "[%d]%s", clientNum, ci->name_original);
	else
		Q_strncpyz(ci->name, ci->name_original, sizeof(ci->name));
}

/*
==================
CG_CheckTeamChangeCommands

Execute commands when player changes team
Called when configstring is updated, so we can rely on server's team information
==================
*/
void CG_CheckTeamChangeCommands(team_t prevTeam, team_t currentTeam)
{

	/* Handle first connection - when player first connects and joins a team */
	if (prevTeam == -1)
	{
		if (currentTeam != TEAM_SPECTATOR && cg_exec_joinGame.string[0])
			trap_SendConsoleCommand(cg_exec_joinGame.string);
		else if (currentTeam == TEAM_SPECTATOR && cg_exec_joinSpec.string[0])
			trap_SendConsoleCommand(cg_exec_joinSpec.string);
		return;
	}

	/* Check if team actually changed */
	if (prevTeam == currentTeam)
	{
		return;
	}

	if (prevTeam == TEAM_SPECTATOR && currentTeam != TEAM_SPECTATOR && cg_exec_joinGame.string[0])
		trap_SendConsoleCommand(cg_exec_joinGame.string);
	if (prevTeam != TEAM_SPECTATOR && currentTeam == TEAM_SPECTATOR && cg_exec_joinSpec.string[0])
		trap_SendConsoleCommand(cg_exec_joinSpec.string);
}
/*
==================
CG_CheckTeamChangeFromConfigstring

Check for team changes and execute commands based on configstring
Called when configstring is updated, so we can rely on server's team information
==================
*/
static void CG_CheckTeamChangeFromConfigstring(const clientInfo_t* ci, const clientInfo_t* newInfo)
{
	static team_t lastKnownTeam = -1;
	team_t prevTeam;
	team_t currentTeam;

	/* Determine team based on gametype for command execution */
	/* Use configstring data - prevTeam from old ci (before update), currentTeam from newInfo */
	if (CG_OSPIsGameTypeCA(cgs.gametype))
	{
		/* Use ci->rt if it's valid, otherwise use lastKnownTeam */
		if (ci->infoValid)
		{
			prevTeam = ci->rt;
		}
		else
		{
			prevTeam = (lastKnownTeam == -1) ? -1 : lastKnownTeam;
		}
		currentTeam = newInfo->rt;
		lastKnownTeam = newInfo->rt;
	}
	else
	{
		/* Use ci->team if it's valid, otherwise use lastKnownTeam */
		if (ci->infoValid)
		{
			prevTeam = ci->team;
		}
		else
		{
			prevTeam = (lastKnownTeam == -1) ? -1 : lastKnownTeam;
		}
		currentTeam = newInfo->team;
		lastKnownTeam = newInfo->team;
	}

	/* Check for team changes and execute commands based on configstring */
	CG_CheckTeamChangeCommands(prevTeam, currentTeam);
}

void CG_NewClientInfo(int clientNum)
{
	clientInfo_t* ci;
	clientInfo_t newInfo;
	const char*  configstring;
	const char*  v;
	const qboolean isOurClient = clientNum == cg.clientNum;
	const qboolean isTeamGame = cgs.gametype >= GT_TEAM;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!configstring[0])
	{
		memset(ci, 0, sizeof(*ci));
		if (!cg.demoPlayback)
		{
			if (cg.scoresRequestTime + 2000 < cg.time)
			{
				cg.scoresRequestTime = cg.time;
				trap_SendClientCommand("score");
			}
		}
		return;     // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&newInfo, 0, sizeof(newInfo));

	CG_PlayerColorsLoadDefault(&newInfo.colors);

	CG_PlayerXIDCalc(configstring, &newInfo);

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name_original, v, sizeof(newInfo.name));
	Q_strncpyz(newInfo.name_clean, v, sizeof(newInfo.name));
	CG_RemoveChatEscapeCharAll(newInfo.name_clean);
	newInfo.nameIsInvisible = CG_PlayerNameIsInvisible(&newInfo);

	CG_StringMakeEscapeCharRAW(newInfo.name_original, newInfo.name_codes, sizeof(newInfo.name_codes));

	CG_PlayerNameBuildFinal(&newInfo, clientNum);

	// bot skill
	v = Info_ValueForKey(configstring, "skill");
	newInfo.botSkill = atoi(v);

	// colors
	v = Info_ValueForKey(configstring, "c1");
	CG_OSPColorsFromString(v, newInfo.colors.railCore, newInfo.colors.head, newInfo.colors.torso, newInfo.colors.legs);

	v = Info_ValueForKey(configstring, "c2");
	CG_OSPColorFromChar(v[0], newInfo.colors.railRings);


	// handicap
	v = Info_ValueForKey(configstring, "hc");
	newInfo.handicap = atoi(v);

	// wins
	v = Info_ValueForKey(configstring, "w");
	newInfo.wins = atoi(v);

	// losses
	v = Info_ValueForKey(configstring, "l");
	newInfo.losses = atoi(v);

	// team
	v = Info_ValueForKey(configstring, "t");
	newInfo.team = atoi(v);

	// hmodel - save original BEFORE any modifications
	v = Info_ValueForKey(configstring, "hmodel");
	Q_strncpyz(newInfo.originalHeadModel, v, sizeof(newInfo.originalHeadModel));

	/* register original head model BEFORE team skin is applied */
	CG_RegisterOriginalHeadModel(&newInfo);

	// rt
	v = Info_ValueForKey(configstring, "rt");
	if (v && v[0])
	{
		newInfo.rt = atoi(v);
		if (!newInfo.rt)
		{
			newInfo.rt = TEAM_SPECTATOR;
		}
	}
	else
	{
		newInfo.rt = newInfo.team;
	}

	/* team task */
	v = Info_ValueForKey(configstring, "tt");
	newInfo.teamTask = atoi(v);

	//st
	v = Info_ValueForKey(configstring, "st");
	if (v && v[0])
	{
		int tmp = atoi(v);

		if (tmp)
		{
			newInfo.team = (tmp & 7) + 5;
		}
		if (tmp & 8)
		{
			newInfo.st = tmp & 7;
		}
	}

	/* If only teamTask changed and we already had valid info, skip full reload (no old configstring available to compare, so we rely on server sending tt updates separately when appropriate) */
	if (newInfo.teamTask != ci->teamTask && ci->infoValid)
	{
		CG_ClientInfoUpdateTeamTask(&newInfo, clientNum);
		ci->teamTask = newInfo.teamTask;
		return;
	}

	/* Full reload needed - update everything */
	CG_ClientInfoUpdateModel(&newInfo, isOurClient, isTeamGame, configstring, clientNum);
	CG_ClientInfoUpdateColors(&newInfo, clientNum);
	CG_ClientInfoUpdateTeamTask(&newInfo, clientNum);

	/* Check for team changes and execute commands based on configstring */
	/* Do this BEFORE updating ci, so we can compare old vs new team */
	if (isOurClient)
	{
		CG_CheckTeamChangeFromConfigstring(ci, &newInfo);
	}

	CG_ForceNewClientInfo(ci, &newInfo);

	/* If we changed team/client/spectator, refresh other clients' info once */
	if (isOurClient)
	{
		static team_t ourTeam = -1;
		static int ourClientNum = -1;
		static qboolean wasSpectator = qtrue;
		qboolean needRefresh;
		qboolean isSpectator;

		isSpectator = (ci->team == TEAM_SPECTATOR || ci->rt == TEAM_SPECTATOR);
		needRefresh = (isTeamGame && (ci->rt != ourTeam || ourClientNum != cg.clientNum)) || (wasSpectator != isSpectator);
		if (needRefresh)
		{
			ourTeam = ci->rt;
			ourClientNum = cg.clientNum;
			wasSpectator = isSpectator;
			numSortedTeamPlayers = 0;
			CG_UpdateOtherClientsInfo();
		}
	}

}


static void CG_UpdateOtherClientsInfo(void)
{
	int index;

	for (index = 0; index < MAX_CLIENTS; ++index)
	{
		/* Skip local client */
		if (index == cg.clientNum)
			continue;

		/* Only update slots with valid clientinfo; avoids extra CG_ConfigString calls */
		if (!cgs.clientinfo[index].infoValid)
			continue;

		CG_NewClientInfo(index);
	}
}

void CG_UpdateAllClientsInfo(void)
{
	int index;

	for (index = 0; index < MAX_CLIENTS; index++)
	{
		/* Only update slots with valid clientinfo; avoids extra CG_ConfigString calls */
		if (!cgs.clientinfo[index].infoValid)
			continue;

		CG_NewClientInfo(index);
	}
}

int CG_CountRealClients(void)
{
	int i;
	int count = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (cgs.clientinfo[i].infoValid)
		{
			count++;
		}
	}
	return count;
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(clientInfo_t* ci, lerpFrame_t* lf, int newAnimation)
{
	animation_t* anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS)
	{
		CG_Error("Bad animation number: %i", newAnimation);
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer)
	{
		CG_Printf("Anim: %i\n", newAnimation);
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int newAnimation, float speedScale)
{
	int         f, numFrames;
	animation_t* anim;

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0)
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation)
	{
		CG_SetLerpFrameAnimation(ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime)
	{
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim->frameLerp)
		{
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime)
		{
			lf->frameTime = lf->animationTime;      // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop)
		{
			numFrames *= 2;
		}
		if (f >= numFrames)
		{
			f -= numFrames;
			if (anim->loopFrames)
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if (anim->reversed)
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f >= anim->numFrames)
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f % anim->numFrames);
		}
		else
		{
			lf->frame = anim->firstFrame + f;
		}
		if (cg.time > lf->frameTime)
		{
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer)
			{
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200)
	{
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time)
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime)
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animationNumber);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t* cent, int* legsOld, int* legs, float* legsBackLerp,
                               int* torsoOld, int* torso, float* torsoBackLerp)
{
	clientInfo_t*    ci;
	int             clientNum;
	float           speedScale;

	clientNum = cent->currentState.clientNum;

	if (cg_noPlayerAnims.integer)
	{
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if (cent->currentState.powerups & (1 << PW_HASTE))
	{
		speedScale = 1.5;
	}
	else
	{
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if (cent->pe.legs.yawing && (cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE)
	{
		CG_RunLerpFrame(ci, &cent->pe.legs, LEGS_TURN, speedScale);
	}
	else
	{
		int animToUse;
		int zu;

		// Check if other player is ducking in air using same logic as CG_AddHitBox
		if (cent->currentState.number != cg.predictedPlayerState.clientNum)
		{
			// Decode bounding box from solid field
			zu = ((cent->currentState.solid >> 16) & 255) - 32;

			// If player is in air and ducking (height reduced)
			if (cent->currentState.groundEntityNum == ENTITYNUM_NONE && zu < 32)
			{
				animToUse = LEGS_IDLECR;  // Force crouch animation
			}
			else
			{
				animToUse = cent->currentState.legsAnim;
			}
		}
		else
		{
			animToUse = cent->currentState.legsAnim;
		}

		CG_RunLerpFrame(ci, &cent->pe.legs, animToUse, speedScale);
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	if (cg_noTaunt.integer & ((cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) == TORSO_GESTURE))
	{
		cent->currentState.torsoAnim = (cent->currentState.torsoAnim & ANIM_TOGGLEBIT) | TORSO_STAND;
	}

	CG_RunLerpFrame(ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale);
	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance,
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
		move = cg.frametime * scale * speed;
		if (move >= swing)
		{
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}
	else if (swing < 0)
	{
		move = cg.frametime * scale * -speed;
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
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch(centity_t* cent, vec3_t torsoAngles)
{
	int     t;
	float   f;

	t = cg.time - cent->pe.painTime;
	if (t >= PAIN_TWITCH_TIME)
	{
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if (cent->pe.painDirection)
	{
		torsoAngles[ROLL] += 20 * f;
	}
	else
	{
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles(centity_t* cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t      legsAngles, torsoAngles, headAngles;
	float       dest;
	static  int movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t      velocity;
	float       speed;
	int         dir, clientNum;
	clientInfo_t*    ci;

	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
	        || (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND)
	{
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;  // always center
		cent->pe.torso.pitching = qtrue;    // always center
		cent->pe.legs.yawing = qtrue;   // always center
	}

	// adjust legs for movement dir
	if (cent->currentState.eFlags & EF_DEAD)
	{
		// don't let dead bodies twitch
		dir = 0;
	}
	else
	{
		dir = cent->currentState.angles2[YAW];
		if (dir < 0 || dir > 7)
		{
			CG_Error("Bad player movement angle");
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles(torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);
	CG_SwingAngles(legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;


	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180)
	{
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[ clientNum ];
		if (ci->fixedtorso)
		{
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy(cent->currentState.pos.trDelta, velocity);
	speed = VectorNormalize(velocity);
	if (speed)
	{
		vec3_t  axis[3];
		float   side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side = speed * DotProduct(velocity, axis[1]);
		if (cg_showPlayerLean.integer != 0)
		{
			legsAngles[ROLL] -= side;
		}

		side = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[ clientNum ];
		if (ci->fixedlegs)
		{
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail(centity_t* cent)
{
	localEntity_t*   smoke;
	vec3_t          origin;
	int             anim;

	if (cent->trailTime > cg.time)
	{
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if (anim != LEGS_RUN && anim != LEGS_BACK)
	{
		return;
	}

	cent->trailTime += 100;
	if (cent->trailTime < cg.time)
	{
		cent->trailTime = cg.time;
	}

	VectorCopy(cent->lerpOrigin, origin);
	origin[2] -= 16;

	smoke = CG_SmokePuff(origin, vec3_origin,
	                     8,
	                     1, 1, 1, 1,
	                     500,
	                     cg.time,
	                     0,
	                     0,
	                     cgs.media.hastePuffShader);

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs(centity_t* cent, refEntity_t* head)
{
	clientInfo_t* ci;
	vec3_t up, origin;
	int contents;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];

	if (!cgs.beServerEnableBreath ||
	        !cg_enableBreath.integer)
	{
		return;
	}
	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		return;
	}
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}
	if (cgs.osp.gameTypeFreeze && (cent->currentState.weapon == WP_NONE) &&
	        (cent->currentState.powerups & (1 << PW_BATTLESUIT)))
	{
		return;
	}
	contents = trap_CM_PointContents(head->origin, 0);
	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}
	if (ci->breathPuffTime > cg.time)
	{
		return;
	}

	VectorSet(up, 0, 0, 8);
	VectorMA(head->origin, 8, head->axis[0], origin);
	VectorMA(origin, -4, head->axis[2], origin);
	CG_SmokePuff(origin, up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader);
	ci->breathPuffTime = cg.time + 2000;
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem(centity_t* cent, qhandle_t hModel)
{
	refEntity_t     ent;
	vec3_t          angles;
	vec3_t          axis[3];

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups(centity_t* cent)
{
	int     powerups;

	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
	{
		int clientNum = cent->currentState.clientNum;
		if (cg_spectGlow.integer || clientNum == cg.snap->ps.generic1 - 32)
		{
			if (cgs.clientinfo[clientNum].team == TEAM_BLUE)
			{
				trap_R_AddLightToScene(cent->lerpOrigin, (float)((rand() & 0x1f) + 200), 0.2, 0.2, 1.0);
				trap_R_AddLightToScene(cent->lerpOrigin, (float)((rand() & 0x1f) + 200), 0.2, 0.2, 1.0);
			}
			else if (cgs.clientinfo[clientNum].team == TEAM_RED)
			{
				trap_R_AddLightToScene(cent->lerpOrigin, (float)((rand() & 0x1f) + 200), 1.0, 0.2, 0.2);
				trap_R_AddLightToScene(cent->lerpOrigin, (float)((rand() & 0x1f) + 200), 1.0, 0.2, 0.2);
			}
			else if (cgs.clientinfo[clientNum].team == TEAM_FREE)
			{
				trap_R_AddLightToScene(cent->lerpOrigin, (float)((rand() & 0x1f) + 0xc8), 1.0, 1.0, 1.0);
			}
		}
	}

	powerups = cent->currentState.powerups;
	if (!powerups)
	{
		return;
	}

	// quad gives a dlight
	if (powerups & (1 << PW_QUAD))
	{
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1);
	}

	// flight plays a looped sound
	if (powerups & (1 << PW_FLIGHT))
	{
		trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound);
	}

	// redflag
	if (powerups & (1 << PW_REDFLAG))
	{
		CG_TrailItem(cent, cgs.media.redFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 0.2f, 0.2f);
	}

	// blueflag
	if (powerups & (1 << PW_BLUEFLAG))
	{
		CG_TrailItem(cent, cgs.media.blueFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1.0);
	}

	// neutralflag
	if (powerups & (1 << PW_NEUTRALFLAG))
	{
		CG_TrailItem(cent, cgs.media.neutralFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 1.0, 1.0);
	}

	// haste leaves smoke trails
	if (powerups & (1 << PW_HASTE))
	{
		CG_HasteTrail(cent);
	}
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite(centity_t* cent, qhandle_t shader, vec4_t color)
{
	int             rf;
	refEntity_t     ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson)
	{
		rf = RF_THIRD_PERSON;       // only show in mirrors
	}
	else
	{
		rf = 0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	if (color)
	{
		MAKERGBA(ent.shaderRGBA, color[0] * 255, color[1] * 255, color[2] * 255, 255);
	}
	else
	{
		MAKERGBA(ent.shaderRGBA, 255, 255, 255, 255);
	}
	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head (connection, talk, medals, friend/frozen tag).
Reward and cg_drawFriend arrow are mutually exclusive. TeamTask overhead hides both.
===============
*/
static void CG_PlayerSprites(centity_t* cent, const clientInfo_t* ci, int clientNum, qboolean useTeamFrozen, qboolean useTeamFoeWh)
{
	qhandle_t     rewardShader = 0;
	qhandle_t     friendShader = 0;
	float*       friendColor = NULL;   /* vec4_t decays to float*; avoid vec4_t* for LCC */
	qboolean     hasOverhead = cgs.teamMarkers.drawTeamTasks && cgs.teamMarkers.clientHasOverhead[clientNum];

	if (hasOverhead)
		return;

	if (cent->currentState.eFlags & EF_CONNECTION)
	{
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader, NULL);
		return;
	}

	if (cent->currentState.eFlags & EF_TALK)
	{
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader, NULL);
		return;
	}

	if (!(cg_drawRewards.integer & DRAW_REWARDS_NOSPRITE) &&
	    (cent->currentState.eFlags & (EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_DEFEND | EF_AWARD_ASSIST | EF_AWARD_CAP)))
	{
		if (cent->currentState.eFlags & EF_AWARD_IMPRESSIVE)
			rewardShader = cgs.media.medalImpressive;
		else if (cent->currentState.eFlags & EF_AWARD_EXCELLENT)
			rewardShader = cgs.media.medalExcellent;
		else if (cent->currentState.eFlags & EF_AWARD_GAUNTLET)
			rewardShader = cgs.media.medalGauntlet;
		else if (cent->currentState.eFlags & EF_AWARD_DEFEND)
			rewardShader = cgs.media.medalDefend;
		else if (cent->currentState.eFlags & EF_AWARD_ASSIST)
			rewardShader = cgs.media.medalAssist;
		else if (cent->currentState.eFlags & EF_AWARD_CAP)
			rewardShader = cgs.media.medalCapture;
	}

	if (!rewardShader && cg_drawFriend.integer && !(cent->currentState.eFlags & EF_DEAD) &&
	    cg.snap->ps.persistant[PERS_TEAM] == ci->team && cgs.gametype >= GT_TEAM)
	{
		if (cg_teamFrozenFoe.integer && cgs.osp.gameTypeFreeze && cent->currentState.powerups & (1 << PW_BATTLESUIT) && cent->currentState.weapon == WP_NONE)
		{
			int frozenClientNum = cent->currentState.otherEntityNum;
			if (frozenClientNum >= 0 && frozenClientNum < MAX_CLIENTS && !cgs.teamMarkers.clientHasFrozenAllyMarkNear[frozenClientNum])
			{
				if (!useTeamFrozen)
					friendShader = cgs.media.frozenFoeTagShader;
				else if (useTeamFoeWh && (cg_friendsWallhack.integer & 1) && cg_friendsWallhackFrozen.integer && useTeamFrozen)
					friendShader = cg_friendsWallhackFrozen.integer ? cgs.media.frozenFoeTagShaderWallhack : cgs.media.frozenFoeTagShader;
				else
					friendShader = cgs.media.frozenFoeTagShader;
			}
		}
		else if (cg_drawFriend.integer != 2)
		{
			friendShader = (useTeamFoeWh && (cg_friendsWallhack.integer & 1)) ? cgs.media.friendShaderWallhack : cgs.media.friendShader;
			friendColor = ((cg_healthColorLevels.integer & 2) || ci->health <= 0) ? NULL : (float*)cgs.healthColorByClient[clientNum];
		}
	}

	if (rewardShader)
		CG_PlayerFloatSprite(cent, rewardShader, NULL);
	else if (friendShader)
		CG_PlayerFloatSprite(cent, friendShader, friendColor);
}

void CG_AddRefEntityFrozenPlayer(refEntity_t* ent, entityState_t* state, int team)
{
	const clientInfo_t* ci;

	if (state->clientNum >= MAX_CLIENTS) return;
	ci = &cgs.clientinfo[state->clientNum];

	ent->shaderRGBA[0] = ci->colors.frozen[0] * 255;
	ent->shaderRGBA[1] = ci->colors.frozen[1] * 255;
	ent->shaderRGBA[2] = ci->colors.frozen[2] * 255;

	ent->shaderRGBA[3] = 255;
	ent->customShader = cgs.media.frozenShader;
	trap_R_AddRefEntityToScene(ent);
}


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups(refEntity_t* ent, entityState_t* state, int team)
{

	if (state->powerups & (1 << PW_INVIS))
	{
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene(ent);
	}
	else
	{
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
		    if (team == TEAM_BLUE)
		        ent->customShader = cgs.media.blueKamikazeShader;
		    else
		        ent->customShader = cgs.media.redKamikazeShader;
		    trap_R_AddRefEntityToScene( ent );
		}
		else {*/
		trap_R_AddRefEntityToScene(ent);
		//}

		if (state->powerups & (1 << PW_QUAD))
		{
			if (team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene(ent);
		}
		if (state->powerups & (1 << PW_REGEN))
		{
			if (((cg.time / 100) % 10) == 1)
			{
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene(ent);
			}
		}

		if (state->powerups & (1 << PW_BATTLESUIT))
		{
			if (state->weapon == WP_NONE)
			{
				CG_AddRefEntityFrozenPlayer(ent, state, team);
			}
			else
			{
				if (!cg_altBattleSuit.integer)
					ent->customShader = cgs.media.battleSuitShader;
				else
					ent->customShader = cgs.media.battleSuitShaderNew;
				trap_R_AddRefEntityToScene(ent);
			}
		}
	}
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts(vec3_t normal, int numVerts, polyVert_t* verts)
{
	int             i, j;
	float           incoming;
	vec3_t          ambientLight;
	vec3_t          lightDir;
	vec3_t          directedLight;

	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);

	for (i = 0; i < numVerts; i++)
	{
		incoming = DotProduct(normal, lightDir);
		if (incoming <= 0)
		{
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = (ambientLight[0] + incoming * directedLight[0]);
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = (ambientLight[1] + incoming * directedLight[1]);
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = (ambientLight[2] + incoming * directedLight[2]);
		if (j > 255)
		{
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_AddHitBox
===============
*/
void CG_AddHitBox(centity_t* cent, team_t team)
{
	polyVert_t verts[4];
	clientInfo_t* ci;
	int i;
	vec3_t mins = {-15, -15, -24};
	vec3_t maxs = {15, 15, 32};
	float extx, exty, extz;
	vec3_t corners[8];
	qhandle_t hitboxShaderEdge, hitboxShaderSide;

	// (KONSTALKER) hitbox offset for not render through map
	const float hbox_offset = 1.0f;

	if (!cg_drawHitBox.integer)
	{
		return;
	}

	if (!cg.demoPlayback)
	{
		if (!cgs.osp.serverConfigXHitBox)
		{
			return;
		}
	}

	// get the player's client info
	ci = &cgs.clientinfo[cent->currentState.clientNum];

	// don't draw it if it's us in first-person, for dead players, for frozen players, for invisible players
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson) ||
	        cent->currentState.eFlags & EF_DEAD ||
	        (cgs.osp.gameTypeFreeze && cent->currentState.weapon == WP_NONE && cent->currentState.powerups & (1 << PW_BATTLESUIT)) ||
	        cent->currentState.powerups & (1 << PW_INVIS))
	{
		return;
	}
	// don't draw it if it's a teammate, spectator or free player
	if (cg.snap->ps.persistant[PERS_TEAM] == team &&
	        cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
	        cg.snap->ps.persistant[PERS_TEAM] != TEAM_FREE)
	{
		return;
	}

	// if they don't exist, forget it
	if (!CG_IsEnemy(ci))
	{
		return;
	}

	// if it's us
	if (cent->currentState.number == cg.predictedPlayerState.clientNum)
	{
		// use the view height
		maxs[2] = cg.predictedPlayerState.viewheight + 6;
	}
	else
	{
		int x, zd, zu;

		// otherwise grab the encoded bounding box
		x = (cent->currentState.solid & 255);
		zd = ((cent->currentState.solid >> 8) & 255);
		zu = ((cent->currentState.solid >> 16) & 255) - 32;

		mins[0] = mins[1] = -x;
		maxs[0] = maxs[1] = x;
		mins[2] = -zd;
		maxs[2] = zu;
	}

	// (KONSTALKER) add visual offset to hitboxes
	mins[0] += hbox_offset;
	mins[1] += hbox_offset;
	mins[2] += hbox_offset;
	maxs[0] -= hbox_offset;
	maxs[1] -= hbox_offset;
	maxs[2] -= hbox_offset;

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];


	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;


	// set the polygon's vertex colors
	for (i = 0; i < 4; i++)
	{
		if (cg_hitBoxColor.integer || cg_hitBoxColor.string[0])
		{
			vec4_t color;
			Vector4Copy(cgs.beHitBoxColor, color);

			color[0] = color[0] * 255;
			color[1] = color[1] * 255;
			color[2] = color[2] * 255;
			color[3] = color[3] * 255;

			Vector4Copy(color, verts[i].modulate);
		}
		else
		{
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 128;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}

	VectorAdd(cent->lerpOrigin, maxs, corners[3]);

	VectorCopy(corners[3], corners[2]);
	corners[2][0] -= extx;

	VectorCopy(corners[2], corners[1]);
	corners[1][1] -= exty;

	VectorCopy(corners[1], corners[0]);
	corners[0][0] += extx;

	for (i = 0; i < 4; i++)
	{
		VectorCopy(corners[i], corners[i + 4]);
		corners[i + 4][2] -= extz;
	}

	if (cg_drawHitBox.integer == 2)
	{
		hitboxShaderEdge = cgs.media.hboxShaderNew_cullback;
		hitboxShaderSide = cgs.media.hboxShaderNew_cullback;
	}
	else if (cg_drawHitBox.integer == 3)
	{
		hitboxShaderEdge = cgs.media.hboxShaderNew;
		hitboxShaderSide = cgs.media.hboxShaderNew_nocull;
	}
	else
	{
		hitboxShaderEdge = cgs.osp.hboxShader;
		hitboxShaderSide = cgs.osp.hboxShader_nocull;
	}

	// top
	VectorCopy(corners[0], verts[0].xyz);
	VectorCopy(corners[1], verts[1].xyz);
	VectorCopy(corners[2], verts[2].xyz);
	VectorCopy(corners[3], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderEdge, 4, verts);

	// bottom
	VectorCopy(corners[7], verts[0].xyz);
	VectorCopy(corners[6], verts[1].xyz);
	VectorCopy(corners[5], verts[2].xyz);
	VectorCopy(corners[4], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderEdge, 4, verts);

	// top side
	VectorCopy(corners[3], verts[0].xyz);
	VectorCopy(corners[2], verts[1].xyz);
	VectorCopy(corners[6], verts[2].xyz);
	VectorCopy(corners[7], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderSide, 4, verts);

	// left side
	VectorCopy(corners[2], verts[0].xyz);
	VectorCopy(corners[1], verts[1].xyz);
	VectorCopy(corners[5], verts[2].xyz);
	VectorCopy(corners[6], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderSide, 4, verts);

	// right side
	VectorCopy(corners[0], verts[0].xyz);
	VectorCopy(corners[3], verts[1].xyz);
	VectorCopy(corners[7], verts[2].xyz);
	VectorCopy(corners[4], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderSide, 4, verts);

	// bottom side
	VectorCopy(corners[1], verts[0].xyz);
	VectorCopy(corners[0], verts[1].xyz);
	VectorCopy(corners[4], verts[2].xyz);
	VectorCopy(corners[5], verts[3].xyz);
	trap_R_AddPolyToScene(hitboxShaderSide, 4, verts);
}

/*
===============
CG_PlayerShadow

Add the shadow (extended: alt shadow shaders, BE color, cg_shadows -1 no yaw).
===============
*/
static qboolean CG_PlayerShadow(centity_t* cent, float* shadowPlane)
{
	vec3_t      shadowEnd = {0, 0, 0};
	vec3_t      shadowMins = {-15, -15, 0};
	vec3_t      shadowMaxs = {15, 15, 2};
	trace_t     shadowTrace;
	float       shadowYawAngle;
	int         shadowAlt;
	qhandle_t   shadowMarkShader;
	vec4_t      shadowColor = {1.0f, 1.0f, 1.0f, 1.0f};
	qboolean    useAltShadow;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0)
	{
		return qfalse;
	}

	if (cent->currentState.powerups & (1 << PW_INVIS))
	{
		return qfalse;
	}

	VectorCopy(cent->lerpOrigin, shadowEnd);
	shadowEnd[2] -= SHADOW_DISTANCE;
	trap_CM_BoxTrace(&shadowTrace, cent->lerpOrigin, shadowEnd, shadowMins, shadowMaxs, 0, MASK_PLAYERSOLID);

	if (shadowTrace.fraction == 1.0f || shadowTrace.startsolid || shadowTrace.allsolid)
	{
		return qfalse;
	}

	*shadowPlane = shadowTrace.endpos[2] + 1;

	if (cg_shadows.integer != 1 && cg_shadows.integer != -1)
	{
		return qtrue;
	}

	useAltShadow = CG_BE_FEATURE_ENABLED(CG_BE_ALT_SHADOW);
	shadowAlt = cg_altShadow.integer;
	shadowMarkShader = cgs.media.shadowMarkShader;
	if (useAltShadow && shadowAlt > 0)
	{
		if (shadowAlt < MAX_ALT_SHADERS)
			shadowMarkShader = cgs.media.shadowMarkShaderNew[shadowAlt - 1];
		else
			shadowMarkShader = cgs.media.shadowMarkShaderNew[0];
	}
	if (useAltShadow && shadowAlt)
	{
		shadowColor[0] = cgs.beAltShadowColor[0];
		shadowColor[1] = cgs.beAltShadowColor[1];
		shadowColor[2] = cgs.beAltShadowColor[2];
		shadowTrace.fraction = shadowTrace.fraction - 0.1875f;
		if (shadowTrace.fraction < 0.0f)
			shadowTrace.fraction = 0.0f;
	}
	shadowColor[0] = shadowColor[0] * (1.0f - shadowTrace.fraction);
	shadowColor[1] = shadowColor[1] * (1.0f - shadowTrace.fraction);
	shadowColor[2] = shadowColor[2] * (1.0f - shadowTrace.fraction);
	shadowYawAngle = (cg_shadows.integer == -1) ? 0.0f : cent->pe.legs.yawAngle;
	CG_ImpactMark(shadowMarkShader, shadowTrace.endpos, shadowTrace.plane.normal,
	              shadowYawAngle, shadowColor[0], shadowColor[1], shadowColor[2], 1, qfalse, 24, qtrue, qtrue);
	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface (same cg_shadows gate as shadow).
===============
*/
static void CG_PlayerSplash(centity_t* cent)
{
	vec3_t      splashStart;
	vec3_t      splashEnd;
	trace_t     splashTrace;
	int         splashContents;
	polyVert_t  splashVerts[4];

	if (!cg_shadows.integer)
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, splashEnd);
	splashEnd[2] -= 24;
	splashContents = trap_CM_PointContents(splashEnd, 0);
	if (!(splashContents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, splashStart);
	splashStart[2] += 32;
	splashContents = trap_CM_PointContents(splashStart, 0);
	if (splashContents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}

	trap_CM_BoxTrace(&splashTrace, splashStart, splashEnd, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));
	if (splashTrace.fraction == 1.0f)
	{
		return;
	}

	VectorCopy(splashTrace.endpos, splashVerts[0].xyz);
	splashVerts[0].xyz[0] -= 32;
	splashVerts[0].xyz[1] -= 32;
	splashVerts[0].st[0] = 0;
	splashVerts[0].st[1] = 0;
	splashVerts[0].modulate[0] = 255;
	splashVerts[0].modulate[1] = 255;
	splashVerts[0].modulate[2] = 255;
	splashVerts[0].modulate[3] = 255;

	VectorCopy(splashTrace.endpos, splashVerts[1].xyz);
	splashVerts[1].xyz[0] -= 32;
	splashVerts[1].xyz[1] += 32;
	splashVerts[1].st[0] = 0;
	splashVerts[1].st[1] = 1;
	splashVerts[1].modulate[0] = 255;
	splashVerts[1].modulate[1] = 255;
	splashVerts[1].modulate[2] = 255;
	splashVerts[1].modulate[3] = 255;

	VectorCopy(splashTrace.endpos, splashVerts[2].xyz);
	splashVerts[2].xyz[0] += 32;
	splashVerts[2].xyz[1] += 32;
	splashVerts[2].st[0] = 1;
	splashVerts[2].st[1] = 1;
	splashVerts[2].modulate[0] = 255;
	splashVerts[2].modulate[1] = 255;
	splashVerts[2].modulate[2] = 255;
	splashVerts[2].modulate[3] = 255;

	VectorCopy(splashTrace.endpos, splashVerts[3].xyz);
	splashVerts[3].xyz[0] += 32;
	splashVerts[3].xyz[1] -= 32;
	splashVerts[3].st[0] = 1;
	splashVerts[3].st[1] = 0;
	splashVerts[3].modulate[0] = 255;
	splashVerts[3].modulate[1] = 255;
	splashVerts[3].modulate[2] = 255;
	splashVerts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, splashVerts);
}

/*
===============
CG_PlayerOutline
Compute outline for this player (once per cent, cached) and add outline pass for the given refentity.
Uses only cent and local player — no loop over clients. Call once per body part (legs, torso, head).
===============
*/
static void CG_PlayerOutline(centity_t* cent, qboolean useOutline, qboolean useMarkTeammate, refEntity_t* ent)
{
	static centity_t* lastOutlineCent;
	static qboolean cachedDraw;
	static qhandle_t cachedShader;
	static byte cachedColorBytes[4];

	int clientNum;
	const clientInfo_t* ci;
	const clientInfo_t* localCi;
	qboolean outlineIsSpec;
	qboolean outlineIsEnemy;
	vec4_t outlineColor = {0, 0, 0, 0};
	team_t myStateTeam, myRealTeam, enemyTeam, ourPerspectiveTeam, targetTeam;
	vec3_t uniqueColorVec = {0, 0, 0};

	/* Recompute only when player entity changes (first of 3 calls per player: legs/torso/head) */
	if (cent != lastOutlineCent)
	{
		lastOutlineCent = cent;
		cachedDraw = qfalse;
		cachedShader = 0;
		cachedColorBytes[0] = 0;
		cachedColorBytes[1] = 0;
		cachedColorBytes[2] = 0;
		cachedColorBytes[3] = 0;

		clientNum = cent->currentState.clientNum;
		if (useOutline && clientNum >= 0 && clientNum < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[clientNum];
			localCi = &cgs.clientinfo[cg.clientNum];
			outlineIsSpec = (cgs.gametype >= GT_CA) ?
			    (localCi->rt == TEAM_SPECTATOR) :
			    (localCi->team == TEAM_SPECTATOR);

			outlineIsEnemy = qfalse;
			if (ci != localCi)
			{
				myStateTeam = cg.snap->ps.persistant[PERS_TEAM];
				myRealTeam = (cgs.gametype >= GT_CA) ? localCi->rt : localCi->team;
				enemyTeam = (cgs.gametype >= GT_CA) ? ci->rt : ci->team;
				if (cgs.gametype <= GT_SINGLE_PLAYER)
					outlineIsEnemy = qtrue;
				else if (myStateTeam == TEAM_RED)
					outlineIsEnemy = (enemyTeam == TEAM_BLUE);
				else if (myStateTeam == TEAM_BLUE)
					outlineIsEnemy = (enemyTeam == TEAM_RED);
				else if (myStateTeam == TEAM_SPECTATOR)
				{
					if (cg_spectPOV.integer && (cg.snap->ps.pm_flags & PMF_FOLLOW) &&
					    cg.snap->ps.clientNum >= 0 && cg.snap->ps.clientNum < MAX_CLIENTS)
					{
						ourPerspectiveTeam = (cgs.gametype >= GT_CA) ?
						    cgs.clientinfo[cg.snap->ps.clientNum].rt :
						    cgs.clientinfo[cg.snap->ps.clientNum].team;
						targetTeam = (cgs.gametype >= GT_CA) ? ci->rt : ci->team;
						outlineIsEnemy = (ourPerspectiveTeam != targetTeam);
					}
					else
						outlineIsEnemy = (myRealTeam == TEAM_RED || myRealTeam == TEAM_SPECTATOR) ?
						    (enemyTeam == TEAM_BLUE) : (enemyTeam == TEAM_RED);
				}
			}

			if (cg_drawOutline.integer && ci->infoValid &&
			    !(cent->currentState.number == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson) &&
			    !(cent->currentState.eFlags & EF_DEAD) &&
			    !(cgs.osp.gameTypeFreeze && cent->currentState.weapon == WP_NONE && (cent->currentState.powerups & (1 << PW_BATTLESUIT))) &&
			    !(cent->currentState.powerups & (1 << PW_INVIS)) &&
			    ((cg_drawOutline.integer == 1 && outlineIsEnemy) || (cg_drawOutline.integer == 2 && !outlineIsEnemy) || (cg_drawOutline.integer == 3)) &&
			    (outlineIsEnemy ? cg_enemyOutlineSize.integer : cg_teamOutlineSize.integer))
			{
				cachedDraw = qtrue;

				/* One level: pick outline color by case */
				if (outlineIsSpec && !cg_spectPOV.integer)
					Vector4Copy(ci->team == TEAM_RED ? cgs.beTeamOutlineColor : cgs.beEnemyOutlineColor, outlineColor);
				else if (outlineIsEnemy && cg_enemyOutlineColorUnique.integer == 0)
					Vector4Copy(cgs.beEnemyOutlineColor, outlineColor);
				else if (outlineIsEnemy)
				{
					VectorCopy(UNIQUE_COLOR(clientNum), uniqueColorVec);
					outlineColor[0] = (cg_enemyOutlineColorUnique.integer & 1) ? uniqueColorVec[0] : cgs.beEnemyOutlineColor[0];
					outlineColor[1] = (cg_enemyOutlineColorUnique.integer & 2) ? uniqueColorVec[1] : cgs.beEnemyOutlineColor[1];
					outlineColor[2] = (cg_enemyOutlineColorUnique.integer & 4) ? uniqueColorVec[2] : cgs.beEnemyOutlineColor[2];
					outlineColor[3] = 1.0f;
				}
				else if (useMarkTeammate && cgs.beMarkedTeam[clientNum])
					Vector4Copy(cgs.beMarkedTeamColor, outlineColor);
				else
					Vector4Copy(cgs.beTeamOutlineColor, outlineColor);

				cachedShader = outlineIsEnemy ? cgs.media.outlineShader : cgs.media.teamOutlineShader;
				cachedColorBytes[0] = (byte)(outlineColor[0] * 255);
				cachedColorBytes[1] = (byte)(outlineColor[1] * 255);
				cachedColorBytes[2] = (byte)(outlineColor[2] * 255);
				cachedColorBytes[3] = (byte)(outlineColor[3] * 255);
			}
		}
	}

	if (cachedDraw && ent != NULL)
	{
		ent->customShader = cachedShader;
		*(int*)ent->shaderRGBA = *(int*)cachedColorBytes;
		trap_R_AddRefEntityToScene(ent);
	}
}

/*
===============
CG_Player
===============
*/
void CG_Player(centity_t* cent)
{
	clientInfo_t*    ci;
	refEntity_t     legs;
	refEntity_t     torso;
	refEntity_t     head;
	int             clientNum;
	int             renderfx;
	qboolean        shadow;
	float           shadowPlane;
	qboolean        paintItBlack;
	qboolean        useOutline;
	qboolean useMarkTeammate;
	qboolean useTeamFrozen;
	qboolean useTeamFoeWh;
	/* QVM_LOGIC: pre-computed paintBlack - avoid 3x branch + 3x triple-assign */
	byte            paintBlackRGBA[4] = {0, 0, 0, 0xff};
	/* QVM: one pointer for many accesses to same global slot - cheaper than repeated cgs.clientinfo[cg.clientNum] */
	const clientInfo_t* localCi = &cgs.clientinfo[cg.clientNum];

	if (cg.clientNum == cent->currentState.clientNum && (global_viewlistFirstOption || localCi->team == TEAM_SPECTATOR))
	{
		return;
	}

	/* Frozen cache: set when we see frozen cent (valid), clear when we see live player (valid) */
	if (cgs.osp.gameTypeFreeze)
	{
		if (CG_IS_FROZEN_ENTITY(cent))
		{
			int frozenClientNum;
			frozenClientNum = cent->currentState.otherEntityNum;
			if (frozenClientNum >= 0 && frozenClientNum < MAX_CLIENTS)
			{
				clientInfo_t* frozenCi = &cgs.clientinfo[frozenClientNum];
				frozenCi->frozenEntity = (int)(cent - cg_entities);
				frozenCi->isFrozenEnt = qtrue;
			}
		}
		else
		{
			int cn;
			cn = cent->currentState.clientNum;
			if (cn >= 0 && cn < MAX_CLIENTS)
			{
				clientInfo_t* liveCi = &cgs.clientinfo[cn];
				liveCi->isFrozenEnt = qfalse;
				liveCi->frozenEntity = -1;
			}
		}
	}

	clientNum = cent->currentState.clientNum;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		CG_Error("Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	if (!ci->infoValid)
	{
		return;
	}
	if (!ci->legsModel && !ci->torsoModel && !ci->headModel)
	{
		return;
	}

	/* QVM_LOGIC: compute feature flags once per player instead of multiple checks */
	useMarkTeammate = CG_BE_FEATURE_ENABLED(CG_BE_MARK_TEAMMATE);
	useOutline = CG_BE_FEATURE_ENABLED(CG_BE_OUTLINE) && cg_drawOutline.integer;
	useTeamFrozen = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FROZEN);
	useTeamFoeWh = CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH);

	renderfx = 0;

	if (cent->currentState.number == cg.snap->ps.clientNum)
	{
		if (!cg.renderingThirdPerson)
		{
			renderfx = RF_THIRD_PERSON;         // only draw in mirrors
		}
		else
		{
			if (cg_cameraMode.integer)
			{
				return;
			}
		}
	}
	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	CG_PlayerAngles(cent, legs.axis, torso.axis, head.axis);
	CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp, &torso.oldframe, &torso.frame, &torso.backlerp);
	/* QVM_LOGIC: skip call when no powerups and not spect glow - CG_PlayerPowerups returns immediately anyway */
	if (cent->currentState.powerups ||
	        (localCi->team == TEAM_SPECTATOR && cg_spectGlow.integer))
		CG_PlayerPowerups(cent);
	CG_PlayerSprites(cent, ci, clientNum, useTeamFrozen, useTeamFoeWh);

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if (cg_drawHudMarkers.integer && CG_BE_FEATURE_ENABLED(CG_BE_HUD_MARKERS) && cgs.gametype >= GT_TEAM &&
	        cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
	        cent->currentState.number != cg.snap->ps.clientNum &&
	        ci->team == cg.snap->ps.persistant[PERS_TEAM])
	{
		qboolean isFrozen;
		qboolean friendVisible;
		vec3_t start;
		vec3_t end;
		trace_t trace;
		float dist;

		isFrozen = CG_IS_FROZEN_ENTITY(cent);
		if ((cent->currentState.eFlags & EF_DEAD) && !isFrozen)
			;
		else
		{
			friendVisible = qtrue;
			if (!(cg_friendsWallhack.integer & 2) || (isFrozen && (!cg_friendsWallhackFrozen.integer || !useTeamFrozen)))
			{
				VectorCopy(cg.refdef.vieworg, start);
				VectorCopy(cent->lerpOrigin, end);
				end[2] += 24.0f;
				CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID);
				friendVisible = (trace.fraction == 1.0f);
				if (friendVisible)
				{
					CG_Trace(&trace, start, vec3_origin, vec3_origin, cent->lerpOrigin, cg.snap->ps.clientNum, CONTENTS_SOLID);
					friendVisible = (trace.fraction == 1.0f);
				}
			}
			if (friendVisible || ((useTeamFoeWh && (isFrozen ? ((cg_friendsWallhack.integer & 1) && cg_friendsWallhackFrozen.integer && useTeamFrozen) : (cg_friendsWallhack.integer & 1)) && (!isFrozen || (useTeamFrozen && cg_friendsWallhackFrozen.integer)))))
			{
				if (!(isFrozen && !friendVisible && (!cg_friendsWallhackFrozen.integer || !useTeamFrozen)))
				{
					dist = Distance(cent->lerpOrigin, cg.predictedPlayerState.origin);
					if (cg_friendHudMarkerMaxDist.integer <= 0 || dist <= cg_friendHudMarkerMaxDist.value)
					{
						float hfov_x;
						float sz;
						vec4_t color;

						dist = MAX(dist, 1.0f);
						hfov_x = cg.refdef.fov_x * (float)M_PI / 360.0f;
						sz = 1.0f / tan(hfov_x) * 150.0f / dist;
						sz = MIN(cg_friendHudMarkerMaxScale.value, MAX(cg_friendHudMarkerMinScale.value, sz));
						if ((cg_healthColorLevels.integer & 2) || ci->health <= 0)
							Vector4Copy(colorWhite, color);
						else
							Vector4Copy(cgs.healthColorByClient[clientNum], color);
						CG_HudBorderMarker(cent->lerpOrigin, color, cg_friendHudMarkerSize.value * sz,
						                   isFrozen ? cgs.media.frozenFoeTagShader : cgs.media.friendShader, 270);
					}
				}
			}
		}
	}

	if (cg_shadows.integer == 3 && shadow)
	{
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all
	//
	paintItBlack = cg_deadBodyBlack.integer && (
	                   cent->currentState.eFlags & EF_DEAD || //dead
	                   (cgs.osp.gameTypeFreeze && (cent->currentState.weapon == WP_NONE) && (cent->currentState.powerups & (1 << PW_BATTLESUIT)))// or frozen
	               );
	/* QVM_LOGIC: pre-compute paintBlack RGBA once, apply 3x via single int copy; lookup table avoids switch */
	if (paintItBlack)
	{
		static const byte paintLevels[] = {55, 165, 110, 55};
		byte level = paintLevels[cg_deadBodyBlack.integer > 2 ? 3 : cg_deadBodyBlack.integer];
		paintBlackRGBA[0] = level;
		paintBlackRGBA[1] = level;
		paintBlackRGBA[2] = level;
		/* [3] already 0xff from init */
	}

	if ((cent->currentState.eFlags & EF_DEAD) && cg_deadBodyInvisible.integer)
	{
		if (cent->currentState.weapon == WP_NONE)   // show player model with weapon during intermission vote - don't make it invisible
		{
			legs.customShader = cgs.media.invisShader;
			torso.customShader = cgs.media.invisShader;
			head.customShader = cgs.media.invisShader;
		}
	}


	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy(cent->lerpOrigin, legs.origin);

	VectorCopy(cent->lerpOrigin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy(legs.origin, legs.oldorigin);    // don't positionally lerp at all

	/* Always set shaderRGBA: paintItBlack override, else use ci->colors (was only when useFullbright - left legs/torso/head black otherwise) */
	if (paintItBlack)
	{
		*(int*)legs.shaderRGBA = *(int*)paintBlackRGBA;
	}
	else
	{
		float tmpf;

		tmpf = 255.0f * ci->colors.legs[0];
		legs.shaderRGBA[0] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.legs[1];
		legs.shaderRGBA[1] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.legs[2];
		legs.shaderRGBA[2] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		legs.shaderRGBA[3] = 0xff;
	}

	CG_AddRefEntityWithPowerups(&legs, &cent->currentState, ci->team);
	CG_PlayerOutline(cent, useOutline, useMarkTeammate, &legs);

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	torso.customSkin = ci->torsoSkin;
	VectorCopy(cent->lerpOrigin, torso.lightingOrigin);
	CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");
	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	/* Always set shaderRGBA: paintItBlack override, else use ci->colors */
	if (paintItBlack)
	{
		*(int*)torso.shaderRGBA = *(int*)paintBlackRGBA;
	}
	else
	{
		float tmpf;

		tmpf = 255.0f * ci->colors.torso[0];
		torso.shaderRGBA[0] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.torso[1];
		torso.shaderRGBA[1] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.torso[2];
		torso.shaderRGBA[2] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		torso.shaderRGBA[3] = 0xff;
	}

	CG_AddRefEntityWithPowerups(&torso, &cent->currentState, ci->team);
	CG_PlayerOutline(cent, useOutline, useMarkTeammate, &torso);

	//
	// add the head
	//
	head.hModel = ci->headModel;
	head.customSkin = ci->headSkin;
	VectorCopy(cent->lerpOrigin, head.lightingOrigin);
	CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");
	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	/* Always set shaderRGBA: paintItBlack override, else use ci->colors */
	if (paintItBlack)
	{
		*(int*)head.shaderRGBA = *(int*)paintBlackRGBA;
	}
	else
	{
		float tmpf;

		tmpf = 255.0f * ci->colors.head[0];
		head.shaderRGBA[0] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.head[1];
		head.shaderRGBA[1] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		tmpf = 255.0f * ci->colors.head[2];
		head.shaderRGBA[2] = (byte)(tmpf > 255.0f ? 255 : tmpf);

		head.shaderRGBA[3] = 0xff;
	}

	if (cgs.osp.gameTypeFreeze != 0)
	{
		CG_BreathPuffs(cent, &head);
	}
	CG_AddRefEntityWithPowerups(&head, &cent->currentState, ci->team);
	CG_PlayerOutline(cent, useOutline, useMarkTeammate, &head);

	CG_AddPlayerWeapon(&torso, NULL, cent, ci->team);

	CG_AddHitBox(cent, ci->team);
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t* cent)
{
	cent->errorTime = -99999;       // guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame(&cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim);
	CG_ClearLerpFrame(&cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim);

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset(&cent->pe.torso, 0, sizeof(cent->pe.legs));
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if (cg_debugPosition.integer)
	{
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle);
	}
}





