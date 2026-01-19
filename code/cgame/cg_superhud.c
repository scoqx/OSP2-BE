#include "cg_local.h"
#include "../qcommon/qcommon.h"
#include "cg_superhud_private.h"

static superhudElement_t* elementsHead;

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
		element->visibility = config->visflags.value;
	}
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

qboolean CG_SHUDLoadConfigPrivate(const char* filename)
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

			// Check for team overlay elements (team1-team16)
			if (statusElement.item && statusElement.item->name &&
			        Q_stricmpn(statusElement.item->name, "team", 4) == 0)
			{
				const char* name = statusElement.item->name;
				// Check if it's team1-team16 (single or double digit)
				if ((name[4] >= '1' && name[4] <= '9') ||
				        (name[4] == '1' && name[5] >= '0' && name[5] <= '6'))
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



	CG_SHUDRoutenesDestroy(elementsHead);
	elementsHead = newElementsHead;

	CG_Printf("^3SuperHUD: loaded config file: %s: %d drawable element%s.\n", superhudFilename, numberOfElementsCreated, numberOfElementsCreated > 1 ? "s" : "");

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
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();

	trap_Cvar_VariableStringBuffer("cg_shud_chatindex", index_str, sizeof(index_str));
	ctx->chat.index = atoi(index_str);

	for (i = 0; i < SHUD_MAX_CHAT_LINES; ++i)
	{
		index = (ctx->chat.index + i) % SHUD_MAX_CHAT_LINES;
		Com_sprintf(cvar_name, MAX_QPATH, "cg_shud_chatmsg%d", index);
		trap_Cvar_VariableStringBuffer(cvar_name, ctx->chat.line[index].message, sizeof(ctx->chat.line[index].message));
	}
	trap_Cvar_VariableStringBuffer("cg_shud_chatindex", index_str, sizeof(index_str));
	ctx->chat.index = atoi(index_str);
}

/*
 * Load SuperHUD config
 */
void CG_SHUDLoadConfig(void)
{
	if (!CG_SHUDLoadConfigPrivate(ch_file.string))
	{
		CG_Printf("^1SuperHUD: couldn't load file %s, going to load default.cfg\n", ch_file.string);
		if (!CG_SHUDLoadConfigPrivate("default"))
		{
			CG_Printf("^1SuperHUD: couldn't default config, going to disable SuperHUD\n", ch_file.string);
			trap_Cvar_Set("cg_shud", "0");
		}
	}
	else
	{
		CG_SHUDChatRestore();
	}
}

/*
================
CG_SHUDCheckWeaponFlags

Check if element should be skipped based on weapon flags.
Returns qtrue if element should be skipped (weapon flags are set but current weapon doesn't match).
================
*/
static qboolean CG_SHUDCheckWeaponFlags(int vflags, int currentWeapon)
{
	int weaponFlags;
	qboolean hasWeaponFlags;
	qboolean weaponMatches;

	weaponFlags = (vflags & (SE_WEAPON_GAUNTLET | SE_WEAPON_MACHINEGUN | SE_WEAPON_SHOTGUN |
	                         SE_WEAPON_GRENADE_LAUNCHER | SE_WEAPON_ROCKET_LAUNCHER |
	                         SE_WEAPON_LIGHTNING | SE_WEAPON_RAILGUN | SE_WEAPON_PLASMAGUN | SE_WEAPON_BFG));
	hasWeaponFlags = (weaponFlags != 0);
	weaponMatches = qfalse;

	if (hasWeaponFlags)
	{
		if ((vflags & SE_WEAPON_GAUNTLET) && currentWeapon == WP_GAUNTLET)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_MACHINEGUN) && currentWeapon == WP_MACHINEGUN)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_SHOTGUN) && currentWeapon == WP_SHOTGUN)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_GRENADE_LAUNCHER) && currentWeapon == WP_GRENADE_LAUNCHER)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_ROCKET_LAUNCHER) && currentWeapon == WP_ROCKET_LAUNCHER)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_LIGHTNING) && currentWeapon == WP_LIGHTNING)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_RAILGUN) && currentWeapon == WP_RAILGUN)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_PLASMAGUN) && currentWeapon == WP_PLASMAGUN)
		{
			weaponMatches = qtrue;
		}
		else if ((vflags & SE_WEAPON_BFG) && currentWeapon == WP_BFG)
		{
			weaponMatches = qtrue;
		}
	}

	return (hasWeaponFlags && !weaponMatches);
}

void CG_SHUDRoutine(void)
{
	superhudElement_t* last = elementsHead;
	int vflags;
	qboolean skip;
	const qboolean is_dead = cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_FREEZE;
	const qboolean is_intermission = cg.predictedPlayerState.pm_type == PM_INTERMISSION;
	const qboolean is_team_game = cgs.gametype >= GT_TEAM;
	const qboolean is_spectator = CG_IsSpectatorOnScreen();
	const qboolean is_scores = cg.showScores || cgs.osp.chud.scoreboard ? qtrue : qfalse;
	const qboolean is_gt_ffa = cgs.gametype == GT_FFA;
	const qboolean is_gt_tourney = cgs.gametype == GT_TOURNAMENT;
	const qboolean is_gt_tdm = CG_OSPIsGameTypeTDM();
	const qboolean is_gt_ctf = cgs.gametype == GT_CTF;
	const qboolean is_gt_ft = CG_OSPIsGameTypeFreeze();
	const qboolean is_gt_ca = CG_OSPIsGameTypeCA(cgs.gametype);
	int currentWeapon;

	CG_DrawCrosshair();
	if (CG_DrawIntermission() == 0)
	{
		CG_OSPDrawCenterString();
	}
	while (last)
	{
		// check visibility
		vflags = last->config.visiblity.isSet ? last->config.visiblity.value : last->element.visibility;

		// Get current weapon (same logic as CG_ExecWeaponCommand)
		currentWeapon = cg.snap ? cg.snap->ps.weapon : WP_NONE;

		skip = (!(vflags & SE_IM) && is_intermission) ||
		       ((vflags & SE_TEAM_ONLY) && (!is_team_game)) ||
		       (!(vflags & SE_DEAD) && is_dead) ||
		       (!(vflags & SE_SPECT) && is_spectator) ||
		       ((vflags & SE_SCORES_HIDE) && is_scores) ||
		       ((vflags & SE_DEMO_HIDE) && cg.demoPlayback) ||
		       ((vflags & SE_KEY1_SHOW) && !cgs.osp.shud.key[0]) ||
		       ((vflags & SE_KEY2_SHOW) && !cgs.osp.shud.key[1]) ||
		       ((vflags & SE_KEY3_SHOW) && !cgs.osp.shud.key[2]) ||
		       ((vflags & SE_KEY4_SHOW) && !cgs.osp.shud.key[3]) ||
		       ((vflags & SE_KEY5_SHOW) && !cgs.osp.shud.key[4]) ||
		       ((vflags & SE_KEY6_SHOW) && !cgs.osp.shud.key[5]) ||
		       ((vflags & SE_KEY7_SHOW) && !cgs.osp.shud.key[6]) ||
		       ((vflags & SE_KEY8_SHOW) && !cgs.osp.shud.key[7]) ||
		       ((vflags & SE_GT_FFA) && is_gt_ffa) ||
		       ((vflags & SE_GT_TDM) && is_gt_tdm) ||
		       ((vflags & SE_GT_CTF) && is_gt_ctf) ||
		       ((vflags & SE_GT_TOURNEY) && is_gt_tourney) ||
		       ((vflags & SE_GT_FREEZETAG) && is_gt_ft) ||
		       ((vflags & SE_GT_CLANARENA) && is_gt_ca) ||
		       ((vflags & SE_HEALTH_LOW) && !cgs.be.isHealthLow) ||
		       CG_SHUDCheckWeaponFlags(vflags, currentWeapon)
		       ;

		if (!skip && last->element.routine)
		{
			last->element.routine(last->element.context);
		}
		last = last->next;
	}
	if (cgs.be.newStats.drawWindow)
	{
		CG_BEStatsShowStatsInfo();
	}
	if (cgs.be.teamMarkers.ttwheelMarkActive)
	{
		CG_DrawSelwheelIcons();
	}

	if (cg_printui.integer)
	{
		CG_DrawPrintUI();
	}

}

void CG_SHUDEventFrag(const char* message)
{
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();
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

void CG_SHUDEventChat(const char* message)
{
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();
	int index;

	index = ctx->chat.index % SHUD_MAX_CHAT_LINES;
	Q_strncpyz(ctx->chat.line[index].message, message, MAX_SAY_TEXT);
	CG_SHUDChatLineSave(message, index);
	ctx->chat.line[index].time = cg.time;
	++ctx->chat.index;
	CG_SHUDChatIndexSave(ctx->chat.index);
}

void CG_SHUDEventTeamChat(const char* message)
{
	int len = 0;
	char* loc_start;
	char* loc_end;
	char* p;
	int size = 1024;
	int index;
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();

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
		loc_start = strchr(message, '(');
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
	++ctx->chat.index;

	CG_SHUDChatIndexSave(ctx->chat.index);
}

void CG_SHUDEventObituaries(int attacker, int target, int mod, qboolean unfrozen)
{
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();
	superhudObituariesEntry_t* entry = &ctx->obituaries.line[ctx->obituaries.index % SHUD_MAX_OBITUARIES_LINES];

	memset(entry, 0, sizeof(*entry));

	entry->attacker = attacker;
	entry->target = target;
	entry->mod = mod;
	entry->unfrozen = unfrozen;
	entry->time = cg.time;
	entry->attackerTeam = cgs.clientinfo[attacker].team;
	entry->targetTeam = cgs.clientinfo[target].team;
	entry->runtime.isInitialized = qfalse;

	++ctx->obituaries.index;
}

void CG_SHUDEventTeamTask(int sender, int taskType)
{
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();
	superhudTeamTasksEntry_t* entry;
	int gameTime;
	int i;
	int checkIndex;
	const int duplicateCheckTime = 100;

	/* Check for duplicate events - same sender and taskType within duplicateCheckTime */
	if (sender >= 0 && sender < MAX_CLIENTS)
	{
		for (i = 0; i < SHUD_MAX_TEAMTASKS_LINES; i++)
		{
			checkIndex = (ctx->teamTasks.index - 1 - i + SHUD_MAX_TEAMTASKS_LINES) % SHUD_MAX_TEAMTASKS_LINES;
			entry = &ctx->teamTasks.line[checkIndex];

			/* Skip empty entries */
			if (entry->time == 0)
			{
				continue;
			}

			/* Check if this is a duplicate (same sender and taskType within time window) */
			if (entry->sender == sender && entry->taskType == taskType)
			{
				int timeDiff;
				timeDiff = cg.time - entry->time;
				if (timeDiff >= 0 && timeDiff < duplicateCheckTime)
				{
					/* Duplicate event within time window - skip adding */
					return;
				}
			}
		}
	}

	entry = &ctx->teamTasks.line[ctx->teamTasks.index % SHUD_MAX_TEAMTASKS_LINES];
	memset(entry, 0, sizeof(*entry));

	entry->sender = sender;
	entry->taskType = taskType;
	entry->time = cg.time;

	/* Calculate game time when task was sent */
	gameTime = cg.time - cgs.levelStartTime;
	entry->gameTime = gameTime;
	entry->runtime.isInitialized = qfalse;

	++ctx->teamTasks.index;
}

void CG_SHUDEventTempAccuracy(int weapon, float accuracy)
{
	superhudGlobalContext_t* ctx = CG_SHUDGetContext();
	superhudTempAccEntry_t* entry = &ctx->tempAcc.weapon[weapon];


	memset(entry, 0, sizeof(*entry));

	entry->tempAccuracy = accuracy;

}

