#ifndef BG_PMOVE_JUMP_H
#define BG_PMOVE_JUMP_H

/*
=============
PM_EvaluateJumpVelocityScale

Calculates the jump velocity scaling multiplier based on the previous
retail jump timestamp and returns the observed time delta.
=============
*/
static float PM_EvaluateJumpVelocityScale( const playerState_t *ps, const pmove_settings_t *settings, int commandTime, int *outTimeDelta ) {
	int		timeDelta;
	float		threshold;
	float		scaleAdd;

	if ( outTimeDelta ) {
		*outTimeDelta = -1;
	}

	if ( !ps || !settings ) {
		return 1.0f;
	}

	timeDelta = -1;
	if ( commandTime >= ps->jumpTime ) {
		timeDelta = commandTime - ps->jumpTime;
	}

	if ( outTimeDelta ) {
		*outTimeDelta = timeDelta;
	}

	if ( timeDelta < 0 ) {
		return 1.0f;
	}

	threshold = settings->jumpVelocityTimeThreshold;
	scaleAdd = settings->jumpVelocityScaleAdd;

	if ( threshold <= 0.0f ) {
		return 1.0f;
	}

	if ( scaleAdd <= 0.0f ) {
		return 1.0f;
	}

	if ( timeDelta <= 0 ) {
		return 1.0f;
	}

	if ( (float)timeDelta >= threshold ) {
		return 1.0f;
	}

	return 1.0f + ( ( threshold - (float)timeDelta ) / threshold ) * scaleAdd;
}

#endif // BG_PMOVE_JUMP_H
