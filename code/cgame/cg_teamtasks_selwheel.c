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
// cg_teamtasks_selwheel.c -- team task selection wheel functions

#include "cg_local.h"

/* External reference to teamTaskCommands array from cg_teamtasks_utils.c */
extern teamTaskCommand_t teamTaskCommands[];
#define NUM_TEAMTASK_COMMANDS 12

/* Constants for selection wheel */
#define SELWHEEL_ICON_RADIUS 20.0f /* Distance from center to icons (in 640x480 coordinates) */
#define SELWHEEL_ICON_SIZE 6.0f /* Base icon size (in 640x480 coordinates) */
#define SELWHEEL_MIN_RADIUS 12.0f /* Small center area for reset */
#define SELWHEEL_MAX_RADIUS_EXTENSION 40.0f /* Extension beyond icons for activation zone */
#define SELWHEEL_MARK_MAX_RADIUS_OFFSET 10.0f /* Offset for mark max radius */
#define SELWHEEL_RETURN_MAX_RADIUS_OFFSET 5.0f /* Offset for return max radius */
#define SELWHEEL_START_ANGLE (M_PI * 1.5f) /* 9 o'clock = 270 degrees = 3π/2 radians */

/*
=================
CG_GetScreenCenter

Get screen center coordinates
=================
*/
static void CG_GetScreenCenter(float* centerX, float* centerY)
{
	*centerX = SCREEN_WIDTH / 2.0f;
	*centerY = SCREEN_HEIGHT / 2.0f;
}

/*
=================
CG_GetSelwheelIconAngle

Calculate icon angle for given index
Returns angle in radians
=================
*/
static float CG_GetSelwheelIconAngle(int index, int elementCount, qboolean hasCenterElement)
{
	float angleStep;
	float iconAngle;

	if (hasCenterElement && elementCount > 1)
	{
		/* Center element exists, distribute other elements (index-1) around circle */
		angleStep = 2.0f * M_PI / (float)(elementCount - 1);
		iconAngle = SELWHEEL_START_ANGLE + angleStep * (float)(index - 1);
	}
	else
	{
		/* No center element, distribute all elements around circle */
		angleStep = 2.0f * M_PI / (float)elementCount;
		iconAngle = SELWHEEL_START_ANGLE + angleStep * (float)index;
	}

	/* Normalize to 0..2π range */
	if (iconAngle < 0.0f)
	{
		iconAngle += 2.0f * M_PI;
	}
	else if (iconAngle >= 2.0f * M_PI)
	{
		iconAngle -= 2.0f * M_PI;
	}

	return iconAngle;
}

void CG_UpdateSelwheelMark(void)
{
	int currentCmdNum;
	usercmd_t currentCmd;
	float sensitivity;
	float pitchDelta;
	float yawDelta;
	int timeSinceMovement;
	float centerX;
	float centerY;
	float returnSpeed;

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	currentCmdNum = trap_GetCurrentCmdNumber();

	// Get current command
	if (!trap_GetUserCmd(currentCmdNum, &currentCmd))
	{
		return;
	}

	// ============================================
	// STEP 1: Check for mouse movement FIRST
	// This must be checked BEFORE any center return logic
	// ============================================
	// Compare current angles with saved angles
	if (currentCmd.angles[PITCH] != cgs.be.teamMarkers.lastCmdAngles[PITCH] ||
	        currentCmd.angles[YAW] != cgs.be.teamMarkers.lastCmdAngles[YAW])
	{

		// ============================================
		// STEP 2: Movement detected - update mark position
		// ============================================
		// Calculate angle differences (angles are in 16-bit short format)
		pitchDelta = (float)(SHORT2ANGLE(currentCmd.angles[PITCH]) - SHORT2ANGLE(cgs.be.teamMarkers.lastCmdAngles[PITCH]));
		yawDelta = (float)(SHORT2ANGLE(currentCmd.angles[YAW]) - SHORT2ANGLE(cgs.be.teamMarkers.lastCmdAngles[YAW]));

		// Normalize angles to -180..180 range
		if (pitchDelta > 180.0f)
		{
			pitchDelta -= 360.0f;
		}
		else if (pitchDelta < -180.0f)
		{
			pitchDelta += 360.0f;
		}

		if (yawDelta > 180.0f)
		{
			yawDelta -= 360.0f;
		}
		else if (yawDelta < -180.0f)
		{
			yawDelta += 360.0f;
		}

		// Update position based on movement
		// Use adjusted wheel sensitivity if bitmask & 8 is set
		if (cgs.be.teamMarkers.sensitivityAdjusted)
		{
			sensitivity = cgs.be.teamMarkers.adjustedWheelSensitivity;
		}
		else
		{
			sensitivity = cg_teamTaskWheelSensitivity.value; // Use cvar to control mark movement speed
		}
		cgs.be.teamMarkers.ttwheelMarkX -= yawDelta * sensitivity; // Inverted: right mouse = right mark
		cgs.be.teamMarkers.ttwheelMarkY += pitchDelta * sensitivity;

		// Clamp to circular boundary (radius slightly larger than icon position)
		{
			float maxRadius;
			float dx;
			float dy;
			float distFromCenter;
			float angle;
			float scale;

			CG_GetScreenCenter(&centerX, &centerY);
			scale = cg_teamTaskWheelScale.value;
			if (scale <= 0.0f)
			{
				scale = 1.0f;
			}
			maxRadius = (SELWHEEL_ICON_RADIUS + SELWHEEL_MARK_MAX_RADIUS_OFFSET) * scale;

			// Calculate distance from center (all in 640x480 coordinates)
			dx = cgs.be.teamMarkers.ttwheelMarkX - centerX;
			dy = cgs.be.teamMarkers.ttwheelMarkY - centerY;
			distFromCenter = sqrt(dx * dx + dy * dy);

			// If cursor is outside max radius, clamp it to the circle boundary
			if (distFromCenter > maxRadius)
			{
				// Calculate angle from center
				angle = atan2(dy, dx);

				// Set position to max radius at this angle
				cgs.be.teamMarkers.ttwheelMarkX = centerX + cos(angle) * maxRadius;
				cgs.be.teamMarkers.ttwheelMarkY = centerY + sin(angle) * maxRadius;
			}
		}

		// Update saved angles immediately - ONLY when movement detected
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
		cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];

		// Reset movement timer - movement detected
		cgs.be.teamMarkers.lastMovementTime = cg.time;

		// Update last command number and RETURN - movement detected, no center return
		cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
		return;
	}

	// ============================================
	// STEP 3: No movement detected - check for return to center
	// ============================================
	timeSinceMovement = cg.time - cgs.be.teamMarkers.lastMovementTime;

	// Return to center if no movement for 0.5 seconds (500ms)
	if (timeSinceMovement > 500)
	{
		float maxRadius;
		float dx;
		float dy;
		float distFromCenter;
		float angle;
		float scale;

		CG_GetScreenCenter(&centerX, &centerY);
		scale = cg_teamTaskWheelScale.value;
		if (scale <= 0.0f)
		{
			scale = 1.0f;
		}
		maxRadius = (SELWHEEL_ICON_RADIUS + SELWHEEL_RETURN_MAX_RADIUS_OFFSET) * scale;

		// Smooth return to center (all in 640x480 coordinates)
		returnSpeed = 0.15f; // Adjust this value to control return speed (0.0 = instant, 1.0 = very slow)

		cgs.be.teamMarkers.ttwheelMarkX += (centerX - cgs.be.teamMarkers.ttwheelMarkX) * returnSpeed;
		cgs.be.teamMarkers.ttwheelMarkY += (centerY - cgs.be.teamMarkers.ttwheelMarkY) * returnSpeed;

		// Clamp to circular boundary after return movement
		dx = cgs.be.teamMarkers.ttwheelMarkX - centerX;
		dy = cgs.be.teamMarkers.ttwheelMarkY - centerY;
		distFromCenter = sqrt(dx * dx + dy * dy);

		if (distFromCenter > maxRadius)
		{
			// Calculate angle from center
			angle = atan2(dy, dx);

			// Set position to max radius at this angle
			cgs.be.teamMarkers.ttwheelMarkX = centerX + cos(angle) * maxRadius;
			cgs.be.teamMarkers.ttwheelMarkY = centerY + sin(angle) * maxRadius;
		}

		// If very close to center, snap to exact center
		if (fabs(cgs.be.teamMarkers.ttwheelMarkX - centerX) < 1.0f && fabs(cgs.be.teamMarkers.ttwheelMarkY - centerY) < 1.0f)
		{
			cgs.be.teamMarkers.ttwheelMarkX = centerX;
			cgs.be.teamMarkers.ttwheelMarkY = centerY;
		}
	}

	// Update last command number for next frame
	cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
}

/*
=================
CG_DrawSelwheelMark

Draw the ttwheel mark on screen
=================
*/
void CG_DrawSelwheelMark(void)
{
	float x;
	float y;
	float size;
	qhandle_t shader;
	vec4_t color;
	float scale;

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	// Get scale value
	scale = cg_teamTaskWheelScale.value;
	if (scale <= 0.0f)
	{
		scale = 1.0f;
	}

	// Use white shader for the mark
	shader = cgs.media.whiteShader;
	if (!shader)
	{
		return;
	}
	size = 1.0f;

	// Mark color (theme color with darkening -0.4)
	Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, color);
	CG_ColorWhitenSmart(color, NULL, -0.4f);
	color[3] = 0.8f;

	x = cgs.be.teamMarkers.ttwheelMarkX;
	y = cgs.be.teamMarkers.ttwheelMarkY;

	// Adjust coordinates from 640x480 to actual screen size
	CG_AdjustFrom640_Old(&x, &y, &size, &size, qfalse);

	size = 4.0f * scale;
	// Draw the mark as a small square
	trap_R_SetColor(color);
	trap_R_DrawStretchPic(
	    x - size * 0.5f,
	    y - size * 0.5f,
	    size,
	    size,
	    0,
	    0,
	    1,
	    1,
	    shader);

	trap_R_SetColor(NULL);
}

/*
=================
CG_GetSelwheelIconAtPosition

Check if position (x, y) is over any icon, return icon index or -1
=================
*/
int CG_GetSelwheelIconAtPosition(float x, float y)
{
	float centerX;
	float centerY;
	float iconRadius;
	float iconSize;
	float distFromCenter;
	float cursorAngle;
	float iconAngle;
	float angleStep;
	float minRadius;
	float maxRadius;
	float angleDiff;
	float normalizedAngleDiff;
	int i;
	int elementCount;

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return -1;
	}

	elementCount = cgs.be.teamMarkers.ttwheelElementCount;
	if (elementCount <= 0)
	{
		return -1;
	}

	// Get scale value
	{
		float scale;
		scale = cg_teamTaskWheelScale.value;
		if (scale <= 0.0f)
		{
			scale = 1.0f;
		}

		// Icons are fixed at screen center, not following the mark
		CG_GetScreenCenter(&centerX, &centerY);
		iconRadius = SELWHEEL_ICON_RADIUS * scale;
		iconSize = SELWHEEL_ICON_SIZE * scale;

		// Adjust coordinates from 640x480 to actual screen size
		CG_AdjustFrom640_Old(&centerX, &centerY, NULL, NULL, qfalse);
		CG_AdjustFrom640_Old(&x, &y, NULL, NULL, qfalse);
		CG_AdjustFrom640_Old(NULL, NULL, &iconRadius, &iconSize, qfalse);

		// Calculate distance from center and angle
		distFromCenter = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
		cursorAngle = atan2(-(y - centerY), x - centerX); // Negative Y because screen Y increases downward

		// Normalize angle to 0..2π range
		if (cursorAngle < 0.0f)
		{
			cursorAngle += 2.0f * M_PI;
		}

		// Define activation zone: from minRadius to maxRadius
		// minRadius: small area in center where we reset selection
		// maxRadius: extend to cover area between center and icons, and beyond
		minRadius = SELWHEEL_MIN_RADIUS * scale;
		maxRadius = iconRadius + SELWHEEL_MAX_RADIUS_EXTENSION * scale;
	}

	// Adjust radii to screen size
	{
		float tempW;
		float tempH;
		tempW = minRadius;
		tempH = minRadius;
		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		minRadius = tempW;

		tempW = maxRadius;
		tempH = maxRadius;
		CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		maxRadius = tempW;
	}

	// Check if bitmask & 2 is set (center element exists)
	if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
	{
		// Center element exists at index 0, check if cursor is in center zone
		if (distFromCenter < minRadius)
		{
			return 0; // Center zone - select center element
		}

		// For other elements (1+), use elementCount - 1 for angle calculation
		angleStep = 2.0f * M_PI / (float)(elementCount - 1);
		i = 1; // Start checking from index 1
	}
	else if ((cg_teamTaskWheel.integer & 2) && elementCount == 1)
	{
		// Only center element exists
		if (distFromCenter < minRadius)
		{
			return 0; // Center zone - select center element
		}
		return -1; // Outside center zone - no selection
	}
	else
	{
		// No center element, check if cursor is too close to center (reset zone)
		if (distFromCenter < minRadius)
		{
			return -1; // Center zone - no selection
		}
		angleStep = 2.0f * M_PI / (float)elementCount;
		i = 0; // Start checking from index 0
	}

	// Check if cursor is too far from center
	if (distFromCenter > maxRadius)
	{
		return -1; // Too far - no selection
	}

	// Check each element's sector (elements start from i)
	for (; i < cgs.be.teamMarkers.ttwheelElementCount; i++)
	{
		// Calculate element angle
		iconAngle = CG_GetSelwheelIconAngle(i, elementCount, (cg_teamTaskWheel.integer & 2) ? qtrue : qfalse);

		// Calculate angle difference
		angleDiff = cursorAngle - iconAngle;

		// Normalize to -π..π range
		if (angleDiff > M_PI)
		{
			angleDiff -= 2.0f * M_PI;
		}
		else if (angleDiff < -M_PI)
		{
			angleDiff += 2.0f * M_PI;
		}

		// Check if cursor is within this element's sector
		// Sector covers half the angle between elements on each side
		normalizedAngleDiff = fabs(angleDiff);
		if (normalizedAngleDiff <= angleStep * 0.5f)
		{
			return i;
		}
	}

	return -1;
}

/*
=================
CG_DrawSelwheelIcons

Draw team task icons in a circle around the ttwheel mark
=================
*/
void CG_DrawSelwheelIcons(void)
{
	float centerX;
	float centerY;
	float iconRadius;
	float iconSize;
	float backgroundSize;
	vec4_t iconColor;
	vec4_t backgroundColor;
	float iconX;
	float iconY;
	float adjustedX;
	float adjustedY;
	float adjustedSize;
	float adjustedBgSize;
	int hoveredIcon;
	int i;
	int font = CG_FontIndexFromCvar(&cg_teamTaskFont);
	float scale;

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	// Get scale value
	scale = cg_teamTaskWheelScale.value;
	if (scale <= 0.0f)
	{
		scale = 1.0f;
	}

	// Icons are fixed at screen center, not following the mark
	CG_GetScreenCenter(&centerX, &centerY);
	iconRadius = SELWHEEL_ICON_RADIUS * scale;
	iconSize = SELWHEEL_ICON_SIZE * scale;
	backgroundSize = iconSize * 1.2f; // Background size when hovered

	// Check for cooldown and display text above icons
	{
		int timeRemaining;
		int seconds;
		int milliseconds;
		float cooldownTextY;
		float cooldownTextX;
		float charWidth;
		float charHeight;
		vec4_t cooldownColor;
		char cooldownText[64];

		timeRemaining = TokenBucket_TimeToNext(&cgs.be.teamMarkers.sendCommandBucket, cg.time);

		/* Only show cooldown if there's actually a cooldown active */
		if (timeRemaining > 0)
		{
			seconds = timeRemaining / 1000;
			milliseconds = timeRemaining % 1000;

			/* Format: "Cooldown: X.X seconds" with 1 decimal place */
			Com_sprintf(cooldownText, sizeof(cooldownText), "Cooldown: %i.%i seconds", seconds, milliseconds / 100);

			/* Position text above icons - use screen coordinates directly */
			CG_GetScreenCenter(&cooldownTextX, NULL);
			cooldownTextY = 100; /* Above icons with some spacing */

			/* Text size */
			charWidth = 8.0f;
			charHeight = 12.0f;

			/* Use theme color */
			Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, cooldownColor);

			/* Apply whitening to cooldown color with medium strength */
			CG_ColorWhitenSmart(cooldownColor, NULL, 0.5f);
			CG_FontSelect(font);
			/* Draw cooldown text */
			CG_OSPDrawStringNew(
			    cooldownTextX, cooldownTextY,
			    cooldownText,
			    cooldownColor,
			    colorBlack,
			    charWidth, charHeight,
			    SCREEN_WIDTH, /* max width for centering */
			    DS_HCENTER | DS_PROPORTIONAL | DS_SHADOW,
			    NULL, NULL, NULL
			);
		}
	}

	// Icon color
	iconColor[0] = 1.0f;
	iconColor[1] = 1.0f;
	iconColor[2] = 1.0f;
	iconColor[3] = 1.0f;

	// Background color (use theme color with alpha 0.3)
	backgroundColor[0] = cgs.be.teamMarkers.teamTaskThemeColor[0];
	backgroundColor[1] = cgs.be.teamMarkers.teamTaskThemeColor[1];
	backgroundColor[2] = cgs.be.teamMarkers.teamTaskThemeColor[2];
	backgroundColor[3] = 0.3f;

	// Draw center wheel background (size slightly smaller than icon radius)
	{
		float wheelX;
		float wheelY;
		float wheelSize;
		vec4_t wheelColor;

		CG_GetScreenCenter(&wheelX, &wheelY);
		wheelSize = iconRadius * 5;

		// Use theme color for wheel background
		wheelColor[0] = 0;
		wheelColor[1] = 0;
		wheelColor[2] = 0;
		wheelColor[3] = 0.7f;

		CG_AdjustFrom640_Old(&wheelX, &wheelY, NULL, NULL, qfalse);
		CG_AdjustFrom640_Old(NULL, NULL, &wheelSize, &wheelSize, qfalse);

		if (cgs.media.teamTasksWheelbg)
		{
			trap_R_SetColor(wheelColor);
			trap_R_DrawStretchPic(
			    wheelX - wheelSize * 0.5f,
			    wheelY - wheelSize * 0.5f,
			    wheelSize,
			    wheelSize,
			    0,
			    0,
			    1,
			    1,
			    cgs.media.teamTasksWheelbg);
			trap_R_SetColor(NULL);
		}
	}

	// Check which icon is hovered
	hoveredIcon = CG_GetSelwheelIconAtPosition(cgs.be.teamMarkers.ttwheelMarkX, cgs.be.teamMarkers.ttwheelMarkY);
	cgs.be.teamMarkers.ttwheelHoveredIcon = hoveredIcon;

	// Fix selected icon when hovered (will be used on release)
	if (hoveredIcon >= 0 && hoveredIcon < cgs.be.teamMarkers.ttwheelElementCount)
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = hoveredIcon;
	}
	else
	{
		// Check if cursor is in center reset zone (only if no center element)
		if (!(cg_teamTaskWheel.integer & 2))
		{
			float centerXCheck;
			float centerYCheck;
			float markXCheck;
			float markYCheck;
			float distFromCenter;
			float minRadiusCheck;
			float tempW;
			float tempH;

			CG_GetScreenCenter(&centerXCheck, &centerYCheck);
			markXCheck = cgs.be.teamMarkers.ttwheelMarkX;
			markYCheck = cgs.be.teamMarkers.ttwheelMarkY;
			minRadiusCheck = SELWHEEL_MIN_RADIUS * scale;

			CG_AdjustFrom640_Old(&centerXCheck, &centerYCheck, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(&markXCheck, &markYCheck, NULL, NULL, qfalse);

			tempW = minRadiusCheck;
			tempH = minRadiusCheck;
			CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
			minRadiusCheck = tempW;

			distFromCenter = sqrt((markXCheck - centerXCheck) * (markXCheck - centerXCheck) +
			                      (markYCheck - centerYCheck) * (markYCheck - centerYCheck));

			// Only reset if cursor is in center zone and no center element
			if (distFromCenter < minRadiusCheck)
			{
				cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
			}
			// Otherwise keep previous selection (don't reset when in empty area)
		}
	}

	// Draw center element first (if bitmask & 2 is set)
	if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 0)
	{
		if (cgs.be.teamMarkers.ttwheelElementShaders[0])
		{
			float centerIconX;
			float centerIconY;
			float centerIconSize;
			float centerIconBgSize;
			float smartIconSize;
			float smartIconX;
			float smartIconY;
			vec4_t smartIconColor;

			CG_GetScreenCenter(&centerIconX, &centerIconY);
			centerIconSize = iconSize * 1.2f; // Larger center icon
			centerIconBgSize = backgroundSize * 1.2f; // Larger background for center icon
			smartIconSize = iconSize * 0.4f; // Smart icon size - small icon in corner

			CG_AdjustFrom640_Old(&centerIconX, &centerIconY, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &centerIconSize, &centerIconSize, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &centerIconBgSize, &centerIconBgSize, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &smartIconSize, &smartIconSize, qfalse);

			// Calculate smart icon position (right top corner of center icon)
			smartIconX = centerIconX + centerIconSize * 0.5f - smartIconSize * 0.5f;
			smartIconY = centerIconY - centerIconSize * 0.5f + smartIconSize * 0.5f;

			// If hovered or selected, draw background first
			if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
			{
				trap_R_SetColor(backgroundColor);
				trap_R_DrawStretchPic(
				    centerIconX - centerIconBgSize * 0.5f,
				    centerIconY - centerIconBgSize * 0.5f,
				    centerIconBgSize,
				    centerIconBgSize,
				    0,
				    0,
				    1,
				    1,
				    cgs.be.teamMarkers.ttwheelElementShaders[0]);
			}

			// Draw center icon
			// Use gray color for non-selected element, white for selected
			// Special handling for tHere: use theme color with whitening 0.8
			if (cgs.be.teamMarkers.ttwheelElementCommands[0][0] != '\0' &&
			        !Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[0], "tHere"))
			{
				// tHere element - use theme color with whitening 0.8
				vec4_t hereColor;
				Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, hereColor);
				CG_ColorWhitenSmart(hereColor, NULL, 0.8f);
				trap_R_SetColor(hereColor);
			}
			else if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
			{
				// Element is selected - use white color
				trap_R_SetColor(iconColor);
			}
			else
			{
				// Element is not selected - use gray color
				vec4_t grayColor;
				Vector4Set(grayColor, 0.75f, 0.75f, 0.75f, 1.0f);
				trap_R_SetColor(grayColor);
			}
			trap_R_DrawStretchPic(
			    centerIconX - centerIconSize * 0.5f,
			    centerIconY - centerIconSize * 0.5f,
			    centerIconSize,
			    centerIconSize,
			    0,
			    0,
			    1,
			    1,
			    cgs.be.teamMarkers.ttwheelElementShaders[0]);
			trap_R_SetColor(NULL);

			// Draw smart icon in right top corner of center icon (always, if command is tSmart)
			// Draw after main icon so it appears on top
			if (cgs.media.teamTasksSmart && cgs.be.teamMarkers.ttwheelElementCommands[0][0] != '\0' &&
			        !Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[0], "tSmart"))
			{
				// White color when element is not selected, theme color when selected
				if (hoveredIcon == 0 || cgs.be.teamMarkers.ttwheelSelectedIcon == 0)
				{
					// Element is selected - use theme color
					Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, smartIconColor);
					CG_ColorWhitenSmart(smartIconColor, NULL, -0.4f);
				}
				else
				{
					// Element is not selected - use white color
					Vector4Set(smartIconColor, 0.0f, 0.0f, 0.0f, 1.0f);
				}
				trap_R_SetColor(smartIconColor);
				trap_R_DrawStretchPic(
				    smartIconX - smartIconSize * 0.5f,
				    smartIconY - smartIconSize * 0.5f,
				    smartIconSize,
				    smartIconSize,
				    0,
				    0,
				    1,
				    1,
				    cgs.media.teamTasksSmart);
				trap_R_SetColor(NULL);
			}
		}

		// Draw other icons around circle (starting from index 1)
		i = 1;
	}
	else
	{
		// No center element, draw all icons around circle
		i = 0;
	}

	// Draw icons around circle
	for (; i < cgs.be.teamMarkers.ttwheelElementCount; i++)
	{
		if (!cgs.be.teamMarkers.ttwheelElementShaders[i])
		{
			continue;
		}

		// Calculate icon position using the SAME logic as CG_GetSelwheelIconAtPosition
		// to ensure icon is centered in activation block
		{
			float centerXIcon;
			float centerYIcon;
			float iconRadiusIcon;
			float iconAngleIcon;

			// Use same calculation as in CG_GetSelwheelIconAtPosition
			CG_GetScreenCenter(&centerXIcon, &centerYIcon);
			iconRadiusIcon = 60.0f * scale;

			// Adjust coordinates from 640x480 to actual screen size
			CG_AdjustFrom640_Old(&centerXIcon, &centerYIcon, NULL, NULL, qfalse);
			CG_AdjustFrom640_Old(NULL, NULL, &iconRadiusIcon, NULL, qfalse);

			// Calculate icon angle (same as in CG_GetSelwheelIconAtPosition)
			iconAngleIcon = CG_GetSelwheelIconAngle(i, cgs.be.teamMarkers.ttwheelElementCount, (cg_teamTaskWheel.integer & 2) ? qtrue : qfalse);

			// Calculate icon position (same as in CG_GetSelwheelIconAtPosition)
			// These coordinates are already scaled
			iconX = centerXIcon + cos(iconAngleIcon) * iconRadiusIcon;
			iconY = centerYIcon - sin(iconAngleIcon) * iconRadiusIcon; // Negative because screen Y increases downward
		}

		// iconX and iconY are already scaled, so use them directly
		adjustedX = iconX;
		adjustedY = iconY;
		adjustedSize = iconSize;
		adjustedBgSize = backgroundSize;

		// Adjust only size, not position (position is already scaled)
		CG_AdjustFrom640_Old(NULL, NULL, &adjustedSize, &adjustedSize, qfalse);
		CG_AdjustFrom640_Old(NULL, NULL, &adjustedBgSize, &adjustedBgSize, qfalse);

		// If hovered, draw background first (cyan, alpha 0.2, size 1.2)
		if (hoveredIcon == i)
		{
			// Draw background icon
			trap_R_SetColor(backgroundColor);
			trap_R_DrawStretchPic(
			    adjustedX - adjustedBgSize * 0.5f,
			    adjustedY - adjustedBgSize * 0.5f,
			    adjustedBgSize,
			    adjustedBgSize,
			    0,
			    0,
			    1,
			    1,
			    cgs.be.teamMarkers.ttwheelElementShaders[i]);
		}

		// Draw icon
		// Use gray color for non-selected elements, white for selected
		// Special handling for tHere: use theme color with whitening 0.8
		if (cgs.be.teamMarkers.ttwheelElementCommands[i][0] != '\0' &&
		        !Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[i], "tHere"))
		{
			// tHere element - use theme color with whitening 0.8
			vec4_t hereColor;
			Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, hereColor);
			CG_ColorWhitenSmart(hereColor, NULL, 0.8f);
			trap_R_SetColor(hereColor);
		}
		else if (hoveredIcon == i || cgs.be.teamMarkers.ttwheelSelectedIcon == i)
		{
			// Element is selected - use white color
			trap_R_SetColor(iconColor);
		}
		else
		{
			// Element is not selected - use gray color
			vec4_t grayColor;
			Vector4Set(grayColor, 0.6f, 0.6f, 0.6f, 1.0f);
			trap_R_SetColor(grayColor);
		}
		trap_R_DrawStretchPic(
		    adjustedX - adjustedSize * 0.5f,
		    adjustedY - adjustedSize * 0.5f,
		    adjustedSize,
		    adjustedSize,
		    0,
		    0,
		    1,
		    1,
		    cgs.be.teamMarkers.ttwheelElementShaders[i]);

		// Draw smart icon in right top corner of icon (always, if command is tSmart)
		if (cgs.media.teamTasksSmart && cgs.be.teamMarkers.ttwheelElementCommands[i][0] != '\0' &&
		        !Q_stricmp(cgs.be.teamMarkers.ttwheelElementCommands[i], "tSmart"))
		{
			float smartIconSizeWheel;
			float smartIconXWheel;
			float smartIconYWheel;
			vec4_t smartIconColorWheel;

			smartIconSizeWheel = adjustedSize * 0.4f; // Smart icon size - small icon in corner

			// Calculate smart icon position (right top corner of icon)
			smartIconXWheel = adjustedX + adjustedSize * 0.5f - smartIconSizeWheel * 0.5f;
			smartIconYWheel = adjustedY - adjustedSize * 0.5f + smartIconSizeWheel * 0.5f;

			// White color when element is not selected, theme color when selected
			if (hoveredIcon == i || cgs.be.teamMarkers.ttwheelSelectedIcon == i)
			{
				// Element is selected - use theme color
				Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, smartIconColorWheel);
				CG_ColorWhitenSmart(smartIconColorWheel, NULL, -0.4f);
			}
			else
			{
				// Element is not selected - use white color
				Vector4Set(smartIconColorWheel, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			trap_R_SetColor(smartIconColorWheel);
			trap_R_DrawStretchPic(
			    smartIconXWheel - smartIconSizeWheel * 0.5f,
			    smartIconYWheel - smartIconSizeWheel * 0.5f,
			    smartIconSizeWheel,
			    smartIconSizeWheel,
			    0,
			    0,
			    1,
			    1,
			    cgs.media.teamTasksSmart);
			trap_R_SetColor(NULL);
		}

		// // Debug: draw activation field border (yellow, sector)
		// // Draw sector boundaries from minRadius to maxRadius
		// {
		//  float centerXDebug;
		//  float centerYDebug;
		//  float iconAngleDebug;
		//  float startAngleDebug;
		//  float angleStepDebug;
		//  float minRadiusDebug;
		//  float maxRadiusDebug;
		//  float sectorStartAngle;
		//  float sectorEndAngle;
		//  float lineX1;
		//  float lineY1;
		//  float lineX2;
		//  float lineY2;
		//  float lineDX;
		//  float lineDY;
		//  float lineLength;
		//  int segmentCount;
		//  int seg;

		//  // EXACT copy of logic from CG_GetSelwheelIconAtPosition
		//  centerXDebug = SCREEN_WIDTH / 2.0f;
		//  centerYDebug = SCREEN_HEIGHT / 2.0f;
		//  minRadiusDebug = 5.0f; // Match minRadius from CG_GetSelwheelIconAtPosition
		//  maxRadiusDebug = 60.0f + 60.0f; // iconRadius + extension (match maxRadius)

		//  // Adjust coordinates from 640x480 to actual screen size
		//  CG_AdjustFrom640_Old(&centerXDebug, &centerYDebug, NULL, NULL, qfalse);
		//  {
		//      float tempW;
		//      float tempH;
		//      tempW = minRadiusDebug;
		//      tempH = minRadiusDebug;
		//      CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		//      minRadiusDebug = tempW;

		//      tempW = maxRadiusDebug;
		//      tempH = maxRadiusDebug;
		//      CG_AdjustFrom640_Old(NULL, NULL, &tempW, &tempH, qfalse);
		//      maxRadiusDebug = tempW;
		//  }

		//  // Start angle: 9 o'clock = 270 degrees = 3π/2 radians
		//  startAngleDebug = M_PI * 1.5f;
		//  if ((cg_teamTaskWheel.integer == 2) && cgs.be.teamMarkers.ttwheelElementCount > 1)
		//  {
		//      // Center element exists, calculate angles for elements around circle (i-1)
		//      angleStepDebug = 2.0f * M_PI / (float)(cgs.be.teamMarkers.ttwheelElementCount - 1);
		//      iconAngleDebug = startAngleDebug + angleStepDebug * (float)(i - 1);
		//  }
		//  else
		//  {
		//      // No center element, calculate angles for all elements
		//  angleStepDebug = 2.0f * M_PI / (float)cgs.be.teamMarkers.ttwheelElementCount;
		//  iconAngleDebug = startAngleDebug + angleStepDebug * (float)i;
		//  }
		//  sectorStartAngle = iconAngleDebug - angleStepDebug * 0.5f;
		//  sectorEndAngle = iconAngleDebug + angleStepDebug * 0.5f;

		//  // Border thickness (1 pixel, scale it)
		//  borderThickness = 1.0f;
		//  CG_AdjustFrom640_Old(NULL, NULL, &borderThickness, NULL, qfalse);

		//  // Yellow color
		//  debugColor[0] = 1.0f; // R
		//  debugColor[1] = 1.0f; // G
		//  debugColor[2] = 0.0f; // B
		//  debugColor[3] = 1.0f; // A

		//  trap_R_SetColor(debugColor);

		//  // Draw sector boundaries: two radial lines from minRadius to maxRadius
		//  // Left boundary (sectorStartAngle)
		//  lineX1 = centerXDebug + cos(sectorStartAngle) * minRadiusDebug;
		//  lineY1 = centerYDebug - sin(sectorStartAngle) * minRadiusDebug;
		//  lineX2 = centerXDebug + cos(sectorStartAngle) * maxRadiusDebug;
		//  lineY2 = centerYDebug - sin(sectorStartAngle) * maxRadiusDebug;
		//  lineDX = lineX2 - lineX1;
		//  lineDY = lineY2 - lineY1;
		//  lineLength = sqrt(lineDX * lineDX + lineDY * lineDY);
		//  // Draw line as rotated rectangle (simplified - draw horizontal line and rotate)
		//  // For simplicity, draw as multiple small segments
		//  segmentCount = (int)(lineLength / borderThickness) + 1;
		//  if (segmentCount > 100)
		//  {
		//      segmentCount = 100;
		//  }
		//  for (seg = 0; seg < segmentCount; seg++)
		//  {
		//      float segFrac;
		//      float segX;
		//      float segY;
		//      segFrac = (float)seg / (float)segmentCount;
		//      segX = lineX1 + lineDX * segFrac;
		//      segY = lineY1 + lineDY * segFrac;
		//      trap_R_DrawStretchPic(segX - borderThickness * 0.5f, segY - borderThickness * 0.5f, borderThickness, borderThickness, 0, 0, 0, 0, cgs.media.whiteShader);
		//  }

		//  // Right boundary (sectorEndAngle)
		//  lineX1 = centerXDebug + cos(sectorEndAngle) * minRadiusDebug;
		//  lineY1 = centerYDebug - sin(sectorEndAngle) * minRadiusDebug;
		//  lineX2 = centerXDebug + cos(sectorEndAngle) * maxRadiusDebug;
		//  lineY2 = centerYDebug - sin(sectorEndAngle) * maxRadiusDebug;
		//  lineDX = lineX2 - lineX1;
		//  lineDY = lineY2 - lineY1;
		//  lineLength = sqrt(lineDX * lineDX + lineDY * lineDY);
		//  segmentCount = (int)(lineLength / borderThickness) + 1;
		//  if (segmentCount > 100)
		//  {
		//      segmentCount = 100;
		//  }
		//  for (seg = 0; seg < segmentCount; seg++)
		//  {
		//      float segFrac;
		//      float segX;
		//      float segY;
		//      segFrac = (float)seg / (float)segmentCount;
		//      segX = lineX1 + lineDX * segFrac;
		//      segY = lineY1 + lineDY * segFrac;
		//      trap_R_DrawStretchPic(segX - borderThickness * 0.5f, segY - borderThickness * 0.5f, borderThickness, borderThickness, 0, 0, 0, 0, cgs.media.whiteShader);
		//  }

		//  trap_R_SetColor(NULL);
		// }
	}

	trap_R_SetColor(NULL);
}

/*
=================
CG_DrawSelwheelPointers

Draw arrows pointing to ttwheel elements (must be called before trap_R_RenderScene)
=================
*/
void CG_DrawSelwheelPointers(void)
{
	float pointerRadius;
	float pointerSize;
	float pointerAngle;
	vec4_t activeColor;
	vec4_t inactiveColor;
	int selectedIcon;
	int elementCount;
	int i;
	int startIndex;

	if (!(cg_teamTaskWheel.integer & 8))
	{
		return;
	}

	if (!cgs.be.teamMarkers.ttwheelMarkActive)
	{
		return;
	}

	// Check if shader is loaded, if not try to use whiteShader as fallback
	if (!cgs.media.teamTasksPointing)
	{
		// Try fallback to whiteShader for testing
		if (!cgs.media.whiteShader)
		{
			return;
		}
		// Will use whiteShader below
	}

	elementCount = cgs.be.teamMarkers.ttwheelElementCount;
	if (elementCount <= 0)
	{
		return;
	}

	selectedIcon = cgs.be.teamMarkers.ttwheelSelectedIcon;

	// Get scale value
	{
		float scale;
		scale = cg_teamTaskWheelScale.value;
		if (scale <= 0.0f)
		{
			scale = 1.0f;
		}

		// Pointer radius: between center and icons (in unused zone)
		// Icons use iconRadius = 20.0f in 640x480 coordinates
		// Use smaller radius for pointers (between center and icons)
		pointerRadius = 24.0f * scale; // Position between center and icons (in 640x480 coordinates)
		pointerSize = 8.0f * scale; // Size of pointer arrow (increased for visibility)
	}

	Vector4Copy(cgs.be.teamMarkers.teamTaskThemeColor, activeColor);
	CG_ColorWhitenSmart(activeColor, NULL, 0.2f);
	// Active color (white) for selected element

	// Inactive color (gray) for non-selected elements
	Vector4Set(inactiveColor, 0.6f, 0.6f, 0.6f, 0.8f);

	// Determine start index based on center element
	if ((cg_teamTaskWheel.integer & 2) && elementCount > 1)
	{
		// Center element exists at index 0, draw pointers for other elements (1+)
		startIndex = 1;
	}
	else if ((cg_teamTaskWheel.integer & 2) && elementCount == 1)
	{
		// Only center element exists, no pointers needed
		return;
	}
	else
	{
		// No center element, draw pointers for all elements
		startIndex = 0;
	}

	// Draw pointers for each element
	for (i = startIndex; i < elementCount; i++)
	{
		// Calculate angle for this element
		pointerAngle = CG_GetSelwheelIconAngle(i, elementCount, (cg_teamTaskWheel.integer & 2) ? qtrue : qfalse);

		// Calculate pointer position
		// CG_DrawRotatedPic expects coordinates in 640x480 virtual system, not scaled
		// Compensate for different X/Y scaling due to FOV to keep perfect circle
		{
			float pointerX;
			float pointerY;
			qhandle_t pointerShader;
			vec4_t pointerColor;
			float centerX;
			float centerY;
			float hfov_x;
			float hfov_y;
			float compensation;
			float dx;
			float dy;

			// Use 640x480 coordinates (320, 240 = center)
			// CG_DrawRotatedPic will handle scaling internally
			CG_GetScreenCenter(&centerX, &centerY);

			// Calculate FOV to compensate for ellipse distortion
			hfov_x = cg.refdef.fov_x * M_PI / 360.0f;
			hfov_y = cg.refdef.fov_y * M_PI / 360.0f;

			// Compensation factor to make circle perfect (compensate for different X/Y scaling)
			// CG_DrawRotatedPic uses: offsetX = dx * tan(hfov_x) / (SCREEN_WIDTH * 0.5f)
			//                        offsetY = dy * tan(hfov_y) / (SCREEN_HEIGHT * 0.5f)
			// To make circle, we need: offsetX/offsetY ratio to be 1:1
			// So we compensate Y coordinate
			compensation = (tan(hfov_x) * SCREEN_HEIGHT) / (tan(hfov_y) * SCREEN_WIDTH);

			// Calculate position in 640x480 coordinates
			dx = -cos(pointerAngle) * pointerRadius;
			dy = -sin(pointerAngle) * pointerRadius;

			// Apply compensation to Y to keep perfect circle
			dy = dy * compensation;

			pointerX = centerX + dx;
			pointerY = centerY + dy; // Negative because screen Y increases downward

			// Use teamTasksPointing if available, otherwise fallback to whiteShader
			if (cgs.media.teamTasksPointing)
			{
				pointerShader = cgs.media.teamTasksPointing;
			}
			else
			{
				pointerShader = cgs.media.whiteShader;
			}

			// Determine color based on selection
			if (selectedIcon == i)
			{
				// Selected element - white
				Vector4Copy(activeColor, pointerColor);
			}
			else
			{
				// Non-selected element - gray
				Vector4Copy(inactiveColor, pointerColor);
			}

			// Draw pointer arrow
			// CG_DrawRotatedPic expects coordinates and size in 640x480 virtual system
			// Arrow should point outward (toward icon), so add 180 degrees to point away from center
			// Rotate 90 degrees counter-clockwise (subtract 90 degrees)
			CG_DrawRotatedPic(
			    pointerX,
			    pointerY,
			    pointerSize,
			    pointerSize,
			    pointerShader,
			    pointerAngle * 180.0f / M_PI + 180.0f + 90.0f, // Convert to degrees, add 180 to point outward, subtract 90 for counter-clockwise rotation
			    pointerColor
			);
		}
	}
}

/*
=================
CG_SelwheelRestoreSensitivity

Restore sensitivity if it was adjusted for wheel
=================
*/
static void CG_SelwheelRestoreSensitivity(void)
{
	float storedSensitivity;
	char sensitivityStr[32];

	if (cgs.be.teamMarkers.sensitivityAdjusted)
	{
		// Update cvar to get latest value
		trap_Cvar_Update(&cg_storedSensitivity);

		// Get stored sensitivity value directly from cvar (float, preserves precision)
		storedSensitivity = cg_storedSensitivity.value;
		if (storedSensitivity < 0.0f)
		{
			storedSensitivity = 0.0f;
		}

		// Restore original sensitivity value
		Com_sprintf(sensitivityStr, sizeof(sensitivityStr), "%f", storedSensitivity);
		trap_Cvar_Set("sensitivity", sensitivityStr);

		cgs.be.teamMarkers.sensitivityAdjusted = qfalse;
	}
}

/*
=================
CG_SelwheelMarkStart

Start ttwheel mark tracking
=================
*/
void CG_SelwheelMarkStart(void)
{
	int currentCmdNum;
	usercmd_t currentCmd;

	/* Check if teamTask is disabled */
	if (cgs.gametype < GT_TEAM)
	{
		return;
	}
	if (!cg_teamTask.integer)
	{
		return;
	}
	if (!CG_IsTeamTaskSupported())
	{
		return;
	}
	if (!CG_BE_FEATURE_ENABLED(CG_BE_TEAMTASKS))
	{
		return;
	}
	/* Check if teamTaskWheel is disabled */
	if (!cg_teamTaskWheel.integer)
	{
		return;
	}
	/* Check if we are not playing (observing another client) */
	// if (!CG_IsLocalClientOnScreen())
	// {
	//  return;
	// }
	// if (CG_IsDead())
	// {
	//  return;
	// }
	/* Check if we are in spectator/following mode */
	// if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	// {
	//  return;
	// }
	/* Check if local player is spectator */
	if (CG_IsLocalClientSpectator())
	{
		return;
	}

	// If already active, don't reset position - just update angles
	if (cgs.be.teamMarkers.ttwheelMarkActive)
	{
		currentCmdNum = trap_GetCurrentCmdNumber();
		if (trap_GetUserCmd(currentCmdNum, &currentCmd))
		{
			cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
			cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
			cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];
		}
		return;
	}

	// First time activation - initialize position to center
	cgs.be.teamMarkers.ttwheelMarkActive = qtrue;
	CG_GetScreenCenter(&cgs.be.teamMarkers.ttwheelMarkX, &cgs.be.teamMarkers.ttwheelMarkY);
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;

	// If smart mode is enabled (bitmask & 2), save target at wheel opening
	if (cg_teamTaskWheel.integer & 2)
	{
		vec3_t itemPos;
		trace_t trace;
		vec3_t start;
		vec3_t end;
		int i;
		centity_t* cent;

		// Check what we're aiming at using CG_CrosshairTarget
		cgs.be.teamMarkers.smartTargetType = CG_CrosshairTarget(itemPos);
		cgs.be.teamMarkers.smartTargetTime = cg.time;
		cgs.be.teamMarkers.smartTargetEntityNum = -1;

		// Save target position and entity number
		if (cgs.be.teamMarkers.smartTargetType == 2)
		{
			// Enemy - get their position and entity number
			VectorCopy(cg.refdef.vieworg, start);
			VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

			CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
			         cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);

			if (trace.entityNum >= 0 && trace.entityNum < MAX_CLIENTS)
			{
				cgs.be.teamMarkers.smartTargetEntityNum = trace.entityNum;
				// Find player entity and get their position
				for (i = 0; i < MAX_GENTITIES; i++)
				{
					cent = &cg_entities[i];
					if (cent->currentState.eType == ET_PLAYER &&
					        cent->currentState.clientNum == trace.entityNum &&
					        cent->currentValid)
					{
						VectorCopy(cent->lerpOrigin, cgs.be.teamMarkers.smartTargetPos);
						cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
						break;
					}
				}
				// Fallback: if enemy entity not found, use trace endpos
				if (i >= MAX_GENTITIES)
				{
					VectorCopy(trace.endpos, cgs.be.teamMarkers.smartTargetPos);
					cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
				}
			}
			else
			{
				// Fallback: use trace endpos
				VectorCopy(trace.endpos, cgs.be.teamMarkers.smartTargetPos);
				cgs.be.teamMarkers.smartTargetPos[2] += 24.0f;
			}
		}
		else if (cgs.be.teamMarkers.smartTargetType == 3 || cgs.be.teamMarkers.smartTargetType == 4 ||
		         cgs.be.teamMarkers.smartTargetType == 7 || cgs.be.teamMarkers.smartTargetType == 8 ||
		         cgs.be.teamMarkers.smartTargetType == 9 || cgs.be.teamMarkers.smartTargetType == 10 ||
		         cgs.be.teamMarkers.smartTargetType == 11 || cgs.be.teamMarkers.smartTargetType == 12)
		{
			// Item (red armor, megahealth, quaddamage, regeneration, battlesuit, haste, yellow armor, health) - use item center position
			VectorCopy(itemPos, cgs.be.teamMarkers.smartTargetPos);
		}
		else if (cgs.be.teamMarkers.smartTargetType == 5 || cgs.be.teamMarkers.smartTargetType == 6)
		{
			// Frozen body (ally or enemy) - use frozen body position
			VectorCopy(itemPos, cgs.be.teamMarkers.smartTargetPos);
		}
		else
		{
			// Nothing - clear target
			cgs.be.teamMarkers.smartTargetType = 1;
			VectorClear(cgs.be.teamMarkers.smartTargetPos);
		}
	}
	else
	{
		// Not smart mode - clear saved target
		cgs.be.teamMarkers.smartTargetType = 1;
		VectorClear(cgs.be.teamMarkers.smartTargetPos);
		cgs.be.teamMarkers.smartTargetTime = 0;
		cgs.be.teamMarkers.smartTargetEntityNum = -1;
	}

	// Initialize default elements if not set
	if (cgs.be.teamMarkers.ttwheelElementCount == 0)
	{
		CG_SelwheelParseTeamTaskWheel();
	}

	// If bitmask & 2 is set, select center element (index 0) by default
	if ((cg_teamTaskWheel.integer & 2) && cgs.be.teamMarkers.ttwheelElementCount > 0)
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = 0;
	}
	else
	{
		cgs.be.teamMarkers.ttwheelSelectedIcon = -1;
	}

	currentCmdNum = trap_GetCurrentCmdNumber();
	if (trap_GetUserCmd(currentCmdNum, &currentCmd))
	{
		cgs.be.teamMarkers.lastCmdNum = currentCmdNum;
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = currentCmd.angles[PITCH];
		cgs.be.teamMarkers.lastCmdAngles[YAW] = currentCmd.angles[YAW];
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = currentCmd.angles[ROLL];
	}
	else
	{
		cgs.be.teamMarkers.lastCmdNum = 0;
		cgs.be.teamMarkers.lastCmdAngles[PITCH] = 0;
		cgs.be.teamMarkers.lastCmdAngles[YAW] = 0;
		cgs.be.teamMarkers.lastCmdAngles[ROLL] = 0;
	}

	cgs.be.teamMarkers.lastMovementTime = cg.time;

	// Adjust sensitivity by default, unless bitmask & 16 is set (disable aim lock)
	if (!(cg_teamTaskWheel.integer & 16))
	{
		float currentSensitivity;
		float currentWheelSensitivity;
		float newWheelSensitivity;
		char sensitivityStr[32];

		// Update cvar to get latest value
		trap_Cvar_Update(&sensitivity);

		// Get current sensitivity value directly from cvar (float, preserves precision)
		currentSensitivity = sensitivity.value;
		if (currentSensitivity < 0.0f)
		{
			currentSensitivity = 0.0f;
		}

		// Get current wheel sensitivity
		currentWheelSensitivity = cg_teamTaskWheelSensitivity.value;

		// Save values
		cgs.be.teamMarkers.savedSensitivity = currentSensitivity;
		cgs.be.teamMarkers.savedWheelSensitivity = currentWheelSensitivity;

		// Store original sensitivity value directly in cvar (float, preserves precision)
		Com_sprintf(sensitivityStr, sizeof(sensitivityStr), "%f", currentSensitivity);
		trap_Cvar_Set("cg_storedSensitivity", sensitivityStr);

		// Calculate new wheel sensitivity to maintain overall sensitivity
		// old_sens * wheel_sens = new_sens * new_wheel_sens
		// new_wheel_sens = (old_sens * wheel_sens) / new_sens
		if (currentSensitivity > 0.002f)
		{
			newWheelSensitivity = (currentSensitivity * currentWheelSensitivity) / 0.002f;
		}
		else
		{
			// If sensitivity is already very low, don't adjust
			newWheelSensitivity = currentWheelSensitivity;
		}

		cgs.be.teamMarkers.adjustedWheelSensitivity = newWheelSensitivity;
		cgs.be.teamMarkers.sensitivityAdjusted = qtrue;

		// Set sensitivity to 0.001 (use precise string to avoid float precision loss)
		trap_Cvar_Set("sensitivity", "0.002");
	}
	else
	{
		cgs.be.teamMarkers.sensitivityAdjusted = qfalse;
	}
}

/*
=================
CG_SelwheelMarkStop

Stop ttwheel mark tracking and execute selected command
=================
*/
void CG_SelwheelMarkStop(void)
{
	int selectedIcon;
	char commandStr[MAX_STRING_CHARS];

	// Use fixed selected icon (was set when cursor hovered over icon)
	selectedIcon = cgs.be.teamMarkers.ttwheelSelectedIcon;

	// Send command based on selected icon
	if (selectedIcon >= 0 && selectedIcon < cgs.be.teamMarkers.ttwheelElementCount)
	{
		// Send console command first (add semicolon if not present)
		if (cgs.be.teamMarkers.ttwheelElementCommands[selectedIcon][0] != '\0')
		{
			int cmdLen;
			Q_strncpyz(commandStr, cgs.be.teamMarkers.ttwheelElementCommands[selectedIcon], sizeof(commandStr));
			// Ensure command ends with semicolon
			cmdLen = strlen(commandStr);
			if (cmdLen > 0 && commandStr[cmdLen - 1] != ';')
			{
				Q_strcat(commandStr, sizeof(commandStr), ";");
			}
			// Send command BEFORE deactivating ttwheel
			trap_SendConsoleCommand(commandStr);
		}

		// Call the action callback for this element
		if (cgs.be.teamMarkers.ttwheelElementActions[selectedIcon])
		{
			cgs.be.teamMarkers.ttwheelElementActions[selectedIcon]();
		}
	}

	// Restore sensitivity if it was adjusted
	CG_SelwheelRestoreSensitivity();

	// Deactivate ttwheel AFTER sending command
	cgs.be.teamMarkers.ttwheelMarkActive = qfalse;
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;
	cgs.be.teamMarkers.ttwheelSelectedIcon = -1;

	// Clear saved smart target
	cgs.be.teamMarkers.smartTargetType = 1;
	VectorClear(cgs.be.teamMarkers.smartTargetPos);
	cgs.be.teamMarkers.smartTargetTime = 0;
	cgs.be.teamMarkers.smartTargetEntityNum = -1;
}

/*
=================
CG_SelwheelMarkClose

Close ttwheel without sending command or executing action
=================
*/
void CG_SelwheelMarkClose(void)
{
	/* Restore sensitivity if it was adjusted */
	CG_SelwheelRestoreSensitivity();

	/* Deactivate ttwheel without sending command or executing action */
	cgs.be.teamMarkers.ttwheelMarkActive = qfalse;
	cgs.be.teamMarkers.ttwheelHoveredIcon = -1;
	cgs.be.teamMarkers.ttwheelSelectedIcon = -1;

	/* Clear saved smart target */
	cgs.be.teamMarkers.smartTargetType = 1;
	VectorClear(cgs.be.teamMarkers.smartTargetPos);
	cgs.be.teamMarkers.smartTargetTime = 0;
	cgs.be.teamMarkers.smartTargetEntityNum = -1;
}

/*
=================
CG_SelwheelSetElements

Set the elements for ttwheel menu
count: number of elements (max MAX_SELWHEEL_ELEMENTS)
shaders: array of shader handles for icons
actions: array of function pointers for actions
commands: array of console command strings (e.g. "tAttack", "tDefence", etc.)
=================
*/
void CG_SelwheelSetElements(int count, qhandle_t* shaders, void (**actions)(void), const char** commands)
{
	int i;

	if (count < 0 || count > MAX_SELWHEEL_ELEMENTS)
	{
		count = 0;
	}

	cgs.be.teamMarkers.ttwheelElementCount = count;

	for (i = 0; i < count; i++)
	{
		if (shaders)
		{
			cgs.be.teamMarkers.ttwheelElementShaders[i] = shaders[i];
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementShaders[i] = 0;
		}

		if (actions)
		{
			cgs.be.teamMarkers.ttwheelElementActions[i] = actions[i];
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementActions[i] = NULL;
		}

		if (commands && commands[i])
		{
			Q_strncpyz(cgs.be.teamMarkers.ttwheelElementCommands[i], commands[i], sizeof(cgs.be.teamMarkers.ttwheelElementCommands[i]));
		}
		else
		{
			cgs.be.teamMarkers.ttwheelElementCommands[i][0] = '\0';
		}
	}

	// Clear remaining slots
	for (i = count; i < MAX_SELWHEEL_ELEMENTS; i++)
	{
		cgs.be.teamMarkers.ttwheelElementShaders[i] = 0;
		cgs.be.teamMarkers.ttwheelElementActions[i] = NULL;
		cgs.be.teamMarkers.ttwheelElementCommands[i][0] = '\0';
	}
}

/*
=================
CG_SelwheelParseTeamTaskWheel

Parse cg_teamTaskWheelSet cvar and set up ttwheel elements
=================
*/
void CG_SelwheelParseTeamTaskWheel(void)
{
	char buffer[MAX_CVAR_VALUE_STRING];
	char* parse;
	char* token;
	int count;
	qhandle_t shaders[MAX_SELWHEEL_ELEMENTS];
	void (*actions[MAX_SELWHEEL_ELEMENTS])(void);
	const char* commands[MAX_SELWHEEL_ELEMENTS];
	int i;
	qboolean hasCenterElement;

	/* Use global teamTaskCommands array */

	count = 0;
	hasCenterElement = (cg_teamTaskWheel.integer & 2) ? qtrue : qfalse;

	/* If bitmask & 2 is set and cg_teamTaskWheelSet is not empty, use first element as center */
	if (hasCenterElement && cg_teamTaskWheelSet.string[0] != '\0')
	{
		char* firstToken;
		char* firstParse;
		teamTaskCommand_t* cmd;
		qhandle_t centerShader;
		qboolean centerElementSet;

		centerElementSet = qfalse;

		/* Parse first token from cvar */
		Q_strncpyz(buffer, cg_teamTaskWheelSet.string, sizeof(buffer));
		firstParse = buffer;

		/* Skip whitespace */
		while (*firstParse == ' ' || *firstParse == '\t')
		{
			firstParse++;
		}

		if (*firstParse != '\0')
		{
			firstToken = firstParse;
			while (*firstParse && *firstParse != ' ' && *firstParse != '\t')
			{
				firstParse++;
			}

			if (*firstParse)
			{
				*firstParse = '\0';
			}

			/* Find matching element using global array */
			cmd = CG_GetTeamTaskCommandByName(firstToken);
			if (cmd && cmd->action != NULL)
			{
				centerShader = CG_GetTeamTaskShaderByType(cmd->taskType);
				if (centerShader)
				{
					shaders[count] = centerShader;
					actions[count] = cmd->action;
					commands[count] = cmd->command;
					count++;
					centerElementSet = qtrue;
				}
			}
		}

		/* If first element from cvar failed, fallback to tSmart */
		if (!centerElementSet)
		{
			cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);
			if (cmd && cmd->action != NULL)
			{
				centerShader = CG_GetTeamTaskShaderByType(cmd->taskType);
				if (centerShader)
				{
					shaders[count] = centerShader;
					actions[count] = CG_TeamTaskMarkSend;
					commands[count] = "tSmart";
					count++;
					centerElementSet = qtrue;
				}
			}
			if (!centerElementSet && cgs.media.teamTasksLocation)
			{
				shaders[count] = cgs.media.teamTasksLocation;
				actions[count] = CG_TeamTaskMarkSend;
				commands[count] = "tSmart";
				count++;
			}
		}
	}
	/* If bitmask & 2 is set but cg_teamTaskWheelSet is empty, add tSmart as center element (index 0) */
	else if (hasCenterElement)
	{
		teamTaskCommand_t* cmd;
		qhandle_t smartShader;

		/* Get default type from cvar (string name) */
		cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);

		/* Check if command is valid and has action (for icon display, isSelectable check is not needed) */
		if (cmd && cmd->action != NULL)
		{
			smartShader = CG_GetTeamTaskShaderByType(cmd->taskType);
			if (smartShader)
			{
				shaders[count] = smartShader;
				actions[count] = CG_TeamTaskMarkSend;
				commands[count] = "tSmart";
				count++;
			}
		}
		/* Fallback to location if cvar is invalid or shader not loaded */
		if (count == 0 && cgs.media.teamTasksLocation)
		{
			shaders[count] = cgs.media.teamTasksLocation;
			actions[count] = CG_TeamTaskMarkSend;
			commands[count] = "tSmart";
			count++;
		}
	}

	/* If cvar is empty, use default elements (all commands with actions, excluding auto-selected) */
	if (cg_teamTaskWheelSet.string[0] == '\0')
	{
		/* Add tSmart as first element if hasCenterElement is not set */
		if (!hasCenterElement && count < MAX_SELWHEEL_ELEMENTS)
		{
			teamTaskCommand_t* cmd;
			qhandle_t smartShader;

			/* Get default type from cvar (string name) */
			cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);

			/* Check if command is valid and has action */
			if (cmd && cmd->action != NULL)
			{
				smartShader = CG_GetTeamTaskShaderByType(cmd->taskType);
				if (smartShader)
				{
					shaders[count] = smartShader;
					actions[count] = CG_TeamTaskMarkSend;
					commands[count] = "tSmart";
					count++;
				}
			}
			/* Fallback to location if cvar is invalid or shader not loaded */
			if (count == 0 && cgs.media.teamTasksLocation)
			{
				shaders[count] = cgs.media.teamTasksLocation;
				actions[count] = CG_TeamTaskMarkSend;
				commands[count] = "tSmart";
				count++;
			}
		}

		/* Use all default elements that have actions (exclude auto-selected: target, redarmor, megahealth) */
		for (i = 0; i < NUM_TEAMTASK_COMMANDS && count < MAX_SELWHEEL_ELEMENTS; i++)
		{
			if (teamTaskCommands[i].action != NULL)
			{
				shaders[count] = CG_GetTeamTaskShaderByType(teamTaskCommands[i].taskType);
				actions[count] = teamTaskCommands[i].action;
				commands[count] = teamTaskCommands[i].command;
				count++;
			}
		}
	}
	else
	{
		/* Parse cvar string - skip first token if it was used as center element */
		Q_strncpyz(buffer, cg_teamTaskWheelSet.string, sizeof(buffer));
		parse = buffer;

		/* Skip first token if center element was set from cvar */
		if (hasCenterElement && count > 0)
		{
			/* Skip whitespace */
			while (*parse == ' ' || *parse == '\t')
			{
				parse++;
			}

			/* Skip first token */
			while (*parse && *parse != ' ' && *parse != '\t')
			{
				parse++;
			}
		}

		while (*parse && count < MAX_SELWHEEL_ELEMENTS)
		{
			/* Skip whitespace */
			while (*parse == ' ' || *parse == '\t')
			{
				parse++;
			}

			if (*parse == '\0')
			{
				break;
			}

			/* Get token */
			token = parse;
			while (*parse && *parse != ' ' && *parse != '\t')
			{
				parse++;
			}

			if (*parse)
			{
				*parse = '\0';
				parse++;
			}

			/* Find matching element using global array */
			{
				teamTaskCommand_t* cmd;
				qboolean elementAdded;

				elementAdded = qfalse;

				/* Check if token is "tSmart" or "smart" */
				if (!Q_stricmp(token, "tSmart") || !Q_stricmp(token, "smart"))
				{
					qhandle_t smartShader;

					/* Get default type from cvar (string name) */
					cmd = CG_GetTeamTaskCommandByName(cg_teamTaskSmartDefault.string);

					/* Check if command is valid and has action */
					if (cmd && cmd->action != NULL)
					{
						smartShader = CG_GetTeamTaskShaderByType(cmd->taskType);
						if (smartShader)
						{
							shaders[count] = smartShader;
							actions[count] = CG_TeamTaskMarkSend;
							commands[count] = "tSmart";
							count++;
							elementAdded = qtrue;
						}
					}
					/* Fallback to location if cvar is invalid or shader not loaded */
					if (!elementAdded && cgs.media.teamTasksLocation)
					{
						shaders[count] = cgs.media.teamTasksLocation;
						actions[count] = CG_TeamTaskMarkSend;
						commands[count] = "tSmart";
						count++;
						elementAdded = qtrue;
					}
				}

				/* If not tSmart, try to find in global array */
				if (!elementAdded)
				{
					cmd = CG_GetTeamTaskCommandByName(token);
					if (cmd && cmd->action != NULL)
					{
						shaders[count] = CG_GetTeamTaskShaderByType(cmd->taskType);
						actions[count] = cmd->action;
						commands[count] = cmd->command;
						count++;
					}
				}
			}
		}
	}

	/* Set elements */
	if (count > 0)
	{
		CG_SelwheelSetElements(count, shaders, actions, commands);
	}
	else
	{
		/* Fallback to default if parsing failed */
		CG_SelwheelSetElements(0, NULL, NULL, NULL);
	}
}
