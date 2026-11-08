#include "../../cg_local.h"
#include "../superhud_private.h"
#include "../../../qcommon/qcommon.h"

/*
announcer: what the announcer says, as text (cg_announcerText). The last ANNOUNCER_MAX_LINES lines
inside the rect, each fading out on its own. direction B (the default): the newest at the bottom,
the older ones go up; direction T: the newest at the top, the older ones go down.
*/

#define SHUD_ANNOUNCER_TIME_DEFAULT 3000

typedef struct
{
	superhudConfig_t        config;
	superhudTextContext_t   ctx[ANNOUNCER_MAX_LINES];
	float                   lineHeight;
	qboolean                newestOnTop;
} shudElementAnnouncer_t;

void* CG_SHUDElementAnnouncerCreate(const superhudConfig_t* config)
{
	shudElementAnnouncer_t* element;
	int i;

	SHUD_ELEMENT_INIT(element, config);

	if (!element->config.time.isSet)
	{
		element->config.time.isSet = qtrue;
		element->config.time.value = SHUD_ANNOUNCER_TIME_DEFAULT;
	}

	for (i = 0; i < ANNOUNCER_MAX_LINES; i++)
	{
		CG_SHUDTextMakeContext(&element->config, &element->ctx[i]);
		CG_SHUDFillAndFrameForText(&element->config, &element->ctx[i]);
	}

	/* the rect holds all the lines; a font size that does not fit makes them overlap less than it spreads them */
	element->lineHeight = element->config.rect.value[3] / ANNOUNCER_MAX_LINES;
	if (element->lineHeight < element->ctx[0].coord.named.h)
	{
		element->lineHeight = element->ctx[0].coord.named.h;
	}
	element->newestOnTop = element->config.direction.isSet && element->config.direction.value == SUPERHUD_DIR_TOP_TO_BOTTOM;

	return element;
}

void CG_SHUDElementAnnouncerRoutine(void* context)
{
	shudElementAnnouncer_t* element = (shudElementAnnouncer_t*)context;
	const float top = element->config.rect.value[1];
	const float bottom = top + element->config.rect.value[3];
	int age;

	if (!cg_announcerText.integer)
	{
		return;
	}

	for (age = 0; age < ANNOUNCER_MAX_LINES; age++)
	{
		superhudTextContext_t* ctx = &element->ctx[age];
		int         time;
		const char* text = CG_AnnouncerLine(age, &time);

		if (!text)
		{
			break;
		}
		if (!CG_SHUDGetFadeColor(ctx->color_origin, ctx->color, &element->config, time))
		{
			/* the older ones are gone too */
			break;
		}
		ctx->text = text;
		if (element->newestOnTop)
		{
			ctx->coord.named.y = top + age * element->lineHeight;
		}
		else
		{
			ctx->coord.named.y = bottom - (age + 1) * element->lineHeight;
		}
		CG_SHUDTextPrint(&element->config, ctx);
	}
}

void CG_SHUDElementAnnouncerDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
