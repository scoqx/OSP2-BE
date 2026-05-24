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
#define UI_STATUS_BAR_MIN_EXPANDED      120.0f
#define UI_STATUS_BAR_MAX_EXPANDED      420.0f
#define UI_STATUS_CHANGELOG_FILE        "version_changelog.txt"
#define UI_STATUS_CHANGELOG_READ_MAX    8192
#define UI_STATUS_CHAR_WIDTH            8.0f
#define UI_STATUS_CHAR_HEIGHT           14.0f
#define UI_STATUS_PANEL_PAD             6.0f
#define UI_STATUS_BODY_EXTRA_PAD        6.0f
#define UI_STATUS_SCROLLBAR_WIDTH       8.0f
#define UI_STATUS_SCROLLBAR_GAP         1.0f
#define UI_STATUS_SCROLL_WHEEL_STEP     (UI_STATUS_CHAR_HEIGHT * 3.0f)
#define UI_STATUS_UPDATE_DATE_LABEL     "Update date"
#define UI_STATUS_MENU_CURSOR_UNSET     (-2)

uiServerStatus_t    uiServerStatus;

static qboolean     ui_statusFetchAttempted;
static qboolean     ui_statusOffline;
static qboolean     ui_statusBarExpanded;
static qboolean     ui_statusFontReady;
static int          ui_statusSavedMenuCursor = UI_STATUS_MENU_CURSOR_UNSET;
static float        ui_statusChangelogScroll;
static qboolean     ui_statusScrollbarDrag;
static float        ui_statusScrollbarDragGrab;

static void UI_ServerStatus_StoreFromValidatedInfo(const char* infostring);
static const char* UI_ServerStatus_LocalVersionCompareString(void);
static void UI_ServerStatus_ClearChangelog(void);
static void UI_ServerStatus_LoadLocalChangelog(void);
static float UI_ServerStatus_ExpandedHeight640(float panelW);
static float UI_ServerStatus_ChangelogContentHeight(float textW);
static qboolean UI_ServerStatus_ChangelogLayout(float panelW, float bodyH, float* outTextW,
        qboolean* outNeedScroll);
static float UI_ServerStatus_DrawMultilineClipped(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom);
static void UI_ServerStatus_DrawText(float x, float y, const char* text, vec4_t color, int flags);
static void UI_ServerStatus_DrawTextWidth(float x, float y, const char* text, vec4_t color,
        int flags, int maxWidth640);
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
static qboolean UI_ServerStatus_HasUpdateHeaderRow(void);
static float UI_ServerStatus_ExpandedHeadersHeight(void);
static void UI_ServerStatus_PanelBodyRect(float panelX, float panelY, float panelW, float panelH,
        float* outBodyY, float* outBodyH);
static void UI_ServerStatus_DrawHeaderRow(float x, float y, float w, float rowH,
        const char* leftText, vec4_t leftColor, const char* rightText, vec4_t rightColor);

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

static void UI_ServerStatus_SetExpanded(qboolean expanded)
{
	if (ui_statusBarExpanded == expanded)
	{
		return;
	}

	if (expanded)
	{
		ui_statusChangelogScroll = 0.0f;
		ui_statusScrollbarDrag = qfalse;
		ui_statusScrollbarDragGrab = 0.0f;
		UI_ServerStatus_MenuModalEnter();
	}
	else
	{
		ui_statusChangelogScroll = 0.0f;
		ui_statusScrollbarDrag = qfalse;
		ui_statusScrollbarDragGrab = 0.0f;
		UI_ServerStatus_MenuModalLeave();
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
	float   totalH;
	float   y;

	*outW = UI_STATUS_BAR_WIDTH;
	*outH = UI_STATUS_BAR_HEIGHT;
	*outX = (640.0f - UI_STATUS_BAR_WIDTH) * 0.5f;
	y = 480.0f - UI_STATUS_BAR_HEIGHT;

	if (ui_statusBarExpanded)
	{
		totalH = UI_ServerStatus_ExpandedHeight640(UI_STATUS_BAR_WIDTH);
		y = 480.0f - totalH;
		*outH = totalH;
	}

	*outY = y;
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

static qboolean UI_ServerStatus_HasUpdateHeaderRow(void)
{
	if (!uiServerStatus.changelogReady)
	{
		return qfalse;
	}

	if (uiServerStatus.changelogUpdateDate[0] || uiServerStatus.changelogNewVersion[0])
	{
		return qtrue;
	}

	return qfalse;
}

static float UI_ServerStatus_ExpandedHeadersHeight(void)
{
	float           h;

	h = UI_STATUS_BAR_HEIGHT;
	if (UI_ServerStatus_HasUpdateHeaderRow())
	{
		h += UI_STATUS_BAR_HEIGHT;
	}
	return h;
}

static void UI_ServerStatus_PanelBodyRect(float panelX, float panelY, float panelW, float panelH,
        float* outBodyY, float* outBodyH)
{
	float           headersH;

	headersH = UI_ServerStatus_ExpandedHeadersHeight();
	*outBodyY = panelY + headersH + UI_STATUS_PANEL_PAD + UI_STATUS_BODY_EXTRA_PAD;
	*outBodyH = panelH - headersH - UI_STATUS_PANEL_PAD * 2.0f - UI_STATUS_BODY_EXTRA_PAD;
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

	if (!uiServerStatus.received || ui_statusOffline || !uiServerStatus.valid)
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

static float UI_ServerStatus_ChangelogContentHeight(float textW)
{
	int             lines;

	if (!uiServerStatus.changelogReady)
	{
		return 0.0f;
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
	float           textW;
	float           contentH;
	qboolean        needScroll;

	textW = panelW - UI_STATUS_PANEL_PAD * 2.0f;
	contentH = UI_ServerStatus_ChangelogContentHeight(textW);
	needScroll = (contentH > bodyH + 0.5f);

	if (needScroll)
	{
		textW = panelW - UI_STATUS_PANEL_PAD * 2.0f - UI_STATUS_SCROLLBAR_WIDTH -
		        UI_STATUS_SCROLLBAR_GAP;
		if (textW < UI_STATUS_CHAR_WIDTH * 8.0f)
		{
			textW = UI_STATUS_CHAR_WIDTH * 8.0f;
		}
		contentH = UI_ServerStatus_ChangelogContentHeight(textW);
		needScroll = (contentH > bodyH + 0.5f);
	}

	*outTextW = textW;
	*outNeedScroll = needScroll;
	return needScroll;
}

static void UI_ServerStatus_ClampChangelogScroll(float bodyH, float contentH)
{
	float           maxScroll;

	maxScroll = contentH - bodyH;
	if (maxScroll < 0.0f)
	{
		maxScroll = 0.0f;
	}

	if (ui_statusChangelogScroll < 0.0f)
	{
		ui_statusChangelogScroll = 0.0f;
	}
	if (ui_statusChangelogScroll > maxScroll)
	{
		ui_statusChangelogScroll = maxScroll;
	}
}

static void UI_ServerStatus_ScrollChangelog(float delta, float bodyH, float contentH)
{
	ui_statusChangelogScroll += delta;
	UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);
}

static float UI_ServerStatus_ExpandedHeight640(float panelW)
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
	contentH = UI_ServerStatus_ChangelogContentHeight(textW);

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

static float UI_ServerStatus_DrawMultilineClipped(float x, float y, float maxW, const char* text,
        vec4_t color, int flags, float clipTop, float clipBottom)
{
	char            lineBuf[256];
	const char*     p;
	const char*     lineStart;
	const char*     lineEnd;
	int             chunk;
	int             remain;
	float           lineY;
	float           startY;
	float           lineTop;
	float           lineBottom;
	int             maxWidthPx;

	if (!text || !text[0] || !ui_statusFontReady)
	{
		return 0.0f;
	}

	maxWidthPx = (int)maxW;
	if (maxWidthPx < 1)
	{
		maxWidthPx = 1;
	}

	startY = y;
	lineY = y;
	p = text;
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
				memcpy(lineBuf, lineStart, chunk);
				lineBuf[chunk] = '\0';
				UI_ServerStatus_DrawTextWidth(x, lineY, lineBuf, color, flags, maxWidthPx);
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
	UI_DrawRect(x, y, w, h, colorWhite);

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
	float           sbX;
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
	UI_ServerStatus_ChangelogLayout(panelW, bodyH, &textW, &needScroll);
	if (!needScroll)
	{
		ui_statusScrollbarDrag = qfalse;
		return;
	}

	contentH = UI_ServerStatus_ChangelogContentHeight(textW);
	sbH = bodyH;
	sbY = bodyY;
	sbX = panelX + panelW - UI_STATUS_PANEL_PAD - UI_STATUS_SCROLLBAR_WIDTH;
	my = uis.cursory;

	UI_ServerStatus_ScrollbarGetThumb(sbH, contentH, ui_statusChangelogScroll, &thumbH, &thumbY,
	                                  &maxScroll);
	trackRange = sbH - thumbH;
	if (trackRange < 1.0f || maxScroll < 1.0f)
	{
		return;
	}

	if (uis.cursorx < sbX || uis.cursorx > sbX + UI_STATUS_SCROLLBAR_WIDTH)
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

	if (!ui_statusBarExpanded || !uiServerStatus.changelogReady || !pressed)
	{
		return qfalse;
	}

	UI_ServerStatus_PanelBodyRect(panelX, panelY, panelW, panelH, &bodyY, &bodyH);
	UI_ServerStatus_ChangelogLayout(panelW, bodyH, &textW, &needScroll);
	if (!needScroll)
	{
		ui_statusScrollbarDrag = qfalse;
		return qfalse;
	}

	contentH = UI_ServerStatus_ChangelogContentHeight(textW);
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

	if (!ui_statusBarExpanded || !uiServerStatus.changelogReady)
	{
		return;
	}

	UI_ServerStatus_BarRect640(&x, &y, &w, &h);
	UI_ServerStatus_PanelBodyRect(x, y, w, h, &bodyY, &bodyH);
	UI_ServerStatus_ChangelogLayout(w, bodyH, &textW, &needScroll);
	if (!needScroll)
	{
		return;
	}

	contentH = UI_ServerStatus_ChangelogContentHeight(textW);
	UI_ServerStatus_ScrollChangelog(delta, bodyH, contentH);
}

static void UI_ServerStatus_DrawTextWidth(float x, float y, const char* text, vec4_t color,
        int flags, int maxWidth640)
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
	                    UI_STATUS_CHAR_WIDTH, UI_STATUS_CHAR_HEIGHT,
	                    maxWidth640, flags | DS_SHADOW | DS_PROPORTIONAL, NULL, NULL, NULL);
}

static void UI_ServerStatus_DrawText(float x, float y, const char* text, vec4_t color, int flags)
{
	UI_ServerStatus_DrawTextWidth(x, y, text, color, flags, SCREEN_WIDTH);
}

static void UI_ServerStatus_DrawHeaderRow(float x, float y, float w, float rowH,
        const char* leftText, vec4_t leftColor, const char* rightText, vec4_t rightColor)
{
	float           rowCenterY;

	UI_DrawRect(x, y, w, rowH, colorWhite);
	rowCenterY = y + rowH * 0.5f;

	if (leftText && leftText[0])
	{
		UI_ServerStatus_DrawText(x + UI_STATUS_PANEL_PAD, rowCenterY, leftText, leftColor,
		                         DS_VCENTER);
	}

	if (rightText && rightText[0])
	{
		UI_ServerStatus_DrawText(x + w - UI_STATUS_PANEL_PAD, rowCenterY, rightText, rightColor,
		                         DS_VCENTER | DS_HRIGHT);
	}
}

static void UI_ServerStatus_DrawStatusRow(float x, float y, float w, float rowH,
        const char* statusLabel, vec4_t statusColor)
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

	UI_ServerStatus_DrawHeaderRow(x, y, w, rowH, statusLabel, statusColor, verBuf, verColor);
}

static void UI_ServerStatus_DrawUpdateHeaderRow(float x, float y, float w, float rowH)
{
	char            verBuf[64];
	const char*     leftText;
	vec4_t          dateColor;
	vec4_t          verColor;

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

	dateColor[0] = 1.0f;
	dateColor[1] = 1.0f;
	dateColor[2] = 1.0f;
	dateColor[3] = 1.0f;

	verColor[0] = 0.2f;
	verColor[1] = 0.85f;
	verColor[2] = 0.2f;
	verColor[3] = 1.0f;

	UI_ServerStatus_DrawHeaderRow(x, y, w, rowH, leftText, dateColor, verBuf, verColor);
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

	return (qboolean)(!uiServerStatus.last);
}

static void UI_ServerStatus_GetBarMessage(const char** outText, vec4_t outColor)
{
	if (!ui_statusFetchAttempted)
	{
		*outText = NULL;
		outColor[3] = 0.0f;
		return;
	}

	if (!uiServerStatus.received || ui_statusOffline || !uiServerStatus.valid)
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

	if (!ui_statusBarExpanded || !ui_statusFetchAttempted)
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

	if (ui_statusBarExpanded)
	{
		dim[0] = 0.0f;
		dim[1] = 0.0f;
		dim[2] = 0.0f;
		dim[3] = 0.45f;
		UI_FillRect(uis.screenXmin, uis.screenYmin,
		            uis.screenXmax - uis.screenXmin,
		            uis.screenYmax - uis.screenYmin, dim);
	}

	bg[0] = 0.0f;
	bg[1] = 0.0f;
	bg[2] = 0.0f;
	bg[3] = 0.75f;
	UI_FillRect(x, y, w, h, bg);
	UI_DrawRect(x, y, w, h, colorWhite);

	textColor[0] = color[0];
	textColor[1] = color[1];
	textColor[2] = color[2];
	textColor[3] = 1.0f;

	rowY = y;

	if (ui_statusBarExpanded)
	{
		float           bodyY;
		float           bodyClipTop;
		float           bodyClipBottom;
		float           textW;
		float           contentH;
		float           contentY;
		qboolean        needScroll;

		UI_ServerStatus_DrawStatusRow(x, rowY, w, UI_STATUS_BAR_HEIGHT, label, textColor);
		rowY += UI_STATUS_BAR_HEIGHT;

		if (UI_ServerStatus_HasUpdateHeaderRow())
		{
			UI_ServerStatus_DrawUpdateHeaderRow(x, rowY, w, UI_STATUS_BAR_HEIGHT);
			rowY += UI_STATUS_BAR_HEIGHT;
		}

		UI_ServerStatus_PanelBodyRect(x, y, w, h, &bodyY, &bodyH);
		bodyClipTop = bodyY;
		bodyClipBottom = bodyY + bodyH;

		if (bodyH > UI_STATUS_CHAR_HEIGHT && uiServerStatus.changelogReady)
		{
			UI_ServerStatus_ChangelogLayout(w, bodyH, &textW, &needScroll);
			contentH = UI_ServerStatus_ChangelogContentHeight(textW);
			UI_ServerStatus_ClampChangelogScroll(bodyH, contentH);

			contentY = bodyY - ui_statusChangelogScroll;

			if (uiServerStatus.changelogBody[0])
			{
				UI_ServerStatus_DrawMultilineClipped(x + UI_STATUS_PANEL_PAD, contentY, textW,
				                                    uiServerStatus.changelogBody, text_color_normal,
				                                    DS_HLEFT, bodyClipTop, bodyClipBottom);
			}

			if (needScroll)
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
		UI_ServerStatus_DrawStatusRow(x, rowY, w, UI_STATUS_BAR_HEIGHT, label, textColor);
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

	UI_ServerStatus_BarRect640(&x, &y, &w, &h);
	UI_ServerStatus_DrawBarPanel(x, y, w, h, label, color);

	if (ui_statusBarExpanded && ui_statusScrollbarDrag)
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

	UI_ServerStatus_BarRect640(&x, &y, &w, &h);

	if (ui_statusBarExpanded)
	{
		if (UI_ServerStatus_HandleScrollbarMouse(x, y, w, h, qtrue))
		{
			return qtrue;
		}

		if (uis.cursorx < x || uis.cursorx > x + w ||
		        uis.cursory < y || uis.cursory > y + h)
		{
			UI_ServerStatus_SetExpanded(qfalse);
		}
		else if (uis.cursorx >= x && uis.cursorx <= x + w &&
		         uis.cursory >= y && uis.cursory <= y + UI_STATUS_BAR_HEIGHT)
		{
			UI_ServerStatus_SetExpanded(qfalse);
		}

		return qtrue;
	}

	if (uis.cursorx >= x && uis.cursorx <= x + w &&
	        uis.cursory >= y && uis.cursory <= y + h)
	{
		UI_ServerStatus_EnsureLocalChangelogForOffline();
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
		UI_ServerStatus_SetExpanded(qfalse);
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
		UI_ServerStatus_ScrollByDelta(-UI_STATUS_SCROLL_WHEEL_STEP);
		return qtrue;
	}

	if (key == K_MWHEELDOWN)
	{
		UI_ServerStatus_ScrollByDelta(UI_STATUS_SCROLL_WHEEL_STEP);
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

void UI_ServerStatus_Clear(void)
{
	memset(&uiServerStatus, 0, sizeof(uiServerStatus));
	UI_ServerStatus_ClearChangelog();
	ui_statusFetchAttempted = qfalse;
	ui_statusOffline = qfalse;
	if (ui_statusBarExpanded)
	{
		UI_ServerStatus_SetExpanded(qfalse);
	}
	else
	{
		ui_statusBarExpanded = qfalse;
		ui_statusSavedMenuCursor = UI_STATUS_MENU_CURSOR_UNSET;
	}
}

static void UI_ServerStatus_FetchPrepare(void)
{
	ui_statusFetchAttempted = qtrue;
	ui_statusOffline = qfalse;
	uiServerStatus.valid = qfalse;
	uiServerStatus.received = qfalse;
	UI_ServerStatus_ClearChangelog();
	if (ui_statusBarExpanded)
	{
		UI_ServerStatus_SetExpanded(qfalse);
	}
}

void UI_ServerStatus_FetchBegin(const char* target)
{
	if (!target || !target[0])
	{
		return;
	}

	UI_ServerStatus_FetchPrepare();
	UI_LANJob_BeginServerStatus(target);
}

void UI_ServerStatus_FetchOnMainMenu(void)
{
	const char*     primary;
	const char*     alternate;

	UI_ServerStatus_EnsureFonts();
	UI_ServerStatus_FetchPrepare();
	primary = UI_GameinfoCfg_GetStatusServerPrimary();
	alternate = UI_GameinfoCfg_GetStatusServerAlternate();
	if (!primary || !primary[0] || !alternate || !alternate[0])
	{
		return;
	}

	UI_LANJob_BeginServerStatusDual(primary, alternate);
}

void UI_ServerStatus_OnFetchTimeout(void)
{
	ui_statusOffline = qtrue;
	UI_ServerStatus_EnsureLocalChangelogForOffline();
}

qboolean UI_ServerStatus_ParseValidatedGetstatus(const char* status, char* infostring,
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

void UI_ServerStatus_ApplyValidatedInfostring(const char* infostring)
{
	if (!infostring || !infostring[0])
	{
		return;
	}

	UI_ServerStatus_StoreFromValidatedInfo(infostring);
}

void UI_ServerStatus_ApplyFromGetstatus(const char* status)
{
	char            infostring[MAX_INFO_STRING];

	if (!UI_ServerStatus_ParseValidatedGetstatus(status, infostring, sizeof(infostring)))
	{
		ui_statusOffline = qtrue;
		UI_ServerStatus_EnsureLocalChangelogForOffline();
		return;
	}

	UI_ServerStatus_StoreFromValidatedInfo(infostring);
}

static const char* UI_ServerStatus_LocalVersionCompareString(void)
{
	const char*     v;

	v = OSP_VERSION;
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

	localVersion = UI_ServerStatus_LocalVersionCompareString();
	uiServerStatus.last = (qboolean)(!strcmp(localVersion, uiServerStatus.version));

	UI_ServerStatus_ClearChangelog();
	if (uiServerStatus.last)
	{
		UI_ServerStatus_LoadLocalChangelog();
	}

	ui_statusOffline = qfalse;
	uiServerStatus.valid = qtrue;
	uiServerStatus.received = qtrue;
}
