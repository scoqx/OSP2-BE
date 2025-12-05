#include "ui_osp2be_shared.h"
/* Named colors - exported for utility functions */
const osp2beNamedColor_t osp2beNamedColors[] = {
    { "white", 1.0f, 1.0f, 1.0f },
    { "black", 0.0f, 0.0f, 0.0f },
    { "red", 1.0f, 0.0f, 0.0f },
    { "green", 0.0f, 1.0f, 0.0f },
    { "blue", 0.0f, 0.0f, 1.0f },
    { "yellow", 1.0f, 1.0f, 0.0f },
    { "magenta", 1.0f, 0.0f, 1.0f },
    { "cyan", 0.0f, 1.0f, 1.0f },
    { "orange", 1.0f, 0.5f, 0.0f },
    { "purple", 0.5f, 0.0f, 0.5f },
    { "pink", 1.0f, 0.75f, 0.8f },
    { "teal", 0.0f, 0.5f, 0.5f },
    { "grey", 0.5f, 0.5f, 0.5f },
    { "gray", 0.5f, 0.5f, 0.5f },
    { NULL, 0.0f, 0.0f, 0.0f }
};

extern double cos( double );

int Osp2Be_StringLength( const char *text ) {
	int length;

	if ( !text ) {
		return 0;
	}

	length = 0;
	while ( text[length] ) {
		length++;
	}

	return length;
}

qboolean Osp2Be_StringContainsChar( const char *text, char ch ) {
	if ( !text ) {
		return qfalse;
	}

	while ( *text ) {
		if ( *text == ch ) {
			return qtrue;
		}
		text++;
	}

	return qfalse;
}

void Osp2Be_SetColorVec4( vec4_t color, float r, float g, float b, float a ) {
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
}

int Osp2Be_HexDigit( char c ) {
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}
	if ( c >= 'a' && c <= 'f' ) {
		return 10 + c - 'a';
	}
	if ( c >= 'A' && c <= 'F' ) {
		return 10 + c - 'A';
	}
	return -1;
}

float Osp2Be_ParseHexComponent( const char *ptr ) {
	int hi;
	int lo;
	int value;

	if ( !ptr || !*(ptr + 1) ) {
		return -1.0f;
	}

	hi = Osp2Be_HexDigit( ptr[0] );
	if ( hi < 0 ) {
		return -1.0f;
	}
	lo = Osp2Be_HexDigit( ptr[1] );
	if ( lo < 0 ) {
		return -1.0f;
	}

	value = ( hi << 4 ) | lo;
	return (float)value / 255.0f;
}

qboolean Osp2Be_ParseHexColor( const char *value, vec4_t out ) {
	float component;

	if ( !value ) {
		return qfalse;
	}

	if ( !value[0] || !value[1] || !value[2] || !value[3] || !value[4] || !value[5] ) {
		return qfalse;
	}

	component = Osp2Be_ParseHexComponent( value );
	if ( component < 0.0f ) {
		return qfalse;
	}
	out[0] = component;

	component = Osp2Be_ParseHexComponent( value + 2 );
	if ( component < 0.0f ) {
		return qfalse;
	}
	out[1] = component;

	component = Osp2Be_ParseHexComponent( value + 4 );
	if ( component < 0.0f ) {
		return qfalse;
	}
	out[2] = component;
	out[3] = 1.0f;
	return qtrue;
}

static qboolean Osp2Be_ParseHexColorShort( const char *value, vec4_t out ) {
	int i;
	int digit;

	if ( !value ) {
		return qfalse;
	}

	for ( i = 0; i < 3; i++ ) {
		digit = Osp2Be_HexDigit( value[i] );
		if ( digit < 0 ) {
			return qfalse;
		}
		out[i] = (float)digit / 15.0f;
	}
	out[3] = 1.0f;
	return qtrue;
}

void Osp2Be_OSPColorFromNumber( int number, vec4_t out ) {
	float red;
	float green;
	float blue;
	double tmp;
	double sixtyRadians;

	red = 1.0f;
	green = 1.0f;
	blue = 1.0f;

	if ( number >= 10 && number < 34 ) {
		tmp = ( ( 360.0 * (double)number ) - 3600.0 ) / 24.0;
		sixtyRadians = 60.0 * M_PI / 180.0;
		if ( tmp <= 120.0 ) {
			tmp = M_PI * tmp / 180.0;
			blue = 0.0f;
			red = (float)((cos( tmp ) / (cos( sixtyRadians - tmp )) + 1.0) / 3.0);
			green = 1.0f - ( red + blue );
		} else if ( tmp <= 240.0 ) {
			tmp = M_PI * ( tmp - 120.0 ) / 180.0;
			red = 0.0f;
			green = (float)((cos( tmp ) / (cos( sixtyRadians - tmp )) + 1.0) / 3.0);
			blue = 1.0f - ( red + green );
		} else {
			tmp = M_PI * ( tmp - 240.0 ) / 180.0;
			green = 0.0f;
			blue = (float)((cos( tmp ) / (cos( sixtyRadians - tmp )) + 1.0) / 3.0);
			red = 1.0f - ( green + blue );
		}
	} else {
		switch ( number ) {
		case 0:
			red = 1.0f;
			green = 1.0f;
			blue = 1.0f;
			break;
		case 1:
			red = 1.0f;
			green = 0.2f;
			blue = 0.2f;
			break;
		case 2:
			red = 0.2f;
			green = 1.0f;
			blue = 0.2f;
			break;
		case 3:
			red = 1.0f;
			green = 1.0f;
			blue = 0.1f;
			break;
		case 4:
			red = 0.2f;
			green = 0.2f;
			blue = 1.0f;
			break;
		case 5:
			red = 0.1f;
			green = 1.0f;
			blue = 1.0f;
			break;
		case 6:
			red = 1.0f;
			green = 0.1f;
			blue = 1.0f;
			break;
		case 7:
			red = 1.0f;
			green = 0.59f;
			blue = 0.12f;
			break;
		case 8:
			red = 0.65f;
			green = 0.2f;
			blue = 0.65f;
			break;
		case 9:
			red = 0.6f;
			green = 0.6f;
			blue = 1.0f;
			break;
		default:
			red = 1.0f;
			green = 1.0f;
			blue = 1.0f;
			break;
		}
	}

	out[0] = red;
	out[1] = green;
	out[2] = blue;
	out[3] = 1.0f;
}

qboolean Osp2Be_ColorFromChar( char code, vec4_t out ) {
	int number;

	if ( code >= '0' && code <= '9' ) {
		number = code - '0';
	} else if ( code >= 'a' && code <= 'z' ) {
		number = code - 'a' + 10;
	} else if ( code >= 'A' && code <= 'Z' ) {
		number = code - 'A' + 10;
	} else {
		return qfalse;
	}

	if ( number < 0 ) {
		return qfalse;
	}

	Osp2Be_OSPColorFromNumber( number, out );
	return qtrue;
}

qboolean Osp2Be_ParseColorValue( const char *value, vec4_t out ) {
	int len;
	int i;
	char lowered[32];
	const osp2beNamedColor_t *named;

	if ( !value ) {
		return qfalse;
	}

	while ( *value == ' ' || *value == '\t' ) {
		value++;
	}

	len = Osp2Be_StringLength( value );
	if ( len <= 0 ) {
		return qfalse;
	}

	if ( value[0] == '^' && len > 1 ) {
		if ( Osp2Be_ColorFromChar( value[1], out ) ) {
			return qtrue;
		}
	}

	if ( value[0] == '#' && len >= 7 ) {
		if ( Osp2Be_ParseHexColor( value + 1, out ) ) {
			return qtrue;
		}
	}

	if ( value[0] == '#' && len == 4 ) {
		if ( Osp2Be_ParseHexColorShort( value + 1, out ) ) {
			return qtrue;
		}
	}

	if ( len == 6 ) {
		if ( Osp2Be_ParseHexColor( value, out ) ) {
			return qtrue;
		}
	}

	if ( len == 3 ) {
		if ( Osp2Be_ParseHexColorShort( value, out ) ) {
			return qtrue;
		}
	}

	if ( len == 1 ) {
		if ( Osp2Be_ColorFromChar( value[0], out ) ) {
			return qtrue;
		}
	}

	if ( len < (int)sizeof( lowered ) ) {
		for ( i = 0; i < len && i < (int)sizeof( lowered ) - 1; i++ ) {
			char c;
			c = value[i];
			if ( c >= 'A' && c <= 'Z' ) {
				c = (char)( c - 'A' + 'a' );
			}
			lowered[i] = c;
		}
		lowered[i] = '\0';

		named = osp2beNamedColors;
		while ( named->name ) {
			if ( !Q_stricmp( lowered, named->name ) ) {
				Osp2Be_SetColorVec4( out, named->r, named->g, named->b, 1.0f );
				return qtrue;
			}
			named++;
		}
	}

	return qfalse;
}

qboolean Osp2Be_ParseModelColors( const char *value, vec4_t head, vec4_t torso, vec4_t legs ) {
	int len;
	int digitCount;
	char digits[4];
	const char *ptr;
	float component;
	qboolean success;
	qboolean hexOnly;
	int i;

	if ( !value ) {
		return qfalse;
	}

	len = Osp2Be_StringLength( value );
	if ( len >= 6 ) {
		hexOnly = qtrue;
		for ( i = 0; i < len; i++ ) {
			if ( Osp2Be_HexDigit( value[i] ) < 0 ) {
				hexOnly = qfalse;
				break;
			}
		}
		if ( hexOnly ) {
			Osp2Be_SetColorVec4( head, 1.0f, 1.0f, 1.0f, 1.0f );
			Osp2Be_SetColorVec4( torso, 1.0f, 1.0f, 1.0f, 1.0f );
			Osp2Be_SetColorVec4( legs, 1.0f, 1.0f, 1.0f, 1.0f );

			component = Osp2Be_ParseHexComponent( value );
			if ( component < 0.0f ) {
				return qfalse;
			}
			head[0] = component;
			component = Osp2Be_ParseHexComponent( value + 2 );
			if ( component < 0.0f ) {
				return qfalse;
			}
			head[1] = component;
			component = Osp2Be_ParseHexComponent( value + 4 );
			if ( component < 0.0f ) {
				return qfalse;
			}
			head[2] = component;

			if ( len >= 12 ) {
				component = Osp2Be_ParseHexComponent( value + 6 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				torso[0] = component;
				component = Osp2Be_ParseHexComponent( value + 8 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				torso[1] = component;
				component = Osp2Be_ParseHexComponent( value + 10 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				torso[2] = component;
			}

			if ( len >= 18 ) {
				component = Osp2Be_ParseHexComponent( value + 12 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				legs[0] = component;
				component = Osp2Be_ParseHexComponent( value + 14 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				legs[1] = component;
				component = Osp2Be_ParseHexComponent( value + 16 );
				if ( component < 0.0f ) {
					return qfalse;
				}
				legs[2] = component;
			}

			head[3] = 1.0f;
			torso[3] = 1.0f;
			legs[3] = 1.0f;
			return qtrue;
		}
	}

	digitCount = 0;
	ptr = value;
	while ( *ptr && digitCount < 3 ) {
		if ( *ptr == '^' ) {
			ptr++;
			continue;
		}
		digits[digitCount] = *ptr;
		digitCount++;
		ptr++;
	}

	if ( digitCount == 3 ) {
		success = Osp2Be_ColorFromChar( digits[0], head );
		if ( !success ) {
			return qfalse;
		}
		success = Osp2Be_ColorFromChar( digits[1], torso );
		if ( !success ) {
			return qfalse;
		}
		success = Osp2Be_ColorFromChar( digits[2], legs );
		if ( !success ) {
			return qfalse;
		}
		return qtrue;
	}

	return qfalse;
}

qhandle_t Osp2Be_GetOutlineShader( int size ) {
	if ( size <= 1 ) {
		if ( !osp2beOutlineThin ) {
			osp2beOutlineThin = trap_R_RegisterShaderNoMip( "outlineThin" );
		}
		return osp2beOutlineThin;
	}

	if ( size == 2 ) {
		if ( !osp2beOutlineMedium ) {
			osp2beOutlineMedium = trap_R_RegisterShaderNoMip( "outlineMedium" );
		}
		return osp2beOutlineMedium;
	}

	if ( !osp2beOutlineWide ) {
		osp2beOutlineWide = trap_R_RegisterShaderNoMip( "outlineWide" );
	}
	return osp2beOutlineWide;
}

void Osp2Be_ReadModelSegment( const char *cvarName, int segment, char *buffer, int bufferSize ) {
	char value[MAX_EDIT_LINE];
	int len;
	int offset;
	int copy;
	int i;
	qboolean hexOnly;
	int count;
	char code;

	if ( bufferSize <= 0 ) {
		return;
	}

	buffer[0] = '\0';
	if ( !cvarName ) {
		return;
	}

	trap_Cvar_VariableStringBuffer( cvarName, value, sizeof( value ) );
	len = Q_PrintStrlen( value );
	if ( len <= 0 ) {
		return;
	}

	hexOnly = qfalse;
	if ( len >= 6 ) {
		hexOnly = qtrue;
		for ( i = 0; i < len; i++ ) {
			if ( Osp2Be_HexDigit( value[i] ) < 0 ) {
				hexOnly = qfalse;
				break;
			}
		}
	}

	if ( hexOnly ) {
		offset = segment * 6;
		if ( offset + 6 <= len ) {
			copy = 6;
			if ( copy >= bufferSize ) {
				copy = bufferSize - 1;
			}
			if ( copy < 0 ) {
				copy = 0;
			}
			for ( i = 0; i < copy; i++ ) {
				buffer[i] = value[offset + i];
			}
			buffer[copy] = '\0';
			return;
		}
		return;
	}

	count = 0;
	i = 0;
	while ( value[i] ) {
		code = value[i];
		if ( code == '^' ) {
			i++;
			if ( !value[i] ) {
				break;
			}
			code = value[i];
		}
		if ( count == segment ) {
			if ( bufferSize > 1 ) {
				buffer[0] = code;
				buffer[1] = '\0';
			}
			return;
		}
		count++;
		i++;
	}
}

void Osp2Be_ReadRailSegment( const char *cvarName, int segment, char *buffer, int bufferSize ) {
	char value[MAX_EDIT_LINE];
	int len;
	int offset;
	int copy;
	int i;
	qboolean hexOnly;
	int count;
	char code;

	if ( bufferSize <= 0 ) {
		return;
	}

	buffer[0] = '\0';
	if ( !cvarName ) {
		return;
	}

	trap_Cvar_VariableStringBuffer( cvarName, value, sizeof( value ) );
	len = Q_PrintStrlen( value );
	if ( len <= 0 ) {
		return;
	}

	hexOnly = qfalse;
	if ( len >= 6 ) {
		hexOnly = qtrue;
		for ( i = 0; i < len; i++ ) {
			if ( Osp2Be_HexDigit( value[i] ) < 0 ) {
				hexOnly = qfalse;
				break;
			}
		}
	}

	if ( hexOnly ) {
		offset = segment * 6;
		if ( offset + 6 <= len ) {
			copy = 6;
			if ( copy >= bufferSize ) {
				copy = bufferSize - 1;
			}
			if ( copy < 0 ) {
				copy = 0;
			}
			for ( i = 0; i < copy; i++ ) {
				buffer[i] = value[offset + i];
			}
			buffer[copy] = '\0';
			return;
		}
		return;
	}

	count = 0;
	i = 0;
	while ( value[i] ) {
		code = value[i];
		if ( code == '^' ) {
			i++;
			if ( !value[i] ) {
				break;
			}
			code = value[i];
		}
		if ( count == segment ) {
			if ( bufferSize > 1 ) {
				buffer[0] = code;
				buffer[1] = '\0';
			}
			return;
		}
		count++;
		i++;
	}
}

void Osp2Be_WriteModelSegments( const char *cvarName, char segments[3][MAX_EDIT_LINE] ) {
	char combined[MAX_EDIT_LINE];
	char head[8];
	char torso[8];
	char legs[8];
	char fallback[8];

	if ( !cvarName ) {
		return;
	}

	/* get head */
	if ( segments[0][0] ) {
		Q_strncpyz( head, segments[0], sizeof( head ) );
	} else {
		Osp2Be_ReadModelSegment( cvarName, 0, fallback, sizeof( fallback ) );
		Q_strncpyz( head, fallback, sizeof( head ) );
	}

	/* get torso */
	if ( segments[1][0] ) {
		Q_strncpyz( torso, segments[1], sizeof( torso ) );
	} else {
		Osp2Be_ReadModelSegment( cvarName, 1, fallback, sizeof( fallback ) );
		Q_strncpyz( torso, fallback, sizeof( torso ) );
	}

	/* get legs */
	if ( segments[2][0] ) {
		Q_strncpyz( legs, segments[2], sizeof( legs ) );
	} else {
		Osp2Be_ReadModelSegment( cvarName, 2, fallback, sizeof( fallback ) );
		Q_strncpyz( legs, fallback, sizeof( legs ) );
	}

	Com_sprintf( combined, sizeof( combined ), "%s%s%s", head, torso, legs );

	trap_Cvar_Set( cvarName, combined );
}

void Osp2Be_WriteRailSegments( const char *cvarName, char segments[2][MAX_EDIT_LINE] ) {
	char combined[MAX_EDIT_LINE];
	char core[8];
	char ring[8];
	char fallback[8];

	if ( !cvarName ) {
		return;
	}

	if ( segments[0][0] ) {
		Q_strncpyz( core, segments[0], sizeof( core ) );
	} else {
		Osp2Be_ReadRailSegment( cvarName, 0, fallback, sizeof( fallback ) );
		Q_strncpyz( core, fallback, sizeof( core ) );
	}

	if ( segments[1][0] ) {
		Q_strncpyz( ring, segments[1], sizeof( ring ) );
	} else {
		Osp2Be_ReadRailSegment( cvarName, 1, fallback, sizeof( fallback ) );
		Q_strncpyz( ring, fallback, sizeof( ring ) );
	}

	Com_sprintf( combined, sizeof( combined ), "%s%s", core, ring );

	trap_Cvar_Set( cvarName, combined );
}

int Osp2Be_GetModelSegmentIndex( osp2beSpecial_t special ) {
	if ( special == OSP2BE_SPECIAL_MODEL_HEAD ) {
		return 0;
	}
	if ( special == OSP2BE_SPECIAL_MODEL_TORSO ) {
		return 1;
	}
	if ( special == OSP2BE_SPECIAL_MODEL_LEGS ) {
		return 2;
	}
	return -1;
}

int Osp2Be_GetRailSegmentIndex( osp2beSpecial_t special ) {
	if ( special == OSP2BE_SPECIAL_RAIL_BEAM ) {
		return 0;
	}
	if ( special == OSP2BE_SPECIAL_RAIL_RING ) {
		return 1;
	}
	return -1;
}

int Osp2Be_FindModelAssemblyIndex( osp2beModelAssembly_t *assemblies, int count, const char *cvarName ) {
	int i;

	for ( i = 0; i < count; i++ ) {
		if ( assemblies[i].cvarName && cvarName && !Q_stricmp( assemblies[i].cvarName, cvarName ) ) {
			return i;
		}
	}

	return -1;
}

int Osp2Be_FindRailAssemblyIndex( osp2beRailAssembly_t *assemblies, int count, const char *cvarName ) {
	int i;

	for ( i = 0; i < count; i++ ) {
		if ( assemblies[i].cvarName && cvarName && !Q_stricmp( assemblies[i].cvarName, cvarName ) ) {
			return i;
		}
	}

	return -1;
}

/* Model buffer functions */
void Osp2Be_LoadBufferedModelsFromCvars( void ) {
	trap_Cvar_VariableStringBuffer( "model", s_osp2be.bufferedPlayerModel, sizeof( s_osp2be.bufferedPlayerModel ) );
	if ( !s_osp2be.bufferedPlayerModel[0] ) {
		Q_strncpyz( s_osp2be.bufferedPlayerModel, "sarge/default", sizeof( s_osp2be.bufferedPlayerModel ) );
	}

	trap_Cvar_VariableStringBuffer( "headmodel", s_osp2be.bufferedPlayerHead, sizeof( s_osp2be.bufferedPlayerHead ) );
	if ( !s_osp2be.bufferedPlayerHead[0] ) {
		Q_strncpyz( s_osp2be.bufferedPlayerHead, s_osp2be.bufferedPlayerModel, sizeof( s_osp2be.bufferedPlayerHead ) );
	}

	trap_Cvar_VariableStringBuffer( "cg_teamModel", s_osp2be.bufferedAllyModel, sizeof( s_osp2be.bufferedAllyModel ) );
	if ( !s_osp2be.bufferedAllyModel[0] ) {
		Q_strncpyz( s_osp2be.bufferedAllyModel, s_osp2be.bufferedPlayerModel, sizeof( s_osp2be.bufferedAllyModel ) );
	}

	trap_Cvar_VariableStringBuffer( "cg_enemyModel", s_osp2be.bufferedEnemyModel, sizeof( s_osp2be.bufferedEnemyModel ) );
	if ( !s_osp2be.bufferedEnemyModel[0] ) {
		Q_strncpyz( s_osp2be.bufferedEnemyModel, s_osp2be.bufferedPlayerModel, sizeof( s_osp2be.bufferedEnemyModel ) );
	}

	s_osp2be.pendingModelCategory = -1;
	s_osp2be.pendingModelButtonIndex = -1;
}

char *Osp2Be_GetModelBufferForCategory( int category ) {
	switch ( category ) {
	case 1:
		return s_osp2be.bufferedPlayerModel;
	case 2:
		return s_osp2be.bufferedAllyModel;
	case 3:
		return s_osp2be.bufferedEnemyModel;
	default:
		return NULL;
	}
}

const char *Osp2Be_GetBufferedModelForCategory( int category ) {
	char *buffer;

	buffer = Osp2Be_GetModelBufferForCategory( category );
	return ( buffer && buffer[0] ) ? buffer : NULL;
}


