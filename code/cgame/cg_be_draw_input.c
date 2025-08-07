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
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

typedef enum
{
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,

	K_LAST_KEY      // this had better be <256!
} keyNum_t;

typedef struct
{
	int key;
	float x, y;
	float w, h;
	const char* label;
} KeyRect_t;


static KeyRect_t keyboardLayout[] =
{
	{ 0,      10,  10, 20, 20, ""    },  // пустое место под тильду
	{ '1',    32,  10, 20, 20, "1" },
	{ '2',    54,  10, 20, 20, "2" },
	{ '3',    76,  10, 20, 20, "3" },
	{ '4',    98,  10, 20, 20, "4" },
	{ '5',   120,  10, 20, 20, "5" },
	{ '6',   142,  10, 20, 20, "6" },
	{ '7',   164,  10, 20, 20, "7" },
	{ '8',   186,  10, 20, 20, "8" },
	{ '9',   208,  10, 20, 20, "9" },
	{ '0',   230,  10, 20, 20, "0" },
	{ K_BACKSPACE, 252, 10, 75, 20, "BKSP" },

	// Вторая строка
	{ K_TAB,  10,  32, 30, 20, "TAB" },
	{ 'Q',    42,  32, 20, 20, "Q" },
	{ 'W',    64,  32, 20, 20, "W" },
	{ 'E',    86,  32, 20, 20, "E" },
	{ 'R',   108,  32, 20, 20, "R" },
	{ 'T',   130,  32, 20, 20, "T" },
	{ 'Y',   152,  32, 20, 20, "Y" },
	{ 'U',   174,  32, 20, 20, "U" },
	{ 'I',   196,  32, 20, 20, "I" },
	{ 'O',   218,  32, 20, 20, "O" },
	{ 'P',   240,  32, 20, 20, "P" },
	{ '[',   262,  32, 20, 20, "[" },
	{ ']',   284,  32, 20, 20, "]" },
	{ K_ENTER, 306, 32, 43, 20, "ENT" },

	// Третья строка
	{ K_CAPSLOCK, 10,  54, 32, 20, "CAPS" },
	{ 'A',        44,  54, 20, 20, "A" },
	{ 'S',        66,  54, 20, 20, "S" },
	{ 'D',        88,  54, 20, 20, "D" },
	{ 'F',       110,  54, 20, 20, "F" },
	{ 'G',       132,  54, 20, 20, "G" },
	{ 'H',       154,  54, 20, 20, "H" },
	{ 'J',       176,  54, 20, 20, "J" },
	{ 'K',       198,  54, 20, 20, "K" },
	{ 'L',       220,  54, 20, 20, "L" },
	{ ';',       242,  54, 20, 20, ";" },
	{ '\'',      264,  54, 20, 20, "'" },
	{ '\\',      286,  54, 20, 20, "\\" },
	{ K_ENTER,   308,  54, 61, 20, "ENTER" },

	// Четвёртая строка
	{ K_SHIFT, 10,  76, 42, 20, "SHIFT" },
	{ 'Z',     54,  76, 20, 20, "Z" },
	{ 'X',     76,  76, 20, 20, "X" },
	{ 'C',     98,  76, 20, 20, "C" },
	{ 'V',    120,  76, 20, 20, "V" },
	{ 'B',    142,  76, 20, 20, "B" },
	{ 'N',    164,  76, 20, 20, "N" },
	{ 'M',    186,  76, 20, 20, "M" },
	{ ',',    208,  76, 20, 20, "," },
	{ '.',    230,  76, 20, 20, "." },
	{ '/',    252,  76, 20, 20, "/" },
	{ K_SHIFT, 274,  76, 95, 20, "SHIFT" },

	// Пятая строка
	{ K_CTRL, 10,  98, 30, 20, "CTRL" },
	{ K_ALT,  42,  98, 30, 20, "ALT" },
	{ K_SPACE, 74, 98, 150, 20, "SPACE" },
	{ K_ALT,  226, 98, 30, 20, "ALT" },
	{ K_CTRL, 258, 98, 30, 20, "CTRL" },

	// ESC и F1–F12
	{ K_ESCAPE, 10, -12, 40, 20, "ESC" },
	{ K_F1,     60, -12, 20, 20, "F1" },
	{ K_F2,     82, -12, 20, 20, "F2" },
	{ K_F3,    104, -12, 20, 20, "F3" },
	{ K_F4,    126, -12, 20, 20, "F4" },
	{ K_F5,    148, -12, 20, 20, "F5" },
	{ K_F6,    170, -12, 20, 20, "F6" },
	{ K_F7,    192, -12, 20, 20, "F7" },
	{ K_F8,    214, -12, 20, 20, "F8" },
	{ K_F9,    236, -12, 20, 20, "F9" },
	{ K_F10,   258, -12, 20, 20, "F10" },
	{ K_F11,   280, -12, 20, 20, "F11" },
	{ K_F12,   302, -12, 20, 20, "F12" },

	// Навигационный блок
	{ K_PAUSE,  390, 10, 20, 20, "PAUSE" },
	{ K_INS,    340, 32, 20, 20, "INS" },
	{ K_HOME,   362, 32, 20, 20, "HOME" },
	{ K_PGUP,   384, 32, 20, 20, "PGUP" },
	{ K_DEL,    340, 54, 20, 20, "DEL" },
	{ K_END,    362, 54, 20, 20, "END" },
	{ K_PGDN,   384, 54, 20, 20, "PGDN" },

	{ K_UPARROW,    362, 76, 20, 20, "UP" },
	{ K_LEFTARROW,  340, 98, 20, 20, "L"  },
	{ K_DOWNARROW,  362, 98, 20, 20, "DN" },
	{ K_RIGHTARROW, 384, 98, 20, 20, "R"  },

	// NumPad
	{ K_KP_NUMLOCK,  420, 10, 20, 20, "NUM"  },
	{ K_KP_SLASH,    442, 10, 20, 20, "/"    },
	{ K_KP_MINUS,    464, 10, 20, 20, "-"    },

	{ K_KP_HOME,     420, 32, 20, 20, "7"    },
	{ K_KP_UPARROW,  442, 32, 20, 20, "8"    },
	{ K_KP_PGUP,     464, 32, 20, 20, "9"    },
	{ K_KP_PLUS,     486, 32, 20, 42, "+"    },

	{ K_KP_LEFTARROW, 420, 54, 20, 20, "4"    },
	{ K_KP_5,        442, 54, 20, 20, "5"    },
	{ K_KP_RIGHTARROW, 464, 54, 20, 20, "6"    },

	{ K_KP_END,        420, 76, 20, 20, "1"  },
	{ K_KP_DOWNARROW,  442, 76, 20, 20, "2"  },
	{ K_KP_PGDN,       464, 76, 20, 20, "3"  },
	{ K_KP_ENTER,      486, 76, 20, 42, "ENT" },

	{ K_KP_INS,        420, 98, 42, 20, "0" },
	{ K_KP_DEL,        464, 98, 20, 20, "." }
};

#define NUM_KEYS (sizeof(keyboardLayout)/sizeof(keyboardLayout[0]))

static vec4_t colorPressed = {0.2f, 0.6f, 0.2f, 1.0f};
static vec4_t colorNotPressed = {0.1f, 0.1f, 0.1f, 1.0f};
static vec4_t colorText = {1.0f, 1.0f, 1.0f, 1.0f};
static vec4_t colorShadow = {0.0f, 0.0f, 0.0f, 0.5f};

void CG_DrawKeyboard(float startX, float startY)
{
	int i, keyCode;
	float x, y, textX, textY;
	qboolean isDown;
	for (i = 0; i < NUM_KEYS; i++)
	{
		KeyRect_t* key = &keyboardLayout[i];

		x = startX + key->x;
		y = startY + key->y;

		keyCode = key->key;

		if (keyCode >= 'A' && keyCode <= 'Z')
		{
			isDown = trap_Key_IsDown(keyCode) || trap_Key_IsDown(keyCode + ('a' - 'A'));
		}
		else
		{
			isDown = trap_Key_IsDown(keyCode);
		}

		CG_FillRect(x, y, key->w, key->h, isDown ? colorPressed : colorNotPressed);

		textX = x + key->w / 2 - (strlen(key->label) * 4);
		textY = y + key->h / 2 - 6;

		CG_OSPDrawStringNew(
		    textX, textY,
		    key->label,
		    colorText,
		    colorShadow,
		    8.0f, 12.0f,
		    0,
		    DS_SHADOW | DS_PROPORTIONAL,
		    NULL,
		    NULL,
		    NULL
		);
	}
}