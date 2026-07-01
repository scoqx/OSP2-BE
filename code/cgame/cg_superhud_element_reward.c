#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef enum
{
	SHUD_REWARD_ICON,
	SHUD_REWARD_COUNT,
} shudRewardType_t;

typedef struct
{
	superhudConfig_t config;
	union
	{
		superhudDrawContext_t d;
		superhudTextContext_t t;
	} ctx;
	shudRewardType_t type;
} shudElementStatusbarRewards;

static void* CG_SHUDElementRewardCreate(const superhudConfig_t* config, shudRewardType_t type)
{
	shudElementStatusbarRewards* element;

	SHUD_ELEMENT_INIT(element, config);

	element->type = type;

	if (type == SHUD_REWARD_ICON)
	{
		CG_SHUDDrawMakeContext(&element->config, &element->ctx.d);
	}
	else
	{
		CG_SHUDTextMakeContext(&element->config, &element->ctx.t);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx.t);
	}

	if (!element->config.text.isSet)
	{
		element->config.text.isSet = qtrue;
		Q_strncpyz(element->config.text.value, "%d", sizeof(element->config.text.value));
	}

	if (!element->config.time.isSet)
	{
		element->config.time.isSet = qtrue;
		element->config.time.value = REWARD_TIME;
	}

	return element;
}

void* CG_SHUDElementRewardIconCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementRewardCreate(config, SHUD_REWARD_ICON);
}

void* CG_SHUDElementRewardCountCreate(const superhudConfig_t* config)
{
	return CG_SHUDElementRewardCreate(config, SHUD_REWARD_COUNT);
}

void CG_SHUDElementRewardRoutine(void* context)
{
	shudElementStatusbarRewards* element = (shudElementStatusbarRewards*)context;

	int i;
	float* color_origin;
	float* color;

	if (!cg_drawRewards.integer)
	{
		return;
	}

	if (element->type == SHUD_REWARD_ICON)
	{
		color_origin = element->ctx.d.color_origin;
		color = element->ctx.d.color;
	}
	else
	{
		color_origin = element->ctx.t.color_origin;
		color = element->ctx.t.color;
	}

	if (!CG_SHUDGetFadeColor(color_origin, color, &element->config, cg.rewardTime))
	{
		if (cg.rewardStack > 0)
		{
			for (i = 0; i < cg.rewardStack; i++)
			{
				cg.rewardSound[i] = cg.rewardSound[i + 1];
				cg.rewardShader[i] = cg.rewardShader[i + 1];
				cg.rewardCount[i] = cg.rewardCount[i + 1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			CG_SHUDGetFadeColor(color_origin, color, &element->config, cg.rewardTime);

			if (!(cg_drawRewards.integer & DRAW_REWARDS_NOSOUND))
			{
				trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
			}
		}
		else
		{
			return;
		}
	}

	if (element->type == SHUD_REWARD_ICON)
	{
		int durationMsec;

		if (!(cg_drawRewards.integer & DRAW_REWARDS_NOICON))
		{
			durationMsec = REWARD_TIME;
			if (element->config.time.isSet && element->config.time.value > 0)
			{
				durationMsec = element->config.time.value;
			}
			if (cg_oldMedals.integer)
			{
				element->ctx.d.image = cg.rewardShader[0];
				element->ctx.d.coordPicture.named.x = 0.0f;
				element->ctx.d.coordPicture.named.y = 0.0f;
				element->ctx.d.coordPicture.named.w = 1.0f;
				element->ctx.d.coordPicture.named.h = 1.0f;
			}
			else
			{
				vec4_t medalTc;

				CG_MedalResolveFrame(cg.rewardShader[0], cg.rewardTime, durationMsec,
				                     &element->ctx.d.image, medalTc);
				element->ctx.d.coordPicture.named.x = medalTc[0];
				element->ctx.d.coordPicture.named.y = medalTc[1];
				element->ctx.d.coordPicture.named.w = medalTc[2];
				element->ctx.d.coordPicture.named.h = medalTc[3];
			}
			CG_SHUDFillAndFrameForElement(&element->config, element->ctx.d.color_origin, element->ctx.d.color);
			CG_SHUDDrawStretchPicCtx(&element->config, &element->ctx.d);
		}
	}
	else
	{
		if (cg.rewardCount[0] && !(cg_drawRewards.integer & DRAW_REWARDS_NOICON))
		{
			element->ctx.t.text = va(element->config.text.value, cg.rewardCount[0]);
			CG_SHUDTextPrint(&element->config, &element->ctx.t);
		}
	}
}


void CG_SHUDElementRewardDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
