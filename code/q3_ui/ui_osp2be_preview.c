#include "ui_osp2be_shared.h"

extern qhandle_t osp2beFrozenOverlay;

static const char *Osp2Be_PickFrozenCvarName( int category );
static qboolean Osp2Be_TryParseColorFromFields( const char *cvarName, vec4_t out );

static void Osp2Be_GetFrozenColorForCategory( int category, vec4_t out ) {
	const char *cvarName;
	char buffer[64];

	cvarName = Osp2Be_PickFrozenCvarName( category );

	if ( Osp2Be_TryParseColorFromFields( cvarName, out ) ) {
		out[3] = 1.0f;
		return;
	}

	trap_Cvar_VariableStringBuffer( cvarName, buffer, sizeof( buffer ) );
	if ( buffer[0] && Osp2Be_ParseColorValue( buffer, out ) ) {
		out[3] = 1.0f;
		return;
	}

	Osp2Be_SetColorVec4( out, 1.0f, 1.0f, 1.0f, 1.0f );
}

static const char *Osp2Be_PickFrozenCvarName( int category ) {
	const char *cvarName;

	switch ( category ) {
	case 2:
		cvarName = "cg_teamFrozenColor";
		break;
	case 3:
		cvarName = "cg_enemyFrozenColor";
		break;
	default:
		cvarName = "cg_playerFrozenColor";
		break;
	}

	return cvarName;
}

static qboolean Osp2Be_TryParseColorFromFields( const char *cvarName, vec4_t out ) {
	int i;

	if ( !cvarName || !cvarName[0] ) {
		return qfalse;
	}

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		const osp2beCvarDef_t *def;

		def = s_osp2be.activeDefs[i];
		if ( !def ) {
			continue;
		}
		if ( def->special != OSP2BE_SPECIAL_COLOR_GENERIC ) {
			continue;
		}
		if ( def->controlType != OSP2BE_CONTROL_FIELD && def->controlType != OSP2BE_CONTROL_COLOR_BUTTON ) {
			continue;
		}
		if ( Q_stricmp( def->cvarName, cvarName ) ) {
			continue;
		}
		if ( s_osp2be.fields[i].field.buffer[0] && Osp2Be_ParseColorValue( s_osp2be.fields[i].field.buffer, out ) ) {
			return qtrue;
		}
		break;
	}

	return qfalse;
}

void Osp2Be_ResetPreview( void ) {
	unsigned char *infoBytes;
	int index;

	infoBytes = (unsigned char *)&s_osp2be.previewInfo;
	for ( index = 0; index < (int)sizeof( s_osp2be.previewInfo ); index++ ) {
		infoBytes[index] = 0;
	}
	s_osp2be.previewModel[0] = '\0';
	s_osp2be.previewCategory = -1;
	Osp2Be_SetColorVec4( s_osp2be.previewInfo.customHeadColor, 0.0f, 0.0f, 0.0f, 0.0f );
	Osp2Be_SetColorVec4( s_osp2be.previewInfo.customTorsoColor, 0.0f, 0.0f, 0.0f, 0.0f );
	Osp2Be_SetColorVec4( s_osp2be.previewInfo.customLegsColor, 0.0f, 0.0f, 0.0f, 0.0f );
	Osp2Be_SetColorVec4( s_osp2be.previewInfo.outlineColor, 0.0f, 0.0f, 0.0f, 0.0f );
	s_osp2be.previewInfo.useCustomSkinColors = qfalse;
	s_osp2be.previewInfo.useOutline = qfalse;
	s_osp2be.previewInfo.outlineShader = 0;
	s_osp2be.previewInfo.freezeActive = qfalse;
	s_osp2be.previewInfo.freezeShader = 0;
	Osp2Be_SetColorVec4( s_osp2be.previewInfo.freezeColor, 0.0f, 0.0f, 0.0f, 0.0f );
}

const char *Osp2Be_GetModelCvarForCategory( int category ) {
	switch ( category ) {
	case 1:
		return "model";
	case 2:
		return "cg_teamModel";
	case 3:
		return "cg_enemyModel";
	case 4:
		return "model";
	case 0:
	default:
		return "model";
	}
}

void Osp2Be_GetModelColorsFromFields( char *buffer, int bufferSize ) {
	int i;
	char head[8];
	char torso[8];
	char legs[8];
	qboolean foundHead;
	qboolean foundTorso;
	qboolean foundLegs;
	const char *targetCvar;
	qboolean isTeamsMode;

	if ( bufferSize <= 0 ) {
		return;
	}

	buffer[0] = '\0';
	head[0] = '\0';
	torso[0] = '\0';
	legs[0] = '\0';
	foundHead = qfalse;
	foundTorso = qfalse;
	foundLegs = qfalse;

	isTeamsMode = ( s_osp2be.activeCategory == 3 );
	if ( isTeamsMode ) {
		targetCvar = s_osp2be.teamsPreviewMode ? "cg_enemyModelColors" : "cg_teamModelColors";
	} else {
		targetCvar = NULL;
	}

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		const osp2beCvarDef_t *def;

		if ( !s_osp2be.activeDefs[i] ) {
			continue;
		}

		def = s_osp2be.activeDefs[i];
		if ( isTeamsMode && targetCvar ) {
			if ( !def->cvarName || Q_stricmp( def->cvarName, targetCvar ) != 0 ) {
				continue;
			}
		}

		if ( def->special == OSP2BE_SPECIAL_MODEL_HEAD ) {
			Q_strncpyz( head, s_osp2be.fields[i].field.buffer, sizeof( head ) );
			foundHead = qtrue;
		} else if ( def->special == OSP2BE_SPECIAL_MODEL_TORSO ) {
			Q_strncpyz( torso, s_osp2be.fields[i].field.buffer, sizeof( torso ) );
			foundTorso = qtrue;
		} else if ( def->special == OSP2BE_SPECIAL_MODEL_LEGS ) {
			Q_strncpyz( legs, s_osp2be.fields[i].field.buffer, sizeof( legs ) );
			foundLegs = qtrue;
		}
	}

	if ( foundHead && foundTorso && foundLegs ) {
		Com_sprintf( buffer, bufferSize, "%s%s%s", head, torso, legs );
	}
}

qboolean Osp2Be_GetOutlineColorFromFields( char *buffer, int bufferSize ) {
	int i;
	const char *targetCvar;

	if ( bufferSize <= 0 ) {
		return qfalse;
	}

	buffer[0] = '\0';

	if ( s_osp2be.activeCategory == 3 ) {
		targetCvar = s_osp2be.teamsPreviewMode ? "cg_enemyOutlineColor" : "cg_teamOutlineColor";
	} else {
		targetCvar = "cg_teamOutlineColor";
	}

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		if ( !s_osp2be.activeDefs[i] ) {
			continue;
		}

		if ( s_osp2be.activeDefs[i]->special == OSP2BE_SPECIAL_COLOR_GENERIC &&
			 ( s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_FIELD ||
			   s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_COLOR_BUTTON ) ) {
			if ( s_osp2be.activeCategory == 3 ) {
				if ( s_osp2be.activeDefs[i]->cvarName && !Q_stricmp( s_osp2be.activeDefs[i]->cvarName, targetCvar ) ) {
					Q_strncpyz( buffer, s_osp2be.fields[i].field.buffer, bufferSize );
					return qtrue;
				}
			} else {
				Q_strncpyz( buffer, s_osp2be.fields[i].field.buffer, bufferSize );
				return qtrue;
			}
		}
	}

	return qfalse;
}

int Osp2Be_GetOutlineSizeFromFields( void ) {
	int i;
	const char *targetCvar;

	if ( s_osp2be.activeCategory == 3 ) {
		targetCvar = s_osp2be.teamsPreviewMode ? "cg_enemyOutlineSize" : "cg_teamOutlineSize";
	} else {
		targetCvar = "cg_teamOutlineSize";
	}

	for ( i = 0; i < OSP2BE_MAX_CONTROLS; i++ ) {
		if ( !s_osp2be.activeDefs[i] ) {
			continue;
		}

		if ( s_osp2be.activeDefs[i]->special == OSP2BE_SPECIAL_OUTLINE_SIZE ) {
			if ( s_osp2be.activeCategory == 3 ) {
				if ( s_osp2be.activeDefs[i]->cvarName && !Q_stricmp( s_osp2be.activeDefs[i]->cvarName, targetCvar ) ) {
					if ( s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_LIST ) {
						return s_osp2be.lists[i].curvalue;
					}
					if ( s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_SIZE_BUTTON ) {
						if ( s_osp2be.bufferedInitialized ) {
							return s_osp2be.bufferedSizeValues[s_osp2be.activeCategory][i];
						}
						return (int)trap_Cvar_VariableValue( s_osp2be.activeDefs[i]->cvarName );
					}
				}
			} else {
				if ( s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_LIST ) {
					return s_osp2be.lists[i].curvalue;
				}
				if ( s_osp2be.activeDefs[i]->controlType == OSP2BE_CONTROL_SIZE_BUTTON ) {
					if ( s_osp2be.bufferedInitialized ) {
						return s_osp2be.bufferedSizeValues[s_osp2be.activeCategory][i];
					}
					return (int)trap_Cvar_VariableValue( s_osp2be.activeDefs[i]->cvarName );
				}
			}
		}
	}

	return -1;
}

void Osp2Be_UpdatePreviewModel( void ) {
	char modelBuffer[MAX_QPATH];
	char normalizedModel[MAX_QPATH];
	const char *modelCvar;
	vec3_t viewAngles;
	char colorBuffer[64];
	vec4_t headColor;
	vec4_t torsoColor;
	vec4_t legsColor;
	qboolean haveModelColors;
	char outlineColorBuffer[64];
	vec4_t outlineColor;
	int drawOutline;
	int outlineSize;
	qhandle_t outlineShader;
	qboolean outlineAllowed;
	int previewCategory;
	qboolean isTeamsMode;

	if ( s_osp2be.activeCategory != 1 && s_osp2be.activeCategory != 3 ) {
		return;
	}

	isTeamsMode = ( s_osp2be.activeCategory == 3 );
	if ( isTeamsMode ) {
		previewCategory = s_osp2be.teamsPreviewMode ? 3 : 2;
	} else {
		previewCategory = s_osp2be.activeCategory;
	}

	{
		const char *bufferedModel;

		bufferedModel = NULL;
		if ( s_osp2be.activeCategory == 1 && s_osp2be.bufferedPlayerModel[0] ) {
			bufferedModel = s_osp2be.bufferedPlayerModel;
		} else if ( isTeamsMode ) {
			if ( s_osp2be.teamsPreviewMode ) {
				if ( s_osp2be.bufferedEnemyModel[0] ) {
					bufferedModel = s_osp2be.bufferedEnemyModel;
				}
			} else {
				if ( s_osp2be.bufferedAllyModel[0] ) {
					bufferedModel = s_osp2be.bufferedAllyModel;
				}
			}
		}

		if ( bufferedModel ) {
			Q_strncpyz( modelBuffer, bufferedModel, sizeof( modelBuffer ) );
		} else {
			modelCvar = Osp2Be_GetModelCvarForCategory( previewCategory );
			trap_Cvar_VariableStringBuffer( modelCvar, modelBuffer, sizeof( modelBuffer ) );

			if ( !modelBuffer[0] || !Q_stricmp( modelBuffer, "0" ) || !Q_stricmp( modelBuffer, "none" ) ) {
				trap_Cvar_VariableStringBuffer( "model", modelBuffer, sizeof( modelBuffer ) );
			}
			if ( !modelBuffer[0] ) {
				Q_strncpyz( modelBuffer, "sarge/default", sizeof( modelBuffer ) );
			}
		}
	}

	if ( !Osp2Be_StringContainsChar( modelBuffer, '/' ) ) {
		Com_sprintf( normalizedModel, sizeof( normalizedModel ), "%s/default", modelBuffer );
	} else {
		Q_strncpyz( normalizedModel, modelBuffer, sizeof( normalizedModel ) );
	}

	if ( s_osp2be.previewCategory != previewCategory ||
		Q_stricmp( normalizedModel, s_osp2be.previewModel ) != 0 ) {
		VectorSet( viewAngles, 0, 150, 0 );
		UI_PlayerInfo_SetModel( &s_osp2be.previewInfo, normalizedModel );
		UI_PlayerInfo_SetInfo( &s_osp2be.previewInfo, LEGS_IDLE, TORSO_STAND, viewAngles, vec3_origin, WP_MACHINEGUN, qfalse );
		Q_strncpyz( s_osp2be.previewModel, normalizedModel, sizeof( s_osp2be.previewModel ) );
		s_osp2be.previewCategory = previewCategory;
	}

	s_osp2be.previewInfo.useCustomSkinColors = qfalse;
	s_osp2be.previewInfo.useOutline = qfalse;
	s_osp2be.previewInfo.outlineShader = 0;

	haveModelColors = qfalse;

	/* Get colors from field buffers instead of cvars */
	Osp2Be_GetModelColorsFromFields( colorBuffer, sizeof( colorBuffer ) );

	if ( Osp2Be_ParseModelColors( colorBuffer, headColor, torsoColor, legsColor ) ) {
		s_osp2be.previewInfo.customHeadColor[0] = headColor[0];
		s_osp2be.previewInfo.customHeadColor[1] = headColor[1];
		s_osp2be.previewInfo.customHeadColor[2] = headColor[2];
		s_osp2be.previewInfo.customHeadColor[3] = headColor[3];
		s_osp2be.previewInfo.customTorsoColor[0] = torsoColor[0];
		s_osp2be.previewInfo.customTorsoColor[1] = torsoColor[1];
		s_osp2be.previewInfo.customTorsoColor[2] = torsoColor[2];
		s_osp2be.previewInfo.customTorsoColor[3] = torsoColor[3];
		s_osp2be.previewInfo.customLegsColor[0] = legsColor[0];
		s_osp2be.previewInfo.customLegsColor[1] = legsColor[1];
		s_osp2be.previewInfo.customLegsColor[2] = legsColor[2];
		s_osp2be.previewInfo.customLegsColor[3] = legsColor[3];
		s_osp2be.previewInfo.useCustomSkinColors = qtrue;
		haveModelColors = qtrue;
	}

	outlineShader = 0;
	outlineAllowed = qfalse;
	{
		float bufferedValue;

		drawOutline = (int)trap_Cvar_VariableValue( "cg_drawOutline" );
		if ( Osp2Be_GetBufferedNumericValue( "cg_drawOutline", &bufferedValue ) ) {
			drawOutline = (int)bufferedValue;
		}
	}

	/* Get outline size from field buffer */
	outlineSize = Osp2Be_GetOutlineSizeFromFields();

	if ( s_osp2be.activeCategory == 1 ) {
		if ( outlineSize > 0 && ( drawOutline == 2 || drawOutline == 3 ) ) {
			outlineAllowed = qtrue;
			if ( Osp2Be_GetOutlineColorFromFields( outlineColorBuffer, sizeof( outlineColorBuffer ) ) ) {
				if ( !Osp2Be_ParseColorValue( outlineColorBuffer, outlineColor ) ) {
					Osp2Be_SetColorVec4( outlineColor, 0.0f, 1.0f, 0.0f, 1.0f );
				}
			} else {
				Osp2Be_SetColorVec4( outlineColor, 0.0f, 1.0f, 0.0f, 1.0f );
			}
			outlineShader = Osp2Be_GetOutlineShader( outlineSize );
		}
	} else if ( isTeamsMode ) {
		if ( outlineSize > 0 && ( drawOutline == 1 || drawOutline == 3 ) ) {
			outlineAllowed = qtrue;
			if ( Osp2Be_GetOutlineColorFromFields( outlineColorBuffer, sizeof( outlineColorBuffer ) ) ) {
				if ( !Osp2Be_ParseColorValue( outlineColorBuffer, outlineColor ) ) {
					Osp2Be_SetColorVec4( outlineColor, 1.0f, 0.0f, 1.0f, 1.0f );
				}
			} else {
				Osp2Be_SetColorVec4( outlineColor, 1.0f, 0.0f, 1.0f, 1.0f );
			}
			outlineShader = Osp2Be_GetOutlineShader( outlineSize );
		} else {
			outlineAllowed = qfalse;
			outlineShader = 0;
		}
	}

	if ( outlineAllowed && outlineShader ) {
		s_osp2be.previewInfo.outlineColor[0] = outlineColor[0];
		s_osp2be.previewInfo.outlineColor[1] = outlineColor[1];
		s_osp2be.previewInfo.outlineColor[2] = outlineColor[2];
		s_osp2be.previewInfo.outlineColor[3] = 1.0f;
		s_osp2be.previewInfo.outlineShader = outlineShader;
		s_osp2be.previewInfo.useOutline = qtrue;
	} else {
		s_osp2be.previewInfo.useOutline = qfalse;
		s_osp2be.previewInfo.outlineShader = 0;
	}

	if ( s_osp2be.previewFrozenActive && osp2beFrozenOverlay ) {
		vec4_t frozenColor;
		int mode;
		float paintLevel;
		qboolean applyPaint;
		float freezeShade;

		mode = (int)trap_Cvar_VariableValue( "cg_deadBodyBlack" );
		applyPaint = ( mode > 0 );
		freezeShade = 1.0f;
		switch ( mode ) {
		case 1:
			paintLevel = 165.0f / 255.0f;
			break;
		case 2:
			paintLevel = 110.0f / 255.0f;
			break;
		case 3:
		default:
			paintLevel = 55.0f / 255.0f;
			break;
		}

		if ( applyPaint ) {
			freezeShade = paintLevel;
			Osp2Be_SetColorVec4( s_osp2be.previewInfo.customHeadColor, paintLevel, paintLevel, paintLevel, 1.0f );
			Osp2Be_SetColorVec4( s_osp2be.previewInfo.customTorsoColor, paintLevel, paintLevel, paintLevel, 1.0f );
			Osp2Be_SetColorVec4( s_osp2be.previewInfo.customLegsColor, paintLevel, paintLevel, paintLevel, 1.0f );
			s_osp2be.previewInfo.useCustomSkinColors = qtrue;
			haveModelColors = qtrue;
		}

		Osp2Be_GetFrozenColorForCategory( previewCategory, frozenColor );

		if ( freezeShade < 0.0f ) {
			freezeShade = 0.0f;
		}
		if ( freezeShade > 1.0f ) {
			freezeShade = 1.0f;
		}
		frozenColor[0] *= freezeShade;
		frozenColor[1] *= freezeShade;
		frozenColor[2] *= freezeShade;

		s_osp2be.previewInfo.useOutline = qfalse;
		s_osp2be.previewInfo.outlineShader = 0;

		s_osp2be.previewInfo.freezeActive = qtrue;
		s_osp2be.previewInfo.freezeShader = osp2beFrozenOverlay;
		s_osp2be.previewInfo.freezeColor[0] = frozenColor[0];
		s_osp2be.previewInfo.freezeColor[1] = frozenColor[1];
		s_osp2be.previewInfo.freezeColor[2] = frozenColor[2];
		s_osp2be.previewInfo.freezeColor[3] = 1.0f;
		s_osp2be.previewInfo.legsAnim = LEGS_IDLE;
		s_osp2be.previewInfo.torsoAnim = TORSO_STAND;
		s_osp2be.previewInfo.legsAnimationTimer = 0;
		s_osp2be.previewInfo.torsoAnimationTimer = 0;
		if ( s_osp2be.previewInfo.legs.animation ) {
			s_osp2be.previewInfo.legs.oldFrame = s_osp2be.previewInfo.legs.frame = s_osp2be.previewInfo.legs.animation->firstFrame;
			s_osp2be.previewInfo.legs.backlerp = 0.0f;
		}
		if ( s_osp2be.previewInfo.torso.animation ) {
			s_osp2be.previewInfo.torso.oldFrame = s_osp2be.previewInfo.torso.frame = s_osp2be.previewInfo.torso.animation->firstFrame;
			s_osp2be.previewInfo.torso.backlerp = 0.0f;
		}
	} else {
		s_osp2be.previewInfo.freezeActive = qfalse;
		s_osp2be.previewInfo.freezeShader = 0;
		Osp2Be_SetColorVec4( s_osp2be.previewInfo.freezeColor, 0.0f, 0.0f, 0.0f, 0.0f );
	}

	if ( !haveModelColors ) {
		Osp2Be_SetColorVec4( s_osp2be.previewInfo.customHeadColor, 0.0f, 0.0f, 0.0f, 0.0f );
		Osp2Be_SetColorVec4( s_osp2be.previewInfo.customTorsoColor, 0.0f, 0.0f, 0.0f, 0.0f );
		Osp2Be_SetColorVec4( s_osp2be.previewInfo.customLegsColor, 0.0f, 0.0f, 0.0f, 0.0f );
	}
}

