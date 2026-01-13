#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"


typedef struct
{
	superhudConfig_t config;
	superhudBarContext_t ctx;
	int maxammo[WP_NUM_WEAPONS];
	int last_weapon;
	int last_maxammo;
	int red;
	int white;
} shudElementStatusbarAmmoBar;

void* CG_SHUDElementSBAmBCreate(const superhudConfig_t* config)
{
	int i;
	shudElementStatusbarAmmoBar* element;

	SHUD_ELEMENT_INIT(element, config);

	/* Initialize all weapons with default value 200 */
	for (i = 0; i < WP_NUM_WEAPONS; ++i)
	{
		element->maxammo[i] = 200;
	}
	element->last_weapon = -1;
	element->last_maxammo = -1;

	/* Parse weaponMaxValue if set */
	if (element->config.weaponMaxValue.isSet)
	{
		CG_SHUDParseWeaponMaxValue(element->config.weaponMaxValue.value, element->maxammo, WP_NUM_WEAPONS);
	}

	return element;
}

void CG_SHUDElementSBAmBRoutine(void* context)
{
	shudElementStatusbarAmmoBar* element = (shudElementStatusbarAmmoBar*)context;
	int wp;
	int ammo;

	if (!cg.snap)
	{
		return;
	}

	wp = cg.snap->ps.weapon;

	if (wp == WP_NONE || wp == WP_GAUNTLET) return;

	ammo = CG_SHUDGetAmmo(wp);

	/* Get the maximum ammo for current weapon - must be exactly what we set */
	{
		int current_maxammo;
		current_maxammo = element->maxammo[wp];

		/* Update context only if weapon changed OR maxammo changed */
		if (wp != element->last_weapon || element->last_weapon == -1 || current_maxammo != element->last_maxammo)
		{
			/* Weapon or maxammo changed, reinitialize */
			element->last_weapon = wp;
			element->last_maxammo = current_maxammo;
			element->white = current_maxammo / 2;
			element->red = current_maxammo / 8;
			CG_SHUDBarMakeContext(&element->config, &element->ctx, current_maxammo);
		}
	}

	element->ctx.color_top[3] = 1.0f;

	if (ammo > element->white)
	{
		VectorCopy(colorWhite, element->ctx.color_top);
	}
	else if (ammo > element->red)
	{
		// Use color2 if set, otherwise use default yellow
		if (element->config.color2.isSet)
		{
			Vector4Copy(element->config.color2.value.rgba, element->ctx.color_top);
		}
		else
		{
			VectorSet(element->ctx.color_top, 1.0f, 0.7f, 0.0f);
		}
	}
	else if (cg.time & 512)
	{
		VectorCopy(colorRed, element->ctx.color_top);
	}
	else
	{
		element->ctx.color_top[3] = 0;
	}

	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);
	CG_SHUDBarPrint(&element->config, &element->ctx, ammo);
}

void CG_SHUDElementSBAmBDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

