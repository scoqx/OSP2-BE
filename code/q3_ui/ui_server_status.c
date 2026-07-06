// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"
#include "ui_status_sign.h"

/*
===============================================================================

SERVER STATUS (getstatus / mod_sig, main menu bar)

===============================================================================
*/

#define UI_STATUS_BAR_WIDTH             360.0f
#define UI_STATUS_BAR_HEIGHT            24.0f
#define UI_STATUS_BAR_HEIGHT_COLLAPSED  14.0f
#define UI_STATUS_COLLAPSED_CHAR_HEIGHT 12.0f
#define UI_STATUS_BAR_MIN_EXPANDED      120.0f
#define UI_STATUS_BAR_MAX_EXPANDED      420.0f
#define UI_STATUS_CHANGELOG_FILE        "version_changelog.txt"
#define UI_STATUS_FULL_CHANGELOG_FILE   "docs/changelog_be_r.txt"
#define UI_STATUS_CHANGELOG_READ_MAX    8192
#define UI_STATUS_FULL_CHANGELOG_MAX    32768
#define UI_STATUS_FULL_CHANGELOG_TOP    200.0f
#define UI_STATUS_FULL_BTN_LABEL        "Full changelog"
#define UI_STATUS_CHAR_WIDTH            8.0f
#define UI_STATUS_CHAR_HEIGHT           12.0f
#define UI_STATUS_PANEL_PAD             6.0f
#define UI_STATUS_BODY_EXTRA_PAD        0.0f // probably need 2.0f
#define UI_STATUS_SCROLLBAR_WIDTH       8.0f
#define UI_STATUS_SCROLLBAR_GAP         1.0f
#define UI_STATUS_SCROLL_WHEEL_STEP     UI_STATUS_CHAR_HEIGHT
#define UI_STATUS_SCROLL_CTRL_LINES     (UI_STATUS_CHAR_HEIGHT * 3.0f)
#define UI_STATUS_UPDATE_DATE_LABEL     "Date"
#define UI_STATUS_MENU_CURSOR_UNSET     (-2)
#define UI_STATUS_AUTOHIDE_DELAY_MS     2000
#define UI_STATUS_AUTOHIDE_SPEED_PPS    4.0f
#define UI_STATUS_AUTOHIDE_TRIGGER_H    10.0f
#define UI_STATUS_EXPAND_ANIM_SPEED     4.0f

uiServerStatus_t    uiServerStatus;

static qboolean     ui_statusFetchAttempted;
static qboolean     ui_statusOffline;
static qboolean     ui_statusBadKey;
static qboolean     ui_statusBarExpanded;
static qboolean     ui_statusFontReady;
static int          ui_statusSavedMenuCursor = UI_STATUS_MENU_CURSOR_UNSET;
static float        ui_statusChangelogScroll;
static qboolean     ui_statusScrollbarDrag;
static float        ui_statusScrollbarDragGrab;
static int          ui_statusAutoHideStartMs = -1;
static int          ui_statusAutoHideLastMs = 0;
static float        ui_statusAutoHideFrac; /* 0 - visible, 1 - hidden below screen */
static const char*  ui_statusAutoHideLabel;
static float        ui_statusExpandFrac;
static int          ui_statusExpandLastMs;
static qboolean     ui_statusExpandModalActive;
static qboolean     ui_statusFullChangelogOpen;
static qboolean     ui_statusFullChangelogReady;
static qboolean     ui_statusFullBtnHover;
static float        ui_statusFullExpandFrac;
static int          ui_statusFullExpandLastMs;
static char         ui_statusFullChangelogText[UI_STATUS_FULL_CHANGELOG_MAX];
static const char*  ui_statusFullStickyLine;
static char         ui_statusFullStickyDate[64];
static char         ui_statusFullStickyVersion[64];

static void UI_ServerStatus_StoreFromValidatedInfo(const char* infostring);
static const char* UI_ServerStatus_LocalVersionCompareString(void);
static int UI_ServerStatus_CompareBeVersion(const char* a, const char* b);
static void UI_ServerStatus_ClearChangelog(void);
static void UI_ServerStatus_LoadLocalChangelog(void);
static float UI_ServerStatus_ExpandedHeight640(float panelW);
static float UI_ServerStatus_ChangelogContentHeight(float textW);
static qboolean UI_ServerStatus_ChangelogLayout(float panelW, float bodyH, float* outTextW,
        qboolean* outNeedScroll);
static qboolean UI_ServerStatus_ChangelogLayoutEx(float panelW, float bodyH, float panelY,
        float bodyY, float* outTextW, qboolean* outNeedScroll);
static float UI_ServerStatus_ChangelogContentHeightEx(float textW, float bodyY, float row2Y,
        float row2Bottom);
static void UI_ServerStatus_ChangelogLayoutSettled(float panelW, float* outTextW,
        qboolean* outNeedScroll, float* outContentH, float* outBodyH);
static float UI_ServerStatus_DrawMultilineClipped(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom);
static float UI_ServerStatus_DrawMultilineClippedSkip(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom, const char* skipLineStart,
        float skipClipTop, float skipClipBottom);
static float UI_ServerStatus_DrawMultilineFullChangelog(float panelX, float y, float textW,
        float versionRowW, const char* text, float clipTop, float clipBottom,
        const char* skipLineStart, float skipClipTop, float skipClipBottom);
static void UI_ServerStatus_DrawText(float x, float y, const char* text, vec4_t color, int flags,
                                     float charW, float charH);
static void UI_ServerStatus_DrawTextWidth(float x, float y, const char* text, vec4_t color,
        int flags, int maxWidth640, float charW, float charH);
static void UI_ServerStatus_DrawScrollbar(float x, float y, float w, float h, float contentH,
        float scroll);
static void UI_ServerStatus_ClampChangelogScroll(float bodyH, float contentH);
static void UI_ServerStatus_ScrollChangelog(float delta, float bodyH, float contentH);
static qboolean UI_ServerStatus_HandleScrollbarMouse(float panelX, float panelY, float panelW,
        float panelH, qboolean pressed);
static void UI_ServerStatus_ScrollbarDragUpdate(float panelX, float panelY, float panelW,
        float panelH);
static void UI_ServerStatus_ScrollbarGetThumb(float sbH, float contentH, float scroll,
        float* outThumbH, float* outThumbY, float* outMaxScroll);
static void UI_ServerStatus_ParseChangelogHeaderMeta(const char* headerLine);
static void UI_ServerStatus_ClearChangelogMeta(void);
static qboolean UI_ServerStatus_HasSecondHeaderRow(void);
static qboolean UI_ServerStatus_HasFullChangelogButton(void);
static float UI_ServerStatus_ExpandedHeadersHeight(void);
static void UI_ServerStatus_FullExpandAnimTick(void);
static qboolean UI_ServerStatus_FullChangelogViewActive(void);
static void UI_ServerStatus_FullChangelogApplyFirstVersionSticky(void);
static const char* UI_ServerStatus_FullChangelogFirstVersionLine(void);
static float UI_ServerStatus_ExpandedHeight640Short(float panelW);
static void UI_ServerStatus_LoadFullChangelog(void);
static void UI_ServerStatus_CloseFullChangelog(void);
static void UI_ServerStatus_FullChangelogUpdateSticky(float panelY, float bodyY, float scroll,
        float textW, float row2Y, float row2Bottom);
static void UI_ServerStatus_DrawSecondHeaderRow(float x, float y, float w, float rowH,
        float charW, float charH);
static void UI_ServerStatus_DrawFullChangelogButton(float x, float y, float w, float rowH);
static qboolean UI_ServerStatus_FullChangelogButtonHit(float x, float w, float rowY, float rowH);
static void UI_ServerStatus_PanelBodyRect(float panelX, float panelY, float panelW, float panelH,
        float* outBodyY, float* outBodyH);
static float UI_ServerStatus_PanelBodyClipBottom(float panelY, float panelH);
static void UI_ServerStatus_DrawHeaderRow(float x, float y, float w, float rowH,
        const char* leftText, vec4_t leftColor, const char* rightText, vec4_t rightColor,
        float charW, float charH);
static qboolean UI_ServerStatus_GetBorderColor(vec4_t borderColor);
static qboolean UI_ServerStatus_AutoHideEligible(const char* label);
static qboolean UI_ServerStatus_AutoHideTriggerHover(float x, float w);
static void UI_ServerStatus_AutoHideTick(const char* label, float x, float w);
static float UI_ServerStatus_EaseSmooth(float t);
static void UI_ServerStatus_ExpandAnimTick(void);
static qboolean UI_ServerStatus_DrawExpandedVisual(void);
static qboolean UI_ServerStatus_ParseValidatedGetstatus(const char* status, char* infostring,
        int infostringSize);

static qboolean UI_ServerStatus_GetBorderColor(vec4_t borderColor)
{
	trap_Cvar_Update(&ui_beStatusBorderColor);
	if (!ui_beStatusBorderColor.string[0])
	{
		return qfalse;
	}
	if (!UI_ParseColorStr(ui_beStatusBorderColor.string, borderColor))
	{
		Vector4Copy(colorWhite, borderColor);
	}
	return qtrue;
}

static qboolean UI_ServerStatus_AutoHideEligible(const char* label)
{
	if (!label || !label[0])
	{
		return qfalse;
	}
	return (qboolean)(!Q_stricmp(label, "Ready to play") ||
	                  !Q_stricmp(label, "Unreleased build") ||
	                  !Q_stricmp(label, "Status servers offline"));
}

static qboolean UI_ServerStatus_AutoHideTriggerHover(float x, float w)
{
	float   y;

	/* Trigger zone at bottom edge even when bar is hidden. */
	y = 480.0f - UI_STATUS_AUTOHIDE_TRIGGER_H;
	if (uis.cursorx >= x && uis.cursorx <= x + w)
	{
		if (uis.cursory >= y && uis.cursory <= 480.0f)
		{
			return qtrue;
		}
	}
	return qfalse;
}

static void UI_ServerStatus_AutoHideTick(const char* label, float x, float w)
{
	int         now;
	int         dtMs;
	float       dt;
	qboolean    eligible;
	qboolean    hover;
	float       target;

	if (ui_statusBarExpanded || ui_statusExpandFrac > 0.001f)
	{
		ui_statusAutoHideStartMs = -1;
		ui_statusAutoHideLabel = NULL;
		ui_statusAutoHideFrac = 0.0f;
		ui_statusAutoHideLastMs = 0;
		return;
	}

	eligible = UI_ServerStatus_AutoHideEligible(label);
	hover = UI_ServerStatus_AutoHideTriggerHover(x, w);
	now = trap_Milliseconds();

	if (!eligible)
	{
		ui_statusAutoHideStartMs = -1;
		ui_statusAutoHideLabel = NULL;
		target = 0.0f;
	}
	else
	{
		if (ui_statusAutoHideStartMs < 0 || ui_statusAutoHideLabel != label)
		{
			ui_statusAutoHideStartMs = now;
			ui_statusAutoHideLabel = label;
		}

		if (hover)
		{
			target = 0.0f;
		}
		else if (now - ui_statusAutoHideStartMs >= UI_STATUS_AUTOHIDE_DELAY_MS)
		{
			target = 1.0f;
		}
		else
		{
			target = 0.0f;
		}
	}

	if (ui_statusAutoHideLastMs <= 0)
	{
		ui_statusAutoHideLastMs = now;
	}
	dtMs = now - ui_statusAutoHideLastMs;
	if (dtMs < 0)
	{
		dtMs = 0;
	}
	if (dtMs > 250)
	{
		dtMs = 250;
	}
	ui_statusAutoHideLastMs = now;

	dt = (float)dtMs * 0.001f;
	if (ui_statusAutoHideFrac < target)
	{
		ui_statusAutoHideFrac += UI_STATUS_AUTOHIDE_SPEED_PPS * dt;
		if (ui_statusAutoHideFrac > target)
		{
			ui_statusAutoHideFrac = target;
		}
	}
	else if (ui_statusAutoHideFrac > target)
	{
		ui_statusAutoHideFrac -= UI_STATUS_AUTOHIDE_SPEED_PPS * dt;
		if (ui_statusAutoHideFrac < target)
		{
			ui_statusAutoHideFrac = target;
		}
	}

	if (ui_statusAutoHideFrac < 0.0f)
	{
		ui_statusAutoHideFrac = 0.0f;
	}
	if (ui_statusAutoHideFrac > 1.0f)
	{
		ui_statusAutoHideFrac = 1.0f;
	}
}

static void UI_ServerStatus_ClearActiveMenuMouseFocus(void)
{
	int                 i;
	menuframework_s*    menu;
	menucommon_s*       item;

	if (!uis.activemenu)
	{
		return;
	}

	menu = uis.activemenu;
	for (i = 0; i < menu->nitems; i++)
	{
		item = (menucommon_s*)menu->items[i];
		item->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

static void UI_ServerStatus_MenuModalEnter(void)
{
	menuframework_s*    menu;

	if (!uis.activemenu)
	{
		return;
	}

	menu = uis.activemenu;
	if (ui_statusSavedMenuCursor == UI_STATUS_MENU_CURSOR_UNSET)
	{
		ui_statusSavedMenuCursor = menu->cursor;
		menu->cursor = -1;
	}

	UI_ServerStatus_ClearActiveMenuMouseFocus();
}

static void UI_ServerStatus_MenuModalLeave(void)
{
	menuframework_s*    menu;

	if (!uis.activemenu)
	{
		ui_statusSavedMenuCursor = UI_STATUS_MENU_CURSOR_UNSET;
		return;
	}

	menu = uis.activemenu;
	if (ui_statusSavedMenuCursor != UI_STATUS_MENU_CURSOR_UNSET)
	{
		if (ui_statusSavedMenuCursor >= 0 && ui_statusSavedMenuCursor < menu->nitems)
		{
			menu->cursor = ui_statusSavedMenuCursor;
		}
		else if (menu->nitems > 0)
		{
			menu->cursor = 0;
		}
		ui_statusSavedMenuCursor = UI_STATUS_MENU_CURSOR_UNSET;
	}

	UI_ServerStatus_ClearActiveMenuMouseFocus();
}

static float UI_ServerStatus_EaseSmooth(float t)
{
	if (t <= 0.0f)
	{
		return 0.0f;
	}
	if (t >= 1.0f)
	{
		return 1.0f;
	}
	return t * t * (3.0f - 2.0f * t);
}

static void UI_ServerStatus_ExpandAnimTick(void)
{
	int         now;
	int         dtMs;
	float       dt;
	float       target;

	target = ui_statusBarExpanded ? 1.0f : 0.0f;
	now = trap_Milliseconds();
	if (ui_statusExpandLastMs <= 0)
	{
		ui_statusExpandLastMs = now;
	}
	dtMs = now - ui_statusExpandLastMs;
	if (dtMs < 0)
	{
		dtMs = 0;
	}
	if (dtMs > 250)
	{
		dtMs = 250;
	}
	ui_statusExpandLastMs = now;
	dt = (float)dtMs * 0.001f;

	if (ui_statusExpandFrac < target)
	{
		ui_statusExpandFrac += UI_STATUS_EXPAND_ANIM_SPEED * dt;
		if (ui_statusExpandFrac > target)
		{
			ui_statusExpandFrac = target;
		}
	}
	else if (ui_statusExpandFrac > target)
	{
		ui_statusExpandFrac -= UI_STATUS_EXPAND_ANIM_SPEED * dt;
		if (ui_statusExpandFrac < target)
		{
			ui_statusExpandFrac = target;
		}
	}

	if (!ui_statusBarExpanded && ui_statusExpandFrac <= 0.001f)
	{
		ui_statusExpandFrac = 0.0f;
		if (ui_statusExpandModalActive)
		{
			UI_ServerStatus_MenuModalLeave();
			ui_statusExpandModalActive = qfalse;
		}
	}
}

static void UI_ServerStatus_FullExpandAnimTick(void)
{
	int         now;
	int         dtMs;
	float       dt;
	float       target;

	target = ui_statusFullChangelogOpen ? 1.0f : 0.0f;
	now = trap_Milliseconds();
	if (ui_statusFullExpandLastMs <= 0)
	{
		ui_statusFullExpandLastMs = now;
	}
	dtMs = now - ui_statusFullExpandLastMs;
	if (dtMs < 0)
	{
		dtMs = 0;
	}
	if (dtMs > 250)
	{
		dtMs = 250;
	}
	ui_statusFullExpandLastMs = now;
	dt = (float)dtMs * 0.001f;

	if (ui_statusFullExpandFrac < target)
	{
		ui_statusFullExpandFrac += UI_STATUS_EXPAND_ANIM_SPEED * dt;
		if (ui_statusFullExpandFrac > target)
		{
			ui_statusFullExpandFrac = target;
		}
	}
	else if (ui_statusFullExpandFrac > target)
	{
		ui_statusFullExpandFrac -= UI_STATUS_EXPAND_ANIM_SPEED * dt;
		if (ui_statusFullExpandFrac < target)
		{
			ui_statusFullExpandFrac = target;
		}
	}

	if (!ui_statusFullChangelogOpen && ui_statusFullExpandFrac <= 0.001f)
	{
		ui_statusFullExpandFrac = 0.0f;
	}
}

static qboolean UI_ServerStatus_FullChangelogViewActive(void)
{
	return (qboolean)((ui_statusFullChangelogOpen || ui_statusFullExpandFrac > 0.001f) &&
	                  ui_statusFullChangelogReady);
}

static qboolean UI_ServerStatus_DrawExpandedVisual(void)
{
	return (qboolean)(UI_ServerStatus_EaseSmooth(ui_statusExpandFrac) > 0.001f);
}

/*
 * Scrollbar only after expand animation finishes. While panel height is still
 * animating, bodyH is smaller than the final value and needScroll becomes true
 * briefly (scrollbar flashes during open/close).
 */
static qboolean UI_ServerStatus_ScrollbarActive(void)
{
	if (!ui_statusBarExpanded || ui_statusExpandFrac < 0.999f)
	{
		return qfalse;
	}
	if (ui_statusFullChangelogOpen && ui_statusFullExpandFrac < 0.999f)
	{
		return qfalse;
	}
	return qtrue;
}

static void UI_ServerStatus_SetExpanded(qboolean expanded)
{
	if (ui_statusBarExpanded == expanded)
	{
		if (expanded && ui_statusExpandFrac >= 0.999f)
		{
			return;
		}
		if (!expanded && ui_statusExpandFrac <= 0.001f)
		{
			return;
		}
	}

	if (expanded)
	{
		if (ui_statusExpandFrac < 0.01f)
		{
			ui_statusChangelogScroll = 0.0f;
			ui_statusScrollbarDrag = qfalse;
			ui_statusScrollbarDragGrab = 0.0f;
		}
		if (!ui_statusExpandModalActive)
		{
			UI_ServerStatus_MenuModalEnter();
			ui_statusExpandModalActive = qtrue;
		}
	}
	else
	{
		ui_statusChangelogScroll = 0.0f;
		ui_statusScrollbarDrag = qfalse;
		ui_statusScrollbarDragGrab = 0.0f;
		UI_ServerStatus_CloseFullChangelog();
	}

	ui_statusBarExpanded = expanded;
}

static void UI_ServerStatus_EnsureFonts(void)
{
	int     fontIndex;

	if (!UI_FontLoaded(2))
	{
		UI_LoadFonts();
	}

	trap_Cvar_Update(&ui_beStatusFont);
	fontIndex = UI_FontIndexFromCvar(&ui_beStatusFont);
	if (!UI_FontAvailable(fontIndex))
	{
		fontIndex = 2;
	}

	if (UI_FontLoaded(fontIndex))
	{
		UI_FontSelect(fontIndex);
		ui_statusFontReady = qtrue;
	}
	else
	{
		ui_statusFontReady = qfalse;
	}
}

static void UI_ServerStatus_BarRect640(float* outX, float* outY, float* outW, float* outH)
{
	float   collapsedH;
	float   expandedH;
	float   easedFrac;
	float   animH;

	collapsedH = UI_STATUS_BAR_HEIGHT_COLLAPSED;
	*outW = UI_STATUS_BAR_WIDTH;
	*outX = (640.0f - UI_STATUS_BAR_WIDTH) * 0.5f;

	easedFrac = UI_ServerStatus_EaseSmooth(ui_statusExpandFrac);
	if (easedFrac > 0.001f)
	{
		expandedH = UI_ServerStatus_ExpandedHeight640(UI_STATUS_BAR_WIDTH);
		animH = collapsedH + (expandedH - collapsedH) * easedFrac;
	}
	else
	{
		animH = collapsedH;
	}
	*outH = animH;
	*outY = 480.0f - animH;
}

static void UI_ServerStatus_ClearChangelog(void)
{
	uiServerStatus.changelogHeader[0] = '\0';
	uiServerStatus.changelogBody[0] = '\0';
	uiServerStatus.changelogReady = qfalse;
	UI_ServerStatus_ClearChangelogMeta();
	ui_statusChangelogScroll = 0.0f;
	ui_statusScrollbarDrag = qfalse;
	ui_statusScrollbarDragGrab = 0.0f;
}

static void UI_ServerStatus_FormatDateYmd(int ymd, char* out, int outSize)
{
	int     y;
	int     m;
	int     d;

	if (!out || outSize < 8)
	{
		return;
	}

	out[0] = '\0';
	if (ymd < 10000101 || ymd > 99991231)
	{
		return;
	}

	y = ymd / 10000;
	m = (ymd / 100) % 100;
	d = ymd % 100;
	Com_sprintf(out, outSize, "%02i.%02i.%04i", d, m, y);
}

static void UI_ServerStatus_ClearChangelogMeta(void)
{
	uiServerStatus.changelogUpdateDate[0] = '\0';
	uiServerStatus.changelogNewVersion[0] = '\0';
}

static void UI_ServerStatus_ParseChangelogHeaderMeta(const char* headerLine)
{
	char            work[128];
	char*           be;
	char*           ver;
	int             i;

	UI_ServerStatus_ClearChangelogMeta();
	if (!headerLine || !headerLine[0])
	{
		return;
	}

	Q_strncpyz(work, headerLine, sizeof(work));
	be = Q_stristr(work, " be ");
	if (be)
	{
		*be = '\0';
		for (i = (int)(be - work) - 1; i >= 0; i--)
		{
			if (work[i] != ' ' && work[i] != '\t')
			{
				break;
			}
			work[i] = '\0';
		}
		if (work[0])
		{
			Q_strncpyz(uiServerStatus.changelogUpdateDate, work,
			           sizeof(uiServerStatus.changelogUpdateDate));
		}

		ver = be + 5;
		if (*ver)
		{
			Q_strncpyz(uiServerStatus.changelogNewVersion, ver,
			           sizeof(uiServerStatus.changelogNewVersion));
		}
		return;
	}

	if (work[0])
	{
		Q_strncpyz(uiServerStatus.changelogUpdateDate, work,
		           sizeof(uiServerStatus.changelogUpdateDate));
	}
}

static qboolean UI_ServerStatus_HasSecondHeaderRow(void)
{
	return uiServerStatus.changelogReady;
}

static qboolean UI_ServerStatus_HasFullChangelogButton(void)
{
	return uiServerStatus.changelogReady;
}

static qboolean UI_ServerStatus_LineIsVersionHeader(const char* lineStart, const char* lineEnd)
{
	int             i;
	int             len;

	if (!lineStart || lineEnd <= lineStart)
	{
		return qfalse;
	}

	len = (int)(lineEnd - lineStart);
	if (len < 14)
	{
		return qfalse;
	}

	for (i = 0; i < 10; i++)
	{
		if (i == 2 || i == 5)
		{
			if (lineStart[i] != '.')
			{
				return qfalse;
			}
		}
		else if (lineStart[i] < '0' || lineStart[i] > '9')
		{
			return qfalse;
		}
	}

	if (lineStart[10] != ' ')
	{
		return qfalse;
	}

	if (Q_strncmp(lineStart + 11, "be v", 4))
	{
		return qfalse;
	}

	return qtrue;
}

static void UI_ServerStatus_ParseVersionLineMeta(const char* headerLine, char* dateOut, int dateSize,
        char* verOut, int verSize)
{
	char            work[128];
	char*           be;
	char*           ver;
	int             i;

	if (dateOut && dateSize > 0)
	{
		dateOut[0] = '\0';
	}
	if (verOut && verSize > 0)
	{
		verOut[0] = '\0';
	}

	if (!headerLine || !headerLine[0])
	{
		return;
	}

	Q_strncpyz(work, headerLine, sizeof(work));
	be = Q_stristr(work, " be ");
	if (be)
	{
		*be = '\0';
		for (i = (int)(be - work) - 1; i >= 0; i--)
		{
			if (work[i] != ' ' && work[i] != '\t')
			{
				break;
			}
			work[i] = '\0';
		}
		if (work[0] && dateOut && dateSize > 0)
		{
			Q_strncpyz(dateOut, work, dateSize);
		}

		ver = be + 5;
		if (*ver && verOut && verSize > 0)
		{
			Q_strncpyz(verOut, ver, verSize);
		}
	}
	else if (work[0] && dateOut && dateSize > 0)
	{
		Q_strncpyz(dateOut, work, dateSize);
	}
}

static void UI_ServerStatus_LoadFullChangelog(void)
{
	fileHandle_t    f;
	int             len;

	if (ui_statusFullChangelogReady)
	{
		return;
	}

	ui_statusFullChangelogText[0] = '\0';
	ui_statusFullStickyLine = NULL;
	ui_statusFullStickyDate[0] = '\0';
	ui_statusFullStickyVersion[0] = '\0';

	len = trap_FS_FOpenFile(UI_STATUS_FULL_CHANGELOG_FILE, &f, FS_READ);
	if (len <= 0)
	{
		return;
	}
	if (len >= (int)sizeof(ui_statusFullChangelogText))
	{
		len = sizeof(ui_statusFullChangelogText) - 1;
	}

	trap_FS_Read(ui_statusFullChangelogText, len, f);
	trap_FS_FCloseFile(f);
	ui_statusFullChangelogText[len] = '\0';

	if (!ui_statusFullChangelogText[0])
	{
		return;
	}

	ui_statusFullChangelogReady = qtrue;
}

static void UI_ServerStatus_CloseFullChangelog(void)
{
	ui_statusFullChangelogOpen = qfalse;
	ui_statusChangelogScroll = 0.0f;
	ui_statusFullStickyLine = NULL;
	ui_statusFullStickyDate[0] = '\0';
	ui_statusFullStickyVersion[0] = '\0';
	ui_statusFullBtnHover = qfalse;
}

static const char* UI_ServerStatus_FullChangelogFirstVersionLine(void)
{
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;

	if (!ui_statusFullChangelogText[0])
	{
		return NULL;
	}

	p = ui_statusFullChangelogText;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	{
		p++;
	}

	while (*p)
	{
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd > lineStart && UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd))
		{
			return lineStart;
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return NULL;
}

static void UI_ServerStatus_FullChangelogApplyFirstVersionSticky(void)
{
	const char*     sticky;
	char            lineBuf[128];
	int             copyLen;

	sticky = UI_ServerStatus_FullChangelogFirstVersionLine();
	if (!sticky)
	{
		return;
	}

	ui_statusFullStickyLine = sticky;
	copyLen = 0;
	while (sticky[copyLen] && sticky[copyLen] != '\n' && sticky[copyLen] != '\r' &&
	        copyLen < (int)sizeof(lineBuf) - 1)
	{
		copyLen++;
	}
	memcpy(lineBuf, sticky, copyLen);
	lineBuf[copyLen] = '\0';
	UI_ServerStatus_ParseVersionLineMeta(lineBuf, ui_statusFullStickyDate,
	                                     sizeof(ui_statusFullStickyDate),
	                                     ui_statusFullStickyVersion,
	                                     sizeof(ui_statusFullStickyVersion));
}

static void UI_ServerStatus_FullChangelogUpdateSticky(float panelY, float bodyY, float scroll,
        float textW, float row2Y, float row2Bottom)
{
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	float           lineY;
	const char*     sticky;
	int             chunk;
	int             remain;

	ui_statusFullStickyLine = NULL;
	ui_statusFullStickyDate[0] = '\0';
	ui_statusFullStickyVersion[0] = '\0';

	if (!ui_statusFullChangelogReady || !ui_statusFullChangelogText[0])
	{
		return;
	}

	sticky = NULL;
	lineY = 0.0f;

	p = ui_statusFullChangelogText;
	while (*p)
	{
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd <= lineStart)
		{
			lineY += UI_STATUS_CHAR_HEIGHT;
			if (p[0] == '\r' && p[1] == '\n')
			{
				p += 2;
			}
			else if (*p)
			{
				p++;
			}
			continue;
		}

		if (UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd))
		{
			float   screenTop;

			screenTop = bodyY - scroll + lineY;
			if (screenTop <= row2Bottom + 0.5f)
			{
				sticky = lineStart;
			}

			lineY += UI_STATUS_CHAR_HEIGHT;
		}
		else
		{
			remain = (int)(lineEnd - lineStart);
			while (remain > 0)
			{
				chunk = UI_OSPTextFitLineWidth640(lineStart, lineEnd, UI_STATUS_CHAR_WIDTH, 1,
				                                  textW);
				if (chunk <= 0)
				{
					chunk = 1;
				}
				if (chunk > remain)
				{
					chunk = remain;
				}
				lineY += UI_STATUS_CHAR_HEIGHT;
				lineStart += chunk;
				remain -= chunk;
			}
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	if (sticky)
	{
		char            lineBuf[128];
		int             copyLen;

		ui_statusFullStickyLine = sticky;
		copyLen = 0;
		while (sticky[copyLen] && sticky[copyLen] != '\n' && sticky[copyLen] != '\r' &&
		        copyLen < (int)sizeof(lineBuf) - 1)
		{
			copyLen++;
		}
		memcpy(lineBuf, sticky, copyLen);
		lineBuf[copyLen] = '\0';
		UI_ServerStatus_ParseVersionLineMeta(lineBuf, ui_statusFullStickyDate,
		                                     sizeof(ui_statusFullStickyDate),
		                                     ui_statusFullStickyVersion,
		                                     sizeof(ui_statusFullStickyVersion));
	}
	else
	{
		UI_ServerStatus_FullChangelogApplyFirstVersionSticky();
	}

	(void)panelY;
	(void)row2Y;
}

static float UI_ServerStatus_ExpandedHeadersHeight(void)
{
	float           h;

	h = UI_STATUS_BAR_HEIGHT_COLLAPSED;
	if (UI_ServerStatus_HasSecondHeaderRow())
	{
		h += UI_STATUS_BAR_HEIGHT_COLLAPSED;
	}
	return h;
}

static void UI_ServerStatus_PanelBodyRect(float panelX, float panelY, float panelW, float panelH,
        float* outBodyY, float* outBodyH)
{
	float           headersH;

	headersH = UI_ServerStatus_ExpandedHeadersHeight();
	*outBodyY = panelY + headersH + UI_STATUS_PANEL_PAD;
	*outBodyH = panelH - headersH - UI_STATUS_PANEL_PAD * 2.0f - UI_STATUS_BODY_EXTRA_PAD;
}

/*
 * Bottom reading margin (UI_STATUS_BODY_EXTRA_PAD) fades in only when fully expanded.
 * While opening/closing, changelog clip reaches the panel bottom so text does not
 * appear to emerge from an inactive black strip.
 */
static float UI_ServerStatus_PanelBodyClipBottom(float panelY, float panelH)
{
	float           eased;
	float           extraPad;

	eased = UI_ServerStatus_EaseSmooth(ui_statusExpandFrac);
	extraPad = UI_STATUS_BODY_EXTRA_PAD * eased;
	return panelY + panelH - UI_STATUS_PANEL_PAD - extraPad;
}

static void UI_ServerStatus_BuildServerChangelogHeader(const char* version, char* out, int outSize)
{
	char    dateBuf[32];

	UI_ServerStatus_FormatDateYmd(uiServerStatus.date, dateBuf, sizeof(dateBuf));
	if (dateBuf[0] && version && version[0])
	{
		Com_sprintf(out, outSize, "%s be v%s", dateBuf, version);
	}
	else if (version && version[0])
	{
		Com_sprintf(out, outSize, "be v%s", version);
	}
	else if (dateBuf[0])
	{
		Q_strncpyz(out, dateBuf, outSize);
	}
	else
	{
		out[0] = '\0';
	}

	UI_ServerStatus_ParseChangelogHeaderMeta(out);
}

static void UI_ServerStatus_DecodePipeBody(const char* encoded, char* body, int bodySize)
{
	const char*     s;
	char*           d;
	int             remain;

	if (!encoded || !body || bodySize < 2)
	{
		if (body && bodySize > 0)
		{
			body[0] = '\0';
		}
		return;
	}

	s = encoded;
	d = body;
	remain = bodySize - 1;
	while (*s && remain > 0)
	{
		if (*s == '|')
		{
			*d++ = '\n';
			s++;
			remain--;
			continue;
		}
		*d++ = *s++;
		remain--;
	}
	*d = '\0';
}

static void UI_ServerStatus_AssembleBeClEncoded(const char* infostring, char* encoded, int encSize)
{
	const char*     val;
	char            key[16];
	int             parts;
	int             i;

	if (!encoded || encSize < 1)
	{
		return;
	}

	encoded[0] = '\0';
	if (!infostring || !infostring[0])
	{
		return;
	}

	val = Info_ValueForKey(infostring, "be_cl");
	if (val[0])
	{
		Q_strncpyz(encoded, val, encSize);
		return;
	}

	parts = atoi(Info_ValueForKey(infostring, "be_cl_parts"));
	if (parts <= 0)
	{
		parts = 6;
	}
	if (parts > 6)
	{
		parts = 6;
	}

	for (i = 0; i < parts; i++)
	{
		Com_sprintf(key, sizeof(key), "be_cl%i", i);
		val = Info_ValueForKey(infostring, key);
		if (!val[0])
		{
			Com_sprintf(key, sizeof(key), "Info%i", i);
			val = Info_ValueForKey(infostring, key);
		}
		if (val[0])
		{
			Q_strcat(encoded, encSize, val);
		}
	}
}

static qboolean UI_ServerStatus_ParseChangelogFromGetinfo(const char* infostring)
{
	char            encoded[MAX_INFO_STRING];
	const char*     clVer;

	if (!infostring || !infostring[0])
	{
		return qfalse;
	}

	UI_ServerStatus_AssembleBeClEncoded(infostring, encoded, sizeof(encoded));
	if (!encoded[0])
	{
		return qfalse;
	}

	clVer = Info_ValueForKey(infostring, "be_cl_ver");
	if (!clVer[0])
	{
		clVer = uiServerStatus.version;
	}

	UI_ServerStatus_BuildServerChangelogHeader(clVer, uiServerStatus.changelogHeader,
	        sizeof(uiServerStatus.changelogHeader));
	UI_ServerStatus_DecodePipeBody(encoded, uiServerStatus.changelogBody,
	                               sizeof(uiServerStatus.changelogBody));
	return (uiServerStatus.changelogBody[0] != '\0');
}

static void UI_ServerStatus_ParseChangelogFileText(const char* text)
{
	const char*     s;
	const char*     lineEnd;

	if (!text || !text[0])
	{
		return;
	}

	s = text;
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
	{
		s++;
	}
	if (!*s)
	{
		return;
	}

	lineEnd = s;
	while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
	{
		lineEnd++;
	}

	if (lineEnd > s)
	{
		int     copyLen;

		copyLen = (int)(lineEnd - s);
		if (copyLen >= (int)sizeof(uiServerStatus.changelogHeader))
		{
			copyLen = sizeof(uiServerStatus.changelogHeader) - 1;
		}
		memcpy(uiServerStatus.changelogHeader, s, copyLen);
		uiServerStatus.changelogHeader[copyLen] = '\0';
	}

	s = lineEnd;
	while (*s == '\r' || *s == '\n')
	{
		s++;
	}

	Q_strncpyz(uiServerStatus.changelogBody, s, sizeof(uiServerStatus.changelogBody));
	UI_ServerStatus_ParseChangelogHeaderMeta(uiServerStatus.changelogHeader);
}

static void UI_ServerStatus_LoadLocalChangelog(void)
{
	fileHandle_t    f;
	int             len;
	static char     fileBuf[UI_STATUS_CHANGELOG_READ_MAX];

	UI_ServerStatus_ClearChangelog();

	len = trap_FS_FOpenFile(UI_STATUS_CHANGELOG_FILE, &f, FS_READ);
	if (len <= 0)
	{
		return;
	}
	if (len >= (int)sizeof(fileBuf))
	{
		len = sizeof(fileBuf) - 1;
	}

	trap_FS_Read(fileBuf, len, f);
	trap_FS_FCloseFile(f);
	fileBuf[len] = '\0';

	UI_ServerStatus_ParseChangelogFileText(fileBuf);
	uiServerStatus.changelogReady = (uiServerStatus.changelogBody[0] != '\0' ||
	                                 uiServerStatus.changelogHeader[0] != '\0');
}

static qboolean UI_ServerStatus_IsOfflineState(void)
{
	if (!ui_statusFetchAttempted)
	{
		return qfalse;
	}

	if (!uiServerStatus.received || ui_statusOffline || ui_statusBadKey || !uiServerStatus.valid)
	{
		return qtrue;
	}

	return qfalse;
}

static void UI_ServerStatus_EnsureLocalChangelogForOffline(void)
{
	if (!UI_ServerStatus_IsOfflineState())
	{
		return;
	}

	if (!uiServerStatus.changelogReady)
	{
		UI_ServerStatus_LoadLocalChangelog();
	}
}

static const char* UI_ServerStatus_SkipPastLine(const char* p)
{
	if (!p)
	{
		return "";
	}

	while (*p && *p != '\n' && *p != '\r')
	{
		p++;
	}
	while (*p == '\n' || *p == '\r')
	{
		p++;
	}

	return p;
}

static const char* UI_ServerStatus_FindNextVersionBlock(const char* p)
{
	const char*     lineStart;
	const char*     lineEnd;

	if (!p)
	{
		return NULL;
	}

	while (*p)
	{
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd > lineStart && UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd))
		{
			return lineStart;
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return NULL;
}

static void UI_ServerStatus_CopyChangelogBodyRange(const char* start, const char* end, char* out,
        int outSize)
{
	int             len;

	if (!out || outSize < 1)
	{
		return;
	}

	out[0] = '\0';
	if (!start || !*start)
	{
		return;
	}

	if (!end)
	{
		end = start + strlen(start);
	}

	while (end > start && (end[-1] == '\n' || end[-1] == '\r'))
	{
		end--;
	}

	while (*start == '\n' || *start == '\r')
	{
		start++;
	}

	len = (int)(end - start);
	if (len <= 0)
	{
		return;
	}

	if (len >= outSize)
	{
		len = outSize - 1;
	}

	memcpy(out, start, len);
	out[len] = '\0';
}

static qboolean UI_ServerStatus_VersionHeaderMatchesLocal(const char* lineStart,
        const char* lineEnd)
{
	char            header[128];
	char            ver[64];
	const char*     localVer;
	int             len;

	if (!lineStart || lineEnd <= lineStart)
	{
		return qfalse;
	}

	len = (int)(lineEnd - lineStart);
	if (len >= (int)sizeof(header))
	{
		len = sizeof(header) - 1;
	}
	memcpy(header, lineStart, len);
	header[len] = '\0';

	UI_ServerStatus_ParseVersionLineMeta(header, NULL, 0, ver, sizeof(ver));
	if (!ver[0])
	{
		return qfalse;
	}

	localVer = UI_ServerStatus_LocalVersionCompareString();
	return (qboolean)(UI_ServerStatus_CompareBeVersion(localVer, ver) == 0);
}

static qboolean UI_ServerStatus_TryLoadUnreleasedChangelogFromFull(void)
{
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	const char*     bodyStart;
	const char*     nextBlock;
	int             copyLen;

	if (!uiServerStatus.unreleased)
	{
		return qfalse;
	}

	UI_ServerStatus_ClearChangelog();
	UI_ServerStatus_LoadFullChangelog();
	if (!ui_statusFullChangelogReady)
	{
		return qfalse;
	}

	p = ui_statusFullChangelogText;
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	{
		p++;
	}

	while (*p)
	{
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd > lineStart && UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd) &&
		        UI_ServerStatus_VersionHeaderMatchesLocal(lineStart, lineEnd))
		{
			copyLen = (int)(lineEnd - lineStart);
			if (copyLen >= (int)sizeof(uiServerStatus.changelogHeader))
			{
				copyLen = sizeof(uiServerStatus.changelogHeader) - 1;
			}
			memcpy(uiServerStatus.changelogHeader, lineStart, copyLen);
			uiServerStatus.changelogHeader[copyLen] = '\0';

			bodyStart = UI_ServerStatus_SkipPastLine(lineStart);
			nextBlock = UI_ServerStatus_FindNextVersionBlock(bodyStart);
			UI_ServerStatus_CopyChangelogBodyRange(bodyStart, nextBlock,
			                                       uiServerStatus.changelogBody,
			                                       sizeof(uiServerStatus.changelogBody));
			UI_ServerStatus_ParseChangelogHeaderMeta(uiServerStatus.changelogHeader);
			uiServerStatus.changelogReady = (qboolean)(uiServerStatus.changelogBody[0] != '\0' ||
			                                uiServerStatus.changelogHeader[0] != '\0');
			return uiServerStatus.changelogReady;
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return qfalse;
}

static void UI_ServerStatus_OpenFullChangelogDirect(void)
{
	UI_ServerStatus_ClearChangelog();
	UI_ServerStatus_LoadFullChangelog();
	if (!ui_statusFullChangelogReady)
	{
		return;
	}

	uiServerStatus.changelogReady = qtrue;
	ui_statusFullChangelogOpen = qtrue;
	ui_statusChangelogScroll = 0.0f;
	ui_statusFullExpandLastMs = 0;
	UI_ServerStatus_FullChangelogApplyFirstVersionSticky();
}

static void UI_ServerStatus_PrepareUnreleasedBarExpand(void)
{
	if (!uiServerStatus.unreleased)
	{
		return;
	}

	if (UI_ServerStatus_TryLoadUnreleasedChangelogFromFull())
	{
		UI_ServerStatus_CloseFullChangelog();
		return;
	}

	UI_ServerStatus_OpenFullChangelogDirect();
}

static void UI_ServerStatus_EnsureChangelogForBarExpand(void)
{
	if (uiServerStatus.unreleased)
	{
		UI_ServerStatus_PrepareUnreleasedBarExpand();
		return;
	}

	UI_ServerStatus_EnsureLocalChangelogForOffline();
}

static float UI_ServerStatus_ShortChangelogContentHeight(float textW)
{
	int             lines;

	if (!uiServerStatus.changelogReady || !uiServerStatus.changelogBody[0])
	{
		return 0.0f;
	}

	lines = UI_OSPTextCountWrappedLines640(uiServerStatus.changelogBody, UI_STATUS_CHAR_WIDTH, 1,
	                                       textW);
	return (float)lines * UI_STATUS_CHAR_HEIGHT;
}

static qboolean UI_ServerStatus_FullChangelogVersionHiddenInBody(const char* physicalLineStart,
        float lineTop, float lineBottom, float skipClipTop, float skipClipBottom)
{
	if (!ui_statusFullStickyLine || physicalLineStart != ui_statusFullStickyLine)
	{
		return qfalse;
	}

	if (ui_statusChangelogScroll < 0.5f)
	{
		return qtrue;
	}

	if (lineBottom > skipClipTop && lineTop < skipClipBottom)
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ServerStatus_FullChangelogVersionHiddenReserveHeight(qboolean hiddenInBody)
{
	if (!hiddenInBody)
	{
		return qtrue;
	}

	/*
	 * At scroll top the first version row is shown only in the sticky header; omit it
	 * from body layout. While scrolling, a version stuck under the header still reserves
	 * one line in body coordinates so the next entry is not clipped above bodyY.
	 */
	return (qboolean)(ui_statusChangelogScroll >= 0.5f);
}

static float UI_ServerStatus_FullChangelogBodyContentHeight(float textW, float bodyY, float scroll,
        float row2Y, float row2Bottom)
{
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	const char*     physicalLineStart;
	float           lineY;
	int             chunk;
	int             remain;
	float           skipClipTop;
	float           skipClipBottom;

	if (!ui_statusFullChangelogText[0])
	{
		return 0.0f;
	}

	skipClipTop = row2Y;
	skipClipBottom = row2Bottom;
	(void)bodyY;

	lineY = 0.0f;
	p = ui_statusFullChangelogText;
	while (*p)
	{
		physicalLineStart = p;
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd <= lineStart)
		{
			lineY += UI_STATUS_CHAR_HEIGHT;
			if (p[0] == '\r' && p[1] == '\n')
			{
				p += 2;
			}
			else if (*p)
			{
				p++;
			}
			continue;
		}

		if (UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd))
		{
			float       lineTop;
			float       lineBottom;
			qboolean    hiddenInBody;

			lineTop = bodyY - scroll + lineY;
			lineBottom = lineTop + UI_STATUS_CHAR_HEIGHT;
			hiddenInBody = UI_ServerStatus_FullChangelogVersionHiddenInBody(physicalLineStart,
			               lineTop, lineBottom, skipClipTop, skipClipBottom);
			if (UI_ServerStatus_FullChangelogVersionHiddenReserveHeight(hiddenInBody))
			{
				lineY += UI_STATUS_CHAR_HEIGHT;
			}
		}
		else
		{
			remain = (int)(lineEnd - lineStart);
			while (remain > 0)
			{
				chunk = UI_OSPTextFitLineWidth640(lineStart, lineEnd, UI_STATUS_CHAR_WIDTH, 1,
				                                  textW);
				if (chunk <= 0)
				{
					chunk = 1;
				}
				if (chunk > remain)
				{
					chunk = remain;
				}
				lineY += UI_STATUS_CHAR_HEIGHT;
				lineStart += chunk;
				remain -= chunk;
			}
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return lineY;
}

static float UI_ServerStatus_ChangelogContentHeightEx(float textW, float bodyY, float row2Y,
        float row2Bottom)
{
	if (!uiServerStatus.changelogReady)
	{
		return 0.0f;
	}

	if (UI_ServerStatus_FullChangelogViewActive() && ui_statusFullChangelogText[0])
	{
		return UI_ServerStatus_FullChangelogBodyContentHeight(textW, bodyY,
		        ui_statusChangelogScroll, row2Y, row2Bottom);
	}

	return UI_ServerStatus_ShortChangelogContentHeight(textW);
}

static float UI_ServerStatus_ChangelogContentHeight(float textW)
{
	int             lines;
	float           contentH;

	if (!uiServerStatus.changelogReady)
	{
		return 0.0f;
	}

	if (UI_ServerStatus_FullChangelogViewActive() && ui_statusFullChangelogText[0])
	{
		lines = UI_OSPTextCountWrappedLines640(ui_statusFullChangelogText, UI_STATUS_CHAR_WIDTH, 1,
		                                       textW);
		contentH = (float)lines * UI_STATUS_CHAR_HEIGHT;
		if (ui_statusFullStickyLine && ui_statusChangelogScroll < 0.5f)
		{
			contentH -= UI_STATUS_CHAR_HEIGHT;
		}
		if (contentH < 0.0f)
		{
			contentH = 0.0f;
		}
		return contentH;
	}

	if (!uiServerStatus.changelogBody[0])
	{
		return 0.0f;
	}

	lines = UI_OSPTextCountWrappedLines640(uiServerStatus.changelogBody, UI_STATUS_CHAR_WIDTH, 1,
	                                       textW);
	return (float)lines * UI_STATUS_CHAR_HEIGHT;
}

static qboolean UI_ServerStatus_ChangelogLayout(float panelW, float bodyH, float* outTextW,
        qboolean* outNeedScroll)
{
	return UI_ServerStatus_ChangelogLayoutEx(panelW, bodyH, 0.0f, 0.0f, outTextW, outNeedScroll);
}

static qboolean UI_ServerStatus_ChangelogLayoutEx(float panelW, float bodyH, float panelY,
        float bodyY, float* outTextW, qboolean* outNeedScroll)
{
	float           textW;
	float           contentH;
	float           row2Y;
	float           row2Bottom;
	qboolean        needScroll;

	textW = panelW - UI_STATUS_PANEL_PAD * 2.0f;
	if (UI_ServerStatus_FullChangelogViewActive() && panelY > 0.0f && bodyY > 0.0f)
	{
		row2Y = panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED;
		row2Bottom = row2Y + UI_STATUS_BAR_HEIGHT_COLLAPSED;
		contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY, row2Y, row2Bottom);
	}
	else
	{
		contentH = UI_ServerStatus_ChangelogContentHeight(textW);
	}
	needScroll = (contentH > bodyH + 0.5f);

	if (needScroll)
	{
		textW = panelW - UI_STATUS_PANEL_PAD * 2.0f - UI_STATUS_SCROLLBAR_WIDTH -
		        UI_STATUS_SCROLLBAR_GAP;
		if (textW < UI_STATUS_CHAR_WIDTH * 8.0f)
		{
			textW = UI_STATUS_CHAR_WIDTH * 8.0f;
		}
		if (UI_ServerStatus_FullChangelogViewActive() && panelY > 0.0f && bodyY > 0.0f)
		{
			row2Y = panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED;
			row2Bottom = row2Y + UI_STATUS_BAR_HEIGHT_COLLAPSED;
			contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY, row2Y, row2Bottom);
		}
		else
		{
			contentH = UI_ServerStatus_ChangelogContentHeight(textW);
		}
		needScroll = (contentH > bodyH + 0.5f);
	}

	*outTextW = textW;
	*outNeedScroll = needScroll;
	return needScroll;
}

/*
 * Line breaks for changelog use the fully expanded body width/height so they do
 * not reflow while the panel height animates.
 */
static void UI_ServerStatus_ChangelogLayoutSettled(float panelW, float* outTextW,
        qboolean* outNeedScroll, float* outContentH, float* outBodyH)
{
	float           expandedH;
	float           bodyY;

	expandedH = UI_ServerStatus_ExpandedHeight640(panelW);
	UI_ServerStatus_PanelBodyRect(0.0f, 0.0f, panelW, expandedH, &bodyY, outBodyH);
	UI_ServerStatus_ChangelogLayout(panelW, *outBodyH, outTextW, outNeedScroll);
	*outContentH = UI_ServerStatus_ChangelogContentHeight(*outTextW);
}

static void UI_ServerStatus_ClampChangelogScroll(float bodyH, float contentH)
{
	float           maxScroll;

	maxScroll = contentH - bodyH;
	if (maxScroll < 0.0f)
	{
		maxScroll = 0.0f;
	}
	if (UI_STATUS_CHAR_HEIGHT > 0.0f)
	{
		maxScroll = (float)((int)(maxScroll / UI_STATUS_CHAR_HEIGHT)) * UI_STATUS_CHAR_HEIGHT;
	}

	if (ui_statusChangelogScroll < 0.0f)
	{
		ui_statusChangelogScroll = 0.0f;
	}
	if (ui_statusChangelogScroll > maxScroll)
	{
		ui_statusChangelogScroll = maxScroll;
	}

	if (UI_STATUS_CHAR_HEIGHT > 0.0f)
	{
		ui_statusChangelogScroll = (float)((int)(ui_statusChangelogScroll / UI_STATUS_CHAR_HEIGHT + 0.5f))
		                           * UI_STATUS_CHAR_HEIGHT;
		if (ui_statusChangelogScroll > maxScroll)
		{
			ui_statusChangelogScroll = maxScroll;
		}
	}
}

static void UI_ServerStatus_ScrollChangelog(float delta, float bodyH, float contentH)
{
	ui_statusChangelogScroll += delta;
	UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);
}

static float UI_ServerStatus_ExpandedHeight640Short(float panelW)
{
	float           h;
	float           textW;
	float           bodyHMax;
	float           contentH;
	qboolean        needScroll;

	h = UI_ServerStatus_ExpandedHeadersHeight() + UI_STATUS_PANEL_PAD * 2.0f + UI_STATUS_BODY_EXTRA_PAD;
	bodyHMax = UI_STATUS_BAR_MAX_EXPANDED - h;
	if (bodyHMax < UI_STATUS_CHAR_HEIGHT * 2.0f)
	{
		bodyHMax = UI_STATUS_CHAR_HEIGHT * 2.0f;
	}

	UI_ServerStatus_ChangelogLayout(panelW, bodyHMax, &textW, &needScroll);
	contentH = UI_ServerStatus_ShortChangelogContentHeight(textW);

	h += contentH;
	if (h > UI_STATUS_BAR_MAX_EXPANDED)
	{
		h = UI_STATUS_BAR_MAX_EXPANDED;
	}
	if (h < UI_STATUS_BAR_MIN_EXPANDED)
	{
		h = UI_STATUS_BAR_MIN_EXPANDED;
	}

	return h;
}

static float UI_ServerStatus_ExpandedHeight640(float panelW)
{
	float           shortH;
	float           fullH;
	float           t;

	shortH = UI_ServerStatus_ExpandedHeight640Short(panelW);

	if (!ui_statusFullChangelogReady && ui_statusFullExpandFrac <= 0.001f)
	{
		return shortH;
	}

	fullH = 480.0f - UI_STATUS_FULL_CHANGELOG_TOP;
	if (fullH > UI_STATUS_BAR_MAX_EXPANDED)
	{
		fullH = UI_STATUS_BAR_MAX_EXPANDED;
	}
	if (fullH < UI_STATUS_BAR_MIN_EXPANDED)
	{
		fullH = UI_STATUS_BAR_MIN_EXPANDED;
	}

	if (fullH <= shortH + 0.5f)
	{
		return fullH;
	}

	t = UI_ServerStatus_EaseSmooth(ui_statusFullExpandFrac);
	return shortH + (fullH - shortH) * t;
}

static qboolean UI_ServerStatus_ChangelogLineMarkerColor(char c, vec4_t out)
{
	switch (c)
	{
		case '*':
			out[0] = 1.0f;
			out[1] = 1.0f;
			out[2] = 0.4f;
			out[3] = 1.0f;
			return qtrue;
		case '+':
			out[0] = 0.4f;
			out[1] = 1.0f;
			out[2] = 0.4f;
			out[3] = 1.0f;
			return qtrue;
		case '-':
			out[0] = 1.0f;
			out[1] = 0.4f;
			out[2] = 0.4f;
			out[3] = 1.0f;
			return qtrue;
		default:
			return qfalse;
	}
}

static qboolean UI_ServerStatus_ChangelogLineMarkerChar(char c)
{
	return (qboolean)(c == '*' || c == '+' || c == '-');
}

static const char* UI_ServerStatus_ChangelogLineMarkerAt(const char* physicalLineStart,
        const char* lineEnd)
{
	const char*     p;

	if (!physicalLineStart || physicalLineStart >= lineEnd)
	{
		return NULL;
	}

	p = physicalLineStart;
	while (p < lineEnd && (*p == ' ' || *p == '\t'))
	{
		p++;
	}
	if (p < lineEnd && UI_ServerStatus_ChangelogLineMarkerChar(*p))
	{
		return p;
	}

	return NULL;
}

static float UI_ServerStatus_TextSpanWidth640(const char* start, const char* end)
{
	const char*     s;
	float           ax;

	ax = 0.0f;
	s = start;
	while (s < end)
	{
		if (s[0] == '^' && s[1])
		{
			s += 2;
			continue;
		}

		ax += UI_OSPCharWidth640(*s, UI_STATUS_CHAR_WIDTH, 1);
		s++;
	}

	return ax;
}

static void UI_ServerStatus_DrawChangelogTextSpan(float x, float y, const char* start, int len,
        vec4_t color, int flags, int maxWidthPx)
{
	char            buf[256];

	if (len <= 0)
	{
		return;
	}

	if (len >= (int)sizeof(buf))
	{
		len = sizeof(buf) - 1;
	}

	memcpy(buf, start, len);
	buf[len] = '\0';
	UI_ServerStatus_DrawTextWidth(x, y, buf, color, flags, maxWidthPx, UI_STATUS_CHAR_WIDTH,
	                              UI_STATUS_CHAR_HEIGHT);
}

static float UI_ServerStatus_ChangelogMarkerColumnWidth(void)
{
	float           w;
	float           wSym;

	w = UI_OSPCharWidth640('*', UI_STATUS_CHAR_WIDTH, 1);
	wSym = UI_OSPCharWidth640('+', UI_STATUS_CHAR_WIDTH, 1);
	if (wSym > w)
	{
		w = wSym;
	}
	wSym = UI_OSPCharWidth640('-', UI_STATUS_CHAR_WIDTH, 1);
	if (wSym > w)
	{
		w = wSym;
	}
	return w;
}

static void UI_ServerStatus_DrawChangelogTextLine(float lineBaseX, float y, const char* chunkStart,
        int chunkLen, const char* physicalLineStart, const char* lineEnd, vec4_t color, int flags,
        int maxWidthPx)
{
	const char*     markerAt;
	const char*     chunkEnd;
	const char*     bodyStart;
	vec4_t          markColor;
	char            markBuf[2];
	int             prefixLen;
	int             bodyLen;
	float           lineX;
	float           markerX;
	float           bodyX;

	if (!chunkStart || chunkLen <= 0)
	{
		return;
	}

	chunkEnd = chunkStart + chunkLen;
	markerAt = UI_ServerStatus_ChangelogLineMarkerAt(physicalLineStart, lineEnd);

	if (!markerAt)
	{
		UI_ServerStatus_DrawChangelogTextSpan(lineBaseX, y, chunkStart, chunkLen, color, flags,
		                                      maxWidthPx);
		return;
	}

	if (chunkStart > markerAt)
	{
		UI_ServerStatus_DrawChangelogTextSpan(lineBaseX, y, chunkStart, chunkLen, color, flags,
		                                      maxWidthPx);
		return;
	}

	lineX = lineBaseX + UI_ServerStatus_TextSpanWidth640(physicalLineStart, chunkStart);

	if (chunkEnd <= markerAt)
	{
		prefixLen = chunkLen;
		UI_ServerStatus_DrawChangelogTextSpan(lineX, y, chunkStart, prefixLen, color, flags,
		                                      maxWidthPx);
		return;
	}

	if (markerAt > chunkStart)
	{
		prefixLen = (int)(markerAt - chunkStart);
		UI_ServerStatus_DrawChangelogTextSpan(lineX, y, chunkStart, prefixLen, color, flags,
		                                      maxWidthPx);
	}

	if (!UI_ServerStatus_ChangelogLineMarkerColor(*markerAt, markColor))
	{
		UI_ServerStatus_DrawChangelogTextSpan(lineX, y, markerAt, (int)(chunkEnd - markerAt),
		                                      color, flags, maxWidthPx);
		return;
	}

	markerX = lineBaseX + UI_ServerStatus_TextSpanWidth640(physicalLineStart, markerAt);
	markBuf[0] = *markerAt;
	markBuf[1] = '\0';
	UI_ServerStatus_DrawTextWidth(markerX, y, markBuf, markColor, flags, maxWidthPx,
	                              UI_STATUS_CHAR_WIDTH, UI_STATUS_CHAR_HEIGHT);

	bodyStart = markerAt + 1;
	if (bodyStart >= chunkEnd)
	{
		return;
	}

	bodyX = markerX + UI_ServerStatus_ChangelogMarkerColumnWidth();
	bodyLen = (int)(chunkEnd - bodyStart);
	UI_ServerStatus_DrawChangelogTextSpan(bodyX, y, bodyStart, bodyLen, color, flags, maxWidthPx);
}

static float UI_ServerStatus_DrawMultilineClipped(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom)
{
	return UI_ServerStatus_DrawMultilineClippedSkip(x, y, maxW, text, color, flags, clipTop,
	        clipBottom, NULL, 0.0f, 0.0f);
}

static float UI_ServerStatus_DrawMultilineClippedSkip(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom, const char* skipLineStart,
        float skipClipTop, float skipClipBottom)
{
	char            lineBuf[256];
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	const char*     physicalLineStart;
	int             chunk;
	int             remain;
	float           lineY;
	float           startY;
	float           lineTop;
	float           lineBottom;
	int             maxWidthPx;
	qboolean        skipBand;

	if (!text || !text[0] || !ui_statusFontReady)
	{
		return 0.0f;
	}

	maxWidthPx = (int)maxW;
	if (maxWidthPx < 1)
	{
		maxWidthPx = 1;
	}

	skipBand = (qboolean)(skipLineStart != NULL);

	startY = y;
	lineY = y;
	p = text;
	while (*p)
	{
		physicalLineStart = p;
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd <= lineStart)
		{
			lineY += UI_STATUS_CHAR_HEIGHT;
			if (p[0] == '\r' && p[1] == '\n')
			{
				p += 2;
			}
			else if (*p)
			{
				p++;
			}
			continue;
		}

		remain = (int)(lineEnd - lineStart);
		while (remain > 0)
		{
			chunk = UI_OSPTextFitLineWidth640(lineStart, lineEnd, UI_STATUS_CHAR_WIDTH, 1, maxW);
			if (chunk <= 0)
			{
				chunk = 1;
			}
			if (chunk > remain)
			{
				chunk = remain;
			}
			if (chunk >= (int)sizeof(lineBuf))
			{
				chunk = sizeof(lineBuf) - 1;
			}

			lineTop = lineY;
			lineBottom = lineY + UI_STATUS_CHAR_HEIGHT;
			if (lineBottom > clipTop && lineTop < clipBottom)
			{
				qboolean    drawLine;

				drawLine = qtrue;
				if (skipBand && physicalLineStart == skipLineStart)
				{
					if (lineBottom > skipClipTop && lineTop < skipClipBottom)
					{
						drawLine = qfalse;
					}
				}

				if (drawLine)
				{
					UI_ServerStatus_DrawChangelogTextLine(x, lineY, lineStart, chunk,
					                                      physicalLineStart, lineEnd, color,
					                                      flags, maxWidthPx);
				}
			}

			lineY += UI_STATUS_CHAR_HEIGHT;
			lineStart += chunk;
			remain -= chunk;
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return lineY - startY;
}

static void UI_ServerStatus_DrawVersionContentRow(float x, float y, float w, float rowH,
        const char* dateText, const char* verText, float clipTop, float clipBottom)
{
	vec4_t          bg;
	vec4_t          dateColor;
	vec4_t          verColor;
	float           lineTop;
	float           lineBottom;

	lineTop = y;
	lineBottom = y + rowH;
	if (lineBottom <= clipTop || lineTop >= clipBottom)
	{
		return;
	}

	bg[0] = 0.25f;
	bg[1] = 0.25f;
	bg[2] = 0.25f;
	bg[3] = 0.25f;
	UI_FillRect(x, y, w, rowH, bg);

	dateColor[0] = 1.0f;
	dateColor[1] = 1.0f;
	dateColor[2] = 1.0f;
	dateColor[3] = 1.0f;

	verColor[0] = 0.8f;
	verColor[1] = 0.8f;
	verColor[2] = 0.8f;
	verColor[3] = 1.0f;

	if (!dateText)
	{
		dateText = "";
	}
	if (!verText)
	{
		verText = "";
	}

	UI_ServerStatus_DrawHeaderRow(x, y, w, rowH, dateText, dateColor, verText, verColor,
	                              UI_STATUS_CHAR_WIDTH, UI_STATUS_CHAR_HEIGHT);
}

static float UI_ServerStatus_ScrollWheelStep(void)
{
	if (trap_Key_IsDown(K_CTRL))
	{
		return UI_STATUS_CHAR_HEIGHT * UI_STATUS_SCROLL_CTRL_LINES;
	}
	return UI_STATUS_SCROLL_WHEEL_STEP;
}

static float UI_ServerStatus_DrawMultilineFullChangelog(float panelX, float y, float textW,
        float versionRowW, const char* text, float clipTop, float clipBottom,
        const char* skipLineStart, float skipClipTop, float skipClipBottom)
{
	char            lineBuf[256];
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	const char*     physicalLineStart;
	int             chunk;
	int             remain;
	float           lineY;
	float           startY;
	float           lineTop;
	float           lineBottom;
	float           contentX;
	int             maxWidthPx;
	qboolean        skipBand;

	if (!text || !text[0] || !ui_statusFontReady)
	{
		return 0.0f;
	}

	contentX = panelX + UI_STATUS_PANEL_PAD;
	maxWidthPx = (int)textW;
	if (maxWidthPx < 1)
	{
		maxWidthPx = 1;
	}

	skipBand = (qboolean)(skipLineStart != NULL);
	startY = y;
	lineY = y;
	p = text;
	while (*p)
	{
		physicalLineStart = p;
		lineStart = p;
		lineEnd = p;
		while (*lineEnd && *lineEnd != '\n' && *lineEnd != '\r')
		{
			lineEnd++;
		}

		if (lineEnd <= lineStart)
		{
			lineY += UI_STATUS_CHAR_HEIGHT;
			if (p[0] == '\r' && p[1] == '\n')
			{
				p += 2;
			}
			else if (*p)
			{
				p++;
			}
			continue;
		}

		if (UI_ServerStatus_LineIsVersionHeader(lineStart, lineEnd))
		{
			char            dateBuf[64];
			char            verBuf[64];
			qboolean        drawLine;
			qboolean        hiddenInBody;

			lineTop = lineY;
			lineBottom = lineY + UI_STATUS_CHAR_HEIGHT;
			hiddenInBody = UI_ServerStatus_FullChangelogVersionHiddenInBody(physicalLineStart,
			               lineTop, lineBottom, skipClipTop, skipClipBottom);
			drawLine = (qboolean)(!hiddenInBody && lineBottom > clipTop && lineTop < clipBottom);

			if (drawLine)
			{
				int     copyLen;

				copyLen = (int)(lineEnd - lineStart);
				if (copyLen >= (int)sizeof(lineBuf))
				{
					copyLen = sizeof(lineBuf) - 1;
				}
				memcpy(lineBuf, lineStart, copyLen);
				lineBuf[copyLen] = '\0';
				UI_ServerStatus_ParseVersionLineMeta(lineBuf, dateBuf, sizeof(dateBuf), verBuf,
				                                     sizeof(verBuf));
				UI_ServerStatus_DrawVersionContentRow(panelX, lineY, versionRowW,
				                                      UI_STATUS_CHAR_HEIGHT, dateBuf, verBuf,
				                                      clipTop, clipBottom);
			}

			if (UI_ServerStatus_FullChangelogVersionHiddenReserveHeight(hiddenInBody))
			{
				lineY += UI_STATUS_CHAR_HEIGHT;
			}
		}
		else
		{
			remain = (int)(lineEnd - lineStart);
			while (remain > 0)
			{
				chunk = UI_OSPTextFitLineWidth640(lineStart, lineEnd, UI_STATUS_CHAR_WIDTH, 1,
				                                  textW);
				if (chunk <= 0)
				{
					chunk = 1;
				}
				if (chunk > remain)
				{
					chunk = remain;
				}
				if (chunk >= (int)sizeof(lineBuf))
				{
					chunk = sizeof(lineBuf) - 1;
				}

				lineTop = lineY;
				lineBottom = lineY + UI_STATUS_CHAR_HEIGHT;
				if (lineBottom > clipTop && lineTop < clipBottom)
				{
					qboolean    drawLine;

					drawLine = qtrue;
					if (skipBand && physicalLineStart == skipLineStart)
					{
						if (lineBottom > skipClipTop && lineTop < skipClipBottom)
						{
							drawLine = qfalse;
						}
					}

					if (drawLine)
					{
						UI_ServerStatus_DrawChangelogTextLine(contentX, lineY, lineStart, chunk,
						                                      physicalLineStart, lineEnd,
						                                      text_color_normal, DS_HLEFT,
						                                      maxWidthPx);
					}
				}

				lineY += UI_STATUS_CHAR_HEIGHT;
				lineStart += chunk;
				remain -= chunk;
			}
		}

		p = lineEnd;
		while (*p == '\n' || *p == '\r')
		{
			p++;
		}
	}

	return lineY - startY;
}

static void UI_ServerStatus_ScrollbarGetThumb(float sbH, float contentH, float scroll,
        float* outThumbH, float* outThumbY, float* outMaxScroll)
{
	float           maxScroll;
	float           thumbH;
	float           thumbY;
	float           trackRange;

	maxScroll = contentH - sbH;
	if (maxScroll < 0.0f)
	{
		maxScroll = 0.0f;
	}

	thumbH = sbH * (sbH / contentH);
	if (thumbH < UI_STATUS_CHAR_HEIGHT)
	{
		thumbH = UI_STATUS_CHAR_HEIGHT;
	}
	if (thumbH > sbH)
	{
		thumbH = sbH;
	}

	trackRange = sbH - thumbH;
	thumbY = 0.0f;
	if (maxScroll > 0.0f && trackRange > 0.0f)
	{
		thumbY = (scroll / maxScroll) * trackRange;
	}

	*outThumbH = thumbH;
	*outThumbY = thumbY;
	*outMaxScroll = maxScroll;
}

static void UI_ServerStatus_DrawScrollbar(float x, float y, float w, float h, float contentH,
        float scroll)
{
	vec4_t          track;
	vec4_t          thumb;
	float           thumbH;
	float           thumbY;
	float           maxScroll;

	if (h <= 0.0f || contentH <= h + 0.5f)
	{
		return;
	}

	UI_ServerStatus_ScrollbarGetThumb(h, contentH, scroll, &thumbH, &thumbY, &maxScroll);
	if (maxScroll < 1.0f)
	{
		return;
	}

	track[0] = 0.25f;
	track[1] = 0.25f;
	track[2] = 0.25f;
	track[3] = 0.85f;
	UI_FillRect(x, y, w, h, track);
	{
		vec4_t  borderColor;

		if (UI_ServerStatus_GetBorderColor(borderColor))
		{
			UI_DrawRect(x, y, w, h, borderColor);
		}
	}

	thumb[0] = 0.65f;
	thumb[1] = 0.65f;
	thumb[2] = 0.65f;
	thumb[3] = 1.0f;
	UI_FillRect(x + 1.0f, y + thumbY, w - 2.0f, thumbH, thumb);
}

static void UI_ServerStatus_ScrollbarDragUpdate(float panelX, float panelY, float panelW,
        float panelH)
{
	float           bodyY;
	float           bodyH;
	float           textW;
	float           contentH;
	qboolean        needScroll;
	float           sbY;
	float           sbH;
	float           maxScroll;
	float           thumbH;
	float           thumbY;
	float           trackRange;
	float           my;

	if (!ui_statusScrollbarDrag)
	{
		return;
	}

	UI_ServerStatus_PanelBodyRect(panelX, panelY, panelW, panelH, &bodyY, &bodyH);
	UI_ServerStatus_ChangelogLayoutEx(panelW, bodyH, panelY, bodyY, &textW, &needScroll);
	if (!needScroll)
	{
		ui_statusScrollbarDrag = qfalse;
		return;
	}

	contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY,
	           panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED,
	           panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED * 2.0f);
	sbH = bodyH;
	sbY = bodyY;
	my = uis.cursory;

	UI_ServerStatus_ScrollbarGetThumb(sbH, contentH, ui_statusChangelogScroll, &thumbH, &thumbY,
	                                  &maxScroll);
	trackRange = sbH - thumbH;
	if (trackRange < 1.0f || maxScroll < 1.0f)
	{
		return;
	}

	ui_statusChangelogScroll = ((my - ui_statusScrollbarDragGrab - sbY) / trackRange) * maxScroll;
	UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);
}

static qboolean UI_ServerStatus_HandleScrollbarMouse(float panelX, float panelY, float panelW,
        float panelH, qboolean pressed)
{
	float           bodyY;
	float           bodyH;
	float           textW;
	float           contentH;
	qboolean        needScroll;
	float           sbX;
	float           sbY;
	float           sbW;
	float           sbH;
	float           maxScroll;
	float           thumbH;
	float           thumbY;
	float           mx;
	float           my;

	if (!UI_ServerStatus_DrawExpandedVisual() || !UI_ServerStatus_ScrollbarActive() ||
	        !uiServerStatus.changelogReady || !pressed)
	{
		return qfalse;
	}

	UI_ServerStatus_PanelBodyRect(panelX, panelY, panelW, panelH, &bodyY, &bodyH);
	UI_ServerStatus_ChangelogLayoutEx(panelW, bodyH, panelY, bodyY, &textW, &needScroll);
	if (!needScroll)
	{
		ui_statusScrollbarDrag = qfalse;
		return qfalse;
	}

	contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY,
	           panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED,
	           panelY + UI_STATUS_BAR_HEIGHT_COLLAPSED * 2.0f);
	UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);

	sbW = UI_STATUS_SCROLLBAR_WIDTH;
	sbH = bodyH;
	sbX = panelX + panelW - UI_STATUS_PANEL_PAD - sbW;
	sbY = bodyY;

	mx = uis.cursorx;
	my = uis.cursory;

	if (mx < sbX || mx > sbX + sbW || my < sbY || my > sbY + sbH)
	{
		return qfalse;
	}

	UI_ServerStatus_ScrollbarGetThumb(sbH, contentH, ui_statusChangelogScroll, &thumbH, &thumbY,
	                                  &maxScroll);
	thumbY += sbY;

	if (my >= thumbY && my <= thumbY + thumbH)
	{
		ui_statusScrollbarDrag = qtrue;
		ui_statusScrollbarDragGrab = my - thumbY;
	}
	else if (my < thumbY)
	{
		ui_statusScrollbarDrag = qfalse;
		UI_ServerStatus_ScrollChangelog(-bodyH, bodyH, contentH);
	}
	else
	{
		ui_statusScrollbarDrag = qfalse;
		UI_ServerStatus_ScrollChangelog(bodyH, bodyH, contentH);
	}

	return qtrue;
}

static void UI_ServerStatus_ScrollByDelta(float delta)
{
	float           x;
	float           y;
	float           w;
	float           h;
	float           bodyY;
	float           bodyH;
	float           textW;
	float           contentH;
	qboolean        needScroll;

	if (!UI_ServerStatus_DrawExpandedVisual() || !UI_ServerStatus_ScrollbarActive() ||
	        !uiServerStatus.changelogReady)
	{
		return;
	}

	UI_ServerStatus_BarRect640(&x, &y, &w, &h);
	UI_ServerStatus_PanelBodyRect(x, y, w, h, &bodyY, &bodyH);
	UI_ServerStatus_ChangelogLayoutEx(w, bodyH, y, bodyY, &textW, &needScroll);
	if (!needScroll)
	{
		return;
	}

	contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY,
	           y + UI_STATUS_BAR_HEIGHT_COLLAPSED, y + UI_STATUS_BAR_HEIGHT_COLLAPSED * 2.0f);
	UI_ServerStatus_ScrollChangelog(delta, bodyH, contentH);
}

static void UI_ServerStatus_DrawTextWidth(float x, float y, const char* text, vec4_t color,
        int flags, int maxWidth640, float charW, float charH)
{
	if (!text || !text[0])
	{
		return;
	}

	if (!ui_statusFontReady)
	{
		return;
	}

	if (maxWidth640 < 1)
	{
		maxWidth640 = 1;
	}

	UI_OSPDrawStringNew(x, y, text, color, color_black,
	                    charW, charH,
	                    maxWidth640, flags | DS_SHADOW | DS_PROPORTIONAL, NULL, NULL, NULL);
}

static void UI_ServerStatus_DrawText(float x, float y, const char* text, vec4_t color, int flags,
                                     float charW, float charH)
{
	UI_ServerStatus_DrawTextWidth(x, y, text, color, flags, SCREEN_WIDTH, charW, charH);
}

static void UI_ServerStatus_DrawHeaderRow(float x, float y, float w, float rowH,
        const char* leftText, vec4_t leftColor, const char* rightText, vec4_t rightColor,
        float charW, float charH)
{
	float           rowCenterY;
	vec4_t          borderColor;

	if (UI_ServerStatus_GetBorderColor(borderColor))
	{
		UI_DrawRect(x, y, w, rowH, borderColor);
	}
	rowCenterY = y + rowH * 0.5f;

	if (leftText && leftText[0])
	{
		UI_ServerStatus_DrawText(x + UI_STATUS_PANEL_PAD, rowCenterY, leftText, leftColor,
		                         DS_VCENTER, charW, charH);
	}

	if (rightText && rightText[0])
	{
		UI_ServerStatus_DrawText(x + w - UI_STATUS_PANEL_PAD, rowCenterY, rightText, rightColor,
		                         DS_VCENTER | DS_HRIGHT, charW, charH);
	}
}

static void UI_ServerStatus_DrawStatusRow(float x, float y, float w, float rowH,
        const char* statusLabel, vec4_t statusColor, float charW, float charH)
{
	char            verBuf[64];
	vec4_t          verColor;
	const char*     localVer;

	localVer = UI_ServerStatus_LocalVersionCompareString();
	if (localVer && localVer[0])
	{
		Q_strncpyz(verBuf, localVer, sizeof(verBuf));
	}
	else
	{
		Q_strncpyz(verBuf, "?", sizeof(verBuf));
	}

	verColor[0] = text_color_disabled[0];
	verColor[1] = text_color_disabled[1];
	verColor[2] = text_color_disabled[2];
	verColor[3] = 1.0f;

	UI_ServerStatus_DrawHeaderRow(x, y, w, rowH, statusLabel, statusColor, verBuf, verColor,
	                              charW, charH);
}

static void UI_ServerStatus_DrawSecondHeaderRow(float x, float y, float w, float rowH,
        float charW, float charH)
{
	char            verBuf[64];
	const char*     leftText;
	vec4_t          dateColor;
	vec4_t          verColor;

	if (UI_ServerStatus_FullChangelogViewActive())
	{
		leftText = ui_statusFullStickyDate;
		if (!leftText[0])
		{
			leftText = UI_STATUS_UPDATE_DATE_LABEL;
		}

		verBuf[0] = '\0';
		if (ui_statusFullStickyVersion[0])
		{
			Q_strncpyz(verBuf, ui_statusFullStickyVersion, sizeof(verBuf));
		}
	}
	else
	{
		leftText = uiServerStatus.changelogUpdateDate;
		if (!leftText[0])
		{
			leftText = UI_STATUS_UPDATE_DATE_LABEL;
		}

		verBuf[0] = '\0';
		if (uiServerStatus.changelogNewVersion[0])
		{
			Q_strncpyz(verBuf, uiServerStatus.changelogNewVersion, sizeof(verBuf));
		}
	}

	dateColor[0] = 1.0f;
	dateColor[1] = 1.0f;
	dateColor[2] = 1.0f;
	dateColor[3] = 1.0f;

	verColor[0] = 0.2f;
	verColor[1] = 0.85f;
	verColor[2] = 0.2f;
	verColor[3] = 1.0f;

	UI_ServerStatus_DrawHeaderRow(x, y, w, rowH, leftText, dateColor, verBuf, verColor, charW,
	                              charH);

	if (UI_ServerStatus_HasFullChangelogButton())
	{
		UI_ServerStatus_DrawFullChangelogButton(x, y, w, rowH);
	}
	else
	{
		ui_statusFullBtnHover = qfalse;
	}
}

static void UI_ServerStatus_FullChangelogButtonRect(float x, float y, float w, float rowH,
        float* outBtnX, float* outBtnY, float* outBtnW, float* outBtnH)
{
	float           btnW;
	float           textW;

	textW = (float)UI_ProportionalStringWidth(UI_STATUS_FULL_BTN_LABEL);
	btnW = textW + 12.0f;
	if (btnW > w - UI_STATUS_PANEL_PAD * 2.0f)
	{
		btnW = w - UI_STATUS_PANEL_PAD * 2.0f;
	}

	*outBtnX = x + (w - btnW) * 0.5f;
	*outBtnY = y;
	*outBtnW = btnW;
	*outBtnH = rowH;
}

static void UI_ServerStatus_DrawFullChangelogButton(float x, float y, float w, float rowH)
{
	vec4_t          textColor;
	float           btnX;
	float           btnY;
	float           btnW;
	float           btnH;
	float           rowCenterX;
	float           rowCenterY;
	qboolean        active;
	qboolean        hover;

	UI_ServerStatus_FullChangelogButtonRect(x, y, w, rowH, &btnX, &btnY, &btnW, &btnH);

	rowCenterX = x + w * 0.5f;
	rowCenterY = y + rowH * 0.5f;

	active = (qboolean)(ui_statusFullChangelogOpen || ui_statusFullExpandFrac > 0.001f);
	hover = (qboolean)(uis.cursorx >= btnX && uis.cursorx <= btnX + btnW &&
	                   uis.cursory >= btnY && uis.cursory <= btnY + btnH);
	ui_statusFullBtnHover = hover;

	if (active)
	{
		if (hover)
		{
			textColor[0] = 0.55f;
			textColor[1] = 0.55f;
			textColor[2] = 0.55f;
		}
		else
		{
			textColor[0] = 1.0f;
			textColor[1] = 1.0f;
			textColor[2] = 1.0f;
		}
	}
	else
	{
		if (hover)
		{
			textColor[0] = 1.0f;
			textColor[1] = 1.0f;
			textColor[2] = 1.0f;
		}
		else
		{
			textColor[0] = 0.55f;
			textColor[1] = 0.55f;
			textColor[2] = 0.55f;
		}
	}
	textColor[3] = 1.0f;

	UI_ServerStatus_DrawText(rowCenterX, rowCenterY, UI_STATUS_FULL_BTN_LABEL, textColor,
	                         DS_VCENTER | DS_HCENTER, UI_STATUS_CHAR_WIDTH,
	                         UI_STATUS_COLLAPSED_CHAR_HEIGHT);
}

static qboolean UI_ServerStatus_FullChangelogButtonHit(float x, float w, float rowY, float rowH)
{
	float           btnW;
	float           btnH;
	float           btnX;
	float           btnY;

	if (!UI_ServerStatus_HasFullChangelogButton())
	{
		return qfalse;
	}

	UI_ServerStatus_FullChangelogButtonRect(x, rowY, w, rowH, &btnX, &btnY, &btnW, &btnH);

	if (uis.cursorx >= btnX && uis.cursorx <= btnX + btnW &&
	        uis.cursory >= btnY && uis.cursory <= btnY + btnH)
	{
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ServerStatus_ClientInHfvList(void)
{
	char            localVer[64];
	char            list[MAX_INFO_VALUE];
	char            token[64];
	const char*     s;
	int             i;

	if (!uiServerStatus.hfv[0] || !Q_stricmp(uiServerStatus.hfv, "0"))
	{
		return qfalse;
	}

	Q_strncpyz(localVer, UI_ServerStatus_LocalVersionCompareString(), sizeof(localVer));
	if (!localVer[0])
	{
		return qfalse;
	}

	if (!Q_stricmp(uiServerStatus.hfv, localVer))
	{
		return qtrue;
	}

	Q_strncpyz(list, uiServerStatus.hfv, sizeof(list));
	s = list;
	while (*s)
	{
		while (*s == ' ' || *s == ',' || *s == ';')
		{
			s++;
		}
		if (!*s)
		{
			break;
		}

		i = 0;
		while (*s && *s != ' ' && *s != ',' && *s != ';' && i < (int)sizeof(token) - 1)
		{
			token[i++] = *s++;
		}
		token[i] = '\0';

		if (token[0] && !Q_stricmp(token, localVer))
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean UI_ServerStatus_UpdateRequired(void)
{
	if (!uiServerStatus.valid || !uiServerStatus.received)
	{
		return qfalse;
	}

	if (uiServerStatus.unreleased)
	{
		return qfalse;
	}

	if (uiServerStatus.flags & 2)
	{
		return qtrue;
	}

	if (uiServerStatus.flags & 4)
	{
		return qtrue;
	}

	if (UI_ServerStatus_ClientInHfvList())
	{
		return qtrue;
	}

	return qfalse;
}

qboolean UI_ServerStatus_WantsChangelog(void)
{
	if (!uiServerStatus.valid || !uiServerStatus.received)
	{
		return qfalse;
	}

	if (uiServerStatus.last || uiServerStatus.unreleased)
	{
		return qfalse;
	}

	return qtrue;
}

static void UI_ServerStatus_GetBarMessage(const char** outText, vec4_t outColor)
{
	if (!ui_statusFetchAttempted)
	{
		*outText = NULL;
		outColor[3] = 0.0f;
		return;
	}

	if (ui_statusOffline)
	{
		*outText = "Status servers offline";
		outColor[0] = 0.55f;
		outColor[1] = 0.55f;
		outColor[2] = 0.55f;
		outColor[3] = 1.0f;
		return;
	}

	if (ui_statusBadKey)
	{
		*outText = "Invalid status";
		outColor[0] = 1.0f;
		outColor[1] = 0.0f;
		outColor[2] = 0.0f;
		outColor[3] = 1.0f;
		return;
	}

	if (!uiServerStatus.received)
	{
		*outText = "Connecting";
		outColor[0] = 1.0f;
		outColor[1] = 1.0f;
		outColor[2] = 1.0f;
		outColor[3] = 1.0f;
		return;
	}

	if (!uiServerStatus.valid)
	{
		*outText = "Offline";
		outColor[0] = 0.55f;
		outColor[1] = 0.55f;
		outColor[2] = 0.55f;
		outColor[3] = 1.0f;
		return;
	}

	if (uiServerStatus.last)
	{
		*outText = "Ready to play";
		outColor[0] = 0.2f;
		outColor[1] = 0.85f;
		outColor[2] = 0.2f;
		outColor[3] = 1.0f;
		return;
	}

	if (uiServerStatus.unreleased)
	{
		*outText = "Unreleased build";
		outColor[0] = 0.0f;
		outColor[1] = 1.0f;
		outColor[2] = 1.0f;
		outColor[3] = 1.0f;
		return;
	}

	if (UI_ServerStatus_UpdateRequired())
	{
		*outText = "Update required!";
		outColor[0] = 1.0f;
		outColor[1] = 0.0f;
		outColor[2] = 0.0f;
		outColor[3] = 1.0f;
		return;
	}

	*outText = "Update available";
	outColor[0] = 1.0f;
	outColor[1] = 1.0f;
	outColor[2] = 0.0f;
	outColor[3] = 1.0f;
}

qboolean UI_ServerStatus_IsModal(void)
{
	const char*     label;
	vec4_t          color;

	if (!ui_statusFetchAttempted)
	{
		return qfalse;
	}

	if (ui_statusExpandFrac <= 0.001f && !ui_statusBarExpanded)
	{
		return qfalse;
	}

	UI_ServerStatus_GetBarMessage(&label, color);
	if (!label || color[3] <= 0.0f)
	{
		return qfalse;
	}

	return qtrue;
}

static void UI_ServerStatus_DrawBarPanel(float x, float y, float w, float h, const char* label, vec4_t color)
{
	vec4_t          bg;
	vec4_t          dim;
	vec4_t          textColor;
	float           rowY;
	float           bodyH;
	float           expandVis;
	qboolean        drawExpanded;

	expandVis = UI_ServerStatus_EaseSmooth(ui_statusExpandFrac);
	drawExpanded = UI_ServerStatus_DrawExpandedVisual();

	if (drawExpanded)
	{
		dim[0] = 0.0f;
		dim[1] = 0.0f;
		dim[2] = 0.0f;
		dim[3] = 0.45f * expandVis;
		UI_FillRect(uis.screenXmin, uis.screenYmin,
		            uis.screenXmax - uis.screenXmin,
		            uis.screenYmax - uis.screenYmin, dim);
	}

	bg[0] = 0.0f;
	bg[1] = 0.0f;
	bg[2] = 0.0f;
	bg[3] = drawExpanded ? (0.25f + 0.5f * expandVis) : 0.25f;
	UI_FillRect(x, y, w, h, bg);

	{
		vec4_t  borderColor;

		if (UI_ServerStatus_GetBorderColor(borderColor))
		{
			UI_DrawRect(x, y, w, h, borderColor);
		}
	}

	textColor[0] = color[0];
	textColor[1] = color[1];
	textColor[2] = color[2];
	textColor[3] = 1.0f;

	rowY = y;

	if (drawExpanded)
	{
		float           bodyY;
		float           bodyClipTop;
		float           bodyClipBottom;
		float           textW;
		float           contentH;
		float           contentY;
		qboolean        needScroll;

		needScroll = qfalse;
		textW = 0.0f;
		contentH = 0.0f;

		UI_ServerStatus_DrawStatusRow(x, rowY, w, UI_STATUS_BAR_HEIGHT_COLLAPSED, label,
		                              textColor, UI_STATUS_CHAR_WIDTH,
		                              UI_STATUS_COLLAPSED_CHAR_HEIGHT);
		rowY += UI_STATUS_BAR_HEIGHT_COLLAPSED;

		UI_ServerStatus_PanelBodyRect(x, y, w, h, &bodyY, &bodyH);
		bodyClipTop = bodyY;
		bodyClipBottom = UI_ServerStatus_PanelBodyClipBottom(y, h);

		if (UI_ServerStatus_HasSecondHeaderRow())
		{
			float           row2Y;
			float           row2Bottom;
			float           rowH;

			rowH = UI_STATUS_BAR_HEIGHT_COLLAPSED;
			row2Y = rowY;
			row2Bottom = row2Y + rowH;

			if (UI_ServerStatus_FullChangelogViewActive())
			{
				UI_ServerStatus_ChangelogLayoutEx(w, bodyH, y, bodyY, &textW, &needScroll);
				UI_ServerStatus_FullChangelogUpdateSticky(y, bodyY, ui_statusChangelogScroll, textW,
				        row2Y, row2Bottom);
			}

			UI_ServerStatus_DrawSecondHeaderRow(x, row2Y, w, rowH, UI_STATUS_CHAR_WIDTH,
			                                    UI_STATUS_COLLAPSED_CHAR_HEIGHT);
			rowY += rowH;
		}

		if (bodyH > UI_STATUS_CHAR_HEIGHT && uiServerStatus.changelogReady)
		{
			float           bodyHSettled;
			float           row2Y;
			float           row2Bottom;

			row2Y = y + UI_STATUS_BAR_HEIGHT_COLLAPSED;
			row2Bottom = row2Y + UI_STATUS_BAR_HEIGHT_COLLAPSED;

			bodyHSettled = bodyH;
			if (UI_ServerStatus_ScrollbarActive())
			{
				UI_ServerStatus_ChangelogLayoutEx(w, bodyH, y, bodyY, &textW, &needScroll);
				contentH = UI_ServerStatus_ChangelogContentHeightEx(textW, bodyY, row2Y,
				           row2Bottom);
			}
			else
			{
				UI_ServerStatus_ChangelogLayoutSettled(w, &textW, &needScroll, &contentH,
				                                       &bodyHSettled);
			}
			UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);

			contentY = bodyY - ui_statusChangelogScroll;

			if (UI_ServerStatus_FullChangelogViewActive() && ui_statusFullChangelogText[0])
			{
				float   versionRowW;

				versionRowW = w;
				if (needScroll && UI_ServerStatus_ScrollbarActive())
				{
					versionRowW = textW + UI_STATUS_PANEL_PAD;
				}
				UI_ServerStatus_DrawMultilineFullChangelog(x, contentY, textW, versionRowW,
				        ui_statusFullChangelogText, bodyClipTop, bodyClipBottom,
				        ui_statusFullStickyLine, row2Y, row2Bottom);
			}
			else if (uiServerStatus.changelogBody[0])
			{
				UI_ServerStatus_DrawMultilineClipped(x + UI_STATUS_PANEL_PAD, contentY, textW,
				                                     uiServerStatus.changelogBody, text_color_normal,
				                                     DS_HLEFT, bodyClipTop, bodyClipBottom);
			}

			if (needScroll && UI_ServerStatus_ScrollbarActive())
			{
				float   sbX;

				sbX = x + w - UI_STATUS_PANEL_PAD - UI_STATUS_SCROLLBAR_WIDTH;
				UI_ServerStatus_DrawScrollbar(sbX, bodyY, UI_STATUS_SCROLLBAR_WIDTH, bodyH,
				                              contentH, ui_statusChangelogScroll);
			}
		}
	}
	else
	{
		UI_ServerStatus_DrawStatusRow(x, rowY, w, UI_STATUS_BAR_HEIGHT_COLLAPSED, label,
		                              textColor, UI_STATUS_CHAR_WIDTH,
		                              UI_STATUS_COLLAPSED_CHAR_HEIGHT);
	}
}

void UI_ServerStatus_DrawBar(void)
{
	const char*     label;
	vec4_t          color;
	float           x;
	float           y;
	float           w;
	float           h;
	float           hideOffset;

	if (!ui_statusFetchAttempted)
	{
		return;
	}

	UI_ServerStatus_EnsureFonts();

	UI_ServerStatus_GetBarMessage(&label, color);
	if (!label || color[3] <= 0.0f)
	{
		return;
	}

	UI_ServerStatus_ExpandAnimTick();
	UI_ServerStatus_FullExpandAnimTick();
	UI_ServerStatus_BarRect640(&x, &y, &w, &h);
	UI_ServerStatus_AutoHideTick(label, x, w);
	hideOffset = ui_statusAutoHideFrac * (UI_STATUS_BAR_HEIGHT_COLLAPSED + 2.0f);
	y += hideOffset;
	UI_ServerStatus_DrawBarPanel(x, y, w, h, label, color);

	if (UI_ServerStatus_DrawExpandedVisual() && UI_ServerStatus_ScrollbarActive() &&
	        ui_statusScrollbarDrag)
	{
		if (trap_Key_IsDown(K_MOUSE1))
		{
			UI_ServerStatus_ScrollbarDragUpdate(x, y, w, h);
		}
		else
		{
			ui_statusScrollbarDrag = qfalse;
			ui_statusScrollbarDragGrab = 0.0f;
		}
	}
}

qboolean UI_ServerStatus_HandleBarClick(int key)
{
	const char*     label;
	vec4_t          color;
	float           x;
	float           y;
	float           w;
	float           h;
	float           triggerY;

	if (key != K_MOUSE1)
	{
		return qfalse;
	}

	if (!ui_statusFetchAttempted)
	{
		return qfalse;
	}

	UI_ServerStatus_GetBarMessage(&label, color);
	if (!label || color[3] <= 0.0f)
	{
		return qfalse;
	}

	UI_ServerStatus_ExpandAnimTick();
	UI_ServerStatus_FullExpandAnimTick();
	UI_ServerStatus_BarRect640(&x, &y, &w, &h);

	if (ui_statusBarExpanded || ui_statusExpandFrac > 0.02f)
	{
		float           btnRowY;
		float           headerCloseMax;

		if (UI_ServerStatus_HandleScrollbarMouse(x, y, w, h, qtrue))
		{
			return qtrue;
		}

		btnRowY = y + UI_STATUS_BAR_HEIGHT_COLLAPSED;

		if (UI_ServerStatus_FullChangelogButtonHit(x, w, btnRowY,
		        UI_STATUS_BAR_HEIGHT_COLLAPSED))
		{
			if (ui_statusFullChangelogOpen || ui_statusFullExpandFrac > 0.001f)
			{
				UI_ServerStatus_CloseFullChangelog();
				trap_S_StartLocalSound(menu_move_sound, CHAN_LOCAL_SOUND);
			}
			else
			{
				UI_ServerStatus_LoadFullChangelog();
				if (ui_statusFullChangelogReady)
				{
					ui_statusFullChangelogOpen = qtrue;
					ui_statusChangelogScroll = 0.0f;
					ui_statusFullExpandLastMs = 0;
					UI_ServerStatus_FullChangelogApplyFirstVersionSticky();
					trap_S_StartLocalSound(menu_move_sound, CHAN_LOCAL_SOUND);
				}
			}
			return qtrue;
		}

		if (uis.cursorx < x || uis.cursorx > x + w ||
		        uis.cursory < y || uis.cursory > y + h)
		{
			UI_ServerStatus_SetExpanded(qfalse);
		}
		else
		{
			headerCloseMax = y + UI_STATUS_BAR_HEIGHT_COLLAPSED;
			if (uis.cursorx >= x && uis.cursorx <= x + w &&
			        uis.cursory >= y && uis.cursory <= headerCloseMax)
			{
				UI_ServerStatus_SetExpanded(qfalse);
			}
		}

		return qtrue;
	}

	/* If hidden/animating, click in trigger zone should open expanded panel. */
	triggerY = 480.0f - UI_STATUS_AUTOHIDE_TRIGGER_H;
	if (ui_statusAutoHideFrac > 0.0f && uis.cursorx >= x && uis.cursorx <= x + w)
	{
		if (uis.cursory >= triggerY && uis.cursory <= 480.0f)
		{
			ui_statusAutoHideFrac = 0.0f;
			ui_statusAutoHideStartMs = -1;
			ui_statusAutoHideLabel = NULL;
			UI_ServerStatus_EnsureChangelogForBarExpand();
			UI_ServerStatus_SetExpanded(qtrue);
			return qtrue;
		}
	}

	/* Normal click when visible. */
	if (uis.cursorx >= x && uis.cursorx <= x + w &&
	        uis.cursory >= y && uis.cursory <= y + h)
	{
		UI_ServerStatus_EnsureChangelogForBarExpand();
		UI_ServerStatus_SetExpanded(qtrue);
		return qtrue;
	}

	return qfalse;
}

qboolean UI_ServerStatus_HandleModalKey(int key)
{
	if (!UI_ServerStatus_IsModal())
	{
		return qfalse;
	}

	if (key == K_ESCAPE)
	{
		if (ui_statusFullChangelogOpen || ui_statusFullExpandFrac > 0.001f)
		{
			UI_ServerStatus_CloseFullChangelog();
		}
		else
		{
			UI_ServerStatus_SetExpanded(qfalse);
		}
		return qtrue;
	}

	if (key == K_MOUSE1)
	{
		if (UI_ServerStatus_HandleBarClick(key))
		{
			return qtrue;
		}
		return qfalse;
	}

	if (key == K_MWHEELUP)
	{
		UI_ServerStatus_ScrollByDelta(-UI_ServerStatus_ScrollWheelStep());
		return qtrue;
	}

	if (key == K_MWHEELDOWN)
	{
		UI_ServerStatus_ScrollByDelta(UI_ServerStatus_ScrollWheelStep());
		return qtrue;
	}

	if (key == K_UPARROW || key == K_PGUP)
	{
		UI_ServerStatus_ScrollByDelta(-UI_STATUS_SCROLL_WHEEL_STEP);
		return qtrue;
	}

	if (key == K_DOWNARROW || key == K_PGDN)
	{
		UI_ServerStatus_ScrollByDelta(UI_STATUS_SCROLL_WHEEL_STEP);
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_ServerStatus_SessionAlreadyChecked(void)
{
	trap_Cvar_Update(&ui_beStatusChecked);
	if (ui_beStatusChecked.integer != 0)
	{
		if (ui_statusFetchAttempted)
		{
			return qtrue;
		}
		/* Cvar outlived UI reload (e.g. vid_restart); cached bar state is gone */
		trap_Cvar_Set("ui_beStatusChecked", "0");
		return qfalse;
	}

	if (!ui_statusFetchAttempted)
	{
		return qfalse;
	}

	if (!uiServerStatus.received && !ui_statusOffline && !ui_statusBadKey)
	{
		return qtrue;
	}

	return qfalse;
}

void UI_ServerStatus_NotifyFetchCycleComplete(void)
{
	if (!uiServerStatus.received)
	{
		return;
	}
	if (UI_ServerStatus_WantsChangelog() && !uiServerStatus.changelogReady)
	{
		return;
	}

	trap_Cvar_Set("ui_beStatusChecked", "1");
}

static void UI_ServerStatus_FetchPrepare(void)
{
	ui_statusFetchAttempted = qtrue;
	ui_statusOffline = qfalse;
	ui_statusBadKey = qfalse;
	uiServerStatus.valid = qfalse;
	uiServerStatus.received = qfalse;
	UI_ServerStatus_ClearChangelog();
	if (ui_statusBarExpanded)
	{
		UI_ServerStatus_SetExpanded(qfalse);
	}
}

void UI_ServerStatus_FetchOnMainMenu(void)
{
	UI_ServerStatus_EnsureFonts();
	if (UI_ServerStatus_SessionAlreadyChecked())
	{
		return;
	}

	UI_ServerStatus_FetchPrepare();
	UI_ServerStatus_Secheck2BeginDual(UI_BeinfoCfg_GetStatusServerPrimary(),
	                                  UI_BeinfoCfg_GetStatusServerAlternate());
}

void UI_ServerStatus_OnFetchTimeout(void)
{
	ui_statusOffline = qtrue;
	ui_statusBadKey = qfalse;
	UI_ServerStatus_EnsureLocalChangelogForOffline();
}

static qboolean UI_ServerStatus_ParseValidatedGetstatus(const char* status, char* infostring,
        int infostringSize)
{
	if (!status || !status[0] || !infostring || infostringSize <= 0)
	{
		return qfalse;
	}

	if (!UI_StatusSign_VerifyInfo(status, NULL, 0, NULL, 0))
	{
		return qfalse;
	}

	UI_StatusSign_CopyInfostringOnly(status, infostring, infostringSize);
	return (qboolean)(infostring[0] != '\0');
}

void UI_ServerStatus_ApplyFromGetstatus(const char* status)
{
	char            infostring[MAX_INFO_STRING];

	if (!UI_ServerStatus_ParseValidatedGetstatus(status, infostring, sizeof(infostring)))
	{
		ui_statusBadKey = qtrue;
		ui_statusOffline = qfalse;
		return;
	}

	UI_ServerStatus_StoreFromValidatedInfo(infostring);
}

typedef struct
{
	int         major;
	int         minor;
	char        suffix[16];
	int         channel;
	int         channelNum;
} ui_beVersionParsed_t;

#define UI_BE_VER_CHANNEL_NONE  0
#define UI_BE_VER_CHANNEL_ALPHA 1
#define UI_BE_VER_CHANNEL_BETA  2
#define UI_BE_VER_CHANNEL_REL   3

static void UI_ServerStatus_NormalizeBeVersion(const char* in, char* out, int outSize)
{
	const char*     s;
	char*           d;
	int             remain;
	int             c;

	if (!out || outSize < 1)
	{
		return;
	}

	out[0] = '\0';
	if (!in)
	{
		return;
	}

	s = in;
	while (*s == ' ')
	{
		s++;
	}

	if (!Q_strncmp(s, "be-", 3))
	{
		s += 3;
	}
	else if (!Q_stricmpn(s, "be", 2))
	{
		s += 2;
		while (*s == '-')
		{
			s++;
		}
	}

	while (*s == ' ')
	{
		s++;
	}

	if (*s == 'v' || *s == 'V')
	{
		s++;
	}

	d = out;
	remain = outSize - 1;
	while (*s && remain > 0)
	{
		c = *s++;
		if (c >= 'A' && c <= 'Z')
		{
			c = c + ('a' - 'A');
		}
		*d++ = (char)c;
		remain--;
	}
	*d = '\0';
}

static qboolean UI_ServerStatus_ParseBeVersion(const char* norm, ui_beVersionParsed_t* v)
{
	const char*     s;
	int             n;
	int             i;

	if (!v)
	{
		return qfalse;
	}

	memset(v, 0, sizeof(*v));
	v->channel = UI_BE_VER_CHANNEL_REL;
	if (!norm || !norm[0])
	{
		return qfalse;
	}

	s = norm;
	n = 0;
	while (*s >= '0' && *s <= '9')
	{
		n = n * 10 + (*s - '0');
		s++;
	}
	v->major = n;

	if (*s != '.')
	{
		return qtrue;
	}
	s++;

	n = 0;
	while (*s >= '0' && *s <= '9')
	{
		n = n * 10 + (*s - '0');
		s++;
	}
	v->minor = n;

	i = 0;
	while (*s && *s != '-' && i < (int)sizeof(v->suffix) - 1)
	{
		if ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z'))
		{
			v->suffix[i++] = *s++;
		}
		else
		{
			break;
		}
	}
	v->suffix[i] = '\0';

	if (*s == '-')
	{
		s++;
		if (!Q_strncmp(s, "alpha", 5))
		{
			v->channel = UI_BE_VER_CHANNEL_ALPHA;
			s += 5;
		}
		else if (!Q_strncmp(s, "beta", 4))
		{
			v->channel = UI_BE_VER_CHANNEL_BETA;
			s += 4;
		}
		else
		{
			return qtrue;
		}

		n = 0;
		while (*s >= '0' && *s <= '9')
		{
			n = n * 10 + (*s - '0');
			s++;
		}
		v->channelNum = (n > 0) ? n : 1;
	}

	return qtrue;
}

static int UI_ServerStatus_CompareSuffix(const char* a, const char* b)
{
	if (!a)
	{
		a = "";
	}
	if (!b)
	{
		b = "";
	}

	while (*a && *b)
	{
		if (*a != *b)
		{
			return (*a < *b) ? -1 : 1;
		}
		a++;
		b++;
	}

	if (*a)
	{
		return 1;
	}
	if (*b)
	{
		return -1;
	}

	return 0;
}

static int UI_ServerStatus_CompareBeVersionNormalized(const char* normA, const char* normB)
{
	ui_beVersionParsed_t    a;
	ui_beVersionParsed_t    b;
	int                     r;

	UI_ServerStatus_ParseBeVersion(normA, &a);
	UI_ServerStatus_ParseBeVersion(normB, &b);

	if (a.major != b.major)
	{
		return a.major - b.major;
	}
	if (a.minor != b.minor)
	{
		return a.minor - b.minor;
	}

	r = UI_ServerStatus_CompareSuffix(a.suffix, b.suffix);
	if (r != 0)
	{
		return r;
	}

	if (a.channel != b.channel)
	{
		return a.channel - b.channel;
	}

	return a.channelNum - b.channelNum;
}

static int UI_ServerStatus_CompareBeVersion(const char* a, const char* b)
{
	char    na[64];
	char    nb[64];

	UI_ServerStatus_NormalizeBeVersion(a, na, sizeof(na));
	UI_ServerStatus_NormalizeBeVersion(b, nb, sizeof(nb));
	return UI_ServerStatus_CompareBeVersionNormalized(na, nb);
}

static const char* UI_ServerStatus_LocalVersionCompareString(void)
{
	const char*     v;

	v = XSTRINGIFY(OSP_VERSION);
	if (!Q_strncmp(v, "be-", 3))
	{
		return v + 3;
	}
	return v;
}

void UI_ServerStatus_ApplyFromGetinfo(const char* info)
{
	if (!info || !info[0])
	{
		return;
	}

	if (uiServerStatus.last || !UI_ServerStatus_WantsChangelog())
	{
		return;
	}

	UI_ServerStatus_ClearChangelog();
	if (UI_ServerStatus_ParseChangelogFromGetinfo(info))
	{
		uiServerStatus.changelogReady = qtrue;
	}
}

static void UI_ServerStatus_StoreFromValidatedInfo(const char* infostring)
{
	const char*     localVersion;

	Q_strncpyz(uiServerStatus.version, Info_ValueForKey(infostring, "version"),
	           sizeof(uiServerStatus.version));
	uiServerStatus.flags = atoi(Info_ValueForKey(infostring, "flags"));
	uiServerStatus.sivj = atoi(Info_ValueForKey(infostring, "sivj"));
	uiServerStatus.date = atoi(Info_ValueForKey(infostring, "date"));
	Q_strncpyz(uiServerStatus.hfv, Info_ValueForKey(infostring, "hfv"), sizeof(uiServerStatus.hfv));
	Q_strncpyz(uiServerStatus.titleText, Info_ValueForKey(infostring, "TitleText"),
	           sizeof(uiServerStatus.titleText));

	{
		int     cmp;

		localVersion = UI_ServerStatus_LocalVersionCompareString();
		cmp = UI_ServerStatus_CompareBeVersion(localVersion, uiServerStatus.version);
		uiServerStatus.last = (qboolean)(cmp == 0);
		uiServerStatus.unreleased = (qboolean)(cmp > 0);
	}

	UI_ServerStatus_ClearChangelog();
	if (uiServerStatus.last)
	{
		UI_ServerStatus_LoadLocalChangelog();
	}

	ui_statusOffline = qfalse;
	uiServerStatus.valid = qtrue;
	uiServerStatus.received = qtrue;
}
