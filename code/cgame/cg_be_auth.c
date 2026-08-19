/*
===========================================================================
cg_be_auth.c

Hidden BE connect verification: read STAT_BE_AUTH challenge, reply with
+button14/+button15 pulses. No sticky MAGIC — local responded + delay.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"

#define BE_AUTH_AUTOLOGIN_DELAY_MS	1000

static qboolean cg_beAuthResponded;
static int cg_beAuthRespondedAt;

qboolean cg_beAuthFrameDone;

void CG_BE_AuthReset(void)
{
	cg_beAuthResponded = qfalse;
	cg_beAuthRespondedAt = 0;
	cg_beAuthFrameDone = qfalse;
	CG_BE_AutologinReset();
}

/*
===============
CG_BE_AuthReady

True after challenge pulses were sent and delay elapsed (server had time to verify).
===============
*/
qboolean CG_BE_AuthReady(void)
{
	if (!cg_beAuthResponded)
	{
		return qfalse;
	}
	if (cg.time - cg_beAuthRespondedAt < BE_AUTH_AUTOLOGIN_DELAY_MS)
	{
		return qfalse;
	}
	return qtrue;
}

/*
===============
CG_BE_AuthFrame

Once per connection: if server put a challenge in stats[STAT_BE_AUTH],
emit Mix(challenge) as button14/15 press-release pulses.
===============
*/
void CG_BE_AuthFrame(void)
{
	unsigned challenge;
	unsigned response;
	unsigned i;
	char cmd[512];
	char pulse[64];

	if (cg_beAuthFrameDone)
	{
		return;
	}
	if (cg.demoPlayback)
	{
		cg_beAuthFrameDone = qtrue;
		return;
	}
	if (cgs.fpsclassico)
	{
		cg_beAuthFrameDone = qtrue;
		return;
	}
	if (!cgs.beSupportedServer)
	{
		cg_beAuthFrameDone = qtrue;
		return;
	}
	if (!cg.snap)
	{
		return;
	}

	if (cg_beAuthResponded)
	{
		CG_BE_AutologinFrame();
		if (cg.time - cg_beAuthRespondedAt >= BE_AUTH_AUTOLOGIN_DELAY_MS)
		{
			cg_beAuthFrameDone = qtrue;
		}
		return;
	}

	challenge = (unsigned)(cg.snap->ps.stats[STAT_BE_AUTH] & 0xFFFF);
	if (challenge == 0)
	{
		return;
	}

	response = BG_BE_AuthResponse(challenge);
	cg_beAuthResponded = qtrue;
	cg_beAuthRespondedAt = cg.time;

	cmd[0] = '\0';
	for (i = 0; i < BE_AUTH_NUM_BITS; i++)
	{
		const char *btn;

		btn = ((response >> i) & 1u) ? "button15" : "button14";
		Com_sprintf(pulse, sizeof(pulse), "+%s;wait 1;-%s;wait 1;", btn, btn);
		Q_strcat(cmd, sizeof(cmd), pulse);
	}

	if (cmd[0])
	{
		trap_SendConsoleCommand(cmd);
	}
}
