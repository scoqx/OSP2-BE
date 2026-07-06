#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"


typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t drawCtx;
	superhudTextContext_t textCtx;
} shudElementStatusbarDecorate;

void* CG_SHUDElementDecorCreate(const superhudConfig_t* config)
{
	shudElementStatusbarDecorate* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);

	if (config->text.isSet)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
		element->textCtx.text = config->text.value;
		CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
	}
	else if (config->image.isSet)
	{
		element->drawCtx.image = trap_R_RegisterShader(config->image.value);
		if (!element->drawCtx.image)
		{
			CG_Printf("^2Decorate image %s is not found\n", config->image.value);
		}
	}

	return element;
}

void CG_SHUDElementDecorRoutine(void* context)
{
	shudElementStatusbarDecorate* element = (shudElementStatusbarDecorate*)context;

	if (element->config.text.isSet)
	{
		CG_SHUDTextPrint(&element->config, &element->textCtx);
		return;
	}

	if (element->drawCtx.image)
	{
		if (!CG_SHUDFill(&element->config))
		{
			CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		}
		else if (element->config.bgcolor.isSet && element->config.fill.isSet)
		{
			CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		}
		CG_SHUDDrawBorder(&element->config);
		return;
	}

	CG_SHUDFill(&element->config);
	CG_SHUDDrawBorder(&element->config);
}

void CG_SHUDElementDecorDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
