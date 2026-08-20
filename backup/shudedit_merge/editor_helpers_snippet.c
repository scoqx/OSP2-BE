}

superhudElement_t* CG_SHUDGetElementsHead(void)
{
	return elementsHead;
}

void CG_SHUDRecreateElement(superhudElement_t* el)
{
	if (!el)
	{
		return;
	}
	if (el->element.destroy && el->element.context)
	{
		el->element.destroy(el->element.context);
		el->element.context = NULL;
	}
	if (el->element.create)
	{
		el->element.context = el->element.create(&el->config);
		CG_SHUDUpdateVisflags(&el->element, &el->config);
		CG_SHUDElementVisPrepare(el);
	}
}

superhudElement_t* CG_SHUDAllocElement(const superHUDConfigElement_t* def, const superhudConfig_t* cfg)
{
	superhudElement_t* el;

	if (!def)
	{
		return NULL;
	}
	el = Z_Malloc(sizeof(*el));
	if (!el)
	{
		return NULL;
	}
	memset(el, 0, sizeof(*el));
	memcpy(&el->element, def, sizeof(el->element));
	el->element.context = NULL;
	if (cfg)
	{
		memcpy(&el->config, cfg, sizeof(el->config));
	}
	CG_SHUDRecreateElement(el);
	return el;
}

void CG_SHUDLinkElementAfter(superhudElement_t* after, superhudElement_t* el)
{
	if (!el)
	{
		return;
	}
	if (!after)
	{
		el->next = elementsHead;
		elementsHead = el;
		return;
	}
	el->next = after->next;
	after->next = el;
}

void CG_SHUDUnlinkElement(superhudElement_t* el)
{
	superhudElement_t* p;

	if (!el)
	{
		return;
	}
	if (elementsHead == el)
	{
		elementsHead = el->next;
		el->next = NULL;
		return;
	}
	p = elementsHead;
	while (p && p->next != el)
	{
		p = p->next;
	}
	if (p)
	{
		p->next = el->next;
		el->next = NULL;
	}
}

void CG_SHUDDestroyElementNode(superhudElement_t* el)
{
	if (!el)
	{
		return;
	}
	if (el->element.destroy && el->element.context)
	{
		el->element.destroy(el->element.context);
		el->element.context = NULL;
	}
	Z_Free(el);
}

int CG_SHUDElementChainIndex(const superhudElement_t* el)
{
	superhudElement_t* p;
	int i;

	p = elementsHead;
	i = 0;
	while (p)
	{
		if (p == el)
		{
			return i;
		}
		p = p->next;
		i++;
	}
	return -1;
}

superhudElement_t* CG_SHUDElementChainAt(int index)
{
	superhudElement_t* p;
	int i;

	if (index < 0)
	{
		return NULL;
	}
	p = elementsHead;
	i = 0;
	while (p && i < index)
	{
		p = p->next;
		i++;
	}
	return p;
}

void CG_SHUDInsertAtChainIndex(superhudElement_t* el, int index)
{
	superhudElement_t* prev;

	if (!el)
	{
		return;
	}
	if (index <= 0 || !elementsHead)
	{
		CG_SHUDLinkElementAfter(NULL, el);
		return;
	}
	prev = CG_SHUDElementChainAt(index - 1);
	if (!prev)
	{
		prev = elementsHead;
		while (prev->next)
		{
			prev = prev->next;
		}
	}
	CG_SHUDLinkElementAfter(prev, el);
}

qboolean CG_SHUDIsDefaultName(const char* name)
{
	return (name && !Q_stricmp(name, "!default")) ? qtrue : qfalse;
}

qboolean CG_SHUDIsDecorName(const char* name)
{
	if (!name)
	{
		return qfalse;
	}
	if (!Q_stricmp(name, "predecorate") || !Q_stricmp(name, "postdecorate"))
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_SHUDNameContains(const char* name, const char* sub)
{
	int i;
	int j;
	char a;
	char b;

	if (!name || !sub || !sub[0])
	{
		return qfalse;
	}
	for (i = 0; name[i]; i++)
	{
		for (j = 0; sub[j]; j++)
		{
			if (!name[i + j])
			{
				break;
			}
			a = name[i + j];
			b = sub[j];
			if (a >= 'A' && a <= 'Z')
			{
				a += 32;
			}
			if (b >= 'A' && b <= 'Z')
			{
				b += 32;
			}
			if (a != b)
			{
				break;
			}
		}
		if (!sub[j])
		{
			return qtrue;
		}
	}
	return qfalse;
}

int CG_SHUDNameTab(const char* name)
{
	if (!name || !name[0] || CG_SHUDIsDefaultName(name) || CG_SHUDIsDecorName(name))
	{
		return -1;
	}
	if (CG_SHUDNameContains(name, "statusbar") || CG_SHUDNameContains(name, "ammomessage"))
	{
		return SHUD_TAB_STATUS;
	}
	if (CG_SHUDNameContains(name, "chat"))
	{
		return SHUD_TAB_CHAT;
	}
	if (CG_SHUDNameContains(name, "obituar") ||
	    CG_SHUDNameContains(name, "attacker") ||
	    CG_SHUDNameContains(name, "fragmessage") ||
	    CG_SHUDNameContains(name, "rankmessage") ||
	    CG_SHUDNameContains(name, "target") ||
	    CG_SHUDNameContains(name, "itempickup") ||
	    CG_SHUDNameContains(name, "reward") ||
	    CG_SHUDNameContains(name, "killconfirm") ||
	    CG_SHUDNameContains(name, "centermessage"))
	{
		return SHUD_TAB_COMBAT;
	}
	if (CG_SHUDNameContains(name, "score_") ||
	    CG_SHUDNameContains(name, "name_nme") ||
	    CG_SHUDNameContains(name, "name_own") ||
	    CG_SHUDNameContains(name, "teamcount") ||
	    CG_SHUDNameContains(name, "flagstatus") ||
	    CG_SHUDNameContains(name, "gametime") ||
	    CG_SHUDNameContains(name, "gametype") ||
	    CG_SHUDNameContains(name, "warmup") ||
	    CG_SHUDNameContains(name, "timelimit") ||
	    CG_SHUDNameContains(name, "thaw") ||
	    CG_SHUDNameContains(name, "laststanding"))
	{
		return SHUD_TAB_SCORE;
	}
	if (CG_SHUDNameContains(name, "team"))
	{
		return SHUD_TAB_TEAM;
	}
	if (CG_SHUDNameContains(name, "weapon") ||
	    CG_SHUDNameContains(name, "powerup") ||
	    CG_SHUDNameContains(name, "currentWeapon"))
	{
		return SHUD_TAB_WEAPONS;
	}
	if (CG_SHUDNameContains(name, "fps") ||
	    CG_SHUDNameContains(name, "playerspeed") ||
	    CG_SHUDNameContains(name, "netgraph") ||
	    CG_SHUDNameContains(name, "localtime") ||
	    CG_SHUDNameContains(name, "localdate") ||
	    CG_SHUDNameContains(name, "player") ||
	    CG_SHUDNameContains(name, "tempAcc") ||
	    CG_SHUDNameContains(name, "location") ||
	    CG_SHUDNameContains(name, "spectator") ||
	    CG_SHUDNameContains(name, "specmessage") ||
	    CG_SHUDNameContains(name, "followmessage") ||
	    CG_SHUDNameContains(name, "respawn"))
	{
		return SHUD_TAB_PLAYER;
	}
	return SHUD_TAB_MISC;
}

int CG_SHUDNameGroup(const char* name)
{
	if (!name || !name[0] || CG_SHUDIsDefaultName(name) || CG_SHUDIsDecorName(name))
	{
		return -1;
	}
	if (CG_SHUDNameContains(name, "statusbar_health"))
	{
		return SHUD_GRP_HEALTH;
	}
	if (CG_SHUDNameContains(name, "statusbar_armor"))
	{
		return SHUD_GRP_ARMOR;
	}
	if (CG_SHUDNameContains(name, "statusbar_ammo") || CG_SHUDNameContains(name, "ammomessage"))
	{
		return SHUD_GRP_AMMO;
	}
	if (CG_SHUDNameContains(name, "chat"))
	{
		return SHUD_GRP_CHAT;
	}
	if (CG_SHUDNameContains(name, "obituar"))
	{
		return SHUD_GRP_OBITUARY;
	}
	if (CG_SHUDNameContains(name, "attacker"))
	{
		return SHUD_GRP_ATTACKER;
	}
	if (CG_SHUDNameContains(name, "fragmessage") ||
	    CG_SHUDNameContains(name, "rankmessage") ||
	    CG_SHUDNameContains(name, "killconfirm"))
	{
		return SHUD_GRP_FRAG;
	}
	if (CG_SHUDNameContains(name, "target"))
	{
		return SHUD_GRP_CROSSHAIR;
	}
	if (CG_SHUDNameContains(name, "itempickup"))
	{
		return SHUD_GRP_PICKUP;
	}
	if (CG_SHUDNameContains(name, "reward"))
	{
		return SHUD_GRP_MEDALS;
	}
	if (CG_SHUDNameContains(name, "score_") || CG_SHUDNameContains(name, "laststanding"))
	{
		return SHUD_GRP_SCORE;
	}
	if (CG_SHUDNameContains(name, "name_nme") || CG_SHUDNameContains(name, "name_own"))
	{
		return SHUD_GRP_NAMES;
	}
	if (CG_SHUDNameContains(name, "flagstatus"))
	{
		return SHUD_GRP_FLAGS;
	}
	if (CG_SHUDNameContains(name, "teamcount"))
	{
		return SHUD_GRP_TEAMCOUNT;
	}
	if (CG_SHUDNameContains(name, "gametime") || CG_SHUDNameContains(name, "timelimit"))
	{
		return SHUD_GRP_CLOCK;
	}
	if (CG_SHUDNameContains(name, "gametype") ||
	    CG_SHUDNameContains(name, "warmup") ||
	    CG_SHUDNameContains(name, "thaw"))
	{
		return SHUD_GRP_WARMUP;
	}
	if (CG_SHUDNameContains(name, "weaponStats") || CG_SHUDNameContains(name, "currentWeaponStats"))
	{
		return SHUD_GRP_WSTATS;
	}
	if (CG_SHUDNameContains(name, "weaponlist") || CG_SHUDNameContains(name, "weaponselection"))
	{
		return SHUD_GRP_WEAPONLIST;
	}
	if (CG_SHUDNameContains(name, "powerup"))
	{
		return SHUD_GRP_POWERUP;
	}
	if (CG_SHUDNameContains(name, "team"))
	{
		return SHUD_GRP_TEAM;
	}
	if (CG_SHUDNameContains(name, "fps") ||
	    CG_SHUDNameContains(name, "netgraph") ||
	    CG_SHUDNameContains(name, "localtime") ||
	    CG_SHUDNameContains(name, "localdate"))
	{
		return SHUD_GRP_PERF;
	}
	if (CG_SHUDNameContains(name, "playerspeed"))
	{
		return SHUD_GRP_SPEED;
	}
	if (CG_SHUDNameContains(name, "tempAcc"))
	{
		return SHUD_GRP_TEMPACC;
	}
	if (CG_SHUDNameContains(name, "location"))
	{
		return SHUD_GRP_LOCATION;
	}
	if (CG_SHUDNameContains(name, "spectator") ||
	    CG_SHUDNameContains(name, "specmessage") ||
	    CG_SHUDNameContains(name, "followmessage") ||
	    CG_SHUDNameContains(name, "respawn"))
	{
		return SHUD_GRP_SPECT;
	}
	if (CG_SHUDNameContains(name, "player"))
	{
		return SHUD_GRP_PLAYER;
	}
	if (!Q_stricmp(name, "grid"))
	{
		return SHUD_GRP_GRID;
	}
	if (CG_SHUDNameContains(name, "vote"))
	{
		return SHUD_GRP_VOTE;
	}
	return SHUD_GRP_MISC;
}

const char* CG_SHUDGroupTitle(int grp)
{
	switch (grp)
	{
		case SHUD_GRP_HEALTH: return "HEALTH";
		case SHUD_GRP_ARMOR: return "ARMOR";
		case SHUD_GRP_AMMO: return "AMMO";
		case SHUD_GRP_CHAT: return "CHAT";
		case SHUD_GRP_OBITUARY: return "OBITUARIES";
		case SHUD_GRP_ATTACKER: return "ATTACKER";
		case SHUD_GRP_FRAG: return "FRAG";
		case SHUD_GRP_CROSSHAIR: return "CROSSHAIR";
		case SHUD_GRP_PICKUP: return "PICKUP";
		case SHUD_GRP_MEDALS: return "MEDALS";
		case SHUD_GRP_SCORE: return "SCORE";
		case SHUD_GRP_NAMES: return "NAMES";
		case SHUD_GRP_FLAGS: return "FLAGS";
		case SHUD_GRP_TEAMCOUNT: return "TEAMCOUNT";
		case SHUD_GRP_CLOCK: return "CLOCK";
		case SHUD_GRP_WARMUP: return "WARMUP";
		case SHUD_GRP_TEAM: return "TEAM";
		case SHUD_GRP_WEAPONLIST: return "WEAPONLIST";
		case SHUD_GRP_POWERUP: return "POWERUPS";
		case SHUD_GRP_WSTATS: return "WEAPONSTATS";
		case SHUD_GRP_PERF: return "PERF";
		case SHUD_GRP_SPEED: return "SPEED";
		case SHUD_GRP_PLAYER: return "PLAYER";
		case SHUD_GRP_SPECT: return "SPECTATOR";
		case SHUD_GRP_TEMPACC: return "TEMPACC";
		case SHUD_GRP_LOCATION: return "LOCATION";
		case SHUD_GRP_GRID: return "GRID";
		case SHUD_GRP_VOTE: return "VOTE";
		case SHUD_GRP_DECOR: return "DECOR";
		default: return "MISC";
	}
}

const char* CG_SHUDTabTitle(int tab)
{
	switch (tab)
	{
		case SHUD_TAB_STATUS: return "STATUS";
		case SHUD_TAB_CHAT: return "CHAT";
		case SHUD_TAB_COMBAT: return "COMBAT";
		case SHUD_TAB_SCORE: return "SCORE";
		case SHUD_TAB_TEAM: return "TEAM";
		case SHUD_TAB_WEAPONS: return "WEAPONS";
		case SHUD_TAB_PLAYER: return "PLAYER";
		default: return "MISC";
	}
}

void CG_SHUDAssignTabs(superhudElement_t* head)
{
	superhudElement_t* pending[64];
	superhudElement_t* el;
	int npending;
	int currentTab;
	int currentGrp;
	int t;
	int g;
	int i;

	npending = 0;
	currentTab = SHUD_TAB_MISC;
	currentGrp = SHUD_GRP_MISC;
	el = head;
	while (el)
	{
		if (CG_SHUDIsDefaultName(el->element.name) || CG_SHUDIsDecorName(el->element.name))
		{
			if (npending < 64)
			{
				pending[npending] = el;
				npending++;
			}
			else
			{
				el->tab = currentTab;
				el->grp = currentGrp;
			}
		}
		else
		{
			t = CG_SHUDNameTab(el->element.name);
			g = CG_SHUDNameGroup(el->element.name);
			if (t < 0)
			{
				t = SHUD_TAB_MISC;
			}
			if (g < 0)
			{
				g = SHUD_GRP_MISC;
			}
			currentTab = t;
			currentGrp = g;
			el->tab = t;
			el->grp = g;
			for (i = 0; i < npending; i++)
			{
				pending[i]->tab = t;
				pending[i]->grp = g;
			}
			npending = 0;
		}
		el = el->next;
	}
	for (i = 0; i < npending; i++)
	{
		pending[i]->tab = currentTab;
		pending[i]->grp = currentGrp;
	}
}

void CG_SHUDSwapWithPrev(superhudElement_t* el)
{
	superhudElement_t* prev;
	superhudElement_t* pprev;

	if (!el || elementsHead == el)
	{
		return;
	}
	pprev = NULL;
	prev = elementsHead;
	while (prev && prev->next != el)
	{
		pprev = prev;
		prev = prev->next;
	}
	if (!prev)
	{
		return;
	}
	prev->next = el->next;
	el->next = prev;
	if (pprev)
	{
		pprev->next = el;
	}
	else
	{
		elementsHead = el;
	}
}

void CG_SHUDSwapWithNext(superhudElement_t* el)
{
	if (el && el->next)
	{
		CG_SHUDSwapWithPrev(el->next);
	}
}

superhudGlobalContext_t* CG_SHUDGetContext(void)
{
	return &superhudGlobalContext;
}

qboolean CG_SHUDElementIsHidden(const superhudElement_t* el)
{
	if (!el)
	{
		return qtrue;
	}
	if (CG_SHUDEditorShowAll())
	{
		return qfalse;
	}
	if (!el->element.routine)
	{
		return qtrue;
	}
	return CG_SHUDElementVisible(el, &superhudGlobalContext.frame, cg.beIsHealthLow) ? qfalse : qtrue;
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
