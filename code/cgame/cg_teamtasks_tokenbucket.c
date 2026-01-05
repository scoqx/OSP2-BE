/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_teamtasks_tokenbucket.c -- token bucket anti-flood system

#include "cg_local.h"

// Token Bucket functions for anti-flood
void TokenBucket_Init(tokenBucket_t* b, int maxTokens, int refillTime, int now)
{
	b->maxTokens = maxTokens;
	b->refillTime = refillTime;
	b->tokens = maxTokens;
	b->lastTime = now;
}

static void TokenBucket_Update(tokenBucket_t* b, int now)
{
	int delta;
	int add;

	delta = now - b->lastTime;
	if (delta <= 0)
	{
		return;
	}

	add = delta / b->refillTime;
	if (add <= 0)
	{
		return;
	}

	b->tokens += add;
	if (b->tokens > b->maxTokens)
	{
		b->tokens = b->maxTokens;
	}

	b->lastTime += add * b->refillTime;
}

qboolean TokenBucket_Consume(tokenBucket_t* b, int now)
{
	TokenBucket_Update(b, now);

	if (b->tokens <= 0)
	{
		return qfalse;
	}

	b->tokens--;
	return qtrue;
}

int TokenBucket_TimeToNext(tokenBucket_t* b, int now)
{
	int timeRemaining;

	/* Update bucket first to get accurate time */
	TokenBucket_Update(b, now);

	if (b->tokens > 0)
	{
		return 0;
	}

	timeRemaining = b->refillTime - (now - b->lastTime);
	if (timeRemaining < 0)
	{
		return 0;
	}

	return timeRemaining;
}


