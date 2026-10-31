#include "../cg_local.h"
#include "superhud.h"
#include "../../qcommon/qcommon.h"
#include "superhud_private.h"

static superhudElement_t* elementsHead;

static const int shudKeyShowVflag[8] =
{
	SE_KEY1_SHOW, SE_KEY2_SHOW, SE_KEY3_SHOW, SE_KEY4_SHOW,
	SE_KEY5_SHOW, SE_KEY6_SHOW, SE_KEY7_SHOW, SE_KEY8_SHOW
};

static const int shudWeaponShowVflag[WP_NUM_WEAPONS] =
{
	0,
	SE_WEAPON_GAUNTLET,
	SE_WEAPON_MACHINEGUN,
	SE_WEAPON_SHOTGUN,
	SE_WEAPON_GRENADE_LAUNCHER,
	SE_WEAPON_ROCKET_LAUNCHER,
	SE_WEAPON_LIGHTNING,
	SE_WEAPON_RAILGUN,
	SE_WEAPON_PLASMAGUN,
	SE_WEAPON_BFG,
	0
};

static void CG_SHUDRoutenesDestroy(superhudElement_t* shud)
{
	superhudElement_t* tmp;

	while (shud)
	{
		if (shud->element.destroy)
		{
			shud->element.destroy(shud->element.context);
		}
		tmp = shud;
		shud = shud->next;
		Z_Free(tmp);
	}

}

static void CG_SHUDUpdateVisflags(superHUDConfigElement_t* element, const superhudConfig_t* config)
{
	if (element && config && config->visflags.isSet)
	{
		/* Merge HUD config visflags with element-table defaults (do not replace).
		 * Positive config bits are ORed; negated bits clear defaults and set Neg. */
		element->visibility =
		    (element->visibility | config->visflags.value) & ~config->visflags.valueNeg;
		element->visibilityNeg =
		    (element->visibilityNeg | config->visflags.valueNeg) & ~config->visflags.value;
		element->visibilityExt =
		    (element->visibilityExt | config->visflags.valueExt) & ~config->visflags.valueNegExt;
		element->visibilityNegExt =
		    (element->visibilityNegExt | config->visflags.valueNegExt) & ~config->visflags.valueExt;
	}
}

static qboolean CG_SHUDStyleVisSkip(const superhudConfig_t* config)
{
	int activeStyle;
	int mask;

	if (!config->visiblity.isSet)
	{
		return qfalse;
	}

	activeStyle = cg_shudStyle.integer;
	if (activeStyle < 1)
	{
		activeStyle = 1;
	}
	else if (activeStyle > 4)
	{
		activeStyle = 4;
	}
	mask = 1 << (activeStyle - 1);

	if ((config->visiblity.valueNeg & mask) != 0)
	{
		return qtrue;
	}
	if ((config->visiblity.value & SE_SHUD_STYLE_ANY) != 0)
	{
		return (config->visiblity.value & mask) == 0;
	}

	return qfalse;
}

static qboolean CG_SHUDGametypeSkipForGt(int vflags, int gt, qboolean gameTypeFreeze)
{
	return ((vflags & SE_GT_FFA) && gt == GT_FFA) ||
	       ((vflags & SE_GT_TDM) && gt == GT_TEAM && !gameTypeFreeze) ||
	       ((vflags & SE_GT_CTF) && gt == GT_CTF) ||
	       ((vflags & SE_GT_TOURNEY) && gt == GT_TOURNAMENT) ||
	       ((vflags & SE_GT_FREEZETAG) && (gt == GT_TEAM || gt == GT_CTF) && gameTypeFreeze) ||
	       ((vflags & SE_GT_CLANARENA) && gt >= GT_CA);
}

static qboolean CG_SHUDVisflagsAreGametypeOnly(int vflags, int vflagsNeg)
{
	int other;

	other = vflags & ~(SE_GT_ANY | SE_SHOW_EMPTY);
	other |= vflagsNeg & ~(SE_GT_ANY | SE_SHOW_EMPTY);
	return ((vflags | vflagsNeg) & SE_GT_ANY) != 0 && other == 0;
}

#define SE_WEAPON_ANY (SE_WEAPON_GAUNTLET | SE_WEAPON_MACHINEGUN | SE_WEAPON_SHOTGUN | \
	SE_WEAPON_GRENADE_LAUNCHER | SE_WEAPON_ROCKET_LAUNCHER | SE_WEAPON_LIGHTNING | \
	SE_WEAPON_RAILGUN | SE_WEAPON_PLASMAGUN | SE_WEAPON_BFG)

/* Optional per-frame visflags rules (subset of SE_*); only evaluated if set in frameChecks. */
#define SHUD_FCHK_KEY_SHOW  (1 << 0)
#define SHUD_FCHK_GT_SHOW   (1 << 1)
#define SHUD_FCHK_TEAM      (1 << 2)
#define SHUD_FCHK_SCORES    (1 << 3)
#define SHUD_FCHK_DEMO      (1 << 4)
#define SHUD_FCHK_KEY_NEG   (1 << 5)
#define SHUD_FCHK_GT_NEG    (1 << 6)
#define SHUD_FCHK_HEALTH    (1 << 7)
#define SHUD_FCHK_WARMUP    (1 << 8)
#define SHUD_FCHK_WEAPON    (1 << 9)
#define SHUD_FCHK_ENTERCHAT (1 << 10)
#define SHUD_FCHK_SHOWCHAT  (1 << 11)
#define SHUD_FCHK_PRINTUI   (1 << 12)
#define SHUD_FCHK_BESTATS   (1 << 13)
#define SHUD_FCHK_CONSOLE   (1 << 14)
#define SHUD_FCHK_MENU      (1 << 15)
#define SHUD_FCHK_PROMODE   (1 << 16)

/*
 * Cache visflags on the element once at config load (after visibility is copied from config).
 */
static void CG_SHUDElementVisPrepare(superhudElement_t* el)
{
	int vflags;
	int vflagsNeg;
	int vflagsExt;
	int vflagsNegExt;
	int combined;
	int combinedExt;
	int frameChecks;
	superhudElementVis_t* vis;

	vflags = el->element.visibility;
	vflagsNeg = el->element.visibilityNeg;
	vflagsExt = el->element.visibilityExt;
	vflagsNegExt = el->element.visibilityNegExt;
	combined = vflags | vflagsNeg;
	combinedExt = vflagsExt | vflagsNegExt;
	frameChecks = 0;

	if (vflags & SE_KEY_ANY)
	{
		frameChecks |= SHUD_FCHK_KEY_SHOW;
	}
	if (vflags & SE_GT_ANY)
	{
		frameChecks |= SHUD_FCHK_GT_SHOW;
	}
	if (combined & SE_TEAM_ONLY)
	{
		frameChecks |= SHUD_FCHK_TEAM;
	}
	if (combined & SE_SCORES_HIDE)
	{
		frameChecks |= SHUD_FCHK_SCORES;
	}
	if (combined & SE_DEMO_HIDE)
	{
		frameChecks |= SHUD_FCHK_DEMO;
	}
	if (vflagsNeg & SE_KEY_ANY)
	{
		frameChecks |= SHUD_FCHK_KEY_NEG;
	}
	if (vflagsNeg & SE_GT_ANY)
	{
		frameChecks |= SHUD_FCHK_GT_NEG;
	}
	if (combined & SE_HEALTH_LOW)
	{
		frameChecks |= SHUD_FCHK_HEALTH;
	}
	if (combined & SE_WARMUP)
	{
		frameChecks |= SHUD_FCHK_WARMUP;
	}
	if (combined & SE_WEAPON_ANY)
	{
		frameChecks |= SHUD_FCHK_WEAPON;
	}
	if (combinedExt & SE_ENTERCHAT)
	{
		frameChecks |= SHUD_FCHK_ENTERCHAT;
	}
	if (combinedExt & SE_SHOWCHAT)
	{
		frameChecks |= SHUD_FCHK_SHOWCHAT;
	}
	if (combinedExt & SE_PRINTUI)
	{
		frameChecks |= SHUD_FCHK_PRINTUI;
	}
	if (combinedExt & SE_BESTATS)
	{
		frameChecks |= SHUD_FCHK_BESTATS;
	}
	if (combinedExt & SE_CONSOLE)
	{
		frameChecks |= SHUD_FCHK_CONSOLE;
	}
	if (combinedExt & SE_MENU)
	{
		frameChecks |= SHUD_FCHK_MENU;
	}
	if (combinedExt & SE_PROMODE)
	{
		frameChecks |= SHUD_FCHK_PROMODE;
	}

	vis = &el->vis;
	vis->vflags = vflags;
	vis->vflagsNeg = vflagsNeg;
	vis->vflagsExt = vflagsExt;
	vis->vflagsNegExt = vflagsNegExt;
	vis->frameChecks = frameChecks;
	vis->showInIntermission = (vflags & SE_IM) != 0 && (vflagsNeg & SE_IM) == 0;
	vis->showWhenDead = (vflags & SE_DEAD) != 0 && (vflagsNeg & SE_DEAD) == 0;
	vis->showWhenSpect = (vflags & SE_SPECT) != 0 && (vflagsNeg & SE_SPECT) == 0;
}

/*
 * Return qtrue if the element should be drawn this frame (visflags vs frame context).
 */
static qboolean CG_SHUDElementVisible(const superhudElement_t* el, const superhudFrameContext_t* fc,
                                      qboolean be_health_low)
{
	const superhudElementVis_t* vis;
	int vflags;
	int vflagsNeg;
	int vflagsExt;
	int vflagsNegExt;
	int frameChecks;
	qboolean is_spectator_for_element;

	vis = &el->vis;
	vflags = vis->vflags;
	vflagsNeg = vis->vflagsNeg;
	vflagsExt = vis->vflagsExt;
	vflagsNegExt = vis->vflagsNegExt;
	frameChecks = vis->frameChecks;

	if (fc->is_intermission && !vis->showInIntermission)
	{
		return qfalse;
	}
	if (fc->is_dead && !vis->showWhenDead)
	{
		return qfalse;
	}

	is_spectator_for_element = fc->is_spectator;
	if (el->element.isFragmessage)
	{
		is_spectator_for_element = fc->is_spectator || (fc->pm_type == PM_SPECTATOR);
	}
	if (is_spectator_for_element && !vis->showWhenSpect)
	{
		return qfalse;
	}

	if (!frameChecks)
	{
		return qtrue;
	}

	if ((frameChecks & SHUD_FCHK_KEY_SHOW) && fc->keyHideMask && (vflags & fc->keyHideMask))
	{
		return qfalse;
	}
	if ((frameChecks & SHUD_FCHK_GT_SHOW) && (vflags & fc->gtHideMask))
	{
		return qfalse;
	}
	if (frameChecks & SHUD_FCHK_TEAM)
	{
		if ((vflags & SE_TEAM_ONLY) && !fc->is_team_game)
		{
			return qfalse;
		}
		if ((vflagsNeg & SE_TEAM_ONLY) && fc->is_team_game)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_SCORES)
	{
		if ((vflags & SE_SCORES_HIDE) && fc->is_scores)
		{
			return qfalse;
		}
		if ((vflagsNeg & SE_SCORES_HIDE) && !fc->is_scores)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_DEMO)
	{
		if ((vflags & SE_DEMO_HIDE) && fc->demo_playback)
		{
			return qfalse;
		}
		if ((vflagsNeg & SE_DEMO_HIDE) && !fc->demo_playback)
		{
			return qfalse;
		}
	}
	if ((frameChecks & SHUD_FCHK_KEY_NEG) && (vflagsNeg & ~fc->keyHideMask & SE_KEY_ANY))
	{
		return qfalse;
	}
	if ((frameChecks & SHUD_FCHK_GT_NEG) && !(vflagsNeg & fc->gtHideMask))
	{
		return qfalse;
	}
	if (frameChecks & SHUD_FCHK_HEALTH)
	{
		if ((vflags & SE_HEALTH_LOW) && !be_health_low)
		{
			return qfalse;
		}
		if ((vflagsNeg & SE_HEALTH_LOW) && be_health_low)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_WARMUP)
	{
		if ((vflags & SE_WARMUP) && !fc->is_warmup)
		{
			return qfalse;
		}
		if ((vflagsNeg & SE_WARMUP) && fc->is_warmup)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_WEAPON)
	{
		if (((vflags & SE_WEAPON_ANY) && !(vflags & fc->weaponVis)) ||
		        ((vflagsNeg & SE_WEAPON_ANY) && (vflagsNeg & fc->weaponVis)))
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_ENTERCHAT)
	{
		if ((vflagsExt & SE_ENTERCHAT) && !fc->is_enterchat)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_ENTERCHAT) && fc->is_enterchat)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_SHOWCHAT)
	{
		if ((vflagsExt & SE_SHOWCHAT) && !fc->is_showchat)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_SHOWCHAT) && fc->is_showchat)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_PRINTUI)
	{
		if ((vflagsExt & SE_PRINTUI) && !fc->is_printui)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_PRINTUI) && fc->is_printui)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_BESTATS)
	{
		if ((vflagsExt & SE_BESTATS) && !fc->is_bestats)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_BESTATS) && fc->is_bestats)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_CONSOLE)
	{
		if ((vflagsExt & SE_CONSOLE) && !fc->is_console)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_CONSOLE) && fc->is_console)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_MENU)
	{
		if ((vflagsExt & SE_MENU) && !fc->is_menu)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_MENU) && fc->is_menu)
		{
			return qfalse;
		}
	}
	if (frameChecks & SHUD_FCHK_PROMODE)
	{
		if ((vflagsExt & SE_PROMODE) && !fc->is_promode)
		{
			return qfalse;
		}
		if ((vflagsNegExt & SE_PROMODE) && fc->is_promode)
		{
			return qfalse;
		}
	}

	return qtrue;
}

static void CG_SHUDFinalizeElementChain(superhudElement_t** head)
{
	superhudElement_t* el;
	superhudElement_t** link;
	int vflags;
	int vflagsNeg;

	if (!head || !*head)
	{
		return;
	}

	link = head;
	el = *head;
	while (el)
	{
		if (!el->element.context || !el->element.routine)
		{
			superhudElement_t* drop;

			drop = el;
			el = el->next;
			*link = el;
			Z_Free(drop);
			continue;
		}

		vflags = el->element.visibility;
		vflagsNeg = el->element.visibilityNeg;
		CG_SHUDElementVisPrepare(el);

		if (CG_SHUDVisflagsAreGametypeOnly(vflags, vflagsNeg) &&
		        CG_SHUDGametypeSkipForGt(vflags, cgs.gametype, cgs.osp.gameTypeFreeze))
		{
			superhudElement_t* drop;

			drop = el;
			el = el->next;
			*link = el;
			if (drop->element.destroy)
			{
				drop->element.destroy(drop->element.context);
			}
			Z_Free(drop);
			continue;
		}

		if (CG_SHUDStyleVisSkip(&el->config))
		{
			superhudElement_t* drop;

			drop = el;
			el = el->next;
			*link = el;
			if (drop->element.destroy)
			{
				drop->element.destroy(drop->element.context);
			}
			Z_Free(drop);
			continue;
		}

		link = &el->next;
		el = el->next;
	}
}

/* Per-frame player/HUD state used by CG_SHUDElementVisible (see superhudElementVis_t). */
static void CG_SHUDFillFrameContext(superhudFrameContext_t* fc)
{
	int gt;
	int i;
	int keyHideMask;
	int pm_type;

	pm_type = cg.predictedPlayerState.pm_type;
	gt = cgs.gametype;
	fc->pm_type = pm_type;
	fc->currentWeapon = cg.snap ? cg.snap->ps.weapon : WP_NONE;
	/*
	 * Free-float only (PM_SPECTATOR): hide player HUD without SE_SPECT.
	 * Chase uses the followed PS (pm_type NORMAL/DEAD/…) so StatusBar etc. still draw.
	 * TeamRed/Blue routines still gate on CG_IS_LOCAL_CLIENT_SPECTATOR().
	 */
	fc->is_spectator = (cg.snap && cg.snap->ps.pm_type == PM_SPECTATOR) ? qtrue : qfalse;
	fc->is_dead = (pm_type == PM_DEAD || pm_type == PM_FREEZE);
	fc->is_intermission = (pm_type == PM_INTERMISSION);
	fc->is_team_game = (gt >= GT_TEAM);
	fc->is_scores = cg.scoreBoardShowing;
	fc->is_warmup = (cg.warmup != 0) ? qtrue : qfalse;
	fc->is_enterchat = CG_BEChat_IsEnterChat();
	fc->is_showchat = CG_BEChat_IsShowChat();
	fc->is_printui = (cg.printuiActiveCount != 0) ? qtrue : qfalse;
	fc->is_bestats = cg.beStatsShowing;
	{
		int catcher;

		catcher = trap_Key_GetCatcher();
		fc->is_console = (catcher & KEYCATCH_CONSOLE) ? qtrue : qfalse;
		fc->is_menu = (catcher & KEYCATCH_UI) ? qtrue : qfalse;
	}
	if (cgs.pmove_phy_movetype >= 0)
	{
		fc->is_promode = (cgs.pmove_phy_movetype == 0) ? qtrue : qfalse; /* OSDF CPM */
	}
#if 0
	else if (cgs.gamename_unfreeze)
	{
		fc->is_promode = qfalse; /* gamename=unfreeze / stock VQ3 */
	}
#endif
	else
	{
		fc->is_promode = (cgs.osp.server_mode & (OSP_SERVER_MODE_PROMODE | OSP_SERVER_MODE_CPMA)) ? qtrue : qfalse;
	}
	fc->demo_playback = cg.demoPlayback;

	fc->weaponVis = 0;
	if (fc->currentWeapon > WP_NONE && fc->currentWeapon < WP_NUM_WEAPONS)
	{
		fc->weaponVis = shudWeaponShowVflag[fc->currentWeapon];
	}

	fc->gtHideMask = 0;
	if (gt == GT_FFA)
	{
		fc->gtHideMask |= SE_GT_FFA;
	}
	if (gt == GT_TOURNAMENT)
	{
		fc->gtHideMask |= SE_GT_TOURNEY;
	}
	if (gt == GT_TEAM && !cgs.osp.gameTypeFreeze)
	{
		fc->gtHideMask |= SE_GT_TDM;
	}
	if (gt == GT_CTF)
	{
		fc->gtHideMask |= SE_GT_CTF;
	}
	if ((gt == GT_TEAM || gt == GT_CTF) && cgs.osp.gameTypeFreeze)
	{
		fc->gtHideMask |= SE_GT_FREEZETAG;
	}
	if (gt >= GT_CA)
	{
		fc->gtHideMask |= SE_GT_CLANARENA;
	}

	keyHideMask = 0;
	i = 0;
	do
	{
		if (!cgs.osp.shud.key[i])
		{
			keyHideMask |= shudKeyShowVflag[i];
		}
	}
	while (++i < 8);
	fc->keyHideMask = keyHideMask;
}

static void CG_SHUDExecuteCommands(const char* commands)
{
	char commandBuffer[SUPERHUD_EXECUTE_MAX_LEN + 2];
	int len;

	if (!commands || !commands[0])
	{
		return;
	}

	Q_strncpyz(commandBuffer, commands, sizeof(commandBuffer));
	len = strlen(commandBuffer);

	if (len > 0 && len < (int)(sizeof(commandBuffer) - 1) && commandBuffer[len - 1] != '\n')
	{
		commandBuffer[len] = '\n';
		commandBuffer[len + 1] = 0;
	}

	trap_SendConsoleCommand(commandBuffer);
}

qboolean CG_SHUDLoadConfigPrivate(const char* filename, qboolean silent)
{
	fileHandle_t fileHandle;
	int rc;
	char* fileContent;
	int fileContentSize;
	qboolean filePrepared;
	qboolean eof = qfalse;
	configFileInfo_t finfo = {0, NULL, NULL};
	superhudElement_t* newElementsHead = NULL;
	superhudElement_t* newElementLast = NULL;
	superhudConfig_t* defaultElement = NULL;
	superhudConfigParseElement_t statusElement;
	superhudConfigParseCommand_t statusCommand;
	char superhudFilename[MAX_QPATH];
	int numberOfElements = 0;
	int numberOfElementsCreated = 0;
	qboolean hasTeamElements = qfalse;

	/* loading config file */
	Com_sprintf(superhudFilename, MAX_QPATH, "hud/%s.cfg", filename);

	rc = trap_FS_FOpenFile(superhudFilename, &fileHandle, FS_READ);
	if (rc < 0 || !fileHandle)
	{
		CG_Printf("^1SuperHUD: config file: %s is not found.\n", superhudFilename);
		return qfalse;
	}
	if (rc == 0)
	{
		CG_Printf("^1SuperHUD: config file: %s is empty.\n", superhudFilename);
		return qfalse;
	}

	fileContentSize = rc + 1;

	fileContent = Z_Malloc(fileContentSize);
	OSP_MEMORY_CHECK(fileContent);
	trap_FS_Read(fileContent, rc, fileHandle);
	trap_FS_FCloseFile(fileHandle);
	fileContent[fileContentSize - 1] = 0;

	CG_SHUDParserInit();

	filePrepared = CG_SHUDFileInfoInit(&finfo, fileContent);
	Z_Free(fileContent);
	if (!filePrepared)
	{
		CG_Printf("^1SuperHUD: config file: could not prepare %s for parsing.\n", superhudFilename);
		CG_SHUDFileInfoTeardown(&finfo);
		return qfalse;
	}

	/* Clear all variables before loading new config */
	CG_SHUDVarClearAll();

	do
	{
		statusElement = CG_SHUDFileInfoGetElementItem(&finfo);
		switch (statusElement.status)
		{
			case SUPERHUD_CONFIG_OK:
				break;
			case SUPERHUD_CONFIG_UNEXPECTED_CHARACTER:
				CG_Printf("^1SuperHUD: found unexpected character: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
				goto error_exit;
			case SUPERHUD_CONFIG_WRONG_ELEMENT_NAME:
				CG_Printf("^1Element not found ^1%s^1: file %s, line %d, pos %d.\n", statusElement.name, superhudFilename, finfo.last_line->line_number, finfo.pos);
				if (!CG_SHUDFileInfoSkipElementBody(&finfo))
				{
					goto error_exit;
				}
				continue;
			case SUPERHUD_CONFIG_LOST_ELEMENT_BODY:
				CG_Printf("^1SuperHUD: found wrong wrong element body: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
				goto error_exit;
			case SUPERHUD_CONFIG_END_OF_FILE:
				eof = qtrue;
				break;
			case SUPERHUD_CONFIG_END_OF_ELEMENT:
			case SUPERHUD_CONFIG_WRONG_COMMAND_NAME:
				CG_Printf("^1SuperHUD: found command name or end of element: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
				goto error_exit;
		}
		if (statusElement.status == SUPERHUD_CONFIG_OK)
		{
			qboolean eoe = qfalse; //end of element
			qboolean is_default = qfalse;
			// qboolean is_var = qfalse;
			qboolean is_execute = qfalse;
			superhudConfigParseStatus_t execParseStatus;
			superhudConfig_t execConfig;

			// /* Check if this is VAR element */
			// is_var = (Q_stricmp(statusElement.item->name, "var") == 0);

			// if (is_var)
			// {
			//  /* Parse VAR element */
			//  varParseStatus = CG_SHUDParseVarElement(&finfo);
			//  if (varParseStatus != SUPERHUD_CONFIG_OK)
			//  {
			//      CG_Printf("^1SuperHUD: error parsing VAR element: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
			//      goto error_exit;
			//  }
			//  /* Skip this element, don't create HUD element for VAR */
			//  continue;
			// }

			is_execute = (Q_stricmp(statusElement.item->name, "execute") == 0) || (Q_stricmp(statusElement.item->name, "exec") == 0);

			/* Check for team overlay elements (team1-team16, teamRed1-8, teamBlue1-8) */
			if (statusElement.item && statusElement.item->name &&
			        Q_stricmpn(statusElement.item->name, "team", 4) == 0)
			{
				const char* name = statusElement.item->name;
				if ((name[4] >= '1' && name[4] <= '9') ||
				        (name[4] == '1' && name[5] >= '0' && name[5] <= '6'))
				{
					hasTeamElements = qtrue;
				}
				else if (Q_stricmpn(name, "teamRed", 7) == 0 || Q_stricmpn(name, "teamBlue", 8) == 0)
				{
					hasTeamElements = qtrue;
				}
			}

			if (is_execute)
			{
				memset(&execConfig, 0, sizeof(execConfig));
				execParseStatus = CG_SHUDParseExecuteElement(&finfo, &execConfig);
				if (execParseStatus != SUPERHUD_CONFIG_OK)
				{
					CG_Printf("^1SuperHUD: error parsing EXECUTE element: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
					goto error_exit;
				}
				if (execConfig.execute.isSet && cg_exec_shud.integer)
				{
					CG_SHUDExecuteCommands(execConfig.execute.value);
				}
				continue;
			}

			// parse element body
			if (!newElementsHead)
			{
				newElementsHead = Z_Malloc(sizeof(*newElementsHead));
				OSP_MEMORY_CHECK(newElementsHead);
			}
			if (!newElementLast)
			{
				newElementLast = newElementsHead;
			}
			else
			{
				newElementLast->next = Z_Malloc(sizeof(*newElementsHead));
				OSP_MEMORY_CHECK(newElementLast->next);
				newElementLast = newElementLast->next;
			}

			is_default = strcmp(statusElement.item->name, "!default") == 0;

			// if this element is not default and we already got default, set it
			if (!is_default && defaultElement)
			{
				memcpy(&newElementLast->config, defaultElement, sizeof(newElementLast->config));
			}

			do
			{
				statusCommand = CG_SHUDFileInfoGetCommandItem(&finfo);
				switch (statusCommand.status)
				{
					case SUPERHUD_CONFIG_OK:
						break;
					case SUPERHUD_CONFIG_UNEXPECTED_CHARACTER:
						CG_Printf("^1SuperHUD: found unexpected character: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
						goto error_exit;
					case SUPERHUD_CONFIG_END_OF_FILE:
						CG_Printf("^1SuperHUD: found unexpected end of file: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
						goto error_exit;
					case SUPERHUD_CONFIG_END_OF_ELEMENT:
						eoe = qtrue;
						break;
					case SUPERHUD_CONFIG_WRONG_ELEMENT_NAME:
					case SUPERHUD_CONFIG_LOST_ELEMENT_BODY:
						CG_Printf("^1SuperHUD: found unexpected result: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
						goto error_exit;
					case SUPERHUD_CONFIG_WRONG_COMMAND_NAME:
						CG_Printf("^3SuperHUD: found unknown command name: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
						goto error_exit;
				}
				if (statusCommand.status == SUPERHUD_CONFIG_OK)
				{
					superhudConfigParseStatus_t parseStatus;
					parseStatus = statusCommand.item->parse(&finfo, &newElementLast->config);
					switch (parseStatus)
					{
						case SUPERHUD_CONFIG_OK:
							break;
						case SUPERHUD_CONFIG_UNEXPECTED_CHARACTER:
							CG_Printf("^1SuperHUD: found unexpected character: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
							goto error_exit;
						case SUPERHUD_CONFIG_END_OF_FILE:
							CG_Printf("^1SuperHUD: found unexpected end of file: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
							goto error_exit;
						case SUPERHUD_CONFIG_END_OF_ELEMENT:
							CG_Printf("^1SuperHUD: found unexpected end of element: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
							goto error_exit;
						case SUPERHUD_CONFIG_WRONG_ELEMENT_NAME:
						case SUPERHUD_CONFIG_WRONG_COMMAND_NAME:
						case SUPERHUD_CONFIG_LOST_ELEMENT_BODY:
							CG_Printf("^1SuperHUD: found unexpected result: file %s, line %d, pos %d.\n", superhudFilename, finfo.last_line->line_number, finfo.pos);
							goto error_exit;
							break;
					}
				}
			}
			while (!eoe);
			++numberOfElements;
			memcpy(&newElementLast->element, statusElement.item, sizeof(newElementLast->element));

			//set default
			if (is_default)
			{
				defaultElement = &newElementLast->config;
			}
		}
	}
	while (!eof);

	if (numberOfElements == 0)
	{
		CG_Printf("^3SuperHUD: ignored empty config file\n");
		goto error_exit;
	}
	else
	{
		/* Check for team overlay elements if cg_drawTeamOverlay is disabled */
		if (cg_drawTeamOverlay.integer == 0 && hasTeamElements)
		{
			CG_Printf("^3[SuperHUD] Teamoverlay elements disabled by ^1cg_drawTeamOverlay^3, set ^21^3 to enable\n");
		}

		/* creating HUD routines */
		newElementLast = newElementsHead;

		while (newElementLast)
		{
			if (newElementLast->element.create)
			{
				newElementLast->element.context = newElementLast->element.create(&newElementLast->config);

				CG_SHUDUpdateVisflags(&newElementLast->element, &newElementLast->config);

				if (newElementLast->element.context)
				{
					++numberOfElementsCreated;
				}
			}
			newElementLast = newElementLast->next;
		}
	}

	/* sorting HUD routines... */
	{
		superhudElement_t* prev;
		superhudElement_t* current;
		superhudElement_t* next;
		int swapped;

		do
		{
			swapped = 0;
			prev = NULL;
			current = newElementsHead;
			next = newElementsHead->next;

			while (current && next)
			{
				if (next->element.order < current->element.order)
				{
					//swap current and next
					// prev->current->next->after next
					if (prev)
					{
						prev->next = next;
					}
					else
					{
						newElementsHead = next;
					}

					current->next = next->next;
					next->next = current;
					++swapped;
				}

				prev = current;
				current = next;
				next = current->next;
			}
		}
		while (swapped);


	}

	CG_SHUDFinalizeElementChain(&newElementsHead);

	{
		superhudElement_t* countEl;

		numberOfElementsCreated = 0;
		for (countEl = newElementsHead; countEl; countEl = countEl->next)
		{
			++numberOfElementsCreated;
		}
	}

	CG_SHUDRoutenesDestroy(elementsHead);
	elementsHead = newElementsHead;
	CG_SHUDRefreshSpectatorItemSpawnsTeamTables();

	if (!silent)
	{
		CG_Printf("^3SuperHUD: loaded config file: %s: %d drawable element%s.\n", superhudFilename, numberOfElementsCreated, numberOfElementsCreated > 1 ? "s" : "");
	}

	CG_SHUDFileInfoTeardown(&finfo);
	return qtrue;

error_exit:
	CG_SHUDRoutenesDestroy(newElementsHead);
	CG_SHUDFileInfoTeardown(&finfo);
	return qfalse;
}
/*
 * Restore chat state.
 */
void CG_SHUDChatRestore(void)
{
	char cvar_name[MAX_QPATH];
	char index_str[MAX_QPATH];
	int i;
	int index;
	superhudGlobalContext_t* ctx = &superhudGlobalContext;

	trap_Cvar_VariableStringBuffer("cg_shud_chatindex", index_str, sizeof(index_str));
	ctx->chat.index = atoi(index_str);

	for (i = 0; i < SHUD_MAX_CHAT_LINES; ++i)
	{
		index = (ctx->chat.index + i) % SHUD_MAX_CHAT_LINES;
		Com_sprintf(cvar_name, MAX_QPATH, "cg_shud_chatmsg%d", index);
		trap_Cvar_VariableStringBuffer(cvar_name, ctx->chat.line[index].message, sizeof(ctx->chat.line[index].message));
		ctx->chat.line[index].senderClientNum = -1;
	}
	trap_Cvar_VariableStringBuffer("cg_shud_chatindex", index_str, sizeof(index_str));
	ctx->chat.index = atoi(index_str);
}

/*
 * Load SuperHUD config
 */
void CG_SHUDRefreshSpectatorItemSpawnsTeamTables(void)
{
	superhudElement_t* el;
	qboolean hasItemSpawnsRed;
	qboolean hasItemSpawnsBlue;

	hasItemSpawnsRed = qfalse;
	hasItemSpawnsBlue = qfalse;

	for (el = elementsHead; el; el = el->next)
	{
		if (!el->element.name)
		{
			continue;
		}
		if (Q_stricmp(el->element.name, "Spectator_ItemSpawnsRed") == 0)
		{
			hasItemSpawnsRed = qtrue;
		}
		else if (Q_stricmp(el->element.name, "Spectator_ItemSpawnsBlue") == 0)
		{
			hasItemSpawnsBlue = qtrue;
		}
	}
	CG_SpectatorItems_SetHudTeamTables(hasItemSpawnsRed, hasItemSpawnsBlue);
}

qboolean CG_SHUDHasSpectatorItemSpawnsElements(void)
{
	superhudElement_t* el;
	const char* name;

	for (el = elementsHead; el; el = el->next)
	{
		if (!el->element.name)
		{
			continue;
		}
		name = el->element.name;
		if (Q_stricmp(name, "Spectator_ItemSpawns") == 0
		        || Q_stricmp(name, "Spectator_ItemSpawnsRed") == 0
		        || Q_stricmp(name, "Spectator_ItemSpawnsBlue") == 0)
		{
			return qtrue;
		}
		/* Spectator_ItemPickups1-8 also need pui / catalog pickups bit */
		if (Q_stricmpn(name, "Spectator_ItemPickups", 21) == 0)
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_SHUDHasSpectatorsElements(void)
{
	superhudElement_t* el;

	for (el = elementsHead; el; el = el->next)
	{
		if (!el->element.name)
		{
			continue;
		}
		if (Q_stricmp(el->element.name, "spectators") == 0)
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_SHUDHasTeamRedBlueElements(void)
{
	superhudElement_t* el;
	const char* name;

	for (el = elementsHead; el; el = el->next)
	{
		if (!el->element.name)
		{
			continue;
		}
		name = el->element.name;
		if (Q_stricmpn(name, "teamRed", 7) == 0
		        || Q_stricmpn(name, "teamBlue", 8) == 0)
		{
			return qtrue;
		}
		/* team1-team16 (digit after "team"; not teamRed/Blue/Task/…) */
		if (Q_stricmpn(name, "team", 4) == 0 && name[4] >= '1' && name[4] <= '9')
		{
			return qtrue;
		}
	}
	return qfalse;
}

void CG_SHUDLoadConfig(qboolean silent)
{
	if (!CG_SHUDLoadConfigPrivate(ch_file.string, silent))
	{
		CG_Printf("^1SuperHUD: couldn't load file %s, going to load default.cfg\n", ch_file.string);
		if (!CG_SHUDLoadConfigPrivate("roamer", silent))
		{
			CG_Printf("^1SuperHUD: couldn't default config, going to disable SuperHUD\n", ch_file.string);
			trap_Cvar_Set("cg_shud", "0");
		}
	}
	else
	{
		CG_SHUDChatRestore();
		if (cgs.beSupportedServer)
		{
			CG_BEUpdateUserInfo();
		}
	}
}

/*
 * SuperHUD draw pass: fill frame context once, then each element's routine if visible.
 */
void CG_SHUDRoutine(void)
{
	superhudElement_t* last = elementsHead;
	superhudFrameContext_t* fc = &superhudGlobalContext.frame;
	qboolean be_health_low;

	CG_DrawCrosshair();
	if (CG_DrawIntermission() == 0)
	{
		if (!CG_SHUDHasCenterMessage())
		{
			CG_OSPDrawCenterString();
		}
	}

	/*
	 * beStatsShowing like scoreBoardShowing: whether the BEStats window is on screen,
	 * not merely +wstats held. Set before visflags; paint after elements (on top).
	 */
	cg.beStatsShowing = CG_BEStatsIsShowing();

	CG_SHUDFillFrameContext(fc);
	be_health_low = cg.beIsHealthLow;

	while (last)
	{
		if (CG_SHUDElementVisible(last, fc, be_health_low) && last->element.routine)
		{
			last->element.routine(last->element.context);
		}
		last = last->next;
	}
	if (cg.beStatsShowing)
	{
		CG_BEStatsShowStatsInfo();
	}
	if (CG_RoundStatsIsShowing())
	{
		CG_RoundStatsShow();
	}
	if (cg.ttwheelMarkActive)
	{
		CG_DrawSelwheelIcons();
	}
	if (cg_printui.integer && cg.printuiActiveCount != 0)
	{
		CG_DrawPrintUI();
	}
}

void CG_SHUDEventFrag(const char* message)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	Q_strncpyz(ctx->fragmessage.message, message, sizeof(ctx->fragmessage.message));
	ctx->fragmessage.time = cg.time;

	if (cgs.gametype < GT_TEAM)
	{
		ctx->rankmessage.time = cg.time;
		Com_sprintf(ctx->rankmessage.message, sizeof(ctx->rankmessage.message), "%s place with %i", CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
	}
	else
	{
		ctx->rankmessage.time = 0;
		ctx->rankmessage.message[0] = 0;
	}
}

void CG_SHUDEventKillConfirm(qboolean confirmed)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;

	Q_strncpyz(ctx->killconfirm.message,
	           confirmed ? "Kill Confirmed!" : "Kill Returned!",
	           sizeof(ctx->killconfirm.message));
	ctx->killconfirm.time = cg.time;
}

static void CG_SHUDChatLineSave(const char* message, int index)
{
	char cvar_name[MAX_QPATH];
	Com_sprintf(cvar_name, MAX_QPATH, "cg_shud_chatmsg%d", index);
	trap_Cvar_Set(cvar_name, message);
}

static void CG_SHUDChatIndexSave(int index)
{
	trap_Cvar_Set("cg_shud_chatindex", va("%d", index));
}

void CG_SHUDEventChat(const char* message, int senderClientNum)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	int index;

	index = ctx->chat.index % SHUD_MAX_CHAT_LINES;
	Q_strncpyz(ctx->chat.line[index].message, message, MAX_SAY_TEXT);
	ctx->chat.line[index].senderClientNum = senderClientNum;
	CG_SHUDChatLineSave(message, index);
	ctx->chat.line[index].time = cg.time;
	++ctx->chat.index;
	CG_SHUDChatIndexSave(ctx->chat.index);
}

void CG_SHUDEventTeamChat(const char* message, int senderClientNum)
{
	int len = 0;
	char* loc_start;
	char* loc_end;
	char* p;
	int size = 1024;
	int index;
	superhudGlobalContext_t* ctx = &superhudGlobalContext;

	if (customLocationsEnabled != 0)
	{
		char* cloc_begin, *cloc_end;
		int free_left;
		vec3_t cloc;
		if (CG_CustomLocationsTeamChatCode(message, cloc, &cloc_begin, &cloc_end))
		{
			int location_len;
			const char* location_name;
			char* tmp;

			location_name = CG_CustomLocationsGetName(cloc);
			location_len = strlen(location_name); //size of message without location

			tmp = Z_Malloc(size);
			OSP_MEMORY_CHECK(tmp);

			Q_strncpyz(tmp, cloc_end, size);
			free_left = size - (cloc_begin - message);
			Q_strncpyz(cloc_begin, location_name, free_left);
			free_left -= location_len;
			Q_strncpyz(cloc_begin + location_len, tmp, free_left);

			Z_Free(tmp);
		}
	}

	if (ch_FilterLocationsTeamchat.integer)
	{
		loc_start = (char*)strchr(message, '(');
		if (loc_start)
		{
			loc_start = strchr(loc_start + 1, '(');
			if (loc_start)
			{
				loc_end = strchr(loc_start, ')');
				if (loc_end)
				{
					strcpy(loc_start - 1, loc_end + 1);
				}
			}
		}
	}

	index = ctx->chat.index % SHUD_MAX_CHAT_LINES;

	p = ctx->chat.line[index].message;
	(*(p) = 0);
	while (*message)
	{
		if (Q_IsColorString(message))
		{
			*p++ = *message++;
			*p++ = *message++;
			continue;
		}
		*p++ = *message++;
		len++;
	}

	*p = 0;
	CG_SHUDChatLineSave(ctx->chat.line[index].message, index);

	ctx->chat.line[index].time = cg.time;
	ctx->chat.line[index].senderClientNum = senderClientNum;
	++ctx->chat.index;

	CG_SHUDChatIndexSave(ctx->chat.index);
}

void CG_SHUDEventObituaries(int attacker, int target, int mod, qboolean unfrozen)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	superhudObituariesEntry_t* entry = &ctx->obituaries.line[ctx->obituaries.index % SHUD_MAX_OBITUARIES_LINES];
	clientInfo_t* ci;
	int dropMask;
	int flagMask;

	memset(entry, 0, sizeof(*entry));

	entry->attacker = attacker;
	entry->target = target;
	entry->mod = mod;
	entry->unfrozen = unfrozen;
	entry->time = cg.time;
	entry->attackerTeam = cgs.clientinfo[attacker].team;
	entry->targetTeam = cgs.clientinfo[target].team;

	if (target >= 0 && target < MAX_CLIENTS && cgs.gametype >= GT_TEAM)
	{
		ci = &cgs.clientinfo[target];

		/*
		 * Thaw is not a death: freeze already armed obituaryDropMask via tinfo
		 * HP->0 (often after the freeze-death line already used teamPowerups).
		 * Never paint drops/flags on unfreeze; still clear leftover masks.
		 */
		if (!unfrozen)
		{
			dropMask = cg.obituaryDropMask[target];
			if (!dropMask)
			{
				dropMask = ci->teamPowerups & CG_OBITUARY_DROP_PU_MASK;
			}
			if (dropMask & (1 << PW_BATTLESUIT) && CG_HIDE_ALLY_BATTLESUIT_UI(ci))
			{
				dropMask &= ~(1 << PW_BATTLESUIT);
			}
			entry->droppedPowerups = dropMask;

			if (cgs.gametype == GT_CTF)
			{
				if (cg.obituaryDropFlagTeam[target] != TEAM_FREE)
				{
					entry->droppedFlagTeam = cg.obituaryDropFlagTeam[target];
				}
				else if (ci->infoValid &&
				         (!CG_CACHED_IS_ENEMY_CI(ci) || CG_IS_LOCAL_CLIENT_SPECTATOR()))
				{
					flagMask = ci->teamPowerups & ((1 << PW_REDFLAG) | (1 << PW_BLUEFLAG));
					if (flagMask & (1 << PW_REDFLAG))
					{
						entry->droppedFlagTeam = TEAM_RED;
					}
					else if (flagMask & (1 << PW_BLUEFLAG))
					{
						entry->droppedFlagTeam = TEAM_BLUE;
					}
				}
			}
		}

		cg.obituaryDropMask[target] = 0;
		cg.obituaryDropFlagTeam[target] = TEAM_FREE;
	}

	entry->runtime.isInitialized = qfalse;

	++ctx->obituaries.index;
}

void CG_SHUDEventTeamTask(int sender, int taskType, qboolean isPickup)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	superhudTeamTasksEntry_t* entry;
	superhudTeamTasksEntry_t keptEntries[SHUD_MAX_TEAMTASKS_LINES];
	int gameTime;
	int i;
	int checkIndex;
	int keepCount;
	int repeatCount;
	int totalCount;
	int startIndex;
	unsigned int newIndex;
	qboolean foundDuplicate;

	if (cgs.gametype < GT_TEAM || taskType == TEAMTASK_PLAYERDEAD)
	{
		return;
	}

	keepCount = 0;
	repeatCount = 1;
	foundDuplicate = qfalse;

	/* Merge duplicates still present in SuperHUD history, then move the merged line to newest. */
	for (i = SHUD_MAX_TEAMTASKS_LINES - 1; i >= 0; i--)
	{
		checkIndex = (ctx->teamTasks.index - 1 - i + SHUD_MAX_TEAMTASKS_LINES) % SHUD_MAX_TEAMTASKS_LINES;
		entry = &ctx->teamTasks.line[checkIndex];

		if (entry->time == 0)
		{
			continue;
		}

		if (entry->time != 0 && entry->sender == sender && entry->taskType == taskType && entry->isPickup == isPickup)
		{
			foundDuplicate = qtrue;
			if (entry->repeatCount > 0)
			{
				repeatCount += entry->repeatCount;
			}
			else
			{
				++repeatCount;
			}
			continue;
		}

		memcpy(&keptEntries[keepCount], entry, sizeof(keptEntries[keepCount]));
		++keepCount;
	}

	if (repeatCount > 999)
	{
		repeatCount = 999;
	}

	if (foundDuplicate)
	{
		memset(ctx->teamTasks.line, 0, sizeof(ctx->teamTasks.line));

		newIndex = ctx->teamTasks.index + 1;
		totalCount = keepCount + 1;
		startIndex = (newIndex + SHUD_MAX_TEAMTASKS_LINES - totalCount) % SHUD_MAX_TEAMTASKS_LINES;

		for (i = 0; i < keepCount; i++)
		{
			memcpy(&ctx->teamTasks.line[(startIndex + i) % SHUD_MAX_TEAMTASKS_LINES], &keptEntries[i], sizeof(keptEntries[i]));
		}

		entry = &ctx->teamTasks.line[(startIndex + keepCount) % SHUD_MAX_TEAMTASKS_LINES];
		ctx->teamTasks.index = newIndex;
	}
	else
	{
		entry = &ctx->teamTasks.line[ctx->teamTasks.index % SHUD_MAX_TEAMTASKS_LINES];
		++ctx->teamTasks.index;
	}

	memset(entry, 0, sizeof(*entry));

	entry->sender = sender;
	entry->taskType = taskType;
	entry->isPickup = isPickup;
	entry->time = cg.time;
	entry->repeatCount = repeatCount;

	/* Calculate game time when task was sent */
	gameTime = cg.time - cgs.levelStartTime;
	entry->gameTime = gameTime;
	entry->runtime.isInitialized = qfalse;

	ctx->teamTasks.hasActive = qtrue;
}

void CG_SHUDEventSpectatorItemPickup(int pickerClient, int group)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	superhudSpectatorItemPickupEntry_t* entry;

	if (pickerClient < 0 || pickerClient >= MAX_CLIENTS || group < 0)
	{
		return;
	}

	entry = &ctx->spectatorItemPickups.line[ctx->spectatorItemPickups.index % SHUD_MAX_SPECTATOR_ITEM_PICKUP_LINES];
	memset(entry, 0, sizeof(*entry));

	entry->pickerClient = pickerClient;
	entry->group = group;
	entry->time = cg.time;
	entry->gameTime = cg.time - cgs.levelStartTime;
	entry->runtime.isInitialized = qfalse;

	++ctx->spectatorItemPickups.index;
	ctx->spectatorItemPickups.hasActive = qtrue;
}

void CG_SHUDEventTempAccuracy(int weapon, float accuracy)
{
	superhudGlobalContext_t* ctx = &superhudGlobalContext;
	superhudTempAccEntry_t* entry = &ctx->tempAcc.weapon[weapon];


	memset(entry, 0, sizeof(*entry));

	entry->tempAccuracy = accuracy;

}
