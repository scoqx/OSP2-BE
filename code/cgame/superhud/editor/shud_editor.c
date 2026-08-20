#include "editor/shud_editor.h"

shudEditorState_t editor;
shudUndoEntry_t editorUndo[SHUD_EDIT_UNDO_MAX];
shudUndoEntry_t editorRedo[SHUD_EDIT_UNDO_MAX];
static qboolean shudEditPreviewLive;
static int shudEditSavedAmmo;
static qboolean shudEditSavedForceChat;
static int shudEditSavedDrawTeamOverlay;
static int shudEditSavedGametype;
static team_t shudEditSavedLocalTeam;
static team_t shudEditSavedLocalRt;
static int shudEditSavedNumSorted;
static int shudEditSavedSorted[TEAM_MAXOVERLAY];
static clientInfo_t shudEditSavedCi[8];
static int shudEditPreviewClients[8];
static int shudEditPreviewClientCount;
static qboolean shudEditTeamPreviewActive;

vec4_t shudEditPanelBg = { 0.02f, 0.02f, 0.04f, 0.82f };
vec4_t shudEditPanelBorder = { 0.35f, 0.75f, 1.0f, 0.85f };
vec4_t shudEditRowSel = { 0.15f, 0.40f, 0.70f, 0.75f };
vec4_t shudEditOutline = { 0.30f, 0.80f, 1.00f, 0.45f };
vec4_t shudEditOutlineHidden = { 0.55f, 0.55f, 0.55f, 0.30f };
vec4_t shudEditSelected = { 1.00f, 0.85f, 0.15f, 0.95f };
vec4_t shudEditHover = { 1.00f, 1.00f, 1.00f, 0.80f };
vec4_t shudEditHandle = { 1.00f, 0.85f, 0.15f, 1.00f };
vec4_t shudEditGrid = { 1.00f, 1.00f, 1.00f, 0.07f };
vec4_t shudEditCursor = { 1.00f, 0.20f, 0.20f, 0.95f };
vec4_t shudEditSliderBg = { 0.12f, 0.12f, 0.16f, 0.95f };
vec4_t shudEditSliderR = { 0.85f, 0.20f, 0.20f, 1.00f };
vec4_t shudEditSliderG = { 0.20f, 0.75f, 0.25f, 1.00f };
vec4_t shudEditSliderB = { 0.25f, 0.45f, 0.95f, 1.00f };
vec4_t shudEditSliderA = { 0.85f, 0.85f, 0.85f, 1.00f };
vec4_t shudEditMarqueeFill = { 0.20f, 0.55f, 1.00f, 0.14f };
vec4_t shudEditMarqueeLine = { 0.35f, 0.80f, 1.00f, 0.90f };
vec4_t shudEditSelMulti = { 1.00f, 0.70f, 0.20f, 0.85f };
const char* shudEditTabShort[SHUD_TAB_COUNT] =
{
	"STAT", "CHAT", "CMBT", "SCOR", "TEAM", "WEAP", "PLYR", "MISC"
};

const char* shudEditImages[SHUD_EDIT_IMG_MAX] =
{
	"white",
	"hud/lowhp",
	"icons/iconh_yellow",
	"icons/iconr_white",
	"icons/icona_red",
	"icons/icona_yellow",
	"icons/icona_green",
	"gfx/2d/grad_right",
	"gfx/2d/grad_left",
	"menu/art/3_cursor2",
	"gfx/2d/numbers/zero_32b",
	"gfx/2d/backtile"
};

const char* shudEditTexts[SHUD_EDIT_TXT_MAX] =
{
	"*",
	"+",
	"HP",
	"ARM",
	"AMMO",
	"..."
};

qboolean CG_SHUDEditorIsProtectedName(const char* name)
{
	if (!name || !name[0])
	{
		return qtrue;
	}
	return (Q_stricmp(name, "default") == 0 || Q_stricmp(name, "roamer") == 0) ? qtrue : qfalse;
}

qboolean CG_SHUDEditorDoSave(const char* name, qboolean allowDefault)
{
	if (!allowDefault && CG_SHUDEditorIsProtectedName(name))
	{
		CG_Printf("^3SuperHUD: not overwriting default.cfg/roamer.cfg. Use ^2shudsave myhud\n");
		return qfalse;
	}
	if (!CG_SHUDSaveConfig(name))
	{
		return qfalse;
	}
	editor.dirty = qfalse;
	return qtrue;
}

void CG_SHUDEditorEnsureObituaryPreview(superhudGlobalContext_t* ctx)
{
	superhudObituariesEntry_t* entry;
	int i;
	qboolean needFill;
	static const char* dummyAtk[8] =
	{
		"^1RedPlayer",
		"^4BluePlayer",
		"^2Green",
		"^3Yellow",
		"^1Alice",
		"^4Bob",
		"^2Carol",
		"^5Dave"
	};
	static const char* dummyTgt[8] =
	{
		"^4BluePlayer",
		"^1RedPlayer",
		"^3Yellow",
		"^2Green",
		"^4Bob",
		"^1Alice",
		"^5Dave",
		"^2Carol"
	};
	static const int dummyMod[8] =
	{
		MOD_RAILGUN,
		MOD_ROCKET,
		MOD_LIGHTNING,
		MOD_SHOTGUN,
		MOD_MACHINEGUN,
		MOD_PLASMA,
		MOD_GRENADE,
		MOD_GAUNTLET
	};

	needFill = qfalse;
	for (i = 0; i < SHUD_MAX_OBITUARIES_LINES; i++)
	{
		entry = &ctx->obituaries.line[i];
		if (!entry->time || !entry->runtime.attackerName[0] || !entry->runtime.targetName[0])
		{
			needFill = qtrue;
			break;
		}
	}
	if (needFill)
	{
		for (i = 0; i < SHUD_MAX_OBITUARIES_LINES; i++)
		{
			entry = &ctx->obituaries.line[i];
			memset(entry, 0, sizeof(*entry));
			entry->time = cg.time;
			entry->attacker = i;
			entry->target = (i + 1) % SHUD_MAX_OBITUARIES_LINES;
			entry->mod = dummyMod[i];
			entry->attackerTeam = TEAM_RED;
			entry->targetTeam = TEAM_BLUE;
			Q_strncpyz(entry->runtime.attackerName, dummyAtk[i], sizeof(entry->runtime.attackerName));
			Q_strncpyz(entry->runtime.targetName, dummyTgt[i], sizeof(entry->runtime.targetName));
			entry->runtime.spacing = 4.0f;
			entry->runtime.isInitialized = qtrue;
		}
		ctx->obituaries.index = SHUD_MAX_OBITUARIES_LINES;
	}
	else
	{
		for (i = 0; i < SHUD_MAX_OBITUARIES_LINES; i++)
		{
			ctx->obituaries.line[i].time = cg.time;
		}
	}
}

void CG_SHUDEditorEnsureTeamTaskPreview(superhudGlobalContext_t* ctx)
{
	superhudTeamTasksEntry_t* entry;
	int i;
	int gameTime;
	qboolean needFill;
	static const int dummyTypes[8] =
	{
		TEAMTASK_YES,
		TEAMTASK_NO,
		TEAMTASK_MEDIC,
		TEAMTASK_TARGET,
		TEAMTASK_OFFENSE,
		TEAMTASK_DEFENCE,
		TEAMTASK_WATCH,
		TEAMTASK_LOCATION
	};
	static const char* dummyEvents[8] =
	{
		"Affirmative",
		"Negative",
		"Medic",
		"Enemy here",
		"Attack",
		"Defence",
		"Watching",
		"On my way"
	};
	static const char* dummySenders[8] =
	{
		"^1RedPlayer",
		"^4BluePlayer",
		"^2Ally",
		"^3Scout",
		"^1Tank",
		"^4Sniper",
		"^2Support",
		"^5Leader"
	};

	needFill = qfalse;
	for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
	{
		entry = &ctx->teamTasks.line[i];
		if (!entry->time || entry->taskType == TEAMTASK_PLAYERDEAD || !entry->runtime.eventName[0])
		{
			needFill = qtrue;
			break;
		}
	}
	gameTime = cg.time - cgs.levelStartTime;
	if (gameTime < 0)
	{
		gameTime = 0;
	}
	if (needFill)
	{
		for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
		{
			entry = &ctx->teamTasks.line[i];
			memset(entry, 0, sizeof(*entry));
			entry->time = cg.time;
			entry->sender = cg.clientNum;
			entry->taskType = dummyTypes[i];
			entry->gameTime = gameTime + i * 1500;
			entry->isPickup = qfalse;
			Q_strncpyz(entry->runtime.senderName, dummySenders[i], sizeof(entry->runtime.senderName));
			Q_strncpyz(entry->runtime.eventName, dummyEvents[i], sizeof(entry->runtime.eventName));
			SHUD_WRITE_MATCH_TIME_STR(entry->runtime.timeString, sizeof(entry->runtime.timeString), entry->gameTime);
			entry->runtime.maxVisibleChars = 13;
			entry->runtime.spacing = 8.0f;
			entry->runtime.maxNameLenPix = 80;
			entry->runtime.isInitialized = qtrue;
		}
		ctx->teamTasks.index = SHUD_MAX_TEAMTASKS_LINES;
		ctx->teamTasks.hasActive = qtrue;
	}
	else
	{
		for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
		{
			entry = &ctx->teamTasks.line[i];
			if (entry->time)
			{
				entry->time = cg.time;
			}
		}
	}
}

void CG_SHUDEditorEnsureTeamOverlayPreview(void)
{
	clientInfo_t* ci;
	int i;
	int cn;
	int fill;
	qboolean wantSpectator;
	const char* selName;
	static const char* redNames[4] =
	{
		"^1RedAlpha",
		"^1RedBeta",
		"^1RedGamma",
		"^1RedDelta"
	};
	static const char* blueNames[4] =
	{
		"^4BlueAlpha",
		"^4BlueBeta",
		"^4BlueGamma",
		"^4BlueDelta"
	};

	if (!shudEditTeamPreviewActive)
	{
		shudEditSavedDrawTeamOverlay = cg_drawTeamOverlay.integer;
		shudEditSavedGametype = cgs.gametype;
		shudEditSavedLocalTeam = cgs.clientinfo[cg.clientNum].team;
		shudEditSavedLocalRt = cgs.clientinfo[cg.clientNum].rt;
		shudEditSavedNumSorted = numSortedTeamPlayers;
		memcpy(shudEditSavedSorted, sortedTeamPlayers, sizeof(shudEditSavedSorted));
		fill = 0;
		for (cn = 0; cn < MAX_CLIENTS && fill < 8; cn++)
		{
			if (cn == cg.clientNum)
			{
				continue;
			}
			shudEditPreviewClients[fill] = cn;
			memcpy(&shudEditSavedCi[fill], &cgs.clientinfo[cn], sizeof(clientInfo_t));
			fill++;
		}
		shudEditPreviewClientCount = fill;
		shudEditTeamPreviewActive = qtrue;
	}
	else
	{
		fill = shudEditPreviewClientCount;
	}

	if (cg_drawTeamOverlay.integer == 0)
	{
		cg_drawTeamOverlay.integer = 1;
	}
	if (cgs.gametype < GT_TEAM)
	{
		cgs.gametype = GT_TEAM;
	}

	wantSpectator = qfalse;
	if (editor.selected && editor.selected->element.name)
	{
		selName = editor.selected->element.name;
		if (Q_stricmpn(selName, "teamRed", 7) == 0 || Q_stricmpn(selName, "teamBlue", 8) == 0)
		{
			wantSpectator = qtrue;
		}
	}

	if (wantSpectator)
	{
		cgs.clientinfo[cg.clientNum].team = TEAM_SPECTATOR;
	}
	else
	{
		cgs.clientinfo[cg.clientNum].team = TEAM_RED;
		cgs.clientinfo[cg.clientNum].rt = TEAM_RED;
	}

	numSortedTeamPlayers = fill;
	for (i = 0; i < fill; i++)
	{
		cn = shudEditPreviewClients[i];
		sortedTeamPlayers[i] = cn;
		ci = &cgs.clientinfo[cn];
		ci->infoValid = qtrue;
		if (i < 4)
		{
			ci->team = TEAM_RED;
			ci->rt = TEAM_RED;
			Q_strncpyz(ci->name, redNames[i], sizeof(ci->name));
			Q_strncpyz(ci->name_clean, redNames[i], sizeof(ci->name_clean));
			Vector4Copy(cgs.beRedTeamColor, cg.healthColorByClient[cn]);
		}
		else
		{
			ci->team = TEAM_BLUE;
			ci->rt = TEAM_BLUE;
			Q_strncpyz(ci->name, blueNames[i - 4], sizeof(ci->name));
			Q_strncpyz(ci->name_clean, blueNames[i - 4], sizeof(ci->name_clean));
			Vector4Copy(cgs.beBlueTeamColor, cg.healthColorByClient[cn]);
		}
		ci->health = 100 - (i * 8);
		if (ci->health < 1)
		{
			ci->health = 1;
		}
		ci->armor = 50 + i * 5;
		if (ci->armor > 100)
		{
			ci->armor = 100;
		}
	}
}

void CG_SHUDEditorRestoreTeamOverlayPreview(void)
{
	int i;

	if (!shudEditTeamPreviewActive)
	{
		return;
	}
	cg_drawTeamOverlay.integer = shudEditSavedDrawTeamOverlay;
	cgs.gametype = shudEditSavedGametype;
	cgs.clientinfo[cg.clientNum].team = shudEditSavedLocalTeam;
	cgs.clientinfo[cg.clientNum].rt = shudEditSavedLocalRt;
	numSortedTeamPlayers = shudEditSavedNumSorted;
	memcpy(sortedTeamPlayers, shudEditSavedSorted, sizeof(sortedTeamPlayers));
	for (i = 0; i < shudEditPreviewClientCount; i++)
	{
		memcpy(&cgs.clientinfo[shudEditPreviewClients[i]], &shudEditSavedCi[i], sizeof(clientInfo_t));
	}
	shudEditTeamPreviewActive = qfalse;
}

void CG_SHUDEditorCatchKeys(qboolean enable)
{
	int catcher;

	catcher = trap_Key_GetCatcher();
	if (enable)
	{
		trap_Key_SetCatcher(catcher | KEYCATCH_CGAME);
	}
	else
	{
		trap_Key_SetCatcher(catcher & ~KEYCATCH_CGAME);
	}
}

void CG_SHUDEditorPreviewStop(void)
{
	if (!shudEditPreviewLive)
	{
		return;
	}
	shudEditPreviewLive = qfalse;
	cg.lowAmmoWarning = shudEditSavedAmmo;
	cgs.osp.shud.forceChat = shudEditSavedForceChat;
	CG_SHUDEditorRestoreTeamOverlayPreview();
}

void CG_SHUDEditorPreviewStart(void)
{
	if (shudEditPreviewLive)
	{
		return;
	}
	CG_SHUDEditorRestoreTeamOverlayPreview();
	shudEditPreviewLive = qtrue;
	shudEditSavedAmmo = cg.lowAmmoWarning;
	shudEditSavedForceChat = cgs.osp.shud.forceChat;
	cgs.osp.shud.forceChat = qtrue;
}

void CG_SHUDEditorPumpPreview(void)
{
	superhudGlobalContext_t* ctx;
	int i;
	qboolean anyChat;
	static const char* dummyChat[8] =
	{
		"^2Player^7: nice shot",
		"^1Red^7: gg",
		"^4Blue^7: wait",
		"^3Spec^7: 1v1?",
		"^2Player^7: rail",
		"^5Bot^7: defending",
		"^1Red^7: mid",
		"^4Blue^7: incoming"
	};

	if (!editor.active)
	{
		return;
	}
	ctx = CG_SHUDGetContext();
	if (!ctx)
	{
		return;
	}

	anyChat = qfalse;
	for (i = 0; i < SHUD_MAX_CHAT_LINES; i++)
	{
		if (ctx->chat.line[i].message[0])
		{
			ctx->chat.line[i].time = cg.time;
			anyChat = qtrue;
		}
	}
	if (!anyChat)
	{
		for (i = 0; i < 8; i++)
		{
			CG_SHUDEventChat(dummyChat[i], -1);
		}
	}

	CG_SHUDEditorEnsureObituaryPreview(ctx);
	CG_SHUDEditorEnsureTeamTaskPreview(ctx);
	CG_SHUDEditorEnsureTeamOverlayPreview(ctx);

	if (!ctx->fragmessage.message[0])
	{
		Q_strncpyz(ctx->fragmessage.message, "You fragged Player", sizeof(ctx->fragmessage.message));
	}
	ctx->fragmessage.time = cg.time;
	if (!ctx->rankmessage.message[0])
	{
		Q_strncpyz(ctx->rankmessage.message, "1st place with 0", sizeof(ctx->rankmessage.message));
	}
	ctx->rankmessage.time = cg.time;

	if (cg.itemPickup <= 0 || cg.itemPickup >= bg_numItems)
	{
		cg.itemPickup = 1;
	}
	CG_RegisterItemVisuals(cg.itemPickup);
	cg.itemPickupTime = cg.time;
	cg.attackerTime = cg.time;
	cg.crosshairClientTime = cg.time;
	if (!cg.rewardShader[0])
	{
		cg.rewardShader[0] = cgs.media.medalExcellent;
		cg.rewardCount[0] = 3;
		cg.rewardStack = 0;
	}
	cg.rewardTime = cg.time;
	cg.lowAmmoWarning = 1;
	CG_SHUDEventTempAccuracy(WP_LIGHTNING, 45.5f);
}

void CG_SHUDEditorActivate(qboolean enable)
{
	if (enable)
	{
		if (!cg_shud.integer)
		{
			CG_Printf("^1SuperHUD editor: enable cg_shud first.\n");
			return;
		}
		if (!CG_SHUDGetElementsHead())
		{
			CG_Printf("^1SuperHUD editor: no HUD loaded.\n");
			return;
		}
		memset(&editor, 0, sizeof(editor));
		CG_SHUDEditorClearUndo();
		editor.active = qtrue;
		editor.showList = qtrue;
		editor.showProps = qtrue;
		editor.typeChannel = -1;
		editor.cursorX = SCREEN_WIDTH * 0.5f;
		editor.cursorY = SCREEN_HEIGHT * 0.5f;
		editor.gridSize = 16;
		editor.showGuides = qtrue;
		editor.coalesceProp = -1;
		editor.selectedIndex = -1;
		CG_SHUDAssignTabs(CG_SHUDGetElementsHead());
		editor.tab = 0;
		editor.showAll = qtrue;
		CG_SHUDEditorEnsureTabDefault(0);
		CG_SHUDEditorSelectIndex(1);
		CG_SHUDEditorCatchKeys(qtrue);
		CG_SHUDEditorPreviewStart();
		CG_Printf("^3SuperHUD editor: on  ^7F1 help, ESC exit, F5 save, N add, / filter\n");
	}
	else if (editor.active)
	{
		CG_SHUDEditorCatchKeys(qfalse);
		CG_SHUDEditorPreviewStop();
		if (editor.dirty)
		{
			CG_Printf("^3SuperHUD editor: off. Unsaved changes - ^2shudsave myhud^3 (F5 will not overwrite default.cfg/roamer.cfg)\n");
		}
		else
		{
			CG_Printf("^3SuperHUD editor: off.\n");
		}
		memset(&editor, 0, sizeof(editor));
		CG_SHUDEditorClearUndo();
	}
}

qboolean CG_SHUDEditorIsActive(void)
{
	return editor.active;
}

qboolean CG_SHUDEditorShowAll(void)
{
	return (editor.active && editor.showAll) ? qtrue : qfalse;
}

void CG_SHUDEditorToggle_f(void)
{
	if (editor.active)
	{
		CG_SHUDEditorActivate(qfalse);
	}
	else
	{
		CG_SHUDEditorActivate(qtrue);
	}
}

void CG_SHUDEditorSave_f(void)
{
	char name[MAX_QPATH];

	if (trap_Argc() >= 2)
	{
		Q_strncpyz(name, CG_Argv(1), sizeof(name));
		if (CG_SHUDEditorDoSave(name, qtrue))
		{
			if (Q_stricmp(ch_file.string, name))
			{
				trap_Cvar_Set("ch_file", name);
			}
		}
	}
	else
	{
		CG_SHUDEditorDoSave(ch_file.string, qfalse);
	}
}

void CG_SHUDEditorShutdown(void)
{
	if (editor.active)
	{
		CG_SHUDEditorCatchKeys(qfalse);
		CG_SHUDEditorPreviewStop();
	}
	memset(&editor, 0, sizeof(editor));
	CG_SHUDEditorClearUndo();
}

void CG_SHUDEditorOnConfigReloaded(void)
{
	if (!editor.active)
	{
		return;
	}
	editor.drag = SHUD_DRAG_NONE;
	editor.dirty = qfalse;
	editor.picker = qfalse;
	CG_SHUDEditorClearUndo();
	if (editor.selectedName[0])
	{
		CG_SHUDEditorSelect(CG_SHUDEditorFindByName(editor.selectedName));
	}
	else
	{
		CG_SHUDEditorSelectIndex(0);
	}
}

