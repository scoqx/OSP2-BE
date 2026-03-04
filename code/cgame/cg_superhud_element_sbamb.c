#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

/* Low ammo thresholds - same as in CG_CheckAmmo */
static int lowAmmoThresholds[WP_NUM_WEAPONS] =
{
	/* WP_NONE */            0,
	/* WP_GAUNTLET */        0,
	/* WP_MACHINEGUN */      30,
	/* WP_SHOTGUN */         5,
	/* WP_GRENADE_LAUNCHER */5,
	/* WP_ROCKET_LAUNCHER */ 5,
	/* WP_LIGHTNING */       30,
	/* WP_RAILGUN */         5,
	/* WP_PLASMAGUN */       30,
	/* WP_BFG */             5,
	/* WP_GRAPPLING_HOOK */  0
};

typedef struct
{
	superhudConfig_t config;
	superhudBarContext_t ctx;
	int maxammo[WP_NUM_WEAPONS];
	int last_weapon;
	int last_maxammo;
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
			CG_SHUDBarMakeContext(&element->config, &element->ctx, current_maxammo);
		}
	}

	element->ctx.color_top[3] = 1.0f;

	/* Limit ammo to maxammo for color calculation and bar display */
	{
		int display_ammo;
		int threshold;
		qboolean isLowAmmo;
		display_ammo = ammo;
		if (display_ammo > element->last_maxammo)
		{
			display_ammo = element->last_maxammo;
		}

		/* Use same thresholds as CG_CheckAmmo */
		threshold = lowAmmoThresholds[wp];
		isLowAmmo = qfalse;
		if (ammo == 0)
		{
			isLowAmmo = qtrue;
		}
		else if (threshold > 0 && ammo <= threshold)
		{
			isLowAmmo = qtrue;
		}

		if (isLowAmmo)
		{
			/* Low ammo - use color2, or red if not set */
			if (element->config.color2.isSet)
			{
				Vector4Copy(element->config.color2.value.rgba, element->ctx.color_top);
			}
			else
			{
				VectorCopy(colorRed, element->ctx.color_top);
			}
		}
		else
		{
			/* High ammo - use color, or white if not set */
			if (element->config.color.isSet)
			{
				Vector4Copy(element->config.color.value.rgba, element->ctx.color_top);
			}
			else
			{
				VectorCopy(colorWhite, element->ctx.color_top);
			}
		}

		CG_SHUDFill(&element->config);
		CG_SHUDDrawBorder(&element->config);
		CG_SHUDBarPrint(&element->config, &element->ctx, display_ammo);
	}
}

void CG_SHUDElementSBAmBDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}

