#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t ctx;
} shudElementCWSIcon_t;

void* CG_SHUDElementCWSIconCreate(const superhudConfig_t* config)
{
	shudElementCWSIcon_t* element;

	SHUD_ELEMENT_INIT(element, config);
	CG_SHUDDrawMakeContext(&element->config, &element->ctx);

	return element;
}

void CG_SHUDElementCWSIconRoutine(void* context)
{
	shudElementCWSIcon_t* element = (shudElementCWSIcon_t*)context;
	int lastWeapon = CG_SHUDGetContext()->customStats.lastTrackedWeapon;

	if (lastWeapon <= 0 || lastWeapon >= MAX_WEAPONS)
	{
		return;
	}

	element->ctx.image = cg_weapons[lastWeapon].weaponIcon;

	CG_SHUDDrawBorder(&element->config);
	CG_SHUDFill(&element->config);
	CG_SHUDDrawStretchPicCtx(&element->config, &element->ctx);
}


void CG_SHUDElementCWSIconDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}