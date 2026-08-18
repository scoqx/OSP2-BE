#include "cg_local.h"
#include "../superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t ctx;
	qhandle_t customImage;
	qhandle_t emptyImage;
} shudElementStatusbarArmorIcon;

void* CG_SHUDElementSBAICreate(const superhudConfig_t* config)
{
	shudElementStatusbarArmorIcon* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDDrawMakeContext(&element->config, &element->ctx);

	if (config->image.isSet)
	{
		element->customImage = trap_R_RegisterShader(element->config.image.value);
	}

	if (SHUD_CHECK_SHOW_EMPTY(element) && !element->customImage)
	{
		element->emptyImage = trap_R_RegisterShader("icons/iconr_white");
	}

	return element;
}

void CG_SHUDElementSBAIRoutine(void* context)
{
	shudElementStatusbarArmorIcon* element = (shudElementStatusbarArmorIcon*)context;
	int ap = cg.snap->ps.stats[STAT_ARMOR];
	unsigned int index;
	vec4_t tint;
	float alpha;

	if (ap <= 0 && !SHUD_CHECK_SHOW_EMPTY(element))
		return;

	alpha = element->config.color.isSet ? element->config.color.value.rgba[3] : 1.0f;

	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);

	if (element->customImage)
	{
		if (ap <= 0)
		{
			Vector4Set(tint, 1.0f, 1.0f, 1.0f, alpha);
		}
		else
		{
			index = cg.snap->ps.stats[STAT_ARMOR_TYPE];
			if (index > 2) index = 0;
			if (index == ARMOR_YELLOW)
			{
				Vector4Set(tint, colorYellow[0], colorYellow[1], colorYellow[2], alpha);
			}
			else if (index == ARMOR_RED)
			{
				Vector4Set(tint, colorRed[0], colorRed[1], colorRed[2], alpha);
			}
			else
			{
				Vector4Set(tint, 0.329412f, 0.717647f, 0.137255f, alpha);
			}
		}
		CG_SHUDDrawStretchPic(element->ctx.coord, element->ctx.coordPicture, tint, element->customImage);
	}
	else if (ap <= 0)
	{
		Vector4Set(tint, 1.0f, 1.0f, 1.0f, alpha);
		CG_SHUDDrawStretchPic(element->ctx.coord, element->ctx.coordPicture, tint, element->emptyImage);
	}
	else
	{
		index = cg.snap->ps.stats[STAT_ARMOR_TYPE];
		if (index > 2) index = 0;
		element->ctx.image = cgs.media.armorIcon[index];
		CG_SHUDDrawStretchPicCtx(&element->config, &element->ctx);
	}
}

void CG_SHUDElementSBAIDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
