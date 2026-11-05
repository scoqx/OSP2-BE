#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudConfig_t tmp_config;
	superhudTextContext_t position;
	superhudTextContext_t measureCtx; /* font metrics for ammo width; built once */
	float x; // main x point of whole weaponlist
	float y; // main y point of whole weaponlist
	float w; // width of one weapon
	float h; // height of one weapon
	float ammoWidth; // width of ammo field
	int ammoMax;
	int weaponNum;
	int cachedStatWeapons;
	int cachedAmmoMaxForWidth;
	int slotWeapon[WP_NUM_WEAPONS];
	char ammo[WP_NUM_WEAPONS][8];
	vec4_t border[WP_NUM_WEAPONS];
	vec4_t borderColor[WP_NUM_WEAPONS];
	superhudDrawContext_t back[WP_NUM_WEAPONS];
	superhudDrawContext_t weaponIcon[WP_NUM_WEAPONS];
	superhudTextContext_t ammoCount[WP_NUM_WEAPONS];

} shudElementWeaponList_t;

void* CG_SHUDElementWeaponListCreate(const superhudConfig_t* config)
{
	shudElementWeaponList_t* element;

	SHUD_ELEMENT_INIT(element, config);

	if (!element->config.textAlign.isSet)
	{
		element->config.textAlign.isSet = qtrue;
		element->config.textAlign.value = SUPERHUD_ALIGNH_CENTER;
	}
	memcpy(&element->tmp_config, &element->config, sizeof(element->tmp_config));

	element->x = element->config.rect.value[0];
	element->y = element->config.rect.value[1];
	element->w = element->config.rect.value[2];
	element->h = element->config.rect.value[3];

	CG_SHUDTextMakeContext(&element->tmp_config, &element->ammoCount[0]);
	CG_SHUDTextMakeContext(&element->tmp_config, &element->measureCtx);

	element->ammoMax = -1; /* force update width */
	element->cachedStatWeapons = -1;
	element->cachedAmmoMaxForWidth = -1;
	element->weaponNum = 0;

	return element;
}

static void CG_SHUDElementWeaponListApplySlotStyle(shudElementWeaponList_t* element, int slot, int wpi, int weapon)
{
	if (wpi != weapon)
	{
		if (element->config.bgcolor.isSet)
		{
			CG_SHUDConfigPickBgColor(&element->tmp_config, element->back[slot].color, qfalse);
		}
		else
		{
			memset(element->back[slot].color, 0, sizeof(element->back[slot].color));
		}
		Vector4Set(element->border[slot], 0, 0, 0, 0);
		Vector4Set(element->borderColor[slot], 0, 0, 0, 0);
		return;
	}

	if (element->config.color2.isSet)
	{
		Vector4Copy(element->tmp_config.color2.value.rgba, element->back[slot].color);
	}
	else
	{
		memset(element->back[slot].color, 0, sizeof(element->back[slot].color));
	}

	if (element->config.border.isSet)
	{
		Vector4Copy(element->config.border.value, element->border[slot]);
	}
	else
	{
		Vector4Set(element->border[slot], 0, 0, 0, 0);
	}

	if (element->config.borderColor.isSet)
	{
		CG_SHUDConfigPickBorderColor(&element->config, element->borderColor[slot], qfalse);
	}
	else
	{
		Vector4Set(element->borderColor[slot], 1, 1, 1, 0);
	}
}

/* What can change while the layout stays: ammo, the icon of a weapon registered late, selection, team colors. */
static void CG_SHUDElementWeaponListUpdateSlots(shudElementWeaponList_t* element, int weapon)
{
	int i;
	int ammo;
	float colorA;

	colorA = element->tmp_config.color.isSet ? element->tmp_config.color.value.rgba[3] : 1.0f;

	for (i = 0; i < element->weaponNum; ++i)
	{
		element->weaponIcon[i].image = cg_weapons[element->slotWeapon[i]].weaponIcon;
		CG_SHUDElementWeaponListApplySlotStyle(element, i, element->slotWeapon[i], weapon);

		ammo = CG_SHUDGetAmmo(element->slotWeapon[i]);
		CG_FormatInt(&element->ammo[i][0], 8, ammo);
		element->ammoCount[i].text = &element->ammo[i][0];

		if (ammo == 0)
		{
			vec4_t tmpColor;
			Vector4Copy(colorRed, tmpColor);
			tmpColor[3] = colorA;
			Vector4Copy(tmpColor, element->ammoCount[i].color);
		}
		else
		{
			CG_SHUDConfigPickColor_Init(&element->tmp_config, element->ammoCount[i].color);
		}
	}
}

static void CG_SHUDElementWeaponListRebuild(shudElementWeaponList_t* element, superhudAlignH_t align,
        int statWeapons, int ammo_max)
{
	int wpi;
	int x, y, w, h;
	int total;

	if (ammo_max > element->ammoMax)
	{
		element->ammoMax = ammo_max;
	}
	if (element->ammoMax < 0)
	{
		element->ammoMax = 0;
	}

	if (element->cachedAmmoMaxForWidth != element->ammoMax)
	{
		element->ammoWidth = CG_OSPDrawStringNewTextWidth(
		                         va("%d", element->ammoMax),
		                         element->measureCtx.coord.named.w,
		                         0.0f,
		                         element->measureCtx.flags,
		                         element->measureCtx.fontIndex);
		element->ammoWidth += element->measureCtx.coord.named.w / 6.0f;
		element->cachedAmmoMaxForWidth = element->ammoMax;
	}

	for (wpi = WP_MACHINEGUN, total = 0; wpi < WP_NUM_WEAPONS; ++wpi)
	{
		if ((statWeapons & (1 << wpi)) != 0)
		{
			++total;
		}
	}

	if (align == SUPERHUD_ALIGNH_CENTER)
	{
		x = element->x - total * (element->w + element->ammoWidth) / 2;
		y = element->y;
	}
	else
	{
		x = element->x;
		y = element->y - total * element->h / 2;
	}
	w = element->w;
	h = element->h;

	element->weaponNum = 0;

	for (wpi = WP_MACHINEGUN; wpi < WP_NUM_WEAPONS; ++wpi)
	{
		if ((statWeapons & (1 << wpi)) == 0)
		{
			continue;
		}

		element->slotWeapon[element->weaponNum] = wpi;

		element->tmp_config.alignV.value = SUPERHUD_ALIGNV_TOP;
		element->tmp_config.alignV.isSet = qtrue;

		if (align != SUPERHUD_ALIGNH_RIGHT)
		{
			element->tmp_config.alignH.value = SUPERHUD_ALIGNH_LEFT;
			element->tmp_config.alignH.isSet = qtrue;
		}
		else
		{
			element->tmp_config.alignH.value = SUPERHUD_ALIGNH_RIGHT;
			element->tmp_config.alignH.isSet = qtrue;
		}

		element->tmp_config.rect.value[0] = x;
		element->tmp_config.rect.value[1] = y;
		element->tmp_config.rect.value[2] = w;
		element->tmp_config.rect.value[3] = h;
		CG_SHUDDrawMakeContext(&element->tmp_config, &element->weaponIcon[element->weaponNum]);

		element->tmp_config.rect.value[0] = x;
		if (align == SUPERHUD_ALIGNH_RIGHT)
		{
			element->tmp_config.rect.value[0] -= element->ammoWidth;
		}
		element->tmp_config.rect.value[1] = y;
		element->tmp_config.rect.value[2] = w + element->ammoWidth;
		element->tmp_config.rect.value[3] = h;
		CG_SHUDDrawMakeContext(&element->tmp_config, &element->back[element->weaponNum]);

		if (align != SUPERHUD_ALIGNH_RIGHT)
		{
			element->tmp_config.rect.value[0] = x + w;
		}
		else
		{
			element->tmp_config.rect.value[0] = x - element->ammoWidth;
		}
		element->tmp_config.rect.value[1] = y;
		element->tmp_config.rect.value[2] = element->ammoWidth;
		element->tmp_config.rect.value[3] = h;

		element->tmp_config.alignV.value = SUPERHUD_ALIGNV_CENTER;
		element->tmp_config.alignV.isSet = qtrue;
		element->tmp_config.alignH.value = SUPERHUD_ALIGNH_CENTER;
		element->tmp_config.alignH.isSet = qtrue;
		element->tmp_config.textAlign.value = SUPERHUD_ALIGNH_CENTER;
		element->tmp_config.textAlign.isSet = qtrue;

		CG_SHUDTextMakeContext(&element->tmp_config, &element->ammoCount[element->weaponNum]);
		element->ammoCount[element->weaponNum].text = &element->ammo[element->weaponNum][0];

		if (align == SUPERHUD_ALIGNH_CENTER)
		{
			x += w + element->ammoWidth;
		}
		else
		{
			y += h;
		}
		++element->weaponNum;
	}

	element->cachedStatWeapons = statWeapons;
}

static void CG_SHUDElementWeaponListSetup(shudElementWeaponList_t* element, superhudAlignH_t align)
{
	int wpi;
	int statWeapons;
	int weapon;
	int ammo_max = 0;

	if (!cg.snap)
	{
		return;
	}

	statWeapons = cg.snap->ps.stats[STAT_WEAPONS];
	weapon = cg.snap->ps.weapon;

	for (wpi = WP_MACHINEGUN; wpi < WP_NUM_WEAPONS; ++wpi)
	{
		if ((statWeapons & (1 << wpi)) != 0)
		{
			int ammo = CG_SHUDGetAmmo(wpi);
			if (ammo_max < ammo)
			{
				ammo_max = ammo;
			}
		}
	}

	/* The layout depends on the owned weapons and the width of the ammo column. */
	if (statWeapons != element->cachedStatWeapons ||
	        element->weaponNum <= 0 ||
	        ammo_max > element->ammoMax ||
	        element->ammoMax < 0)
	{
		CG_SHUDElementWeaponListRebuild(element, align, statWeapons, ammo_max);
	}

	CG_SHUDElementWeaponListUpdateSlots(element, weapon);
}

void CG_SHUDElementWeaponListRoutine(void* context)
{
	shudElementWeaponList_t* element = (shudElementWeaponList_t*)context;
	int i;

	CG_SHUDElementWeaponListSetup(element, element->config.textAlign.value);

	for (i = 0; i < element->weaponNum; ++i)
	{
		CG_SHUDFillWithColor(&element->back[i].coord, element->back[i].color);
		CG_SHUDDrawStretchPicCtx(&element->config, &element->weaponIcon[i]);
		CG_SHUDTextPrintNew(&element->config, &element->ammoCount[i], qfalse);
		CG_SHUDDrawBorderDirect(&element->back[i].coord, element->border[i], element->borderColor[i]);
	}
}

void CG_SHUDElementWeaponListDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
