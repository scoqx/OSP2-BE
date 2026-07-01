#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t drawCtx;
} shudElementGridContext;

void* CG_SHUDElementGridCreate(const superhudConfig_t* config)
{
	shudElementGridContext* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDDrawMakeContext(&element->config, &element->drawCtx);

	return element;
}

void CG_SHUDElementGridRoutine(void* context)
{
	shudElementGridContext* element = (shudElementGridContext*)context;
	int index;
	float x = element->config.rect.value[0];
	float y = element->config.rect.value[1];
	float cellWidth = element->config.rect.value[2];
	float cellHeight = element->config.rect.value[3];
	vec4_t color1, color2;
	float col, row;
	float startCol, startRow;
	superhudCoord_t coord;
	qboolean hasColor2 = element->config.color2.isSet;
	qboolean style1 = element->config.style.isSet && element->config.style.value == 1;

	Vector4Copy(element->config.color.value.rgba, color1);
	if (hasColor2)
	{
		Vector4Copy(element->config.color2.value.rgba, color2);
	}

	startCol = x;
	while (startCol > 0)
	{
		startCol -= cellWidth;
	}

	startRow = y;
	while (startRow > 0)
	{
		startRow -= cellHeight;
	}

	index = 0;
	for (col = startCol; col <= 640; col += cellWidth, index++)
	{
		coord.named.x = col;
		coord.named.y = 0;
		coord.named.w = 1;
		coord.named.h = 480;

		if (hasColor2 && (index & 1))
		{
			CG_SHUDFillWithColor(&coord, color2);
		}
		else
		{
			CG_SHUDFillWithColor(&coord, color1);
		}
	}

	index = 0;
	for (row = startRow; row <= 480; row += cellHeight, index++)
	{
		coord.named.x = 0;
		coord.named.y = row;
		coord.named.w = 640;
		coord.named.h = 1;

		if (hasColor2 && (index & 1))
		{
			CG_SHUDFillWithColor(&coord, color2);
		}
		else
		{
			CG_SHUDFillWithColor(&coord, color1);
		}
	}

	if (style1)
	{
		superhudConfig_t labelConfig;
		superhudTextContext_t textCtx;
		char textBuf[64];
		char textBufLeft[64];

		memcpy(&labelConfig, &element->config, sizeof(labelConfig));
		labelConfig.textAlign.value = SUPERHUD_ALIGNH_LEFT;
		labelConfig.textAlign.isSet = qtrue;
		labelConfig.alignV.value = SUPERHUD_ALIGNV_BOTTOM;
		labelConfig.alignV.isSet = qtrue;
		labelConfig.fontsize.value[0] = cellWidth / 4.0f;
		labelConfig.fontsize.value[1] = cellHeight / 2.5f;
		labelConfig.fontsize.isSet = qtrue;
		labelConfig.color.value.rgba[3] = 1.0f;
		labelConfig.color.isSet = qtrue;

		// top labels along the first row
		for (col = startCol; col <= 640; col += cellWidth)
		{
			float labelX;
			if (col < 0.0f)
			{
				continue;
			}

			labelX = col;
			Com_sprintf(textBuf, sizeof(textBuf), "%d", (int)(labelX + 0.5f));

			labelConfig.rect.value[0] = labelX;
			labelConfig.rect.value[1] = y - cellHeight;
			labelConfig.rect.value[2] = cellWidth;
			labelConfig.rect.value[3] = cellHeight;
			labelConfig.rect.isSet = qtrue;

			CG_SHUDTextMakeContext(&labelConfig, &textCtx);
			textCtx.text = textBuf;
			CG_SHUDTextPrintNew(&labelConfig, &textCtx, qfalse);
		}

		// left labels along the first column
		for (row = startRow; row <= 480; row += cellHeight)
		{
			float labelY;
			if (row < 0.0f)
			{
				continue;
			}

			labelY = row - cellHeight;
			Com_sprintf(textBufLeft, sizeof(textBufLeft), "%d", (int)(row + 0.5f));

			labelConfig.rect.value[0] = x;
			labelConfig.rect.value[1] = labelY;
			labelConfig.rect.value[2] = cellWidth;
			labelConfig.rect.value[3] = cellHeight;
			labelConfig.rect.isSet = qtrue;

			CG_SHUDTextMakeContext(&labelConfig, &textCtx);
			textCtx.text = textBufLeft;
			CG_SHUDTextPrintNew(&labelConfig, &textCtx, qfalse);
		}
	}
}

void CG_SHUDElementGridDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
