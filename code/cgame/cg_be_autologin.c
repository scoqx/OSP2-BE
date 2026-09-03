/*
===========================================================================
cg_be_autologin.c

Server-issued opaque autologin token (not password).
  altoken     -> create-once player.be
  altoken_rot -> replace player.be after passwd
  After BE auth pulses + delay -> logintoken <token>

player.be wire: every on-disk byte has bit7 set (no CR/LF/NUL/ASCII),
salted header, stream XOR + checksum. Looks like one unbroken mojibake
line in notepad; partial copy fails integrity check.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"

#define BE_AL_FILE		"player.be"
#define BE_AL_VERSION	2
#define BE_AL_NAME_MAX	64
#define BE_AL_TOKEN_MAX	80
#define BE_AL_HDR_SIZE	8
#define BE_AL_INNER_MAX	(1 + BE_AL_HDR_SIZE + BE_AL_NAME_MAX + BE_AL_TOKEN_MAX + 4)
#define BE_AL_WIRE_MAX	(BE_AL_INNER_MAX * 2)

/* Obfuscated tag (XORed with per-file salt on disk). */
static const byte beAlTag[4] = { 0xC3, 0x9A, 0x7E, 0xB1 };

static qboolean cg_beAlTried;
static char cg_beAlPendingName[BE_AL_NAME_MAX];
static char cg_beAlPendingToken[BE_AL_TOKEN_MAX];
static qboolean cg_beAlPendingRot;
static qboolean cg_beAlPendingValid;

void CG_BE_AutologinReset(void)
{
	cg_beAlTried = qfalse;
	cg_beAlPendingValid = qfalse;
	cg_beAlPendingRot = qfalse;
	cg_beAlPendingName[0] = '\0';
	cg_beAlPendingToken[0] = '\0';
}

/*
 * Stream byte at position i — derived key material (not a single XOR constant).
 */
static byte CG_BE_AlStreamByte(int i, int version, int nameLen, int tokenLen, byte salt)
{
	unsigned x;

	x = 0xA5C37E91u ^ (unsigned)version * 0x9E3779B9u;
	x ^= (unsigned)nameLen * 0x85EBCA6Bu;
	x ^= (unsigned)tokenLen * 0xC2B2AE35u;
	x ^= (unsigned)salt * 0x165667B1u;
	x ^= (unsigned)i * 0x27D4EB2Du;
	x ^= (x >> 16);
	x *= 0x45d9f3bu;
	x ^= (x >> 13);
	return (byte)(x & 0xFFu);
}

static void CG_BE_AlCrypt(byte *data, int len, int version, int nameLen, int tokenLen,
	byte salt, int pos0)
{
	int i;

	for (i = 0; i < len; i++)
	{
		data[i] ^= CG_BE_AlStreamByte(pos0 + i, version, nameLen, tokenLen, salt);
	}
}

/* FNV-1a 32-bit over buf[0..len) */
static unsigned CG_BE_AlChecksum(const byte *buf, int len)
{
	unsigned h;
	int i;

	h = 2166136261u;
	for (i = 0; i < len; i++)
	{
		h ^= (unsigned)buf[i];
		h *= 16777619u;
	}
	return h;
}

static void CG_BE_AlStoreU32LE(byte *dst, unsigned v)
{
	dst[0] = (byte)(v & 0xFFu);
	dst[1] = (byte)((v >> 8) & 0xFFu);
	dst[2] = (byte)((v >> 16) & 0xFFu);
	dst[3] = (byte)((v >> 24) & 0xFFu);
}

static unsigned CG_BE_AlLoadU32LE(const byte *src)
{
	return (unsigned)src[0]
		| ((unsigned)src[1] << 8)
		| ((unsigned)src[2] << 16)
		| ((unsigned)src[3] << 24);
}

/* Pack to bytes with bit7 always set — no CR/LF/NUL; one continuous line in editors. */
static int CG_BE_AlWireEncode(const byte *in, int inLen, byte *out, int outMax)
{
	int i;
	int o;

	if (inLen < 0 || inLen * 2 > outMax)
	{
		return -1;
	}
	o = 0;
	for (i = 0; i < inLen; i++)
	{
		out[o++] = (byte)(0x80u | ((unsigned)in[i] >> 4));
		out[o++] = (byte)(0x80u | ((unsigned)in[i] & 0x0Fu));
	}
	return o;
}

static int CG_BE_AlWireDecode(const byte *in, int inLen, byte *out, int outMax)
{
	int i;
	int o;

	if (inLen < 2 || (inLen & 1) || (inLen / 2) > outMax)
	{
		return -1;
	}
	for (i = 0; i < inLen; i++)
	{
		if (!(in[i] & 0x80))
		{
			return -1;
		}
	}
	o = 0;
	for (i = 0; i < inLen; i += 2)
	{
		out[o++] = (byte)(((in[i] & 0x0Fu) << 4) | (in[i + 1] & 0x0Fu));
	}
	return o;
}

static qboolean CG_BE_AlFileExists(void)
{
	fileHandle_t f;
	int len;

	len = trap_FS_FOpenFile(BE_AL_FILE, &f, FS_READ);
	if (len <= 0 || !f)
	{
		return qfalse;
	}
	trap_FS_FCloseFile(f);
	return qtrue;
}

static qboolean CG_BE_AlParseInner(const byte *inner, int innerLen,
	char *nameOut, int nameSize, char *tokenOut, int tokenSize)
{
	byte salt;
	int version;
	int nameLen;
	int tokenLen;
	int bodyLen;
	unsigned expect;
	unsigned got;

	if (innerLen < 1 + BE_AL_HDR_SIZE + 4)
	{
		return qfalse;
	}

	salt = inner[0];
	if ((inner[1] ^ salt) != beAlTag[0] || (inner[2] ^ salt) != beAlTag[1]
	        || (inner[3] ^ salt) != beAlTag[2] || (inner[4] ^ salt) != beAlTag[3])
	{
		return qfalse;
	}

	version = inner[5];
	nameLen = inner[6];
	tokenLen = inner[7];
	if (version != BE_AL_VERSION || nameLen <= 0 || tokenLen <= 0)
	{
		return qfalse;
	}
	if (nameLen >= nameSize || tokenLen >= tokenSize)
	{
		return qfalse;
	}

	bodyLen = 1 + BE_AL_HDR_SIZE + nameLen + tokenLen;
	if (innerLen != bodyLen + 4)
	{
		return qfalse;
	}

	expect = CG_BE_AlChecksum(inner, bodyLen);
	got = CG_BE_AlLoadU32LE(inner + bodyLen);
	if (expect != got)
	{
		return qfalse;
	}

	memcpy(nameOut, inner + 1 + BE_AL_HDR_SIZE, nameLen);
	nameOut[nameLen] = '\0';
	memcpy(tokenOut, inner + 1 + BE_AL_HDR_SIZE + nameLen, tokenLen);
	tokenOut[tokenLen] = '\0';
	CG_BE_AlCrypt((byte *)nameOut, nameLen, version, nameLen, tokenLen, salt, 0);
	CG_BE_AlCrypt((byte *)tokenOut, tokenLen, version, nameLen, tokenLen, salt, nameLen);
	return nameOut[0] && tokenOut[0];
}

/* Legacy v1 (fixed magic, may contain CR/LF) — still redeemable once. */
static qboolean CG_BE_AlParseLegacyV1(const byte *buf, int len,
	char *nameOut, int nameSize, char *tokenOut, int tokenSize)
{
	int version;
	int nameLen;
	int tokenLen;
	int bodyLen;
	unsigned expect;
	unsigned got;

	if (len < BE_AL_HDR_SIZE + 4)
	{
		return qfalse;
	}
	if (buf[0] != beAlTag[0] || buf[1] != beAlTag[1]
	        || buf[2] != beAlTag[2] || buf[3] != beAlTag[3])
	{
		return qfalse;
	}
	version = buf[4];
	nameLen = buf[5];
	tokenLen = buf[6];
	if (version != 1 || nameLen <= 0 || tokenLen <= 0)
	{
		return qfalse;
	}
	if (nameLen >= nameSize || tokenLen >= tokenSize)
	{
		return qfalse;
	}
	bodyLen = BE_AL_HDR_SIZE + nameLen + tokenLen;
	if (len != bodyLen + 4)
	{
		return qfalse;
	}
	expect = CG_BE_AlChecksum(buf, bodyLen);
	got = CG_BE_AlLoadU32LE(buf + bodyLen);
	if (expect != got)
	{
		return qfalse;
	}
	memcpy(nameOut, buf + BE_AL_HDR_SIZE, nameLen);
	nameOut[nameLen] = '\0';
	memcpy(tokenOut, buf + BE_AL_HDR_SIZE + nameLen, tokenLen);
	tokenOut[tokenLen] = '\0';
	/* v1 stream had salt=0 */
	CG_BE_AlCrypt((byte *)nameOut, nameLen, 1, nameLen, tokenLen, 0, 0);
	CG_BE_AlCrypt((byte *)tokenOut, tokenLen, 1, nameLen, tokenLen, 0, nameLen);
	return nameOut[0] && tokenOut[0];
}

static qboolean CG_BE_AlRead(char *nameOut, int nameSize, char *tokenOut, int tokenSize)
{
	fileHandle_t f;
	int len;
	byte wire[BE_AL_WIRE_MAX];
	byte inner[BE_AL_INNER_MAX];
	int innerLen;

	if (!nameOut || nameSize <= 0 || !tokenOut || tokenSize <= 0)
	{
		return qfalse;
	}
	nameOut[0] = '\0';
	tokenOut[0] = '\0';

	len = trap_FS_FOpenFile(BE_AL_FILE, &f, FS_READ);
	if (len <= 0 || !f)
	{
		if (f)
		{
			trap_FS_FCloseFile(f);
		}
		return qfalse;
	}
	if (len > (int)sizeof(wire))
	{
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(wire, len, f);
	trap_FS_FCloseFile(f);

	/* v2 wire (all high-bit) */
	innerLen = CG_BE_AlWireDecode(wire, len, inner, sizeof(inner));
	if (innerLen > 0 && CG_BE_AlParseInner(inner, innerLen, nameOut, nameSize, tokenOut, tokenSize))
	{
		return qtrue;
	}

	/* legacy v1 */
	return CG_BE_AlParseLegacyV1(wire, len, nameOut, nameSize, tokenOut, tokenSize);
}

static void CG_BE_AlWrite(const char *name, const char *token, qboolean allowReplace)
{
	fileHandle_t f;
	byte inner[BE_AL_INNER_MAX];
	byte wire[BE_AL_WIRE_MAX];
	char nameEnc[BE_AL_NAME_MAX];
	char tokenEnc[BE_AL_TOKEN_MAX];
	int nameLen;
	int tokenLen;
	int bodyLen;
	int wireLen;
	byte salt;
	unsigned sum;

	if (!name || !name[0] || !token || !token[0])
	{
		return;
	}
	if (!allowReplace && CG_BE_AlFileExists())
	{
		return;
	}

	Q_strncpyz(nameEnc, name, sizeof(nameEnc));
	Q_strncpyz(tokenEnc, token, sizeof(tokenEnc));
	nameLen = (int)strlen(nameEnc);
	tokenLen = (int)strlen(tokenEnc);
	if (nameLen <= 0 || tokenLen <= 0 || nameLen > 255 || tokenLen > 255)
	{
		return;
	}

	salt = (byte)((trap_Milliseconds() ^ (nameLen * 131) ^ (tokenLen * 17) ^ (unsigned)token[0]) & 0xFFu);
	if (salt == 0)
	{
		salt = 0x5Au;
	}

	CG_BE_AlCrypt((byte *)nameEnc, nameLen, BE_AL_VERSION, nameLen, tokenLen, salt, 0);
	CG_BE_AlCrypt((byte *)tokenEnc, tokenLen, BE_AL_VERSION, nameLen, tokenLen, salt, nameLen);

	bodyLen = 1 + BE_AL_HDR_SIZE + nameLen + tokenLen;
	if (bodyLen + 4 > (int)sizeof(inner))
	{
		return;
	}

	inner[0] = salt;
	inner[1] = (byte)(beAlTag[0] ^ salt);
	inner[2] = (byte)(beAlTag[1] ^ salt);
	inner[3] = (byte)(beAlTag[2] ^ salt);
	inner[4] = (byte)(beAlTag[3] ^ salt);
	inner[5] = (byte)BE_AL_VERSION;
	inner[6] = (byte)nameLen;
	inner[7] = (byte)tokenLen;
	inner[8] = (byte)((salt * 0x1Du) ^ 0xA7u); /* decoy / mix */
	memcpy(inner + 1 + BE_AL_HDR_SIZE, nameEnc, nameLen);
	memcpy(inner + 1 + BE_AL_HDR_SIZE + nameLen, tokenEnc, tokenLen);
	sum = CG_BE_AlChecksum(inner, bodyLen);
	CG_BE_AlStoreU32LE(inner + bodyLen, sum);

	wireLen = CG_BE_AlWireEncode(inner, bodyLen + 4, wire, sizeof(wire));
	if (wireLen <= 0)
	{
		return;
	}

	trap_FS_FOpenFile(BE_AL_FILE, &f, FS_WRITE);
	if (!f)
	{
		return;
	}
	trap_FS_Write(wire, wireLen, f);
	trap_FS_FCloseFile(f);
}

static void CG_BE_AlFlushPending(void)
{
	if (!cg_beAlPendingValid)
	{
		return;
	}
	if (!CG_BE_AuthReady())
	{
		return;
	}
	CG_BE_AlWrite(cg_beAlPendingName, cg_beAlPendingToken, cg_beAlPendingRot);
	cg_beAlPendingValid = qfalse;
}

/*
===============
CG_BE_Autologin_ServerCommand

Handles altoken / altoken_rot from engine.
===============
*/
void CG_BE_Autologin_ServerCommand(const char *cmd)
{
	const char *name;
	const char *token;
	qboolean rot;

	if (cg.demoPlayback)
	{
		return;
	}
	if (!cmd)
	{
		return;
	}

	rot = (Q_stricmp(cmd, "altoken_rot") == 0);
	if (!rot && Q_stricmp(cmd, "altoken") != 0)
	{
		return;
	}

	name = CG_Argv(1);
	token = CG_Argv(2);
	if (!name || !name[0] || !token || !token[0])
	{
		return;
	}

	Q_strncpyz(cg_beAlPendingName, name, sizeof(cg_beAlPendingName));
	Q_strncpyz(cg_beAlPendingToken, token, sizeof(cg_beAlPendingToken));
	cg_beAlPendingRot = rot;
	cg_beAlPendingValid = qtrue;

	if (CG_BE_AuthReady())
	{
		CG_BE_AlFlushPending();
	}
}

/*
===============
CG_BE_AutologinFrame

After BE auth delay: flush pending save; if player.be valid, logintoken once.
===============
*/
void CG_BE_AutologinFrame(void)
{
	char name[BE_AL_NAME_MAX];
	char token[BE_AL_TOKEN_MAX];

	if (cg.demoPlayback)
	{
		return;
	}
	if (CG_GetUserInfoProfile() != BE_USERINFO_BE)
	{
		return;
	}
	if (!cgs.beSupportedServer)
	{
		return;
	}
	if (!CG_BE_AuthReady())
	{
		return;
	}

	CG_BE_AlFlushPending();

	if (cg_beAlTried)
	{
		return;
	}
	cg_beAlTried = qtrue;

	if (!CG_BE_AlRead(name, sizeof(name), token, sizeof(token)))
	{
		return;
	}
	trap_SendClientCommand(va("logintoken %s", token));
}
