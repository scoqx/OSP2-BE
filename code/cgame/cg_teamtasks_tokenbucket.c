/*
===========================================================================
cg_teamtasks_tokenbucket.c

Token bucket anti-flood system

Copyright (c) 2026 diwoc
Contact: diwoc@outlook.com

This file contains original work created by diwoc.
It is included in the OSP2-BE modification for Quake III Arena and is
distributed under the terms of the project's primary license.
===========================================================================
*/

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



