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


qboolean CG_IsDead(void)
{
	if (cg.snap && (cg.snap->ps.pm_type == PM_DEAD || cg.snap->ps.pm_type == PM_SPINTERMISSION))
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}


qboolean CG_IsLocalClientSpectator(void)
{
	const clientInfo_t* ci = &cgs.clientinfo[cg.clientNum];

	if (cgs.gametype == GT_CA)
	{
		return (ci->rt == TEAM_SPECTATOR) ? qtrue : qfalse;
	}
	else
	{
		return (ci->team == TEAM_SPECTATOR) ? qtrue : qfalse;
	}
}

qboolean CG_IsLocalClientOnScreen(void)
{
	if (!cg.snap || cg.snap->ps.clientNum != cg.clientNum)
	{
		return qfalse;
	}
	return qtrue;
}

qboolean CG_IsEnemy(const clientInfo_t* target)
{
	const clientInfo_t* local = &cgs.clientinfo[cg.clientNum];
	int myStateTeam = cg.snap->ps.persistant[PERS_TEAM];
	int myRealTeam  = local->team;
	int enemyTeam = target->team;
	team_t ourPerspectiveTeam;

	if (CG_OSPIsGameTypeCA(cgs.gametype))
	{
		myRealTeam = local->rt;
		enemyTeam = target->rt;
	}


	if (target == local)
	{
		return qfalse;
	}

	if (cgs.gametype <= GT_SINGLE_PLAYER)
	{
		return qtrue;
	}

	if (myStateTeam == TEAM_SPECTATOR)
	{
		/* BE: Enhanced spectator perspective logic - exact copy from OSP2-BE */
		if (cg_spectPOV.integer && cg.snap->ps.pm_flags & PMF_FOLLOW && cg.snap->ps.clientNum >= 0 && cg.snap->ps.clientNum < MAX_CLIENTS)
		{
			qboolean result;
			team_t targetTeam;

			if (CG_OSPIsGameTypeCA(cgs.gametype))
			{
				ourPerspectiveTeam = cgs.clientinfo[cg.snap->ps.clientNum].rt;
				targetTeam = target->rt;
			}
			else
			{
				ourPerspectiveTeam = cgs.clientinfo[cg.snap->ps.clientNum].team;
				targetTeam = target->team;
			}

			result = (ourPerspectiveTeam != targetTeam);

			// /* Debug output */
			// if (cg_debugAnim.integer)
			// {
			//  CG_Printf("[CG_IsEnemy] Following %d (team %d) | Target: %s (team %d) | isEnemy=%d\n",
			//            cg.snap->ps.clientNum, ourPerspectiveTeam,
			//            target->name, targetTeam, result);
			// }

			return result;
		}
		else
		{
			/* Original PBE logic */
			if (myRealTeam == TEAM_RED || myRealTeam == TEAM_SPECTATOR)
			{
				return enemyTeam == TEAM_BLUE;
			}
			if (myRealTeam == TEAM_BLUE)
			{
				return enemyTeam == TEAM_RED;
			}
			return qfalse;
		}
	}

	if (myStateTeam == TEAM_RED)
	{
		return enemyTeam == TEAM_BLUE;
	}
	if (myStateTeam == TEAM_BLUE)
	{
		return enemyTeam == TEAM_RED;
	}

	return qfalse;
}

/*
=================
CG_IsEnemySimple

Simple version that just compares teams from clientInfo
=================
*/
qboolean CG_IsEnemySimple(const clientInfo_t* target)
{
	const clientInfo_t* local;
	team_t localTeam;
	team_t targetTeam;

	if (!target)
	{
		return qfalse;
	}

	local = &cgs.clientinfo[cg.clientNum];

	// Check if target is local player
	if (target == local)
	{
		return qfalse;
	}

	// Get teams - use rt for CA gametype, otherwise use team
	if (CG_OSPIsGameTypeCA(cgs.gametype))
	{
		localTeam = local->rt;
		targetTeam = target->rt;
	}
	else
	{
		localTeam = local->team;
		targetTeam = target->team;
	}

	// Compare teams - enemy if different teams
	return (localTeam != targetTeam);
}


qboolean CG_IsFrozenEntity(const centity_t* cent)
{
	int powerUpType;
	if (CG_IsFPSClassicoServer())
	{
		powerUpType = PW_INVIS;
	}
	else
	{
		powerUpType = PW_BATTLESUIT;
	}
	if (!cent->currentValid)
		return qfalse;

	if (!cgs.osp.gameTypeFreeze)
		return qfalse;

	if (cent->currentState.eType != ET_PLAYER)
		return qfalse;

	if (!(cent->currentState.powerups & (1 << powerUpType)))
		return qfalse;

	if (cent->currentState.weapon != WP_NONE)
		return qfalse;

	if (cent - cg_entities >= MAX_CLIENTS &&
	        cent->currentState.otherEntityNum >= 0 &&
	        cent->currentState.otherEntityNum <= MAX_CLIENTS)
	{
		return qtrue;
	}

	return qfalse;
}

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

void CG_UpdateOurClientInfo(void);
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

	if (cg.snap->ps.pm_flags & PMF_FOLLOW && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		myteam = cgs.clientinfo[cg.snap->ps.clientNum].team;
		myself = &cgs.clientinfo[cg.snap->ps.clientNum];
	}
	else
	{
		myteam = cg.snap->ps.persistant[PERS_TEAM];
		myself = &cgs.clientinfo[cg.clientNum];
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
	team_t team;

	ci->originalHeadModelHandle = 0;
	ci->originalHeadSkinHandle = 0;
	ci->originalModelIcon = 0;

	if (ci->originalHeadModel[0] == '\0')
	{
		return;
	}

	/* Parse model/skin from hmodel string */
	Q_strncpyz(headModelName, ci->originalHeadModel, sizeof(headModelName));
	skinPtr = strchr(headModelName, '/');
	if (skinPtr)
	{
		*skinPtr = '\0';
		Q_strncpyz(headSkinName, skinPtr + 1, sizeof(headSkinName));
	}
	else
	{
		Q_strncpyz(headSkinName, "default", sizeof(headSkinName));
	}

	/* Register head model */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headModelName);
	ci->originalHeadModelHandle = trap_R_RegisterModel(filename);

	/* If model not found, use sarge as default and reset skin to default */
	if (ci->originalHeadModelHandle == 0)
	{
		Q_strncpyz(headModelName, "sarge", sizeof(headModelName));
		Q_strncpyz(headSkinName, "default", sizeof(headSkinName));
		Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headModelName);
		ci->originalHeadModelHandle = trap_R_RegisterModel(filename);
	}

	/* Determine team color skin if default skin and team game */
	if (Q_stricmp(headSkinName, "default") == 0)
	{
		if (cgs.gametype >= GT_TEAM)
		{
			team = ci->team;
			if (team == TEAM_BLUE)
			{
				Q_strncpyz(headSkinName, cg_swapSkins.integer ? "red" : "blue", sizeof(headSkinName));
			}
			else if (team == TEAM_RED)
			{
				Q_strncpyz(headSkinName, cg_swapSkins.integer ? "blue" : "red", sizeof(headSkinName));
			}
		}
	}

	/* Register head skin */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/head_%s.skin", headModelName, headSkinName);
	ci->originalHeadSkinHandle = trap_R_RegisterSkin(filename);

	/* Register head icon */
	Com_sprintf(filename, sizeof(filename), "models/players/%s/icon_%s.tga", headModelName, headSkinName);
	ci->originalModelIcon = trap_R_RegisterShaderNoMip(filename);
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
	clientNum = ci - cgs.clientinfo;
	for (i = 0 ; i < MAX_GENTITIES ; i++)
	{
		if (cg_entities[i].currentState.clientNum == clientNum
		        && cg_entities[i].currentState.eType == ET_PLAYER)
		{
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

vec3_t* CG_GetAimContactPoint(int clientNum)
{
	static vec3_t impactPoint;
	int i;
	entityState_t es;
	vec3_t axis[3];
	vec3_t start;
	vec3_t end;
	trace_t trace;
	float viewHeight;

	/* Use predicted viewheight for local player, default for others */
	if (clientNum == cg.clientNum)
	{
		viewHeight = cg.predictedPlayerState.viewheight;
	}
	else
	{
		viewHeight = DEFAULT_VIEWHEIGHT;
	}

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		es = cg_entities[i].currentState;
		if (es.eType == ET_PLAYER && es.clientNum == clientNum && !(es.eFlags & EF_DEAD))
		{
			AnglesToAxis(es.apos.trBase, axis);
			VectorCopy(es.pos.trBase, start);
			start[2] += viewHeight;
			VectorMA(start, 131072, axis[0], end);
			CG_Trace(&trace, start, vec3_origin, vec3_origin, end, clientNum, CONTENTS_SOLID);

			VectorCopy(trace.endpos, impactPoint);
			return &impactPoint;
		}
	}
	return NULL;
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

	if (cgs.gametype < GT_TEAM)
	{
		return;
	}

	if (!cg_teamTask.integer)
	{
		return;
	}

	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}

	ci = &cgs.clientinfo[clientNum];

	if (!newInfo || !ci->infoValid || !cg.nextSnap)
	{
		return;
	}

	/* Spectators don't see teamtask marks */
	if (CG_IsLocalClientSpectator())
	{
		return;
	}

	/* Check if sender is in our team - ignore teamtask from enemies */
	if (CG_IsEnemySimple(newInfo))
	{
		return;
	}

	/* Check if sender is muted for team tasks */
	if (CG_TeamTaskFilterIsNameMuted(ci->name_original))
	{
		return;
	}


	// {
	//  /* Non-team game: only process teamtask from ourselves */
	//  if (clientNum != cg.clientNum)
	//  {
	//      return;
	//  }
	// }

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
	CG_OSPTeamTaskUnpack(newInfo->teamTask, clientNum, &toClient, &templateID, &qx, &qy, &qz);

	/* Mark that player supports team task only if unpack was successful (templateID != 0) */
	if (templateID != 0)
	{
		ci->supportsTeamTask = qtrue;
	}

	coord[0] = Dequantize(qx, -3000, 3000, 7);
	coord[1] = Dequantize(qy, -3000, 3000, 7);
	coord[2] = Dequantize(qz, -3000, 3000, 7);

	/* Check if this is an overhead mark type (YES or NO) - if so, track player dynamically */
	/* For TEAMTASK_TARGET, try to find enemy at received position, otherwise use received coordinates */
	/* For other marks, try to use aim contact point for more accurate positioning */
	/* clientNum is the sender's client number (always filled), toClient is the recipient (0 = all) */
	if (CG_IsTeamTaskOverhead(templateID))
	{
		/* Overhead mark - will track player dynamically */
		/* Pass sender's clientNum so mark tracks the sender's head */
		CG_TeamTaskMark(coord, templateID, clientNum, toClient);
	}
	else if (templateID == TEAMTASK_TARGET) /* TARGET is auto-selected by smart mode */
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

		/* Try to find enemy player near received coordinates */
		for (i = 0; i < MAX_GENTITIES; i++)
		{
			cent = &cg_entities[i];
			if (cent->currentState.eType == ET_PLAYER && cent->currentValid)
			{
				/* Check if this is an enemy */
				if (cent->currentState.clientNum >= 0 && cent->currentState.clientNum < MAX_CLIENTS)
				{
					clientInfo_t* targetCi;
					targetCi = &cgs.clientinfo[cent->currentState.clientNum];
					if (targetCi->infoValid && CG_IsEnemy(targetCi))
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

		/* If enemy not found, use coordinates from message */
		if (!foundEnemy)
		{
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
		}
	}
	else if (templateID == TEAMTASK_FROZEN_ALLY || templateID == TEAMTASK_FROZEN_ENEMY) /* FROZEN_ALLY and FROZEN_ENEMY are auto-selected by smart mode */
	{
		/* For frozen bodies, try to find frozen entity at received position */
		/* If found, use frozen entity's current position with +24 height, otherwise use received coordinates */
		int i;
		centity_t* cent;
		vec3_t frozenPos;
		qboolean foundFrozen;
		float searchRadius;
		float dist;
		int frozenClientNum;
		clientInfo_t* frozenCi;

		foundFrozen = qfalse;
		searchRadius = 128.0f; /* Search radius for finding frozen entity near received coordinates */

		/* Try to find frozen entity near received coordinates */
		for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
		{
			cent = &cg_entities[i];
			if (cent->currentState.eType == ET_PLAYER && cent->currentValid)
			{
				/* Check if this is a frozen entity */
				if (CG_IsFrozenEntity(cent))
				{
					frozenClientNum = cent->currentState.otherEntityNum;
					if (frozenClientNum >= 0 && frozenClientNum < MAX_CLIENTS)
					{
						frozenCi = &cgs.clientinfo[frozenClientNum];
						if (frozenCi->infoValid)
						{
							/* Check if frozen body matches the expected type (ally or enemy) */
							qboolean isEnemy;
							qboolean matchesType;

							isEnemy = CG_IsEnemy(frozenCi);
							if (templateID == TEAMTASK_FROZEN_ALLY)
							{
								matchesType = !isEnemy;
							}
							else /* TEAMTASK_FROZEN_ENEMY */
							{
								matchesType = isEnemy;
							}

							if (matchesType)
							{
								/* Check if frozen entity is near received coordinates */
								dist = Distance(cent->lerpOrigin, coord);
								if (dist <= searchRadius)
								{
									/* Found frozen entity - use its position with +24 height */
									VectorCopy(cent->lerpOrigin, frozenPos);
									frozenPos[2] += 24.0f;
									CG_TeamTaskMark(frozenPos, templateID, clientNum, toClient);
									foundFrozen = qtrue;
									break;
								}
							}
						}
					}
				}
			}
		}

		/* If frozen entity not found, use coordinates from message */
		if (!foundFrozen)
		{
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
		}
	}
	else if (templateID == TEAMTASK_REDARMOR || templateID == TEAMTASK_MEGAHEALTH ||
	         templateID == TEAMTASK_QUADDAMAGE || templateID == TEAMTASK_REGENERATION ||
	         templateID == TEAMTASK_BATTLESUIT || templateID == TEAMTASK_HASTE ||
	         templateID == TEAMTASK_YELLOWARMOR || templateID == TEAMTASK_HEALTH ||
	         (cgs.gametype == GT_CTF && (templateID == TEAMTASK_REDFLAG || templateID == TEAMTASK_BLUEFLAG))) /* Items auto-selected by smart mode */
	{
		/* For items, find the item entity near received coordinates */
		/* and use its actual position instead of trace coordinates */
		int i;
		centity_t* cent;
		gitem_t* item;
		int itemIndex;
		vec3_t itemPos;
		qboolean foundItem;
		float searchRadius;
		float dist;
		const char* targetClassname;

		foundItem = qfalse;
		searchRadius = 128.0f; /* Search radius for finding item near received coordinates */

		/* Determine which classname to search for */
		if (templateID == TEAMTASK_REDARMOR)
		{
			targetClassname = "item_armor_body";
		}
		else if (templateID == TEAMTASK_MEGAHEALTH)
		{
			targetClassname = "item_health_mega";
		}
		else if (templateID == TEAMTASK_QUADDAMAGE)
		{
			targetClassname = "item_quad";
		}
		else if (templateID == TEAMTASK_REGENERATION)
		{
			targetClassname = "item_regen";
		}
		else if (templateID == TEAMTASK_BATTLESUIT)
		{
			targetClassname = "item_enviro";
		}
		else if (templateID == TEAMTASK_HASTE)
		{
			targetClassname = "item_haste";
		}
		else if (templateID == TEAMTASK_YELLOWARMOR)
		{
			targetClassname = "item_armor_combat";
		}
		else if (cgs.gametype == GT_CTF && templateID == TEAMTASK_REDFLAG)
		{
			targetClassname = "team_CTF_redflag";
		}
		else if (cgs.gametype == GT_CTF && templateID == TEAMTASK_BLUEFLAG)
		{
			targetClassname = "team_CTF_blueflag";
		}
		else /* TEAMTASK_HEALTH */
		{
			/* For health, we need to check both item_health and item_health_large */
			/* This will be handled in the loop below */
			targetClassname = NULL; /* Will check both */
		}

		/* Try to find item entity near received coordinates */
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
						/* For health, check both item_health and item_health_large */
						if (templateID == TEAMTASK_HEALTH)
						{
							if (!Q_stricmp(item->classname, "item_health") ||
							    !Q_stricmp(item->classname, "item_health_large"))
							{
								/* Check if item is near received coordinates */
								dist = Distance(cent->lerpOrigin, coord);
								if (dist <= searchRadius)
								{
									/* Found item - use its actual position (original position before bob effect) */
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
							/* Check if item is near received coordinates */
							dist = Distance(cent->lerpOrigin, coord);
							if (dist <= searchRadius)
							{
								/* Found item - use its actual position (original position before bob effect) */
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

		/* If item not found, use coordinates from message */
		if (!foundItem)
		{
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
		}
	}
	else if (cgs.gametype == GT_CTF && (templateID == TEAMTASK_REDFLAG_CARRIER || templateID == TEAMTASK_BLUEFLAG_CARRIER))
	{
		/* For flag carriers, try to find player with flag near received coordinates */
		int i;
		centity_t* cent;
		vec3_t carrierPos;
		qboolean foundCarrier;
		float searchRadius;
		float dist;
		int powerupFlag;

		foundCarrier = qfalse;
		searchRadius = 256.0f; /* Search radius for finding flag carrier near received coordinates */

		/* Determine which powerup to check for */
		if (templateID == TEAMTASK_REDFLAG_CARRIER)
		{
			powerupFlag = PW_REDFLAG;
		}
		else /* TEAMTASK_BLUEFLAG_CARRIER */
		{
			powerupFlag = PW_BLUEFLAG;
		}

		/* Try to find player with flag near received coordinates */
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			cent = &cg_entities[i];
			if (cent->currentState.eType == ET_PLAYER && cent->currentValid)
			{
				/* Check if player has the flag */
				if (cent->currentState.powerups & (1 << powerupFlag))
				{
					/* Check if player is near received coordinates */
					dist = Distance(cent->lerpOrigin, coord);
					if (dist <= searchRadius)
					{
						/* Found flag carrier - use their position with +24 height */
						VectorCopy(cent->lerpOrigin, carrierPos);
						carrierPos[2] += 24.0f;
						CG_TeamTaskMark(carrierPos, templateID, clientNum, toClient);
						foundCarrier = qtrue;
						break;
					}
				}
			}
		}

		/* If flag carrier not found, use coordinates from message */
		if (!foundCarrier)
		{
			CG_TeamTaskMark(coord, templateID, clientNum, toClient);
		}
	}
	else if (templateID == TEAMTASK_PLAYERDEAD)
	{
		/* For death marks, use coordinates from message directly (position of death) */
		CG_TeamTaskMark(coord, templateID, clientNum, toClient);
	}
	else if (templateID == TEAMTASK_HERE_DEAD || templateID == TEAMTASK_WARN_DEAD)
	{
		/* For HERE_DEAD and WARN_DEAD marks, use coordinates from message directly (death position) */
		CG_TeamTaskMark(coord, templateID, clientNum, toClient);
	}
	else if (templateID == TEAMTASK_WARN_ALIVE)
	{
		/* For WARN_ALIVE, try to use aim contact point for more accurate positioning (like other marks) */
		vec3_t* aimPoint;
		aimPoint = CG_GetAimContactPoint(clientNum);
		if (aimPoint != NULL)
		{
			CG_TeamTaskMark(*aimPoint, templateID, clientNum, toClient); /* Static mark, but clientNum always filled */
		}
		else
		{
			/* Fallback to coord if aimPoint is not available */
			CG_TeamTaskMark(coord, templateID, clientNum, toClient); /* Static mark, but clientNum always filled */
		}
	}
	else
	{
		/* For other marks, try to use aim contact point for more accurate positioning */
		vec3_t* aimPoint;
		aimPoint = CG_GetAimContactPoint(clientNum);
		if (aimPoint != NULL)
		{
			CG_TeamTaskMark(*aimPoint, templateID, clientNum, toClient); /* Static mark, but clientNum always filled */
		}
		else
		{
			/* Fallback to coord if aimPoint is not available */
			CG_TeamTaskMark(coord, templateID, clientNum, toClient); /* Static mark, but clientNum always filled */
		}
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

	if (CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT))
	{
		isFbSkin = (nameSkin && (Q_stricmp(nameSkin, "fb") == 0)) ? qtrue : qfalse;
	}
	else
	{
		if (nameSkin && (Q_stricmp(nameSkin, "fb") == 0))
		{
			isPmSkin = qtrue;
		}
		nameSkin = "pm";
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

/*
 * Generate replacement name for noname/invisible/duplicate players
 * Mix: 33% animals, 33% funny names, 34% thematic (space/demonic/sci-fi)
 */
static const char* replacementNames[] =
{
	"Darth Sumrak", "Amongus", "JustNamed", "Tiger", "Eagle", "Hawkeye", "Falcon Punch", "Shark",
	"Fox", "Lynx-Lynx", "Panther", "Jaguar^0V12", "Coyote", "Grand_Hyena", "Asteri^0xXx", "Wolverine",
	"Raven", "Owl", "Crow", "Vulture", "Condor", "Albatross", "PopCitizen_AUX07", "Heron",
	"Potato", "T0mat0^1Ju1c3", "PickleRobot", "Noodle", "Waffle", "Pancake", "Donut", "HelloRazer",
	"Chicken", "Duck", "Gosuvity", "Sakla_Norez", "Sloth", "KeeKee", "Panda", "Raccoon",
	"Toaster", "KNWR(5)", "Blender", "senoB-suonimuL", "Vacuum", "Dishwasher", "WashingMachine", "Kettle",
	"Fiend", "Hellhound", "PFE-12", "Scrag", "Spawn", "Revenant", "Archvile", "Titan",
	"Daedalus", "Core", "Unsaint", "^X00FFFFQuantum", "Nanite", "WinstonSon", "Neural", "Hologram",
	"Starfighter", "Destroyer", "Cruiser", "Frigate", "XWING", "Dreadnought", "Carrier", "Fleet",
	"Da^3em^1on", "Hellfire", "Oh Man", "Chaos Knight", "Torment", "Inferno", "Abyss", "Johny^3Br^0a^3v^0o",
	"DroneX", "Botoed", "Mech", "^XFF00FFPrizrak", "Golem", "Automaton", "Construct", "Lol",
	"BladeON", "^XFFFFFFScyth3666", "Claw", "Fang", "Thorn", "SpikeOS", "Barb", "Stinger",
	"Bobo", "Predator", "Skaarj", "Protoss", "Umieko", "Collector", "Vorcha", "Krogan"
};

#define REPLACEMENT_NAMES_COUNT (sizeof(replacementNames) / sizeof(replacementNames[0]))

/*
 * Check if a replacement name is already used by any client (comparison without colors)
 */
static qboolean CG_IsReplacementNameUsed(const char* candidateName)
{
	int p;
	char cleanCandidate[MAX_QPATH];
	char otherClean[MAX_QPATH];

	Q_strncpyz(cleanCandidate, candidateName, sizeof(cleanCandidate));
	CG_RemoveChatEscapeCharAll(cleanCandidate);

	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (!cgs.clientinfo[p].infoValid)
		{
			continue;
		}

		Q_strncpyz(otherClean, cgs.clientinfo[p].name_clean, sizeof(otherClean));
		CG_RemoveChatEscapeCharAll(otherClean);

		if (!otherClean[0])
		{
			continue;
		}

		if (Q_stricmp(cleanCandidate, otherClean) == 0)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
 * Extract first color code from name (format: ^digit where digit is 0-9)
 * Returns -1 if no color code found, otherwise returns the digit (0-9)
 */
static int CG_ExtractFirstColorCode(const char* name)
{
	int i;

	if (!name || !name[0])
	{
		return -1;
	}

	for (i = 0; name[i]; i++)
	{
		if (name[i] == '^' && name[i + 1] >= '0' && name[i + 1] <= '9')
		{
			return name[i + 1] - '0';
		}
	}

	return -1;
}

static void CG_GenerateReplacementName(unsigned short xid, const char* originalName, int clientNum, char* outName, int outSize)
{
	unsigned short hash;
	char xidStr[16];
	char clientNumStr[16];
	int nameIndex;
	int colorIndex;
	int originalColorCode;
	const char* selectedName;
	int attempt;
	char tmpName[MAX_QPATH];

	/* Initialize hash */
	CRC_Init(&hash);

	/* Add XID to hash */
	Com_sprintf(xidStr, sizeof(xidStr), "%hu", xid);
	CRC_ContinueProcessString(&hash, xidStr, strlen(xidStr));

	/* Add original name to hash */
	if (originalName && originalName[0])
	{
		CRC_ContinueProcessString(&hash, originalName, strlen(originalName));
	}

	/* Add clientNum to hash for uniqueness even with same XID and name */
	Com_sprintf(clientNumStr, sizeof(clientNumStr), "%d", clientNum);
	CRC_ContinueProcessString(&hash, clientNumStr, strlen(clientNumStr));

	/* Select replacement name based on hash */
	nameIndex = hash % REPLACEMENT_NAMES_COUNT;
	colorIndex = (hash >> 8) % 10; /* 0-9 for ^0-^9 */

	/* Try to extract color code from original name */
	originalColorCode = CG_ExtractFirstColorCode(originalName);
	if (originalColorCode >= 0)
	{
		/* Use color from original name if found */
		colorIndex = originalColorCode;
	}

	selectedName = replacementNames[nameIndex];
	Com_sprintf(outName, outSize, "^%d%s^0*", colorIndex, selectedName);

	if (CG_IsReplacementNameUsed(outName))
	{
		for (attempt = 0; attempt < REPLACEMENT_NAMES_COUNT; ++attempt)
		{
			nameIndex = (nameIndex + 1) % REPLACEMENT_NAMES_COUNT;
			selectedName = replacementNames[nameIndex];
			Com_sprintf(tmpName, sizeof(tmpName), "^%d%s^0*", colorIndex, selectedName);
			if (!CG_IsReplacementNameUsed(tmpName))
			{
				Q_strncpyz(outName, tmpName, outSize);
				break;
			}
		}
	}
}

/*
 * Check if name should be replaced with replacement name
 */
static qboolean CG_ShouldReplaceWithReplacementName(int clientNum, qboolean nameIsInvisible, qboolean is_exists, const char* name_original)
{
	if (!cg_giveName.integer)
	{
		return qfalse;
	}

	/* Never replace name for local player - always keep original name */
	if (clientNum == cg.clientNum)
	{
		return qfalse;
	}

	/* For value 1: replace names for all players except local player */
	if (cg_giveName.integer == 1)
	{
		return qtrue;
	}

	/* For value 2: replace names only for duplicates, invisible, and NONAME */
	if (cg_giveName.integer == 2)
	{
		if (nameIsInvisible)
		{
			return qtrue;
		}

		if (is_exists)
		{
			return qtrue;
		}

		if (strcmp(name_original, "^1NONAME") == 0)
		{
			return qtrue;
		}
	}

	return qfalse;
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

/* BE: Detect spectator target changes */
static void CG_CheckSpectatorTargetChange(void)
{
	static int lastSpectatorTarget = -1;
	int currentSpectatorTarget = -1;

	/* Only check if we're spectating */
	if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		lastSpectatorTarget = -1;
		return;
	}

	/* Get current spectator target */
	if (cg.snap->ps.pm_flags & PMF_FOLLOW &&
	        cg.snap->ps.clientNum >= 0 &&
	        cg.snap->ps.clientNum < MAX_CLIENTS)
	{
		currentSpectatorTarget = cg.snap->ps.clientNum;
	}

	/* If target changed, update all client models */
	if (currentSpectatorTarget != lastSpectatorTarget)
	{
		lastSpectatorTarget = currentSpectatorTarget;
		if (cgs.gametype >= GT_TEAM || cg_spectPOV.integer)
		{
			CG_UpdateOtherClientsInfo();
		}
	}
}

/*
===================
CG_PlayerNameCheckDuplicate

Check if a player name already exists among other clients.
Returns qtrue if duplicate found, qfalse otherwise.
===================
*/
static qboolean CG_PlayerNameCheckDuplicate(int clientNum, const char* nameClean)
{
	int p;
	char otherOriginalClean[MAX_QPATH];

	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (p == clientNum)
		{
			continue;
		}
		if (!cgs.clientinfo[p].infoValid)
		{
			continue;
		}

		/* Only check non-invisible names for duplicates */
		/* Compare using original names, not name_clean which may be already replaced */
		if (!cgs.clientinfo[p].nameIsInvisible)
		{
			Q_strncpyz(otherOriginalClean, cgs.clientinfo[p].name_original, sizeof(otherOriginalClean));
			CG_RemoveChatEscapeCharAll(otherOriginalClean);
			if (strcmp(nameClean, otherOriginalClean) == 0)
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
===================
CG_PlayerNameBuild

Build player name with specified ID and XID display rules.
showID: 0 = never, 1 = always, 2 = only when duplicate exists
showXID: 0 = never, 1 = always, 2 = only when duplicate exists
===================
*/
static void CG_PlayerNameBuild(clientInfo_t* ci, int clientNum, qboolean is_exists, int showID, int showXID)
{
	qboolean useID;
	qboolean useXID;
	qboolean needShowID;
	qboolean needShowXID;
	qboolean willGetReplacementName;
	char replacementName[MAX_QPATH];
	qboolean useReplacement;
	const char* displayName;
	qboolean isNoname;

	isNoname = (strcmp(ci->name_original, "^1NONAME") == 0);
	willGetReplacementName = CG_ShouldReplaceWithReplacementName(clientNum, ci->nameIsInvisible, is_exists, ci->name_original);

	/* For value 1: always show, for value 2: show only if duplicate/NONAME/invisible */
	/* If cg_giveName is enabled and duplicate exists, don't show ID/XID for value 2 */
	/* (both original and duplicate players should not show ID/XID when cg_giveName handles duplicates) */
	if (showID == 1)
	{
		needShowID = qtrue;
	}
	else if (showID == 2)
	{
		needShowID = (is_exists || isNoname || ci->nameIsInvisible);
		/* If cg_giveName is enabled and duplicate exists, don't show ID/XID */
		/* This applies to both original (who keeps name) and duplicates (who get replacement) */
		if (cg_giveName.integer == 2 && is_exists)
		{
			needShowID = qfalse;
		}
		else if (cg_giveName.integer && willGetReplacementName)
		{
			needShowID = qfalse;
		}
	}
	else
	{
		needShowID = qfalse;
	}

	if (showXID == 1)
	{
		needShowXID = qtrue;
	}
	else if (showXID == 2)
	{
		needShowXID = (is_exists || isNoname || ci->nameIsInvisible);
		/* If cg_giveName is enabled and duplicate exists, don't show ID/XID */
		/* This applies to both original (who keeps name) and duplicates (who get replacement) */
		if (cg_giveName.integer == 2 && is_exists)
		{
			needShowXID = qfalse;
		}
		else if (cg_giveName.integer && willGetReplacementName)
		{
			needShowXID = qfalse;
		}
	}
	else
	{
		needShowXID = qfalse;
	}

	useID = needShowID;
	useXID = needShowXID;
	/* Always use replacement name if willGetReplacementName is true, regardless of ID/XID settings */
	useReplacement = willGetReplacementName;

	if (useReplacement)
	{
		/* Extract color from ci->name if it exists and contains color code (already processed name) */
		/* Otherwise use name_original */
		const char* nameForColor = ci->name_original;
		if (ci->name[0] && CG_ExtractFirstColorCode(ci->name) >= 0)
		{
			nameForColor = ci->name;
		}
		CG_GenerateReplacementName(ci->xid, nameForColor, clientNum, replacementName, sizeof(replacementName));
		Q_strncpyz(ci->name_clean, replacementName, sizeof(ci->name_clean));
		CG_RemoveChatEscapeCharAll(ci->name_clean);
		displayName = replacementName;
	}
	else
	{
		displayName = ci->nameIsInvisible ? ci->name_codes : ci->name_original;
	}

	if (useID && useXID)
	{
		Com_sprintf(ci->name, sizeof(ci->name), ci->nameIsInvisible && !useReplacement ?
		            "[%d]^8%s^7:\'%s^7\'" : "[%d]^8%s^7:%s",
		            clientNum, ci->xidStr, displayName);
	}
	else if (useXID)
	{
		Com_sprintf(ci->name, sizeof(ci->name), ci->nameIsInvisible && !useReplacement ?
		            "^8%s^7:\'%s^7\'" : "^8%s^7:%s",
		            ci->xidStr, displayName);
	}
	else if (useID)
	{
		Com_sprintf(ci->name, sizeof(ci->name), ci->nameIsInvisible && !useReplacement ?
		            "[%d]\'%s\'" : "[%d]%s",
		            clientNum, displayName);
	}
	else
	{
		if (useReplacement)
		{
			Q_strncpyz(ci->name, replacementName, sizeof(ci->name));
		}
		else
		{
			Q_strncpyz(ci->name, ci->name_original, sizeof(ci->name));
		}
	}
}

/*
===================
CG_PlayerNameFindAndUpdateExisting

Update existing player's name when duplicate is detected.
===================
*/
static void CG_PlayerNameFindAndUpdateExisting(int clientNum, const char* nameClean, int showID, int showXID)
{
	int p;
	char otherOriginalClean[MAX_QPATH];

	/* Update all players with this name (including original) */
	/* Compare using original names, not name_clean which may be already replaced */
	for (p = 0; p < MAX_CLIENTS; ++p)
	{
		if (p == clientNum)
		{
			continue;
		}
		if (!cgs.clientinfo[p].infoValid)
		{
			continue;
		}

		/* Compare with original name (clean version) */
		Q_strncpyz(otherOriginalClean, cgs.clientinfo[p].name_original, sizeof(otherOriginalClean));
		CG_RemoveChatEscapeCharAll(otherOriginalClean);

		if (!cgs.clientinfo[p].nameIsInvisible &&
		        strcmp(nameClean, otherOriginalClean) == 0)
		{
			/* Mark as duplicate so all players get replacement names if cg_giveName is enabled */
			CG_PlayerNameBuild(&cgs.clientinfo[p], p, qtrue, showID, showXID);
		}
	}
}

/*
===================
CG_PlayerNameBuildFinal

Build final player name with ID/XID prefix based on cvar settings.
===================
*/
static void CG_PlayerNameBuildFinal(clientInfo_t* ci, int clientNum)
{
	int showID;
	int showXID;
	qboolean is_exists;
	char originalClean[MAX_QPATH];

	showID = cg_playersID.integer;
	showXID = cg_playersXID.integer;

	/* Check for duplicates using original name (before replacement) */
	/* Create clean version of original name for comparison */
	Q_strncpyz(originalClean, ci->name_original, sizeof(originalClean));
	CG_RemoveChatEscapeCharAll(originalClean);
	is_exists = CG_PlayerNameCheckDuplicate(clientNum, originalClean);

	/* If duplicate found, update all existing players with this name (including original) */
	/* Use original name for finding duplicates */
	if (is_exists)
	{
		CG_PlayerNameFindAndUpdateExisting(clientNum, originalClean, showID, showXID);
	}

	CG_PlayerNameBuild(ci, clientNum, is_exists, showID, showXID);
}

/*
===================
CG_IsOnlyTeamTaskChanged

Check if only teamTask field changed in client info.
Returns qtrue if only teamTask changed, qfalse otherwise.
===================
*/
static qboolean CG_IsOnlyTeamTaskChanged(const clientInfo_t* ci, const clientInfo_t* newInfo, const char* configstring)
{
	int clientNum;
	const char* oldConfigstring;
	char newCS[MAX_STRING_CHARS];
	char oldCS[MAX_STRING_CHARS];

	if (!ci->infoValid)
	{
		return qfalse;
	}

	/* Get old configstring */
	clientNum = ci - cgs.clientinfo;
	oldConfigstring = CG_ConfigString(clientNum + CS_PLAYERS);

	if (!oldConfigstring || !oldConfigstring[0])
	{
		return qfalse;
	}

	/* Copy configstrings and remove "tt" field from both */
	Q_strncpyz(newCS, configstring, sizeof(newCS));
	Q_strncpyz(oldCS, oldConfigstring, sizeof(oldCS));

	Info_RemoveKey(newCS, "tt");
	Info_RemoveKey(oldCS, "tt");

	/* Compare configstrings without "tt" field */
	if (Q_stricmp(newCS, oldCS) == 0)
	{
		/* All fields except teamTask match - check if teamTask actually changed */
		if (newInfo->teamTask != ci->teamTask)
		{
			/* Only teamTask changed */
			return qtrue;
		}
	}

	return qfalse;
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
		/* If player immediately joined a game team (not spectator), execute join game command */
		if (currentTeam != TEAM_SPECTATOR)
		{
			CG_ExecCommandIfSet(&cg_exec_joinGame);
		}
		/* If player immediately joined spectator team, execute join spec command */
		else if (currentTeam == TEAM_SPECTATOR)
		{
			CG_ExecCommandIfSet(&cg_exec_joinSpec);
		}
		return;
	}

	/* Check if team actually changed */
	if (prevTeam == currentTeam)
	{
		return;
	}

	/* Execute join game command when leaving spectator team */
	if (prevTeam == TEAM_SPECTATOR && currentTeam != TEAM_SPECTATOR)
	{
		CG_ExecCommandIfSet(&cg_exec_joinGame);
	}

	/* Execute join spec command when joining spectator team */
	if (prevTeam != TEAM_SPECTATOR && currentTeam == TEAM_SPECTATOR)
	{
		CG_ExecCommandIfSet(&cg_exec_joinSpec);
	}
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
			cg.realNumClients = CG_CountRealClients();

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

	/* Check if only teamTask changed - if so, skip full reload */
	if (CG_IsOnlyTeamTaskChanged(ci, &newInfo, configstring))
	{
		/* Only teamTask changed - update it without full reload */
		CG_ClientInfoUpdateTeamTask(&newInfo, clientNum);
		ci->teamTask = newInfo.teamTask;
		/* Don't reset team overlay, don't reload models, don't update other clients */
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

		/* If we changed team, update models & colors */
		if (isOurClient)
		{
			static team_t ourTeam = -1;
			static int ourClientNum = -1;
			static qboolean wasSpectator = qtrue;
			qboolean isSpectator;
			qboolean spectatorStateChanged;
			const qboolean isTeamChanged = cgs.clientinfo[cg.clientNum].rt != ourTeam;
			const qboolean isClientIdChanged = ourClientNum != cg.clientNum;
			numSortedTeamPlayers = 0; //reset team overlay
			if (isTeamGame && isTeamChanged)
			{
				ourTeam = cgs.clientinfo[cg.clientNum].rt;
				CG_UpdateMyTeamColors();
				CG_UpdateOtherClientsInfo();
			}

			if (isTeamGame && isClientIdChanged)
			{
				ourClientNum = cg.clientNum;
				CG_UpdateOtherClientsInfo();
			}

			isSpectator = (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR || cgs.clientinfo[cg.clientNum].rt == TEAM_SPECTATOR);
			spectatorStateChanged = (wasSpectator != isSpectator);
			if (spectatorStateChanged)
			{
				wasSpectator = isSpectator;
				CG_UpdateOtherClientsInfo();
			}
		}

	// Update real client count when client info changes (not in demo mode)
	if (!cg.demoPlayback)
	{
		cg.realNumClients = CG_CountRealClients();
	}
}

void CG_UpdateOurClientInfo(void)
{
	CG_NewClientInfo(cg.clientNum);
}

static void CG_UpdateOtherClientsInfo(void)
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

void CG_UpdateAllClientsInfo(void)
{
	int index;

	for (index = 0; index < MAX_CLIENTS; ++index)
	{
		if (CG_ConfigString(CS_PLAYERS + index))
		{
			CG_NewClientInfo(index);
		}
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

	if (!cg_enableBreath.integer)
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

static qhandle_t CG_GetPlayerSpriteShader(centity_t* cent)
{
	if (CG_IsFrozenEntity(cent))
	{
		return cgs.media.frozenFoeTagShader;
	}
	return cgs.media.friendShader;
}

static qboolean CG_FriendVisible(centity_t* cent)
{
	vec3_t start, end;
	trace_t trace;

	VectorCopy(cg.refdef.vieworg, start);
	VectorCopy(cent->lerpOrigin, end);
	end[2] += 24.0f;
	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.snap->ps.clientNum, CONTENTS_SOLID);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, cent->lerpOrigin,
	         cg.snap->ps.clientNum,
	         CONTENTS_SOLID);

	if (trace.fraction == 1.0)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean CG_IsPlayerValidAndVisible(int clientOrEntityNum, qboolean wallhack)
{
	centity_t* cent;
	clientInfo_t* ci;

	int clientNum;
	int freezeLimit = MAX_CLIENTS;

	if (cgs.osp.gameTypeFreeze && (cg_teamIndicator.integer & PI_FROZEN))
	{
		freezeLimit = MAX_GENTITIES;
	}

	if (clientOrEntityNum < 0 || clientOrEntityNum >= freezeLimit)
		return qfalse;

	cent = &cg_entities[clientOrEntityNum];

	if (!cent->currentValid || cent->currentState.eType != ET_PLAYER)
		return qfalse;

	clientNum = clientOrEntityNum;

	if (clientOrEntityNum >= MAX_CLIENTS && CG_IsFrozenEntity(cent))
	{
		clientNum = cent->currentState.otherEntityNum;

		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
			return qfalse;

		ci = &cgs.clientinfo[clientNum];
		if (!ci->infoValid)
			return qfalse;

		if (clientOrEntityNum < 0 || clientOrEntityNum >= MAX_GENTITIES)
			return qfalse;

		if (!cg_entities[clientOrEntityNum].currentValid)
			return qfalse;

		ci->isFrozenEnt = qtrue;
		ci->frozenEntity = clientOrEntityNum;

		cent = &cg_entities[clientOrEntityNum];
	}
	else
	{
		if (clientNum < 0 || clientNum >= MAX_CLIENTS)
			return qfalse;

		ci = &cgs.clientinfo[clientNum];
		if (!ci->infoValid)
			return qfalse;

		ci->isFrozenEnt = qfalse;
		ci->frozenEntity = 0;
	}

	if (ci->team == TEAM_SPECTATOR)
		return qfalse;

	if (CG_IsLocalClientSpectator())
	{
		if (cg_teamIndicator.integer & PI_SPECTATOR)
		{
			;
		}
		else
		{
			if (CG_IsEnemy(ci))
				return qfalse;
		}
	}
	else
	{
		if (CG_IsEnemy(ci))
			return qfalse;
	}

	if (!wallhack)
	{
		if (!CG_FriendVisible(cent))
			return qfalse;
	}

	return qtrue;
}



static void CG_FriendHudMarker(centity_t* cent)
{
	int team;
	float distance;
	float hfov_x;
	float size;
	vec4_t color;
	clientInfo_t* cl;

	team = cgs.clientinfo[cent->currentState.clientNum].team;
	if (cgs.gametype < GT_TEAM
	        || !cg_teamIndicator.integer
	        || cg.snap->ps.persistant[PERS_TEAM] != team
	        || (cent->currentState.eFlags & EF_DEAD && !CG_IsFrozenEntity(cent))
	        || cent->currentState.number == cg.snap->ps.clientNum
	        || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}

	if ((!(cg_friendsWallhack.integer & 1) && !CG_FriendVisible(cent)) || (!CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH) && !CG_FriendVisible(cent)))
	{
		return;
	}


	distance = Distance(cent->lerpOrigin, cg.predictedPlayerState.origin);
	if (cg_friendHudMarkerMaxDist.integer > 0 && distance > cg_friendHudMarkerMaxDist.value)
	{
		return;
	}

	distance = MAX(distance, 1.0);
	hfov_x = cg.refdef.fov_x * M_PI / 360;
	size = 1.0 / tan(hfov_x) * 150 / distance;
	size = MIN(cg_friendHudMarkerMaxScale.value, size);
	size = MAX(cg_friendHudMarkerMinScale.value, size);

	cl = &cgs.clientinfo[cent->currentState.clientNum];
	if (!(cg_healthColorLevels.integer & 2) && cl->health > 0)
	{
		CG_GetColorForHealth(cl->health, cl->armor, color, NULL);
	}
	else
	{
		Vector4Copy(colorWhite, color);
	}

	CG_HudBorderMarker(
	    cent->lerpOrigin,
	    color,
	    cg_friendHudMarkerSize.value * size,
	    CG_GetPlayerSpriteShader(cent),
	    270
	);
}


/*
===============
CG_HasActiveTeamTaskOverhead

Check if player has active team task mark above head
===============
*/
static qboolean CG_HasActiveTeamTaskOverhead(int clientNum)
{
	int i;
	teamTaskMark_t* mark;

	if (!cg_teamTask.integer && !CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return qfalse;
	}

	if (cgs.gametype < GT_TEAM)
	{
		return qfalse;
	}

	if (cgs.be.teamMarkers.numTeamTaskMarks == 0)
	{
		return qfalse;
	}

	for (i = 0; i < cgs.be.teamMarkers.numTeamTaskMarks; i++)
	{
		mark = &cgs.be.teamMarkers.teamTaskMarks[i];

		if (cg.time >= mark->endTime)
		{
			continue;
		}

		if (mark->isOverhead && mark->clientNum == clientNum)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(centity_t* cent)
{
	clientInfo_t* cl;
	int clientNum;

	clientNum = cent->currentState.clientNum;

	if (clientNum >= 0 && clientNum < MAX_CLIENTS)
	{
		if (CG_HasActiveTeamTaskOverhead(clientNum))
		{
			return;
		}
	}

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

	if (!(cg_drawRewards.integer & DRAW_REWARDS_NOSPRITE))
	{
		if (cent->currentState.eFlags & EF_AWARD_IMPRESSIVE)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalImpressive, NULL);
			return;
		}

		if (cent->currentState.eFlags & EF_AWARD_EXCELLENT)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalExcellent, NULL);
			return;
		}

		if (cent->currentState.eFlags & EF_AWARD_GAUNTLET)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalGauntlet, NULL);
			return;
		}

		if (cent->currentState.eFlags & EF_AWARD_DEFEND)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalDefend, NULL);
			return;
		}

		if (cent->currentState.eFlags & EF_AWARD_ASSIST)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalAssist, NULL);
			return;
		}

		if (cent->currentState.eFlags & EF_AWARD_CAP)
		{
			CG_PlayerFloatSprite(cent, cgs.media.medalCapture, NULL);
			return;
		}
	}

	cl = &cgs.clientinfo[ cent->currentState.clientNum ];

	if (cg_drawFriend.integer)
	{
		if (!(cent->currentState.eFlags & EF_DEAD) && cg.snap->ps.persistant[PERS_TEAM] == cl->team && cgs.gametype >= GT_TEAM)
		{
			if (cg_teamFrozenFoe.integer && cgs.osp.gameTypeFreeze && cent->currentState.powerups & (1 << PW_BATTLESUIT) && cent->currentState.weapon == WP_NONE)
			{
				/* Check if there is an active TEAMTASK_FROZEN_ALLY mark for this frozen ally */
				qboolean hasActiveMark;
				qhandle_t shader;

				hasActiveMark = CG_HasActiveFrozenAllyMark(cent);

				/* Only draw frozenFoeTagShader if there is no active team task mark */
				if (!hasActiveMark)
				{
					if (CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH) && cg_friendsWallhack.integer & 1)
						shader = cgs.media.frozenFoeTagShaderWallhack;
					else
						shader = cgs.media.frozenFoeTagShader;
					CG_PlayerFloatSprite(cent, shader, NULL);
				}
			}
			else if (cg_drawFriend.integer != 2)
			{
				vec4_t color;
				qhandle_t shader;
				// Black color for low hp is transparent, skip it
				if (!(cg_healthColorLevels.integer & 2) && cl->health > 0)
				{
					CG_GetColorForHealth(cl->health, cl->armor, color, NULL);
				}
				else
				{
					VectorCopy(colorWhite, color);
				}
				if (CG_BE_FEATURE_ENABLED(CG_BE_TEAM_FOE_WH) && cg_friendsWallhack.integer & 1)
					shader = cgs.media.friendShaderWallhack;
				else
					shader = cgs.media.friendShader;
				CG_PlayerFloatSprite(cent, shader, color);
			}
			return;
		}
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE     128
static qboolean CG_PlayerShadow(centity_t* cent, float* shadowPlane)
{
	vec3_t      end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t     trace;
	float       yawAngle = cent->pe.legs.yawAngle;
	int         alt = cg_altShadow.integer;
	qhandle_t shadowMarkShader;
	vec4_t      color = { 1, 1, 1, 1 };
	*shadowPlane = 0;

	if (cg_shadows.integer == 0)
	{
		return qfalse;
	}

	// no shadows when invisible
	if (cent->currentState.powerups & (1 << PW_INVIS))
	{
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

	// no shadow if too high
	if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid)
	{
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if (cg_shadows.integer != 1 && cg_shadows.integer != -1) // no mark for stencil or projection shadows
	{
		return qtrue;
	}




	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )

	// use alternate shader
	if (alt <= 0)
	{
		shadowMarkShader = cgs.media.shadowMarkShader;
	}
	else if (CG_BE_FEATURE_ENABLED(CG_BE_ALT_SHADOW) && alt < MAX_ALT_SHADERS)
	{
		shadowMarkShader = cgs.media.shadowMarkShaderNew[alt - 1];
	}
	else
	{
		if (CG_BE_FEATURE_ENABLED(CG_BE_ALT_SHADOW))
			shadowMarkShader = cgs.media.shadowMarkShaderNew[0];
		else
			shadowMarkShader = cgs.media.shadowMarkShader;
	}

	if (CG_BE_FEATURE_ENABLED(CG_BE_ALT_SHADOW) && cg_altShadow.integer)
	{
		color[0] = cgs.be.altShadowColor[0];
		color[1] = cgs.be.altShadowColor[1];
		color[2] = cgs.be.altShadowColor[2];
		trace.fraction = trace.fraction - 0.1875; // brightness fix for alt shadows
		if (trace.fraction < 0)
		{
			trace.fraction = 0;
		}
	}
	// fade the shadow out with height
	color[0] = color[0] * (1.0f - trace.fraction);
	color[1] = color[1] * (1.0f - trace.fraction);
	color[2] = color[2] * (1.0f - trace.fraction);
	// make shadow mark static
	if (cg_shadows.integer == -1)
	{
		yawAngle = 0;
	}

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark(shadowMarkShader, trace.endpos, trace.plane.normal,
	              yawAngle, color[0], color[1], color[2], 1, qfalse, 24, qtrue, qtrue);

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash(centity_t* cent)
{
	vec3_t      start, end;
	trace_t     trace;
	int         contents;
	polyVert_t  verts[4];

	if (!cg_shadows.integer)
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)))
	{
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if (trace.fraction == 1.0)
	{
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
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
			Vector4Copy(cgs.be.hitBoxColor, color);

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

void CG_AddOutline(refEntity_t* ent, centity_t* cent)
{
	clientInfo_t* ci;
	int clientNum, i;
	qboolean isEnemy;
	qboolean isSpectator = CG_IsLocalClientSpectator();

	vec4_t color;

	clientNum = cent->currentState.clientNum;

	if (!cg_drawOutline.integer || clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return;
	}

	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid)
	{
		return;
	}

	if ((cent->currentState.number == cg.predictedPlayerState.clientNum && !cg.renderingThirdPerson) ||
	        (cent->currentState.eFlags & EF_DEAD) ||
	        (cgs.osp.gameTypeFreeze && cent->currentState.weapon == WP_NONE && (cent->currentState.powerups & (1 << PW_BATTLESUIT))) ||
	        (cent->currentState.powerups & (1 << PW_INVIS)))
	{
		return;
	}

	isEnemy = CG_IsEnemy(ci);

	if (!((cg_drawOutline.integer == 1 && isEnemy) ||
	        (cg_drawOutline.integer == 2 && !isEnemy) ||
	        (cg_drawOutline.integer == 3)))
	{
		return;
	}
	if (isEnemy && !cg_enemyOutlineSize.integer)
	{
		return;
	}
	if (!isEnemy && !cg_teamOutlineSize.integer)
	{
		return;
	}

	ent->customShader = isEnemy ? cgs.media.outlineShader : cgs.media.teamOutlineShader;

	if (isSpectator && !cg_spectPOV.integer)
	{
		if (ci->team == TEAM_RED)
			Vector4Copy(cgs.be.teamOutlineColor, color);
		else
			Vector4Copy(cgs.be.enemyOutlineColor, color);
	}
	else if (isEnemy)
	{
		if (cg_enemyOutlineColorUnique.integer == 0)
		{
			Vector4Copy(cgs.be.enemyOutlineColor, color);
		}
		else
		{
			vec3_t uniqueColorVec;
			vec4_t tmpColor;

			VectorCopy(UNIQUE_COLOR(clientNum), uniqueColorVec);
			tmpColor[0] = uniqueColorVec[0];
			tmpColor[1] = uniqueColorVec[1];
			tmpColor[2] = uniqueColorVec[2];
			tmpColor[3] = 1.0f;

			for (i = 0; i < 3; i++)
			{
				if (cg_enemyOutlineColorUnique.integer & (1 << i))
				{
					color[i] = tmpColor[i];
				}
				else
				{
					color[i] = cgs.be.enemyOutlineColor[i];
				}
			}
			color[3] = 1.0f;
		}
	}
	else
	{
		if (CG_BE_FEATURE_ENABLED(CG_BE_MARK_TEAMMATE) && cgs.be.markedTeam[clientNum])
		{
			Vector4Copy(cgs.be.markedTeamColor, color);
		}
		else
		{
			Vector4Copy(cgs.be.teamOutlineColor, color);
		}
	}

	for (i = 0; i < 4; i++)
	{
		ent->shaderRGBA[i] = (unsigned char)(color[i] * 255);
	}

	trap_R_AddRefEntityToScene(ent);
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
	float           paintBlackLevel;

	if (cg_spectPOV.integer)
	{
		static int lastFrameTime = 0;
		if (cg.time != lastFrameTime)
		{
			lastFrameTime = cg.time;
			CG_CheckSpectatorTargetChange();
		}
	}

	if (cg.clientNum == cent->currentState.clientNum && (global_viewlistFirstOption || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR))
	{
		return;
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
	CG_PlayerPowerups(cent);
	CG_PlayerSprites(cent);

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if (cg_drawHudMarkers.integer)
	{
		CG_FriendHudMarker(cent);
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
	if (paintItBlack)
	{
		switch (cg_deadBodyBlack.integer)
		{
			case 1:
				paintBlackLevel = 165;
				break;
			case 2:
				paintBlackLevel = 110;
				break;
			default:
			case 3:
				paintBlackLevel = 55;
				break;
		}
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

	if (ci->isPmSkin || (ci->isFbSkin && CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT)))
	{
		float tmpf;
		const float maxf = (float)MAX_QINT;

		tmpf = 255.0 * ci->colors.legs[0];
		if (tmpf >= maxf)
		{
			legs.shaderRGBA[0] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			legs.shaderRGBA[0] = tmpf;
		}

		tmpf = 255.0 * ci->colors.legs[1];
		if (tmpf >= maxf)
		{
			legs.shaderRGBA[1] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			legs.shaderRGBA[1] = tmpf;
		}

		tmpf = 255.0 * ci->colors.legs[2];
		if (tmpf >= maxf)
		{
			legs.shaderRGBA[2] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			legs.shaderRGBA[2] = tmpf;
		}
		legs.shaderRGBA[3] = 0xff;
	}

	if (paintItBlack)
	{
		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = paintBlackLevel;
	}

	CG_AddRefEntityWithPowerups(&legs, &cent->currentState, ci->team);

	if (CG_BE_FEATURE_ENABLED(CG_BE_OUTLINE) && cg_drawOutline.integer)
		CG_AddOutline(&legs, cent);

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	torso.customSkin = ci->torsoSkin;
	VectorCopy(cent->lerpOrigin, torso.lightingOrigin);
	CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");
	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	if (ci->isPmSkin || (ci->isFbSkin && CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT)))
	{
		float tmpf;
		const float maxf = (float)MAX_QINT;

		tmpf = 255.0 * ci->colors.torso[0];
		if (tmpf >= maxf)
		{
			torso.shaderRGBA[0] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			torso.shaderRGBA[0] = tmpf;
		}

		tmpf = 255.0 * ci->colors.torso[1];
		if (tmpf >= maxf)
		{
			torso.shaderRGBA[1] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			torso.shaderRGBA[1] = tmpf;
		}

		tmpf = 255.0 * ci->colors.torso[2];
		if (tmpf >= maxf)
		{
			torso.shaderRGBA[2] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			torso.shaderRGBA[2] = tmpf;
		}
		torso.shaderRGBA[3] = 0xff;
	}

	if (paintItBlack)
	{
		torso.shaderRGBA[0] = torso.shaderRGBA[1] = torso.shaderRGBA[2] = paintBlackLevel;
	}

	CG_AddRefEntityWithPowerups(&torso, &cent->currentState, ci->team);

	if (CG_BE_FEATURE_ENABLED(CG_BE_OUTLINE) && cg_drawOutline.integer)
		CG_AddOutline(&torso, cent);

	//
	// add the head
	//
	head.hModel = ci->headModel;
	head.customSkin = ci->headSkin;
	VectorCopy(cent->lerpOrigin, head.lightingOrigin);
	CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");
	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	if (ci->isPmSkin || (ci->isFbSkin && CG_BE_FEATURE_ENABLED(CG_BE_FULLBRIGHT)))
	{
		float tmpf;
		const float maxf = (float)MAX_QINT;

		tmpf = 255.0 * ci->colors.head[0];
		if (tmpf >= maxf)
		{
			head.shaderRGBA[0] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			head.shaderRGBA[0] = tmpf;
		}

		tmpf = 255.0 * ci->colors.head[1];
		if (tmpf >= maxf)
		{
			head.shaderRGBA[1] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			head.shaderRGBA[1] = tmpf;
		}

		tmpf = 255.0 * ci->colors.head[2];
		if (tmpf >= maxf)
		{
			head.shaderRGBA[2] = -1 * (tmpf - (float)MAX_QINT);
		}
		else
		{
			head.shaderRGBA[2] = tmpf;
		}
		head.shaderRGBA[3] = 0xff;
	}

	if (paintItBlack)
	{
		head.shaderRGBA[0] = head.shaderRGBA[1] = head.shaderRGBA[2] = paintBlackLevel;
	}

	if (cgs.osp.gameTypeFreeze != 0)
	{
		CG_BreathPuffs(cent, &head);
	}
	CG_AddRefEntityWithPowerups(&head, &cent->currentState, ci->team);

	if (CG_BE_FEATURE_ENABLED(CG_BE_OUTLINE) && cg_drawOutline.integer)
		CG_AddOutline(&head, cent);

	CG_AddPlayerWeapon(&torso, NULL, cent, ci->team);

	if (cg_drawHitBox.integer)
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





