#include "cg_local.h"
#include "../game/bg_local.h"
#include "superhud/superhud.h"




extern qboolean localClient;


qboolean CG_OSPIsGameTypeFreeze()
{
	return (cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF) && cgs.osp.gameTypeFreeze;
}

/*
=================
CG_ClientInfoIsFreezeTinfoFrozen

Freeze mode: scoreboard/tinfo 0 HP and 0 armor means frozen body (no live entity to tag).
=================
*/
qboolean CG_ClientInfoIsFreezeTinfoFrozen(const clientInfo_t* ci)
{
	if (!ci->infoValid || !CG_OSPIsGameTypeFreeze())
	{
		return qfalse;
	}
	if (ci->health > 0 || ci->armor > 0)
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
CG_ClientMarkerIsFrozen

Team indicator / stack: prefer freeze-body or live model in snap; tinfo only when
no drawable player (teamProxy / globalPos). Avoids stale tinfo after thaw.
=================
*/
qboolean CG_ClientMarkerIsFrozen(const clientInfo_t* ci)
{
	centity_t* cent;
	int clientNum;

	if (!ci || !ci->infoValid)
	{
		return qfalse;
	}
	if (ci->isFrozenEnt)
	{
		return qtrue;
	}

	clientNum = (int)(ci - cgs.clientinfo);
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		return CG_ClientInfoIsFreezeTinfoFrozen(ci);
	}

	cent = &cg_entities[clientNum];
	if (cent->currentValid && cent->currentState.eType == ET_PLAYER
	        && !CG_IS_TEAM_PROXY(cent))
	{
		return qfalse;
	}

	return CG_ClientInfoIsFreezeTinfoFrozen(ci);
}

qboolean CG_OSPIsGameTypeTDM()
{
	return cgs.gametype == GT_TEAM && !(cgs.osp.gameTypeFreeze);
}

qboolean CG_OSPIsStatsHidden(qboolean check_gametype, qboolean check_warmup)
{

	if (check_gametype)
	{
		if (cgs.gametype == GT_TOURNAMENT)
		{
			return CG_OSPIsStatsHidden(qfalse, check_warmup);
		}
		return qfalse;
	}

	if (check_warmup && cg.warmup)
	{
		return qfalse;
	}
	if (cg.intermissionStarted)
	{
		return qfalse;
	}
	if (cg.demoPlayback)
	{
		return qfalse;
	}
	if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR)
	{
		return qtrue;
	}
	return qfalse;
}



void CG_PmovePhysicsResetBaseGlobals(void)
{
	extern float pm_spectatorfriction;
	extern float modeSwimScale1;
	extern float modeSwimScale2;

	pm_stopspeed = 100.0f;
	pm_duckScale = 0.25f;
	pm_swimScale = 0.50f;
	pm_wadeScale = 0.70f;
	pm_accelerate = 10.0f;
	pm_airaccelerate = 1.0f;
	pm_wateraccelerate = 4.0f;
	pm_flyaccelerate = 8.0f;
	pm_friction = 6.0f;
	pm_waterfriction = 1.0f;
	pm_flightfriction = 3.0f;
	pm_spectatorfriction = 5.0f;
	/* VQ3 / baseq3a / metarena — not OSP promode swim scales */
	modeSwimScale1 = 0.50f;
	modeSwimScale2 = 0.70f;
}

void CG_OSPSetMode(int value)
{
#define OSP_SET_MODE_VARIANT(VAL, VAL_VQ3, VAL_CQ3, VAL_PROMODE) (VAL&OSP_SERVER_MODE_PROMODE ? VAL_PROMODE : (VAL&OSP_SERVER_MODE_CQ3 ? VAL_CQ3 : VAL_VQ3))

	/* UnFreeZe / fpsclassico: stock VQ3 movement + uf_* phys only (ignore OSP CS 806) */
	if (cgs.fpsclassico)
	{
		value = OSP_SERVER_MODE_VQ3;
	}

	modeCpmaPmove                      = 0;
	modeJumpVelocity                   = 270;

	if (value & OSP_SERVER_MODE_CPMA)
	{
		modePromodePhysKoeff               = 105;
		modeJumpVelocity                   = 275;
		modeCpmaPmove                      = 1;
		pm_friction                        = 6;
		modePredictionKoeff1               = 1;
		modePromode_pm_airaccelerate_1     = 2.5f;
		modePredictionKoeff2               = 150;
		modePromode_pm_airaccelerate_2     = 70;
		modeWishspeedLimit                 = 30;
		pm_accelerate                      = 15;
		modeSwimScale1                     = 0.54f;
		modeSwimScale2                     = 0.585f;
		pm_waterfriction                   = 0.5f;
		pm_wateraccelerate                 = 5.0f;
		if (!cgs.fpsclassico)
		{
			modeGrenadeTime = 800;
			modeFinishWeaponChangeTime = 0;
		}
		else
		{
			/* Overwritten by CG_ApplyUnfreezePhysFromServerinfo Path A/B. */
			modeFinishWeaponChangeTime = 200;
			modeGrenadeTime = 600;
		}
		modeBeginWeaponChangeTime          = 0;
		modePMNoAmmoTime                   = 100;
		modeShotgunKoeff                   = 1000;
		modeShotgunNumberOfPellets         = 16;
		modeMaxAmmoShotgun                 = 200;
		modeMaxAmmoGrenade                 = 200;
		modeMaxAmmoRocket                  = 200;
		modeMaxAmmoRail                    = 200;
		modeShotgunPromode                 = 2;
		modeHitLevelSounds                 = qfalse;
		modePickupDistance                 = 66;
		pm_armorPromode                    = 1;
	}
	else
	{
		modePromodePhysKoeff               = OSP_SET_MODE_VARIANT(value, 0, 0, 100);//-V583
		pm_friction                        = OSP_SET_MODE_VARIANT(value, 6, 6, 6);//-V583
		modePredictionKoeff1               = OSP_SET_MODE_VARIANT(value, 0, 0, 1);//-V583
		modePromode_pm_airaccelerate_1     = OSP_SET_MODE_VARIANT(value, 1.0f, 1.0f, 2.5f);//-V583
		modePredictionKoeff2               = OSP_SET_MODE_VARIANT(value, 0, 0, 150);//-V583
		modePromode_pm_airaccelerate_2     = OSP_SET_MODE_VARIANT(value, 1, 1, 70);//-V583
		modeWishspeedLimit                 = OSP_SET_MODE_VARIANT(value, 400, 400, 30);//-V583
		pm_accelerate                      = OSP_SET_MODE_VARIANT(value, 10, 10, 15);//-V583
		modeSwimScale1                     = OSP_SET_MODE_VARIANT(value, 0.5f, 0.54f, 0.54f);//-V583
		modeSwimScale2                     = OSP_SET_MODE_VARIANT(value, 0.7f, 0.585f, 0.585f);//-V583
		pm_waterfriction                   = OSP_SET_MODE_VARIANT(value, 1.0f, 0.5f, 0.5f);//-V583
		pm_wateraccelerate                 = OSP_SET_MODE_VARIANT(value, 4.0f, 5.0f, 5.0f);//-V583
		modeShotgunKoeff                   = OSP_SET_MODE_VARIANT(value, 700, 900, 900);//-V583
		modeShotgunNumberOfPellets         = OSP_SET_MODE_VARIANT(value, 11, 16, 16);//-V583
		modeMaxAmmoShotgun                 = OSP_SET_MODE_VARIANT(value, 200, 100, 100);//-V583
		modeMaxAmmoGrenade                 = OSP_SET_MODE_VARIANT(value, 200, 100, 100);//-V583
		modeMaxAmmoRocket                  = OSP_SET_MODE_VARIANT(value, 200, 100, 100);//-V583
		modeMaxAmmoRail                    = OSP_SET_MODE_VARIANT(value, 200, 100, 100);//-V583
		if (!cgs.fpsclassico)
		{
			modeGrenadeTime = OSP_SET_MODE_VARIANT(value, 800, 800, 700);//-V583
			modeFinishWeaponChangeTime         = OSP_SET_MODE_VARIANT(value, 250, 250, 0);//-V583
		}
		else
		{
			modeFinishWeaponChangeTime = 200;
			modeGrenadeTime = 600;
		}
		modeBeginWeaponChangeTime          = OSP_SET_MODE_VARIANT(value, 200, 200, 0);//-V583
		modePMNoAmmoTime                   = OSP_SET_MODE_VARIANT(value, 500, 500, 100);//-V583
		modeShotgunPromode                 = OSP_SET_MODE_VARIANT(value, 0, 2, 1);
		modeHitLevelSounds                 = OSP_SET_MODE_VARIANT(value, qfalse, qtrue, qtrue);
		modePickupDistance                 = OSP_SET_MODE_VARIANT(value, 36, 66, 66);
		pm_armorPromode                       = OSP_SET_MODE_VARIANT(value, 0, 1, 1);//-V583
	}
	modeUnknown2                       = value & OSP_SERVER_MODE_PROMODE_OPT1 ? 1 : 0;
	modeUnknown3                       = value & OSP_SERVER_MODE_PROMODE_OPT2 ? 200 : 0;
	modeUnknown4                       = value & OSP_SERVER_MODE_PROMODE_OPT2 ? 100 : 0;
	modeFPSClassico                    = cgs.fpsclassico ? 1 : 0;
	if (cgs.fpsclassico)
	{
		/* Path A/B sets finish + grenade times from SERVERINFO (uf_baseq3). */
		CG_ApplyUnfreezePhysFromServerinfo();
	}
}

/*
=================
CG_ApplyUnfreezePhysFromServerinfo

Mirror UnFreeZe CG_ParseServerinfo phys flags for fpsclassico client predict.
Path B (gamename=unfreeze, no g_baseq3): empty g_rampjump → rampjump ON.
=================
*/
void CG_ApplyUnfreezePhysFromServerinfo(void)
{
	const char* info;
	const char* gamename;
	const char* gBaseq3;
	const char* gOsp;
	const char* gRampjump;
	qboolean pathA;

	uf_baseq3 = 0;
	uf_osp = 0;
	uf_rampjump = 0;
	cgs.uf_baseq3 = qfalse;
	cgs.uf_osp = qfalse;
	cgs.uf_rampjump = qfalse;

	if (!cgs.fpsclassico)
	{
		return;
	}

	info = CG_ConfigString(CS_SERVERINFO);
	gamename = Info_ValueForKey(info, "gamename");
	gBaseq3 = Info_ValueForKey(info, "g_baseq3");
	gOsp = Info_ValueForKey(info, "g_osp");
	gRampjump = Info_ValueForKey(info, "g_rampjump");

	pathA = (!Q_stricmp(gamename, "baseq3") || !Q_stricmp(gamename, "osp")
	         || (gBaseq3[0] && Q_stricmp(gBaseq3, "0") != 0));

	if (pathA)
	{
		uf_baseq3 = 1;
		if (!Q_stricmp(gamename, "osp") || (gOsp[0] && Q_stricmp(gOsp, "0") != 0))
		{
			uf_osp = 1;
		}
		uf_rampjump = (Q_stricmp(gRampjump, "1") == 0) ? 1 : 0;
		/* UnFreeZe baseq3: finish 250, GL 800, MG 100 (MG in PM_Weapon) */
		modeFinishWeaponChangeTime = 250;
		modeGrenadeTime = 800;
	}
	else
	{
		/* Path B — UnFreeZe.fpsclasico.de et al. */
		uf_baseq3 = 0;
		uf_osp = 0;
		uf_rampjump = (!gRampjump[0] || Q_stricmp(gRampjump, "0") != 0) ? 1 : 0;
		/* UnFreeZe !baseq3: finish 200, GL 600, MG 75 */
		modeFinishWeaponChangeTime = 200;
		modeGrenadeTime = 600;
	}

	cgs.uf_baseq3 = uf_baseq3 ? qtrue : qfalse;
	cgs.uf_osp = uf_osp ? qtrue : qfalse;
	cgs.uf_rampjump = uf_rampjump ? qtrue : qfalse;
}


static qboolean teamoverlay_registered = qfalse;

int CG_GetUserInfoProfile(void)
{
	char buf[MAX_CVAR_VALUE_STRING];
	int hide;

	if (cgs.fpsclassico)
	{
		return BE_USERINFO_UNFREEZE; /* always unfreeze userinfo on fpsclassico */
	}
	trap_Cvar_VariableStringBuffer("be_hide", buf, sizeof(buf));
	if (!buf[0])
	{
		return BE_USERINFO_BE;
	}
	hide = atoi(buf);
	if (hide < 0)
	{
		return BE_USERINFO_BE;
	}
	if (hide > BE_USERINFO_VANILLA)
	{
		return BE_USERINFO_VANILLA;
	}
	return hide;
}

void CG_UserInfoStripAll(void)
{
	trap_Cvar_Set("osp_client", "");
	trap_Cvar_Update(&osp_client);
	trap_Cvar_Set("be_features", "");
	trap_Cvar_Update(&be_features);
	trap_Cvar_Set("compilation_build", "");
	trap_Cvar_Update(&compilation_build);

	trap_Cvar_Register(&cg_smoothClients, "cg_smoothClients", cg_smoothClients.string, CVAR_ARCHIVE);
	trap_Cvar_Update(&cg_smoothClients);
	trap_Cvar_Register(&cg_delag, "cg_delag", cg_delag.string, CVAR_ARCHIVE);
	trap_Cvar_Update(&cg_delag);
	trap_Cvar_Register(&cg_damagePlums, "cg_damagePlums", cg_damagePlums.string, CVAR_ARCHIVE);
	trap_Cvar_Update(&cg_damagePlums);
	trap_Cvar_Register(&cg_damagePlumsWeapons, "cg_damagePlumsWeapons", cg_damagePlumsWeapons.string, CVAR_ARCHIVE);
	trap_Cvar_Update(&cg_damagePlumsWeapons);
	trap_Cvar_Register(&cg_scorePlums, "cg_scorePlums", cg_scorePlums.string, CVAR_ARCHIVE);
	trap_Cvar_Update(&cg_scorePlums);

	trap_Cvar_Set("teamoverlay", "");
	trap_Cvar_Update(&teamoverlay);
	/* Empty value drops key from userinfo; cvar stays in table for UnFreeZe register-order parity */
	trap_Cvar_Set("teamoverlay2", "");
	trap_Cvar_Update(&teamoverlay2);

	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_ARCHIVE);
}

static void CG_UserInfoEnableVmCvar(vmCvar_t* vm, const char* name, const char* value, int flags)
{
	trap_Cvar_Register(vm, name, value, flags);
	trap_Cvar_Set(name, value);
	trap_Cvar_Update(vm);
}

/* Empty Set removes key from userinfo; value is restored later in enable order. */
static void CG_UserInfoDetachVmCvar(vmCvar_t* vm, const char* name, const char* saved, int flags)
{
	trap_Cvar_Register(vm, name, saved[0] ? saved : "0", flags | CVAR_USERINFO);
	trap_Cvar_Set(name, "");
	trap_Cvar_Register(vm, name, saved[0] ? saved : "0", flags);
	trap_Cvar_Update(vm);
}

static void CG_UserInfoEnableModels(const char* model, const char* headmodel)
{
	trap_Cvar_Register(NULL, "model", model[0] ? model : DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("model", model[0] ? model : DEFAULT_MODEL);
	trap_Cvar_Register(NULL, "headmodel", headmodel[0] ? headmodel : DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("headmodel", headmodel[0] ? headmodel : DEFAULT_MODEL);
}

static void CG_UserInfoEnableTeamModels(const char* team_model, const char* team_headmodel)
{
	trap_Cvar_Register(NULL, "team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL);
	trap_Cvar_Register(NULL, "team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD);
}

static void CG_UserInfoEnableTeamoverlay(void)
{
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}
	if (!teamoverlay_registered)
	{
		trap_Cvar_Register(&teamoverlay, "teamoverlay", "1", CVAR_USERINFO | CVAR_ROM);
		teamoverlay_registered = qtrue;
	}
	else
	{
		trap_Cvar_Register(&teamoverlay, "teamoverlay", teamoverlay.string, CVAR_USERINFO | CVAR_ROM);
	}
	trap_Cvar_Set("teamoverlay", "1");
	trap_Cvar_Update(&teamoverlay);
}

static void CG_UserInfoEnableTeamoverlay2(void)
{
	/* Already in cvarTable (after cg_drawTeamOverlay) — only restore USERINFO value */
	trap_Cvar_Register(&teamoverlay2, "teamoverlay2", "1", CVAR_USERINFO | CVAR_ROM | CVAR_NOTABCOMPLETE);
	trap_Cvar_Set("teamoverlay2", "1");
	trap_Cvar_Update(&teamoverlay2);
}

static void CG_UserInfoEnableOspClient(const char* value)
{
	trap_Cvar_Register(&osp_client, "osp_client", value, CVAR_USERINFO | CVAR_ROM);
	trap_Cvar_Set("osp_client", value);
	trap_Cvar_Update(&osp_client);
}

/*
=================
CG_ApplyUserInfoProfile

Strip optional USERINFO keys, then re-enable in native register order:
  OSP2 / OSP1.03 / BE: osp_client, [BE keys], teamoverlay, scorePlums, smoothClients, model*, team_*
  UnFreeZe:             teamoverlay2, scorePlums, cg_delag, model*, team_*
  Vanilla / baseq3a:    model, headmodel
=================
*/
void CG_ApplyUserInfoProfile(int profile)
{
	char model[MAX_QPATH];
	char headmodel[MAX_QPATH];
	char team_model[MAX_QPATH];
	char team_headmodel[MAX_QPATH];
	char scorePlums[MAX_CVAR_VALUE_STRING];
	char smoothClients[MAX_CVAR_VALUE_STRING];
	char delag[MAX_CVAR_VALUE_STRING];
	char damagePlums[MAX_CVAR_VALUE_STRING];
	char damagePlumsWeapons[MAX_CVAR_VALUE_STRING];
	char beFeatures[MAX_CVAR_VALUE_STRING];
	char compilationBuild[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("model", model, sizeof(model));
	trap_Cvar_VariableStringBuffer("headmodel", headmodel, sizeof(headmodel));
	trap_Cvar_VariableStringBuffer("team_model", team_model, sizeof(team_model));
	trap_Cvar_VariableStringBuffer("team_headmodel", team_headmodel, sizeof(team_headmodel));

	Q_strncpyz(scorePlums, cg_scorePlums.string, sizeof(scorePlums));
	Q_strncpyz(smoothClients, cg_smoothClients.string, sizeof(smoothClients));
	Q_strncpyz(delag, cg_delag.string, sizeof(delag));
	Q_strncpyz(damagePlums, cg_damagePlums.string, sizeof(damagePlums));
	Q_strncpyz(damagePlumsWeapons, cg_damagePlumsWeapons.string, sizeof(damagePlumsWeapons));
	Q_strncpyz(beFeatures, be_features.string, sizeof(beFeatures));
	Q_strncpyz(compilationBuild, compilation_build.string, sizeof(compilationBuild));

	CG_UserInfoStripAll();

	/*
	 * Detach only keys this profile will re-append. Detaching others and then
	 * restoring via Set() can re-insert them into userinfo if USERINFO flags stick.
	 */
	trap_Cvar_Register(NULL, "model", model[0] ? model : DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("model", "");
	trap_Cvar_Register(NULL, "headmodel", headmodel[0] ? headmodel : DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Set("headmodel", "");

	switch (profile)
	{
		case BE_USERINFO_BE:
			CG_UserInfoDetachVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE);
			CG_UserInfoDetachVmCvar(&cg_smoothClients, "cg_smoothClients", smoothClients, CVAR_ARCHIVE);
			CG_UserInfoDetachVmCvar(&cg_damagePlums, "cg_damagePlums", damagePlums, CVAR_ARCHIVE);
			CG_UserInfoDetachVmCvar(&cg_damagePlumsWeapons, "cg_damagePlumsWeapons", damagePlumsWeapons, CVAR_ARCHIVE);
			trap_Cvar_Register(NULL, "team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_model", "");
			trap_Cvar_Register(NULL, "team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_headmodel", "");

			/* OSP2 order + BE keys after osp_client */
			CG_UserInfoEnableOspClient("1008_OSP2_" OSP_VERSION);
			CG_UserInfoEnableVmCvar(&be_features, "be_features", beFeatures, CVAR_ROM | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&compilation_build, "compilation_build", compilationBuild, CVAR_ROM | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&cg_damagePlums, "cg_damagePlums", damagePlums, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&cg_damagePlumsWeapons, "cg_damagePlumsWeapons", damagePlumsWeapons, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableTeamoverlay();
			CG_UserInfoEnableVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&cg_smoothClients, "cg_smoothClients", smoothClients, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableModels(model, headmodel);
			CG_UserInfoEnableTeamModels(team_model, team_headmodel);
			CG_UpdateBeFeatures();
			trap_Cvar_Update(&be_features);
			break;

		case BE_USERINFO_OSP2:
		case BE_USERINFO_OSP103:
			CG_UserInfoDetachVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE);
			CG_UserInfoDetachVmCvar(&cg_smoothClients, "cg_smoothClients", smoothClients, CVAR_ARCHIVE);
			trap_Cvar_Register(NULL, "team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_model", "");
			trap_Cvar_Register(NULL, "team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_headmodel", "");

			CG_UserInfoEnableOspClient(profile == BE_USERINFO_OSP103 ? "1008" : "1008_OSP2_" OSP_VERSION);
			CG_UserInfoEnableTeamoverlay();
			CG_UserInfoEnableVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&cg_smoothClients, "cg_smoothClients", smoothClients, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableModels(model, headmodel);
			CG_UserInfoEnableTeamModels(team_model, team_headmodel);
			break;

		case BE_USERINFO_UNFREEZE:
			CG_UserInfoDetachVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE);
			CG_UserInfoDetachVmCvar(&cg_delag, "cg_delag", delag, CVAR_ARCHIVE);
			trap_Cvar_Register(NULL, "team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_model", "");
			trap_Cvar_Register(NULL, "team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
			trap_Cvar_Set("team_headmodel", "");

			/* UnFreeZe: teamoverlay2, scorePlums, cg_delag, then models */
			CG_UserInfoEnableTeamoverlay2();
			CG_UserInfoEnableVmCvar(&cg_scorePlums, "cg_scorePlums", scorePlums, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableVmCvar(&cg_delag, "cg_delag", delag, CVAR_ARCHIVE | CVAR_USERINFO);
			CG_UserInfoEnableModels(model, headmodel);
			CG_UserInfoEnableTeamModels(team_model, team_headmodel);
			break;

		case BE_USERINFO_VANILLA:
		default:
			/* baseq3a: model / headmodel only (no team_* USERINFO) */
			CG_UserInfoEnableModels(model, headmodel);
			trap_Cvar_Register(NULL, "team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL, CVAR_ARCHIVE);
			trap_Cvar_Set("team_model", team_model[0] ? team_model : DEFAULT_TEAM_MODEL);
			trap_Cvar_Register(NULL, "team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD, CVAR_ARCHIVE);
			trap_Cvar_Set("team_headmodel", team_headmodel[0] ? team_headmodel : DEFAULT_TEAM_HEAD);
			break;
	}

	cgs.userinfoProfile = profile;
}

void CG_OSPUpdateUserInfo(qboolean arg)
{
	int flags;
	char* cmd;
	int using_jpeg;
	if (cg.demoPlayback)
	{
		return;
	}
	if (CG_GetUserInfoProfile() > BE_USERINFO_OSP103)
	{
		return;
	}
	cgs.osp.nextCvarsUpdateTime = 0;
	flags = 0;
	cmd = arg ? "uvinfo" : "uinfo";

	if (cg_followpowerup.integer != 0)
	{
		flags |=  1;
	}
	if (cg_followkiller.integer != 0)
	{
		flags |= 2;
	}
	if (cg_followviewcam.integer != 0)
	{
		flags |= 4;
	}
	using_jpeg = cg_useScreenShotJPEG.integer ? qtrue : qfalse;
	trap_SendClientCommand(va("%s %d %d %d %d %d %d %d\n", cmd, cl_maxpackets.integer, snaps.integer, cl_timenudge.integer, customLocationsEnabled, flags, cg_autoAction.integer, using_jpeg));
}

void CG_OSPEnforceCustomClientSettings(void)
{
	if (cgs.localServer)
	{
		return;
	}

	if (cgs.osp.custom_client & OSP_CUSTOM_CLIENT_TIMER_FLAG)
	{
		if (cg_drawTimer.integer)
		{
			CG_PrintOnce("^3The timer has been disabled.\n");
			trap_Cvar_Set("cg_drawTimer", "0");
		}
	}

	// trap_Cvar_Update(&cg_altLightning);
	// trap_Cvar_Update(&cg_enableOSPHUD);

	// if ((cgs.osp.custom_client & OSP_CUSTOM_CLIENT_ALT_WEAPON_FLAG) == 0)
	// {
	//  if (cg_altPlasma.integer || cg_altGrenades.integer || cg_altLightning.integer)
	//  {

	//  }
	// }

	// if ((cgs.osp.custom_client & OSP_CUSTOM_CLIENT_OSP_HUD_FLAG) == 0)
	// {
	//  if (cg_enableOSPHUD.integer && !(cgs.osp.server_mode & OSP_SERVER_MODE_PROMODE))
	//  {
	//      CG_Printf("^3The OSP-based HUD has been disabled.\n");
	//      trap_Cvar_Set("cg_enableOSPHUD", "0");          // lol
	//  }
	// }

}

/* CS 889 (CS_OSP2BE_TOURNEY_RESTRICTIONS) */
void CG_BETourneyRestrict(void)
{
	int fps;

	if (cgs.beTourneyMode & OSP2BE_TOURNEY_MODE_FPS_LOCK)
	{
		trap_Cvar_Update(&com_maxfps);
		fps = com_maxfps.integer;
		if (fps != 125 && fps != 250)
		{
			fps = fps > 250 ? 250 : 125;
		}
		cgs.beTourneyLockedMaxfps = fps;
		CG_OSP2BEOnComMaxfpsChanged(com_maxfps.integer);
	}
}

void CG_OSPCvarsRestrictValues(void)
{
	qboolean changed = qfalse;

	if (cg_trueLightning.integer > 1)
	{
		trap_Cvar_Set("cg_trueLightning", "1");
	}

	if (cgs.localServer)
	{
		return;
	}

	// why blocked?
	// if (r_fullbright.integer)
	// {
	//  trap_Cvar_Set("r_fullbright", "0");
	//  changed = qtrue;
	// }

	if (r_lodCurveError.integer != 250)
	{
		trap_Cvar_Set("r_lodcurveerror", "250");
		changed = qtrue;
	}

	if (r_subdivisions.integer > 80)
	{
		trap_Cvar_Set("r_subdivisions", "80");
		changed = qtrue;
	}

	if (r_znear.integer != 4)
	{
		trap_Cvar_Set("r_znear", "4");
		changed = qtrue;
	}

	if (r_shownormals.integer != 0)
	{
		trap_Cvar_Set("r_shownormals", "0");
		changed = qtrue;
	}

	if (r_showtris.integer != 0)
	{
		trap_Cvar_Set("r_showtris", "0");
		changed = qtrue;
	}

	if (cg_shadows.integer > 1)
	{
		trap_Cvar_Set("cg_shadows", "1");
		changed = qtrue;
	}

	if (cg_shadows.integer < -1)
	{
		trap_Cvar_Set("cg_shadows", "0");
		changed = qtrue;
	}

	if (changed)
	{
		trap_SendConsoleCommand("vid_restart;\n");
	}
}

/********************************************************************
 * Config string changes
 ********************************************************************/
/*
 * Restrict pmove_fixed
 */
void CG_OSPConfigPmoveSet(int value)
{
	cgs.osp.allow_pmove = value;
	CG_CvarTouch("pmove_fixed");
}

/*
 * Restrict cl_maxpackets
 */
void CG_OSPConfigMinMaxPacketsSet(int value)
{
	cgs.osp.serverConfigMinimumMaxpackets = value;
	CG_CvarTouch("cl_maxpackets");
}
void CG_OSPConfigMaxMaxPacketsSet(int value)
{
	cgs.osp.serverConfigMaximumMaxpackets = value;
	CG_CvarTouch("cl_maxpackets");
}

/*
 * Restrict cl_timenudge
 */
void CG_OSPConfigMinTimenudgeSet(int value)
{
	cgs.osp.serverConfigMinimumTimenudge = value;
	CG_CvarTouch("cl_timenudge");
}

void CG_OSPConfigMaxTimenudgeSet(int value)
{
	cgs.osp.serverConfigMaximumTimenudge = value;
	CG_CvarTouch("cl_timenudge");
}

/* Supported OSP2-BE server (CS 887). Only when visible as BE (be_hide 0). */
void CG_OSPSupportedBEServer(qboolean value)
{
	const char* info;
	const char* gamename;

	if (CG_GetUserInfoProfile() != BE_USERINFO_BE)
	{	
		cgs.beSupportedServer = qfalse;
		return;
	}

	info = CG_ConfigString(CS_SERVERINFO);
	gamename = Info_ValueForKey(info, "gamename");
	if (Q_stricmp(gamename, "baseq3") && Q_stricmp(gamename, "osp"))
	{
		cgs.beSupportedServer = qfalse;
		return;
	}

	cgs.beSupportedServer = value;
}

void CG_BEUpdateUserInfo(void)
{
	int spectatorHud;
	int freezeLook;       /* BE_INFO_FREEZE_LOOK — unused */
	int callvoteMaplist;  /* BE_INFO_CALLVOTE_MAPLIST */
	int extendedAwards;   /* BE_INFO_EXTENDED_AWARDS bitmask */
	int teamProxy;        /* BE_INFO_TEAM_PROXY */
	int mapdata;          /* BE_INFO_MAPDATA */

	if (cg.demoPlayback)
	{
		return;
	}

	if (CG_GetUserInfoProfile() != BE_USERINFO_BE)
	{
		return;
	}

	if (!cgs.beSupportedServer)
	{
		return;
	}

	spectatorHud = 0;
	if (CG_SHUDHasSpectatorItemSpawnsElements())
	{
		spectatorHud |= 1; /* BE_SPECTATOR_HUD_PICKUPS */
	}
	if (CG_SHUDHasSpectatorsElements())
	{
		spectatorHud |= 2; /* BE_SPECTATOR_HUD_SPECTATORS */
	}
	/* bit 4: full tInfo for spectators (any teamIndicator, drawFriend mode/bit4, and/or team1-16/teamRed/teamBlue).
	 * Nonzero teamIndicator covers follow-spec ally indicators without PI_SPECTATOR. */
	if (cg_teamIndicator.integer || CG_DrawFriendMode() || CG_DrawFriendForSpecs() ||
	        CG_SHUDHasTeamRedBlueElements())
	{
		spectatorHud |= 4; /* BE_SPECTATOR_HUD_TEAM_INDICATOR */
	}
	freezeLook = 0;
	callvoteMaplist = 1;
	/* bit1 awards (off for now), bit2 = CA roundstats / roundstatsall */
	extendedAwards = 2; /* BE_EXTENDED_CA_ROUNDSTATS */

	teamProxy = 0;
	if (cgs.gametype >= GT_TEAM
	        && ((cg_teamIndicator.integer != 0)
	            || CG_DrawFriendMode()
	            || (cg_friendsWallhack.integer & (CG_FRIENDS_WALLHACK_TEAM_INDICATOR | CG_FRIENDS_WALLHACK_GLOBAL_POS))))
	{
		teamProxy = 1;
	}
	else if (cgs.gametype < GT_TEAM
	         && CG_IS_LOCAL_CLIENT_SPECTATOR()
	         && (((cg_teamIndicator.integer & PI_SPECTATOR) != 0)
	             || CG_DrawFriendForSpecs()
	             || CG_DrawFriendMode()
	             || cg_specMode.integer == 2
	             || (cg_friendsWallhack.integer & (CG_FRIENDS_WALLHACK_TEAM_INDICATOR | CG_FRIENDS_WALLHACK_GLOBAL_POS))))
	{
		/* FFA/non-team: spectators only — living players must not opt into all-player proxies. */
		teamProxy = 1;
	}

	mapdata = 0;
	if (cg_shadowItems.integer)
	{
		mapdata |= 1; /* BE_MAPDATA_SHADOW */
	}
	if (spectatorHud & 1)
	{
		mapdata |= 2; /* BE_MAPDATA_PUI */
	}
	/* Need bits: missing local mapdata only — independent of draw cvars */
	if (!CG_MapSpawns_FromMapdata())
	{
		mapdata |= 4; /* BE_MAPDATA_NEED_SPAWNS */
	}
	if (!CG_MapTPExit_FromMapdata())
	{
		mapdata |= 8; /* BE_MAPDATA_NEED_TPEXITS */
	}

	trap_SendClientCommand(va("beinfo %d %d %d %d %d %d %d %d\n",
	                          spectatorHud, cg_respawnWeapon.integer,
	                          freezeLook, callvoteMaplist, extendedAwards,
	                          teamProxy, mapdata, 0));
}

// Disable be features
void CG_OSPConfigDisableBEFeatures(int value)
{
	cgs.beDisableFeatures = CG_ApplyFpsclassicoDisableMask(value);
	CG_PrintDisabledFeatures(qfalse);
	CG_UpdateBeFeatures();
}

void CG_OSP2BEOnComMaxfpsChanged(int value)
{
	int allowed;
	int canChange;

	if (!(cgs.beTourneyMode & OSP2BE_TOURNEY_MODE_FPS_LOCK))
	{
		return;
	}

	allowed = value;
	if (allowed != 125 && allowed != 250)
	{
		allowed = allowed > 250 ? 250 : 125;
	}

	canChange = (cg.warmup || !cg.snap ||
	             cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR);

	if (canChange)
	{
		if (allowed != value)
		{
			trap_SendConsoleCommand(va("set com_maxfps %i\n", allowed));
			CG_PrintOnce("^3com_maxfps^7 must be ^2125^7 or ^2250^7 in tournament mode.\n");
		}
		cgs.beTourneyLockedMaxfps = allowed;
		return;
	}

	if (value != cgs.beTourneyLockedMaxfps)
	{
		trap_SendConsoleCommand(va("set com_maxfps %i\n", cgs.beTourneyLockedMaxfps));
		CG_PrintOnce("^3com_maxfps^7 cannot be changed during tournament match. Change in ^2warmup^7 or as ^2spectator^7.\n");
	}
}

/*
 * Unknown cs 0x368
 */
void CG_OSPConfig0x368Set(qboolean value)
{
	cgs.osp.configString_0x368 = value;
}

/*
 * Clan base TDM
 */
void CG_OSPConfigClanBaseTDMSet(qboolean value)
{
	cgs.osp.clanBaseTeamDM = value;
}

/*
 * Custom client
 */
void CG_OSPConfigCustomClientSet(int value)
{
	cgs.osp.custom_client = value;
	CG_CvarTouch("cg_shud");
	CG_OSPEnforceCustomClientSettings();
}

/*
 * Custom client 2
 */
void CG_OSPConfigCustomClient2Set(int value)
{
	cgs.osp.custom_client_2 = value;
	CG_UpdateDamageInfoAllowed();
}

/*
 * Server mode
 */
void CG_OSPConfigModeSet(int value)
{
	if (value & OSP_SERVER_MODE_CPMA)
	{
		cgs.osp.server_mode = OSP_SERVER_MODE_CPMA;
	}
	else if (value & 1)
	{
		cgs.osp.server_mode = OSP_SERVER_MODE_PROMODE;
	}
	else if (value & 2)
	{
		cgs.osp.server_mode = OSP_SERVER_MODE_CQ3;
	}
	else
	{
		cgs.osp.server_mode = OSP_SERVER_MODE_VQ3;
	}
	if (cgs.pmoveMode == PMODE_OSP)
	{
		CG_OSPSetMode(cgs.osp.server_mode);
	}
}
void CG_OSPConfigFreezeModeSet(int value)
{
	if (value == 0)
	{
		cgs.osp.gameTypeFreeze = 0;
	}
	else if (value == 2)
	{
		cgs.osp.gameTypeFreeze = 2;
	}
	else
	{
		cgs.osp.gameTypeFreeze = 1;
	}
}

qboolean CG_IsSpectatorOnScreen(void)
{
	return (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);
}

qboolean CG_IsClientFreeFloat(void)
{
	return cg.snap->ps.pm_type == PM_SPECTATOR;
}

qboolean CG_IsFollowing(void)
{
	if (!cg.snap)
		return qfalse;

	if (!cg.demoPlayback)
	{
		if (!(cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			return qfalse;
		}
		else if (cg.snap->ps.pm_type == PM_FREEZE || cg.intermissionStarted || (cg.snap->ps.clientNum == cg.clientNum && CG_OSPIsGameTypeFreeze()))
		{
			return qfalse;
		}
	}
	else if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return qfalse;
	}
	return qtrue;
}
