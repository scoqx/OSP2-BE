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
// cg_printui.c -- printui console command and drawing functions

#include "cg_local.h"
#include "cg_superhud.h"

/*
=================
CG_ParseVec4FromArgs

Parse 4 float values from command arguments starting at index i
Returns number of arguments consumed (4 if successful, 0 if failed)
=================
*/
static int CG_ParseVec4FromArgs(int argc, int* i, vec4_t out)
{
	if (*i + 4 >= argc)
	{
		return 0;
	}

	(*i)++;
	out[0] = atof(CG_Argv(*i));
	(*i)++;
	out[1] = atof(CG_Argv(*i));
	(*i)++;
	out[2] = atof(CG_Argv(*i));
	(*i)++;
	out[3] = atof(CG_Argv(*i));

	return 4;
}

/*
=================
CG_ParseFloatFromArgs

Parse 1 float value from command arguments starting at index i
Returns 1 if successful, 0 if failed
=================
*/
static int CG_ParseFloatFromArgs(int argc, int* i, float* out)
{
	if (*i + 1 >= argc)
	{
		return 0;
	}

	(*i)++;
	*out = atof(CG_Argv(*i));

	return 1;
}

/*
=================
CG_ParseVec4FromTokens

Parse 4 float values from token string
Returns qtrue if successful, qfalse if failed
=================
*/
static qboolean CG_ParseVec4FromTokens(char** token, char** nextToken, vec4_t out)
{
	if (!*nextToken || !** nextToken)
	{
		return qfalse;
	}

	*token = *nextToken;
	*nextToken = strchr(*token, ' ');
	if (*nextToken)
	{
		*(*nextToken) = '\0';
		(*nextToken)++;
	}
	out[0] = atof(*token);

	if (!*nextToken || !** nextToken)
	{
		return qfalse;
	}
	*token = *nextToken;
	*nextToken = strchr(*token, ' ');
	if (*nextToken)
	{
		*(*nextToken) = '\0';
		(*nextToken)++;
	}
	out[1] = atof(*token);

	if (!*nextToken || !** nextToken)
	{
		return qfalse;
	}
	*token = *nextToken;
	*nextToken = strchr(*token, ' ');
	if (*nextToken)
	{
		*(*nextToken) = '\0';
		(*nextToken)++;
	}
	out[2] = atof(*token);

	if (!*nextToken || !** nextToken)
	{
		return qfalse;
	}
	*token = *nextToken;
	*nextToken = strchr(*token, ' ');
	if (*nextToken)
	{
		*(*nextToken) = '\0';
		(*nextToken)++;
	}
	out[3] = atof(*token);

	return qtrue;
}

/*
=================
CG_ParseFloatFromTokens

Parse 1 float value from token string
Returns qtrue if successful, qfalse if failed
=================
*/
static qboolean CG_ParseFloatFromTokens(char** token, char** nextToken, float* out)
{
	if (!*nextToken || !** nextToken)
	{
		return qfalse;
	}

	*token = *nextToken;
	*nextToken = strchr(*token, ' ');
	if (*nextToken)
	{
		*(*nextToken) = '\0';
		(*nextToken)++;
	}
	*out = atof(*token);

	return qtrue;
}

/*
=================
CG_ParsePrintUITemplate

Parse template string from cvar and apply parameters
=================
*/
static void CG_ParsePrintUITemplate(const char* templateStr, vec4_t textColor, vec4_t shadowColor, vec4_t background, vec4_t border, vec4_t borderColor, float* charWidth, float* charHeight, int* flags, int* fontIndex, float* duration, float* fadeTime, qboolean* hasBackground, qboolean* hasBorder, int* hAlign, float* x, float* y, qboolean* hasPos)
{
	char templateCopy[MAX_STRING_CHARS];
	char* token;
	char* nextToken;
	const char* colorStr;
	const char* shadowColorStr;
	char* xPos;

	if (!templateStr || !templateStr[0])
	{
		return; // Empty template
	}

	Q_strncpyz(templateCopy, templateStr, sizeof(templateCopy));

	token = templateCopy;
	while (token && *token)
	{
		// Skip leading spaces
		while (*token == ' ')
		{
			token++;
		}
		if (!*token)
		{
			break;
		}

		// Find next space
		nextToken = strchr(token, ' ');
		if (nextToken)
		{
			*nextToken = '\0';
			nextToken++;
		}

		// Color parsing (c prefix)
		if (token[0] == 'c' && token[1] != 's' && token[1] != '\0')
		{
			colorStr = token + 1;
			CG_ParseColorStr(colorStr, textColor);
		}
		// Shadow color parsing (cs prefix)
		else if (token[0] == 'c' && token[1] == 's' && token[2] != '\0')
		{
			shadowColorStr = token + 2;
			CG_ParseColorStr(shadowColorStr, shadowColor);
		}
		// Text size (10x14 format)
		else if (strchr(token, 'x') != NULL)
		{
			xPos = strchr(token, 'x');
			*xPos = '\0';
			*charWidth = atof(token);
			*charHeight = atof(xPos + 1);
			*xPos = 'x';
		}
		// Horizontal alignment
		else if (token[0] == 'h' && token[1] != '\0' && token[2] == '\0')
		{
			if (token[1] == 'R')
			{
				*hAlign = DS_HRIGHT;
			}
			else if (token[1] == 'L')
			{
				*hAlign = DS_HLEFT;
			}
			else if (token[1] == 'C')
			{
				*hAlign = DS_HCENTER;
			}
		}
		// Font
		else if (token[0] == 'f' && Q_stricmp(token, "font") == 0 && nextToken && *nextToken)
		{
			char* fontToken;
			fontToken = nextToken;
			nextToken = strchr(fontToken, ' ');
			if (nextToken)
			{
				*nextToken = '\0';
				nextToken++;
			}
			*fontIndex = CG_FontIndexFromString(fontToken);
			if (*fontIndex < 0)
			{
				*fontIndex = 2; // Default to sansman
			}
		}
		// Border size
		else if (token[0] == 'b' && token[1] == '\0')
		{
			if (CG_ParseVec4FromTokens(&token, &nextToken, border))
			{
				*hasBorder = qtrue;
			}
		}
		// Border color
		else if (token[0] == 'b' && token[1] == 'c' && token[2] == '\0')
		{
			CG_ParseVec4FromTokens(&token, &nextToken, borderColor);
		}
		// Background color
		else if (token[0] == 'b' && token[1] == 'g' && token[2] == '\0')
		{
			if (CG_ParseVec4FromTokens(&token, &nextToken, background))
			{
				*hasBackground = qtrue;
			}
		}
		// Duration
		else if (token[0] == 't' && Q_stricmp(token, "time") == 0 && nextToken && *nextToken)
		{
			char* timeValueToken;
			char* savedNextToken;

			timeValueToken = nextToken;
			savedNextToken = nextToken;
			nextToken = strchr(timeValueToken, ' ');
			if (nextToken)
			{
				*nextToken = '\0';
				nextToken++;
			}

			// Parse time values: 1/s = 3.5s, 2/m = 10s, 3/l = 60s, u/4 = unlimited
			if (Q_stricmp(timeValueToken, "1") == 0 || Q_stricmp(timeValueToken, "s") == 0)
			{
				*duration = 3.5f; // Short: 3.5 seconds
			}
			else if (Q_stricmp(timeValueToken, "2") == 0 || Q_stricmp(timeValueToken, "m") == 0)
			{
				*duration = 10.0f; // Medium: 10 seconds
			}
			else if (Q_stricmp(timeValueToken, "3") == 0 || Q_stricmp(timeValueToken, "l") == 0)
			{
				*duration = 60.0f; // Long: 60 seconds
			}
			else if (Q_stricmp(timeValueToken, "u") == 0 || Q_stricmp(timeValueToken, "4") == 0 || Q_stricmp(timeValueToken, "unlimited") == 0)
			{
				*duration = -1.0f; // Unlimited
			}
			else
			{
				// Default to short if invalid value
				*duration = 3.5f;
			}
		}
		// Fade time
		else if (token[0] == 'f' && (Q_stricmp(token, "fadeTime") == 0 || Q_stricmp(token, "fade") == 0) && nextToken && *nextToken)
		{
			char* fadeTimeValueToken;

			fadeTimeValueToken = nextToken;
			nextToken = strchr(fadeTimeValueToken, ' ');
			if (nextToken)
			{
				*nextToken = '\0';
				nextToken++;
			}

			// Parse fadeTime values: 0 = no fade, 1/s = 0.25s, 2/m = 0.5s, 3/l = 1s
			if (Q_stricmp(fadeTimeValueToken, "0") == 0)
			{
				*fadeTime = 0.0f; // No fade
			}
			else if (Q_stricmp(fadeTimeValueToken, "1") == 0 || Q_stricmp(fadeTimeValueToken, "s") == 0)
			{
				*fadeTime = 0.25f; // Short: 0.25 seconds
			}
			else if (Q_stricmp(fadeTimeValueToken, "2") == 0 || Q_stricmp(fadeTimeValueToken, "m") == 0)
			{
				*fadeTime = 0.5f; // Medium: 0.5 seconds
			}
			else if (Q_stricmp(fadeTimeValueToken, "3") == 0 || Q_stricmp(fadeTimeValueToken, "l") == 0)
			{
				*fadeTime = 1.0f; // Long: 1 second
			}
			else
			{
				// Default to no fade if invalid value
				*fadeTime = 0.0f;
			}
		}
		// Position (pos x y)
		// Only set position from template if it hasn't been set from command arguments
		else if (token[0] == 'p' && Q_stricmp(token, "pos") == 0 && !*hasPos)
		{
			if (CG_ParseFloatFromTokens(&token, &nextToken, x))
			{
				if (CG_ParseFloatFromTokens(&token, &nextToken, y))
				{
					*hasPos = qtrue;
				}
			}
		}
		// Position as two numbers (x y) - check if current token is a number and next is also a number
		// Only if position hasn't been set and token is not a command
		// This check must be after all command checks, but before flags
		else if (!*hasPos && nextToken && *nextToken)
		{
			// Check if current token looks like a number (not a command)
			qboolean isNumber;
			char* testNextToken;

			isNumber = qfalse;
			// Check if it's not a known command
			if (token[0] != 'c' && token[0] != 'h' && token[0] != 'f' &&
			        token[0] != 'b' && token[0] != 't' && token[0] != 'p' &&
			        Q_stricmp(token, "font") != 0 && Q_stricmp(token, "pos") != 0)
			{
				// Check if it looks like a number
				if ((token[0] >= '0' && token[0] <= '9') ||
				        token[0] == '+' || token[0] == '-' || token[0] == '.')
				{
					isNumber = qtrue;
				}
			}

			// Check if next token is also a number
			if (isNumber)
			{
				testNextToken = nextToken;
				// Skip spaces
				while (*testNextToken == ' ')
				{
					testNextToken++;
				}
				if (*testNextToken)
				{
					// Check if it's not a command
					if (testNextToken[0] != 'c' && testNextToken[0] != 'h' && testNextToken[0] != 'f' &&
					        testNextToken[0] != 'b' && testNextToken[0] != 't' && testNextToken[0] != 'p' &&
					        Q_stricmpn(testNextToken, "font", 4) != 0 && Q_stricmpn(testNextToken, "pos", 3) != 0 &&
					        Q_stricmpn(testNextToken, "time", 4) != 0 && Q_stricmpn(testNextToken, "fadeTime", 8) != 0 && Q_stricmpn(testNextToken, "fade", 4) != 0)
					{
						// Check if it looks like a number
						if ((testNextToken[0] >= '0' && testNextToken[0] <= '9') ||
						        testNextToken[0] == '+' || testNextToken[0] == '-' || testNextToken[0] == '.')
						{
							// Found two numbers in a row - treat as position
							// Only set position if it hasn't been set from command arguments
							if (!*hasPos)
							{
								*x = atof(token);

								// Parse second number
								token = nextToken;
								nextToken = strchr(token, ' ');
								if (nextToken)
								{
									*nextToken = '\0';
									nextToken++;
								}
								*y = atof(token);
								*hasPos = qtrue;
							}
						}
					}
				}
			}

			// If we didn't recognize it as position, check flags
			if (!*hasPos)
			{
				// Flags - quick check by first character
				if (token[0] == 'f')
				{
					if (Q_stricmp(token, "fMono") == 0)
					{
						*flags &= ~DS_PROPORTIONAL;
					}
					else if (Q_stricmp(token, "fNoShadow") == 0)
					{
						*flags &= ~DS_SHADOW;
					}
				}
			}
		}
		// Flags (fallback if position check above didn't match)
		else if (token[0] == 'f')
		{
			if (Q_stricmp(token, "fMono") == 0)
			{
				*flags &= ~DS_PROPORTIONAL;
			}
			else if (Q_stricmp(token, "fNoShadow") == 0)
			{
				*flags &= ~DS_SHADOW;
			}
		}

		token = nextToken;
	}
}

/*
=================
CG_ApplyPrintUITemplate

Apply template from cvar (helper function to avoid code duplication)
=================
*/
static void CG_ApplyPrintUITemplate(const char* cvarName, vec4_t textColor, vec4_t shadowColor, vec4_t background, vec4_t border, vec4_t borderColor, float* charWidth, float* charHeight, int* flags, int* fontIndex, float* duration, float* fadeTime, qboolean* hasBackground, qboolean* hasBorder, int* hAlign, float* x, float* y, qboolean* hasPos, qboolean* templatePos)
{
	char templateStr[MAX_STRING_CHARS];
	qboolean hadPosBeforeTemplate;
	qboolean templateHasPos;

	// Remember if position was already set from command arguments
	hadPosBeforeTemplate = *hasPos;
	templateHasPos = qfalse;

	trap_Cvar_VariableStringBuffer(cvarName, templateStr, sizeof(templateStr));
	CG_ParsePrintUITemplate(templateStr, textColor, shadowColor, background, border, borderColor, charWidth, charHeight, flags, fontIndex, duration, fadeTime, hasBackground, hasBorder, hAlign, x, y, &templateHasPos);

	// Only set hasPos from template if it wasn't already set from command arguments
	// Position from command has priority over position from template
	if (templateHasPos && !hadPosBeforeTemplate)
	{
		*hasPos = qtrue;
		*templatePos = qtrue;
	}
	else
	{
		*templatePos = qfalse;
	}
}

/*
=================
CG_PrintUI_f

Console command: /printui <"Text"> <x> <y> [options...]
=================
*/
void CG_PrintUI_f(void)
{
	int argc;
	char text[MAX_STRING_CHARS];
	char arg[MAX_TOKEN_CHARS];
	float x, y;
	int i;
	int printuiIndex = -1;
	int textArgEnd;
	vec4_t textColor;
	vec4_t shadowColor;
	vec4_t background;
	vec4_t border;
	vec4_t borderColor;
	float charWidth = 10.0f;
	float charHeight = 14.0f;
	int flags = DS_PROPORTIONAL | DS_SHADOW;
	int fontIndex = 2; // Default to sansman
	float duration = 5.0f;
	float fadeTime = 0.25f;
	qboolean hasBackground = qfalse;
	qboolean hasBorder = qfalse;
	const char* colorStr;
	const char* shadowColorStr;
	int hAlign = DS_HLEFT;
	char templateStr[MAX_STRING_CHARS];
	qboolean hasPos = qfalse;
	qboolean templateHasPos = qfalse;

	argc = trap_Argc();

	if (cg_printui.integer == 0)
	{
		CG_PrintOnce("^3cg_printui disabled!^7\n");
		return;
	}

	// Show help if no arguments provided
	if (argc <= 1)
	{
		CG_Printf("^91. Usage:^7 ^3printui ^2<\"Text\"> <x> <y> [options...]^7\n");
		CG_Printf("^3          printui ^2list^7\n");
		CG_Printf("^3          printui ^2clear\n");
		CG_Printf("^3          printui ^2clear [<number>]^7\n");
		CG_Printf("^7example:  ^3printui ^2\"Hello World!\" 240 240^7\n");
		CG_Printf("^7example:  ^3printui ^2\"There's Example\" t1^7\n");
		CG_Printf("^7example:  ^3printui ^2\"Of Print UI\" t2^7\n");
		CG_Printf("^92. Options: ^332 ^7rows of printui are available\n");
		CG_Printf("  ^5c^2<color>^7 - text color (e.g., ^2cWhite^7, ^2c7^7, ^2cFF^7)\n");
		CG_Printf("  ^5cs^2<color>^7 - shadow color\n");
		CG_Printf("  ^5<^2w^5>^2x<^2h^5>^7 - text size (e.g., ^210x14^7)\n");
		CG_Printf("  ^2hR^5/^2hL^5/^2hC^7 - horizontal alignment (right/left/center)\n");
		CG_Printf("  ^5font <^2name^5|^2number^5>^7 - font\n");
		CG_Printf("  ^5b <^2r^5> <^2g^5> <^2b^5> <^2a^5>^7 - border size (e.g., ^2b 1 1 1 1^7)\n");
		CG_Printf("  ^5bc <^2r^5> <^2g^5> <^2b^5> <^2a^5>^7 - border color (e.g., ^2bc 1 1 1 1^7)\n");
		CG_Printf("  ^5bg <^2r^5> <^2g^5> <^2b^5> <^2a^5>^7 - background color (e.g., ^2bg 1 1 1 1^7)\n");
		CG_Printf("  ^5time <^21^5|^22^5|^23^5|^24^5|^2s^5|^2m^5|^2l^5|^2u^5>^7 - display duration (^21^7/^2s^7=3.5s, ^22^7/^2m^7=10s, ^23^7/^2l^7=60s, ^24^7/^2u^7=unlimited)\n");
		CG_Printf("  ^5fade^5/^5fadeTime <^20^5|^21^5|^22^5|^23^5|^2s^5|^2m^5|^2l^5>^7 - fade duration (^20^7=no fade, ^21^5/^2s^7=0.25s, ^22^5/^2m^7=0.5s, ^23^5/^2l^7=1s)\n");
		CG_Printf("  ^5fMono^7 - disable proportional font\n");
		CG_Printf("  ^5fNoShadow^7 - disable shadow\n");
		CG_Printf("  ^5t1^7 - apply template from ^3cg_printui_template^7\n");
		CG_Printf("  ^5t2^7 - apply template from ^3cg_printui_template2^7\n");
		CG_Printf("^93. Note:^7 Template parameters can be ^2overridden^7 by passing arguments after template\n");
		CG_Printf("      Example: ^3printui ^2\"Text\" 120 120 t1 cRed^7 (overrides template color)\n");
		return;
	}

	// Handle clear command
	if (argc >= 2 && Q_stricmp(CG_Argv(1), "clear") == 0)
	{
		if (argc >= 3)
		{
			// Clear specific element by number
			int clearNum = atoi(CG_Argv(2));
			int activeCount = 0;
			int currentNum = 0;

			for (i = 0; i < 32; i++)
			{
				if (cgs.osp.printui[i].active)
				{
					currentNum++;
					if (currentNum == clearNum)
					{
						cgs.osp.printui[i].active = qfalse;
						CG_Printf("^2Printui: Cleared element %d\n", clearNum);
						return;
					}
					activeCount++;
				}
			}
			CG_Printf("^1Error: Element number %d not found\n", clearNum);
		}
		else
		{
			// Clear all elements
			int cleared = 0;
			for (i = 0; i < 32; i++)
			{
				if (cgs.osp.printui[i].active)
				{
					cgs.osp.printui[i].active = qfalse;
					cleared++;
				}
			}
			CG_Printf("^2Printui: Cleared %d element(s)\n", cleared);
		}
		return;
	}

	// Handle list command
	if (argc >= 2 && Q_stricmp(CG_Argv(1), "list") == 0)
	{
		int activeCount = 0;
		int currentNum = 0;

		for (i = 0; i < 32; i++)
		{
			if (cgs.osp.printui[i].active)
			{
				currentNum++;
				activeCount++;
				CG_Printf("^3%d.^7 \"%s\"^7 at (%.0f, %.0f)\n",
				          currentNum,
				          cgs.osp.printui[i].text,
				          cgs.osp.printui[i].x,
				          cgs.osp.printui[i].y);
			}
		}

		if (activeCount == 0)
		{
			CG_Printf("^3No active Printui elements\n");
		}
		return;
	}

	// Parse text from quoted string
	// Text is always the first argument(s) and must be in quotes
	// Collect arguments starting from first until we find closing quote or numeric argument
	{
		int pos;
		int len;
		qboolean foundClosingQuote;
		qboolean startsWithQuote;

		textArgEnd = 1;
		text[0] = '\0';
		foundClosingQuote = qfalse;
		startsWithQuote = qfalse;

		if (argc <= 1)
		{
			// No arguments
			return;
		}

		// Check if first argument starts with quote
		Q_strncpyz(arg, CG_Argv(1), sizeof(arg));
		if (arg[0] == '"')
		{
			startsWithQuote = qtrue;
			len = strlen(arg);
			if (len > 1 && arg[len - 1] == '"')
			{
				// Quote closed in first argument
				if (len > 2)
				{
					memmove(text, arg + 1, len - 2);
					text[len - 2] = '\0';
				}
				textArgEnd = 1;
				foundClosingQuote = qtrue;
			}
			else
			{
				// Quote spans multiple arguments, remove opening quote
				if (len > 1)
				{
					memmove(text, arg + 1, len - 1);
					pos = len - 1;
				}
				else
				{
					pos = 0;
				}
			}
		}
		else
		{
			// First argument doesn't start with quote, but might be part of quoted text
			// Try using trap_Args() to get full string with quotes
			{
				char argsStr[MAX_STRING_CHARS];
				char* startQuote;
				char* endQuote;
				int textLen;

				trap_Args(argsStr, sizeof(argsStr));
				startQuote = strchr(argsStr, '"');

				if (startQuote != NULL)
				{
					endQuote = strchr(startQuote + 1, '"');
					if (endQuote != NULL)
					{
						// Extract text between quotes
						textLen = endQuote - startQuote - 1;
						if (textLen > 0 && textLen < sizeof(text))
						{
							memcpy(text, startQuote + 1, textLen);
							text[textLen] = '\0';

							// Find which argument contains closing quote
							for (i = 1; i < argc; i++)
							{
								Q_strncpyz(arg, CG_Argv(i), sizeof(arg));
								len = strlen(arg);
								if (len > 0 && arg[len - 1] == '"')
								{
									textArgEnd = i;
									foundClosingQuote = qtrue;
									break;
								}
							}
							// If not found, estimate: count words in extracted text
							if (!foundClosingQuote)
							{
								int wordCount;
								char* p;
								wordCount = 1;
								for (p = text; *p; p++)
								{
									if (*p == ' ')
									{
										wordCount++;
									}
								}
								textArgEnd = wordCount;
								if (textArgEnd >= argc)
								{
									textArgEnd = argc - 1;
								}
							}
							// Skip manual collection
							startsWithQuote = qtrue;
						}
					}
				}
			}

			// If still no quote found, but text should be in quotes,
			// collect arguments until we hit numeric argument or template
			if (!startsWithQuote)
			{
				Q_strncpyz(text, arg, sizeof(text));
				pos = strlen(text);

				// Collect arguments until numeric or template
				for (i = 2; i < argc; i++)
				{
					char nextArg[MAX_TOKEN_CHARS];
					float testNum;
					char* endPtr;

					Q_strncpyz(nextArg, CG_Argv(i), sizeof(nextArg));
					testNum = atof(nextArg);
					endPtr = nextArg;
					while (*endPtr == ' ' || *endPtr == '\t') endPtr++;

					// Check if numeric or template - if so, stop here
					if ((testNum != 0.0f || (*endPtr >= '0' && *endPtr <= '9') || *endPtr == '-' || *endPtr == '+') ||
					        Q_stricmp(nextArg, "t1") == 0 || Q_stricmp(nextArg, "t2") == 0)
					{
						textArgEnd = i - 1;
						break;
					}

					// Add this argument to text
					Q_strncpyz(arg, CG_Argv(i), sizeof(arg));
					len = strlen(arg);
					if (pos + len + 2 < sizeof(text))
					{
						text[pos] = ' ';
						pos++;
						memcpy(text + pos, arg, len);
						pos += len;
						text[pos] = '\0';
					}
					else
					{
						break;
					}
				}

				if (i >= argc)
				{
					textArgEnd = argc - 1;
				}
			}
		}

		// If quote spans multiple arguments, collect them
		if (startsWithQuote && !foundClosingQuote)
		{
			// Continue collecting arguments
			for (i = 2; i < argc; i++)
			{
				Q_strncpyz(arg, CG_Argv(i), sizeof(arg));
				len = strlen(arg);

				// Check if this argument ends with quote
				if (len > 0 && arg[len - 1] == '"')
				{
					// Remove closing quote and add to text
					if (pos + len < sizeof(text))
					{
						text[pos] = ' ';
						pos++;
						memcpy(text + pos, arg, len - 1);
						pos += len - 1;
						text[pos] = '\0';
					}
					textArgEnd = i;
					foundClosingQuote = qtrue;
					break;
				}
				else
				{
					// Add argument with space
					if (pos + len + 2 < sizeof(text))
					{
						text[pos] = ' ';
						pos++;
						memcpy(text + pos, arg, len);
						pos += len;
						text[pos] = '\0';
					}
					else
					{
						break;
					}
				}

				// Check if next argument is numeric (x coordinate) or template
				// If so, stop collecting (quote might be missing)
				if (i + 1 < argc)
				{
					char nextArg[MAX_TOKEN_CHARS];
					float testNum;
					char* endPtr;

					Q_strncpyz(nextArg, CG_Argv(i + 1), sizeof(nextArg));
					testNum = atof(nextArg);
					endPtr = nextArg;
					while (*endPtr == ' ' || *endPtr == '\t') endPtr++;

					// Check if numeric or template
					if ((testNum != 0.0f || (*endPtr >= '0' && *endPtr <= '9') || *endPtr == '-' || *endPtr == '+') ||
					        Q_stricmp(nextArg, "t1") == 0 || Q_stricmp(nextArg, "t2") == 0)
					{
						// Stop here, this was the last text argument
						textArgEnd = i;
						break;
					}
				}
			}

			if (!foundClosingQuote && i >= argc)
			{
				textArgEnd = argc - 1;
			}
		}

		// Fallback: if still no text, use first argument
		if (text[0] == '\0' && argc > 1)
		{
			Q_strncpyz(text, CG_Argv(1), sizeof(text));
		}
	}

	// Check if template is used and if it has position
	if (argc >= 3)
	{
		// Start checking from after text arguments
		for (i = textArgEnd + 1; i < argc; i++)
		{
			Q_strncpyz(arg, CG_Argv(i), sizeof(arg));
			if (Q_stricmp(arg, "t1") == 0 || Q_stricmp(arg, "t2") == 0)
			{
				// Check if template has position
				if (Q_stricmp(arg, "t1") == 0)
				{
					trap_Cvar_VariableStringBuffer("cg_printui_template", templateStr, sizeof(templateStr));
				}
				else
				{
					trap_Cvar_VariableStringBuffer("cg_printui_template2", templateStr, sizeof(templateStr));
				}
				if (templateStr[0])
				{
					// Check if template has position (either "pos x y" or two numbers)
					if (strstr(templateStr, "pos") != NULL)
					{
						templateHasPos = qtrue;
					}
					else
					{
						// Check if there are two numbers in a row (not part of commands)
						// Simple approach: try to parse the template to see if position gets set
						// We'll create temporary variables and parse, but only check hasPos
						{
							vec4_t dummyColor;
							vec4_t dummyShadow;
							vec4_t dummyBg;
							vec4_t dummyBorder;
							vec4_t dummyBorderColor;
							float dummyWidth;
							float dummyHeight;
							int dummyFlags;
							int dummyFont;
							float dummyDuration;
							float dummyFade;
							qboolean dummyHasBg;
							qboolean dummyHasBorder;
							int dummyHAlign;
							float dummyX;
							float dummyY;
							qboolean dummyHasPos;

							dummyHasPos = qfalse;
							dummyX = 0.0f;
							dummyY = 0.0f;

							// Try parsing template to see if position is set
							CG_ParsePrintUITemplate(templateStr, dummyColor, dummyShadow, dummyBg, dummyBorder, dummyBorderColor,
							                        &dummyWidth, &dummyHeight, &dummyFlags, &dummyFont, &dummyDuration, &dummyFade,
							                        &dummyHasBg, &dummyHasBorder, &dummyHAlign, &dummyX, &dummyY, &dummyHasPos);

							if (dummyHasPos)
							{
								templateHasPos = qtrue;
							}
						}
					}
				}
				break;
			}
		}
	}

	// Require at least text, or text + template with position
	// Check if we have enough arguments after text (either x y or template)
	if (textArgEnd + 1 >= argc && !templateHasPos)
	{
		CG_Printf("^2usage: printui <\"Text\"> <x> <y> [options...]^7\n");
		CG_Printf("^3        printui list^7\n");
		CG_Printf("^3        printui clear\n");
		CG_Printf("^3        printui clear [<number>]^7\n");
		CG_Printf("^2Example: printui \"Hello World!\" 240 240^7\n");
		CG_Printf("^2Example: printui \"There's Example\" t1^7\n");
		CG_Printf("^2Example: printui \"Of Print UI\" t2^7\n");
		CG_Printf("Options:\n");
		CG_Printf("  c<color> - text color (e.g., cWhite, c7, cFF)\n");
		CG_Printf("  cs<color> - shadow color\n");
		CG_Printf("  <w>x<h> - text size (e.g., 10x14)\n");
		CG_Printf("  hR/hL/hC - horizontal alignment (right/left/center)\n");
		CG_Printf("  font <name|number> - font\n");
		CG_Printf("  b <r> <g> <b> <a> - border size\n");
		CG_Printf("  bc <r> <g> <b> <a> - border color\n");
		CG_Printf("  bg <r> <g> <b> <a> - background color\n");
		CG_Printf("  time <1|2|3|4 or s|m|l|u> - display duration (1/s=3.5s, 2/m=10s, 3/l=60s, 4/u=unlimited)\n");
		CG_Printf("  fade/fadeTime <0|1|2|3|s|m|l> - fade duration (0=no fade, 1/s=0.25s, 2/m=0.5s, 3/l=1s)\n");
		CG_Printf("  fMono - disable proportional font\n");
		CG_Printf("  fNoShadow - disable shadow\n");
		CG_Printf("  t1 - apply template from cg_printui_template\n");
		CG_Printf("  t2 - apply template from cg_printui_template2\n");
		CG_Printf("Note: Template parameters can be overridden by passing arguments after template\n");
		CG_Printf("      Example: printui \"Text\" 120 120 t1 cRed (overrides template color)\n");
		CG_Printf("Note: Position (x, y) can be specified in template using 'pos x y' or just 'x y'\n");
		CG_Printf("      Example template: \"cWhite pos 240 240 12x16 hC\" or \"120 120 c1\"\n");
		CG_Printf("      If using template with position, you can skip x y: printui \"Text\" t1\n");
		return;
	}

	// Find free slot
	for (i = 0; i < 32; i++)
	{
		if (!cgs.osp.printui[i].active)
		{
			printuiIndex = i;
			break;
		}
	}

	if (printuiIndex == -1)
	{
		CG_Printf("^1Error: Maximum 32 printui elements active\n");
		return;
	}

	// Parse x and y (may be overridden by template)
	// Text was already parsed above, now parse x and y after text arguments
	// Start parsing after text arguments (textArgEnd + 1)
	if (textArgEnd + 2 < argc)
	{
		// Check if next argument after text is a template
		Q_strncpyz(arg, CG_Argv(textArgEnd + 1), sizeof(arg));
		if (Q_stricmp(arg, "t1") != 0 && Q_stricmp(arg, "t2") != 0)
		{
			// Not a template, parse x and y
			x = atof(CG_Argv(textArgEnd + 1));
			y = atof(CG_Argv(textArgEnd + 2));
			hasPos = qtrue;
		}
	}
	else if (textArgEnd + 1 < argc)
	{
		// Check if next argument after text is a template
		Q_strncpyz(arg, CG_Argv(textArgEnd + 1), sizeof(arg));
		if (Q_stricmp(arg, "t1") != 0 && Q_stricmp(arg, "t2") != 0)
		{
			// Not a template, but we need y too, so this is an error case
			x = atof(CG_Argv(textArgEnd + 1));
			y = 0.0f;
		}
	}
	else
	{
		// Default values if not provided
		x = 0.0f;
		y = 0.0f;
	}

	// Default colors
	Vector4Copy(colorWhite, textColor);
	Vector4Copy(colorBlack, shadowColor);
	Vector4Set(borderColor, 1.0f, 1.0f, 1.0f, 1.0f);

	// Check for template usage (t1 or t2) and apply template parameters first
	// Start from argument after text to catch t1/t2 even if x/y are not provided
	// textArgEnd is still in scope from the block above
	for (i = textArgEnd + 1; i < argc; i++)
	{
		Q_strncpyz(arg, CG_Argv(i), sizeof(arg));

		// Apply template 1
		if (Q_stricmp(arg, "t1") == 0)
		{
			qboolean templatePos = qfalse;
			CG_ApplyPrintUITemplate("cg_printui_template", textColor, shadowColor, background, border, borderColor, &charWidth, &charHeight, &flags, &fontIndex, &duration, &fadeTime, &hasBackground, &hasBorder, &hAlign, &x, &y, &hasPos, &templatePos);
			// Don't apply alignment here - it will be applied at the end after all command arguments are parsed
			// This allows command arguments to override template alignment
			continue;
		}
		// Apply template 2
		else if (Q_stricmp(arg, "t2") == 0)
		{
			qboolean templatePos = qfalse;
			CG_ApplyPrintUITemplate("cg_printui_template2", textColor, shadowColor, background, border, borderColor, &charWidth, &charHeight, &flags, &fontIndex, &duration, &fadeTime, &hasBackground, &hasBorder, &hAlign, &x, &y, &hasPos, &templatePos);
			// Don't apply alignment here - it will be applied at the end after all command arguments are parsed
			// This allows command arguments to override template alignment
			continue;
		}

		// Color parsing (c prefix)
		if (arg[0] == 'c' && arg[1] != 's' && arg[1] != '\0')
		{
			colorStr = arg + 1;
			if (!CG_ParseColorStr(colorStr, textColor))
			{
				CG_Printf("^1Warning: Invalid color '%s'\n", colorStr);
			}
		}
		// Shadow color parsing (cs prefix)
		else if (Q_stricmpn(arg, "cs", 2) == 0 && arg[2] != '\0')
		{
			shadowColorStr = arg + 2;
			if (!CG_ParseColorStr(shadowColorStr, shadowColor))
			{
				CG_Printf("^1Warning: Invalid shadow color '%s'\n", shadowColorStr);
			}
		}
		// Text size (10x14 format)
		else if (strchr(arg, 'x') != NULL)
		{
			char* xPos = strchr(arg, 'x');
			*xPos = '\0';
			charWidth = atof(arg);
			charHeight = atof(xPos + 1);
			*xPos = 'x';
		}
		// Horizontal alignment
		else if (Q_stricmp(arg, "hR") == 0)
		{
			hAlign = DS_HRIGHT;
		}
		else if (Q_stricmp(arg, "hL") == 0)
		{
			hAlign = DS_HLEFT;
		}
		else if (Q_stricmp(arg, "hC") == 0)
		{
			hAlign = DS_HCENTER;
		}
		// Font
		else if (Q_stricmp(arg, "font") == 0 && i + 1 < argc)
		{
			i++;
			fontIndex = CG_FontIndexFromString(CG_Argv(i));
			if (fontIndex < 0)
			{
				fontIndex = 2; // Default to sansman
			}
		}
		// Border size
		else if (Q_stricmp(arg, "b") == 0)
		{
			if (CG_ParseVec4FromArgs(argc, &i, border) == 4)
			{
				hasBorder = qtrue;
			}
		}
		// Border color
		else if (Q_stricmp(arg, "bc") == 0)
		{
			CG_ParseVec4FromArgs(argc, &i, borderColor);
		}
		// Background color
		else if (Q_stricmp(arg, "bg") == 0)
		{
			if (CG_ParseVec4FromArgs(argc, &i, background) == 4)
			{
				hasBackground = qtrue;
			}
		}
		// Duration
		else if (Q_stricmp(arg, "time") == 0 && i + 1 < argc)
		{
			i++;
			// Parse time values: 1/s = 3.5s, 2/m = 10s, 3/l = 60s, u/4 = unlimited
			if (Q_stricmp(CG_Argv(i), "1") == 0 || Q_stricmp(CG_Argv(i), "s") == 0)
			{
				duration = 3.5f; // Short: 3.5 seconds
			}
			else if (Q_stricmp(CG_Argv(i), "2") == 0 || Q_stricmp(CG_Argv(i), "m") == 0)
			{
				duration = 10.0f; // Medium: 10 seconds
			}
			else if (Q_stricmp(CG_Argv(i), "3") == 0 || Q_stricmp(CG_Argv(i), "l") == 0)
			{
				duration = 60.0f; // Long: 60 seconds
			}
			else if (Q_stricmp(CG_Argv(i), "u") == 0 || Q_stricmp(CG_Argv(i), "4") == 0 || Q_stricmp(CG_Argv(i), "unlimited") == 0)
			{
				duration = -1.0f; // Unlimited
			}
			else
			{
				// Default to short if invalid value
				duration = 3.5f;
			}
		}
		// Fade time
		else if ((Q_stricmp(arg, "fadeTime") == 0 || Q_stricmp(arg, "fade") == 0) && i + 1 < argc)
		{
			i++;
			// Parse fadeTime values: 0 = no fade, 1/s = 0.25s, 2/m = 0.5s, 3/l = 1s
			if (Q_stricmp(CG_Argv(i), "0") == 0)
			{
				fadeTime = 0.0f; // No fade
			}
			else if (Q_stricmp(CG_Argv(i), "1") == 0 || Q_stricmp(CG_Argv(i), "s") == 0)
			{
				fadeTime = 0.25f; // Short: 0.25 seconds
			}
			else if (Q_stricmp(CG_Argv(i), "2") == 0 || Q_stricmp(CG_Argv(i), "m") == 0)
			{
				fadeTime = 0.5f; // Medium: 0.5 seconds
			}
			else if (Q_stricmp(CG_Argv(i), "3") == 0 || Q_stricmp(CG_Argv(i), "l") == 0)
			{
				fadeTime = 1.0f; // Long: 1 second
			}
			else
			{
				// Default to no fade if invalid value
				fadeTime = 0.0f;
			}
		}
		// Flags
		else if (Q_stricmp(arg, "fMono") == 0)
		{
			flags &= ~DS_PROPORTIONAL;
		}
		else if (Q_stricmp(arg, "fNoShadow") == 0)
		{
			flags &= ~DS_SHADOW;
		}
	}

	// Apply horizontal alignment (clear old alignment bits first)
	flags &= ~(DS_HLEFT | DS_HRIGHT | DS_HCENTER);
	flags |= hAlign;

	// Check if position is set (either from arguments or template)
	if (!hasPos)
	{
		CG_Printf("^1Error: Position (x, y) is required for output.\n");
		CG_Printf("^3       Specify position as: <x> <y> arguments or use template with 'pos x y'\n");
		CG_Printf("^3       Example: printui \"Text\" 240 240\n");
		CG_Printf("^3       Example template: \"cWhite pos 240 240\"\n");
		return;
	}

	// Check for existing elements with same position and horizontal alignment
	// Remove them to avoid overlapping
	{
		int existingHAlign;
		int currentHAlign;
		int j;

		currentHAlign = flags & (DS_HLEFT | DS_HRIGHT | DS_HCENTER);

		for (j = 0; j < 32; j++)
		{
			if (cgs.osp.printui[j].active)
			{
				// Check if position matches (with small epsilon for float comparison)
				if (fabs(cgs.osp.printui[j].x - x) < 0.1f &&
				        fabs(cgs.osp.printui[j].y - y) < 0.1f)
				{
					// Check if horizontal alignment matches
					existingHAlign = cgs.osp.printui[j].flags & (DS_HLEFT | DS_HRIGHT | DS_HCENTER);
					if (existingHAlign == currentHAlign)
					{
						// Deactivate the existing element
						cgs.osp.printui[j].active = qfalse;
					}
				}
			}
		}
	}

	// Store printui element
	Q_strncpyz(cgs.osp.printui[printuiIndex].text, text, sizeof(cgs.osp.printui[printuiIndex].text));
	cgs.osp.printui[printuiIndex].x = x;
	cgs.osp.printui[printuiIndex].y = y;
	Vector4Copy(textColor, cgs.osp.printui[printuiIndex].color);
	Vector4Copy(shadowColor, cgs.osp.printui[printuiIndex].shadowColor);
	cgs.osp.printui[printuiIndex].charWidth = charWidth;
	cgs.osp.printui[printuiIndex].charHeight = charHeight;
	cgs.osp.printui[printuiIndex].flags = flags;
	cgs.osp.printui[printuiIndex].fontIndex = fontIndex;
	cgs.osp.printui[printuiIndex].startTime = cg.time;
	cgs.osp.printui[printuiIndex].duration = duration;
	cgs.osp.printui[printuiIndex].fadeTime = fadeTime;
	cgs.osp.printui[printuiIndex].active = qtrue;

	if (hasBackground)
	{
		Vector4Copy(background, cgs.osp.printui[printuiIndex].background);
	}
	else
	{
		Vector4Clear(cgs.osp.printui[printuiIndex].background);
	}

	if (hasBorder)
	{
		Vector4Copy(border, cgs.osp.printui[printuiIndex].border);
		Vector4Copy(borderColor, cgs.osp.printui[printuiIndex].borderColor);
	}
	else
	{
		Vector4Clear(cgs.osp.printui[printuiIndex].border);
		Vector4Copy(borderColor, cgs.osp.printui[printuiIndex].borderColor);
	}
}

/*
=================
CG_DrawPrintUI

Draw all active printui elements
=================
*/
void CG_DrawPrintUI(void)
{
	int i;
	float elapsed;
	float fade;
	vec4_t finalColor;
	vec4_t finalShadowColor;
	vec4_t finalBackground;
	vec4_t finalBorder;
	vec4_t finalBorderColor;

	if (!cg.time)
	{
		return;
	}

	for (i = 0; i < 32; i++)
	{
		if (!cgs.osp.printui[i].active)
		{
			continue;
		}

		elapsed = (cg.time - cgs.osp.printui[i].startTime) / 1000.0f;

		if (elapsed < 0.0f)
		{
			// Time hasn't started yet, skip
			continue;
		}

		// Check if expired (skip check if duration is -1.0f which means unlimited)
		if (cgs.osp.printui[i].duration >= 0.0f && elapsed >= cgs.osp.printui[i].duration)
		{
			cgs.osp.printui[i].active = qfalse;
			continue;
		}

		// Calculate fade
		if (cgs.osp.printui[i].fadeTime <= 0.0f)
		{
			fade = 1.0f;
		}
		else if (elapsed < cgs.osp.printui[i].fadeTime)
		{
			// Fade in
			fade = elapsed / cgs.osp.printui[i].fadeTime;
		}
		else if (cgs.osp.printui[i].duration >= 0.0f && elapsed > cgs.osp.printui[i].duration - cgs.osp.printui[i].fadeTime)
		{
			// Fade out (only if not unlimited)
			fade = (cgs.osp.printui[i].duration - elapsed) / cgs.osp.printui[i].fadeTime;
		}
		else
		{
			fade = 1.0f;
		}

		// Apply fade to colors
		Vector4Copy(cgs.osp.printui[i].color, finalColor);
		finalColor[3] *= fade;

		Vector4Copy(cgs.osp.printui[i].shadowColor, finalShadowColor);
		finalShadowColor[3] *= fade;

		// Background and border
		if (cgs.osp.printui[i].background[3] > 0.0f)
		{
			Vector4Copy(cgs.osp.printui[i].background, finalBackground);
			finalBackground[3] *= fade;
		}

		if (cgs.osp.printui[i].border[0] > 0.0f ||
		        cgs.osp.printui[i].border[1] > 0.0f ||
		        cgs.osp.printui[i].border[2] > 0.0f ||
		        cgs.osp.printui[i].border[3] > 0.0f
		   )
		{
			Vector4Copy(cgs.osp.printui[i].border, finalBorder);
			Vector4Copy(cgs.osp.printui[i].borderColor, finalBorderColor);
		}

		// Set font
		if (cgs.osp.printui[i].fontIndex >= 0 && cgs.osp.printui[i].fontIndex < CG_GetFontCount())
		{
			CG_FontSelect(cgs.osp.printui[i].fontIndex);
		}
		else
		{
			// Default to sansman (index 2)
			CG_FontSelect(2);
		}

		// Draw the text
		CG_OSPDrawStringNew(
		    cgs.osp.printui[i].x,
		    cgs.osp.printui[i].y,
		    cgs.osp.printui[i].text,
		    finalColor,
		    finalShadowColor,
		    cgs.osp.printui[i].charWidth,
		    cgs.osp.printui[i].charHeight,
		    SCREEN_WIDTH,
		    cgs.osp.printui[i].flags,
		    (cgs.osp.printui[i].background[3] > 0.0f) ? finalBackground : NULL,
		    (cgs.osp.printui[i].border[3] > 0.0f) ? finalBorder : NULL,
		    (cgs.osp.printui[i].border[3] > 0.0f) ? finalBorderColor : NULL
		);
	}
}

