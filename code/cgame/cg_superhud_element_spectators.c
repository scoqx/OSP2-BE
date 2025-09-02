#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef struct
{
    superhudConfig_t config;
    superhudTextContext_t ctx;
} shudElementSpectators_t;

void* CG_SHUDElementSpectatorsCreate(const superhudConfig_t* config)
{
    shudElementSpectators_t* element;

    SHUD_ELEMENT_INIT(element, config);

    CG_SHUDTextMakeContext(&element->config, &element->ctx);
    CG_SHUDFillAndFrameForText(&element->config, &element->ctx);

    return element;
}

static qboolean CG_SHUD_SpectatorsBuildString(char* out, int outSize)
{
    int i;
    int len = 0;
    const int count = cg.specsinfo.count;

    if (count <= 0)
    {
        out[0] = '\0';
        return qfalse;
    }

    Q_strncpyz(out, "Spectators: ", outSize);
    len = strlen(out);
    for (i = 0; i < count; ++i)
    {
        const char* name = cg.specsinfo.names[i];
        if (!name || !name[0])
        {
            continue;
        }
        if (i > 0)
        {
            Q_strncpyz(out + len, ", ", outSize - len);
            len = strlen(out);
            if (len >= outSize)
                break;
        }
        Q_strncpyz(out + len, name, outSize - len);
        len = strlen(out);
        if (len >= outSize)
            break;
    }
    return qtrue;
}

void CG_SHUDElementSpectatorsRoutine(void* context)
{
    shudElementSpectators_t* element = (shudElementSpectators_t*)context;
    static char buffer[MAX_STRING_CHARS];

    if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
    {
        // In spectator mode ourselves: nothing to show for following list of us
        return;
    }

    if (!CG_SHUD_SpectatorsBuildString(buffer, sizeof(buffer)))
    {
        if (!SHUD_CHECK_SHOW_EMPTY(element))
            return;
        Q_strncpyz(buffer, "Spectators:", sizeof(buffer));
    }

    element->ctx.text = buffer;
    CG_SHUDTextPrint(&element->config, &element->ctx);
}

void CG_SHUDElementSpectatorsDestroy(void* context)
{
    if (context)
    {
        Z_Free(context);
    }
}


