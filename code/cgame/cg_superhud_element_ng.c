#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	superhudConfig_t config;
	superhudDrawContext_t ctx;
	superhudTextContext_t tctx;
} shudElementNG_t;

void* CG_SHUDElementNGCreate(const superhudConfig_t* config)
{
	shudElementNG_t* element;

	SHUD_ELEMENT_INIT(element, config);

	CG_SHUDDrawMakeContext(&element->config, &element->ctx);

	if (config->image.isSet)
	{
		element->ctx.image = trap_R_RegisterShader(element->config.image.value);
	}

	CG_SHUDTextMakeContext(&element->config, &element->tctx);
	CG_SHUDFillAndFrameForText(&element->config, &element->tctx);

	element->tctx.coord.named.x = 320;
	element->tctx.coord.named.y = 232;

	element->tctx.coord.named.h = 16;
	element->tctx.coord.named.w = 12;

	element->tctx.flags |= DS_HCENTER;
	element->tctx.text = "^1Connection Interrupted";

	Vector4Set(element->tctx.color, 1.0f, 1.0f, 1.0f, 1.0f);

	return element;
}

void CG_SHUDElementNGRoutine(void* context)
{
	shudElementNG_t* element = (shudElementNG_t*) context;
	float ax, ay, aw, ah;
	int cmdNum;
	usercmd_t cmd;
	static int delay = 0;

	ax = element->ctx.coord.named.x;
	ay = element->ctx.coord.named.y;
	aw = element->ctx.coord.named.w;
	ah = element->ctx.coord.named.h;

	CG_SHUDDrawBorder(&element->config);

	trap_R_SetColor(NULL);
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	if (element->config.fill.isSet)
	{
		if (element->config.bgcolor.isSet)
		{
			trap_R_SetColor(element->config.bgcolor.value.rgba);
			trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, cgs.media.whiteShader);
			trap_R_SetColor(NULL);
		}
	}
	else if (element->config.image.isSet)
	{
		trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, element->ctx.image);
	}
	else
	{
		trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, cgs.media.lagometerShader);
	}

	CG_DrawLagometerGraphsInRect(ax, ay, aw, ah);

	if (!cg.snap)
	{
		return;
	}

	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time)
	{
		delay = cg.time;
		return;
	}

	if (delay + 500 > cg.time)
	{
		return;
	}

	CG_SHUDTextPrint(&element->config, &element->tctx);

	if ((cg.time >> 9) & 1)
	{
		return;
	}

	trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, cgs.media.connectionShader);
}

void CG_SHUDElementNGDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}



