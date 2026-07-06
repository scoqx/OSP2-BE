#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t drawCtx;
	superhudTextContext_t textCtx;
} shudElementRecordMessage_t;

void* CG_SHUDElementRecordMessageCreate(const superhudConfig_t* config)
{
	shudElementRecordMessage_t* element;
	const char* imagePath;

	SHUD_ELEMENT_INIT(element, config);

	if (!element->config.color.isSet)
	{
		element->config.color.isSet = qtrue;
		element->config.color.value.type = SUPERHUD_COLOR_RGBA;
		Vector4Set(element->config.color.value.rgba, 1.0f, 0.0f, 0.0f, 1.0f);
	}

	CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);

	imagePath = NULL;
	if (config->image.isSet)
	{
		imagePath = config->image.value;
	}
	else if (!config->text.isSet)
	{
		/* Keep legacy default REC icon only when text is not provided. */
		imagePath = "gfx/2d/hudres/rec.tga";
	}

	element->drawCtx.image = imagePath ? trap_R_RegisterShader(imagePath) : 0;
	if (imagePath && !element->drawCtx.image)
	{
		CG_Printf("^1RecordMessage image %s is not found\n", imagePath);
	}

	if (config->text.isSet)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textCtx);
		element->textCtx.text = config->text.value;
		CG_SHUDFillAndFrameForText(&element->config, &element->textCtx);
	}

	return element;
}

void CG_SHUDElementRecordMessageRoutine(void* context)
{
	shudElementRecordMessage_t* element = (shudElementRecordMessage_t*)context;
	static int nextUpdateTime = 0;
	static qboolean isRecordingDemo = qfalse;

	if (cg.time >= nextUpdateTime)
	{
		isRecordingDemo = CG_IsRecordingDemo();
		nextUpdateTime = cg.time + 1000;
	}

	if (!isRecordingDemo)
	{
		return;
	}

	if (element->config.style.isSet && element->config.style.value == 1)
	{
		if (((cg.time / 750) & 1) != 0)
		{
			return;
		}
	}

	if (element->drawCtx.image)
	{
		/* Base rect fill/frame are relevant only for image mode. */
		CG_SHUDFill(&element->config);
		CG_SHUDDrawStretchPicCtx(&element->config, &element->drawCtx);
		CG_SHUDDrawBorder(&element->config);
	}

	if (element->textCtx.text)
	{
		CG_SHUDTextPrint(&element->config, &element->textCtx);
	}
}

void CG_SHUDElementRecordMessageDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
