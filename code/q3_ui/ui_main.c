// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/


#include "ui_local.h"


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
DLLEXPORT intptr_t vmMain(int command, int arg0, int arg1)
{
	switch (command)
	{
		case UI_GETAPIVERSION:
			return UI_API_VERSION;

		case UI_INIT:
			UI_Init();
			return 0;

		case UI_SHUTDOWN:
			UI_Shutdown();
			return 0;

		case UI_KEY_EVENT:
			UI_KeyEvent(arg0, arg1);
			return 0;

		case UI_MOUSE_EVENT:
			UI_MouseEvent(arg0, arg1);
			return 0;

		case UI_REFRESH:
			UI_Refresh(arg0);
			return 0;

		case UI_IS_FULLSCREEN:
			return UI_IsFullscreen();

		case UI_SET_ACTIVE_MENU:
			UI_SetActiveMenu(arg0);
			return 0;

		case UI_CONSOLE_COMMAND:
			return UI_ConsoleCommand(arg0);

		case UI_DRAW_CONNECT_SCREEN:
			UI_DrawConnectScreen(arg0);
			return 0;

		case UI_HASUNIQUECDKEY: // mod authors need to observe this
			return qtrue;       // change this to qfalse for mods!
	}

	return -1;
}


/*
================
cvars
================
*/

typedef struct
{
	vmCvar_t*    vmCvar;
	char*        cvarName;
	char*        defaultString;
	int         cvarFlags;
} cvarTable_t;

#define DECLARE_UI_CVAR
#include "ui_cvar.h"
#undef DECLARE_UI_CVAR

// bk001129 - made static to avoid aliasing.
static cvarTable_t      cvarTable[] =
{

#define UI_CVAR_LIST
#include "ui_cvar.h"
#undef UI_CVAR_LIST

};

// bk001129 - made static to avoid aliasing
static int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_ScanCustomConfigFiles

Scan beconfigs/ directory for CustomConfig_*.cfg files
and set uc_shudBindsList cvar with space-separated list of config names
=================
*/
static void UI_ScanCustomConfigFiles(void)
{
	char        filelist[8192];
	char*        fileptr;
	int         numfiles;
	int         len;
	char        filename[MAX_QPATH];
	char        bindsList[8192];
	int         bindsListLen;
	int         prefixLen;
	int         suffixLen;
	int         nameLen;
	char*        ext;
	int         i;
	int         j;

	bindsList[0] = '\0';
	bindsListLen = 0;
	prefixLen = 13; // length of "CustomConfig_"
	suffixLen = 4;  // length of ".cfg"

	numfiles = trap_FS_GetFileList("beconfigs", ".cfg", filelist, sizeof(filelist));
	if (numfiles > 0)
	{
		fileptr = filelist;
		for (i = 0; i < numfiles && bindsListLen < (int)sizeof(bindsList) - 64; i++)
		{
			// Debug: check if we're processing files correctly
			// Manual strlen for C89 compatibility
			len = 0;
			while (fileptr[len] != '\0')
			{
				len++;
			}

			if (len == 0)
			{
				fileptr++;
				continue;
			}

			if (len > 0 && len < MAX_QPATH)
			{
				Q_strncpyz(filename, fileptr, sizeof(filename));

				// Check if filename starts with "customConfig_" (case-insensitive)
				if (len >= prefixLen + suffixLen)
				{
					qboolean matchesPrefix;
					char c1;
					char c2;
					matchesPrefix = qtrue;
					for (j = 0; j < prefixLen; j++)
					{
						c1 = filename[j];
						c2 = "customConfig_"[j];
						// Convert to lowercase for comparison
						if (c1 >= 'A' && c1 <= 'Z')
						{
							c1 = c1 + ('a' - 'A');
						}
						if (c2 >= 'A' && c2 <= 'Z')
						{
							c2 = c2 + ('a' - 'A');
						}
						if (c1 != c2)
						{
							matchesPrefix = qfalse;
							break;
						}
					}

					// Check if filename ends with ".cfg"
					if (matchesPrefix)
					{
						ext = filename + len - suffixLen;
						if (ext[0] == '.' && ext[1] == 'c' && ext[2] == 'f' && ext[3] == 'g')
						{
							// Extract the name part (between "customConfig_" and ".cfg")
							nameLen = len - prefixLen - suffixLen;
							if (nameLen > 0 && nameLen <= 64)
							{
								// Add to list (separated by spaces)
								if (bindsListLen > 0)
								{
									bindsList[bindsListLen] = ' ';
									bindsListLen++;
								}
								// Copy name part (preserve case from filename)
								for (j = 0; j < nameLen; j++)
								{
									bindsList[bindsListLen + j] = filename[prefixLen + j];
								}
								/* First letter of each name to uppercase (engine may return lowercase) */
								if (nameLen > 0 && bindsList[bindsListLen] >= 'a' && bindsList[bindsListLen] <= 'z')
								{
									bindsList[bindsListLen] = bindsList[bindsListLen] - ('a' - 'A');
								}
								bindsListLen += nameLen;
								bindsList[bindsListLen] = '\0';
							}
						}
					}
				}
			}

			fileptr += len + 1;
		}
	}

	trap_Cvar_Set("uc_shudBindsList", bindsList);
}

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars(void)
{
	int         i;
	cvarTable_t* cv;
	float       sensitivityValue;

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}

	// Save current sensitivity value to uc_sensitivity
	sensitivityValue = trap_Cvar_VariableValue("sensitivity");
	if (sensitivityValue < 0.0f)
	{
		sensitivityValue = 0.0f;
	}
	trap_Cvar_SetValue("uc_sensitivity", sensitivityValue);

	/* uc_gamma: when ui_forceGamma 1, save r_gamma at UI start; when 0 we don't write */
	if (trap_Cvar_VariableValue("ui_forceGamma"))
	{
		float rg = trap_Cvar_VariableValue("r_gamma");
		if (rg < 0.99f || rg > 1.01f)
		{
			trap_Cvar_SetValue("uc_gamma", rg);
		}
	}

	// Scan beconfigs/ directory for customConfig_*.cfg files
	UI_ScanCustomConfigFiles();
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars(void)
{
	int         i;
	cvarTable_t* cv;

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		trap_Cvar_Update(cv->vmCvar);
	}
}


/*
=================
UI_VideoCheck
=================
*/
void UI_VideoCheck(int time)
{
	if (abs(time - uis.lastVideoCheck) > 1000)
	{

		int oldWidth, oldHeight;
		oldWidth = uis.glconfig.vidWidth;
		oldHeight = uis.glconfig.vidHeight;

		trap_GetGlconfig(&uis.glconfig);

		if (uis.glconfig.vidWidth != oldWidth || uis.glconfig.vidHeight != oldHeight)
		{
			uis.biasY = 0.0;
			uis.biasX = 0.0;
			// for 640x480 virtualized screen
			if (uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640)
			{
				// wide screen, scale by height
				uis.scale = uis.glconfig.vidHeight * (1.0 / 480.0);
				uis.biasX = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0 / 480.0)));
			}
			else
			{
				// no wide screen, scale by width
				uis.scale = uis.glconfig.vidWidth * (1.0 / 640.0);
				uis.biasY = 0.5 * (uis.glconfig.vidHeight - (uis.glconfig.vidWidth * (480.0 / 640)));
			}

			uis.screenXmin = 0.0 - (uis.biasX / uis.scale);
			uis.screenXmax = 640.0 + (uis.biasX / uis.scale);

			uis.screenYmin = 0.0 - (uis.biasY / uis.scale);
			uis.screenYmax = 480.0 + (uis.biasY / uis.scale);

			uis.cursorScaleR = 1.0 / uis.scale;
			if (uis.cursorScaleR < 0.5)
			{
				uis.cursorScaleR = 0.5;
			}
		}

		uis.lastVideoCheck = time;
	}
}
