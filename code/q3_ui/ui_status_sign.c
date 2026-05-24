/*
===========================================================================
ui_status_sign.c — MD5(payload + secret), payload v=;f=;j=;d=
===========================================================================
*/

#include "ui_local.h"
#include "ui_md5.h"
#include "ui_status_sign.h"

#define UI_STATUS_SIGN_XOR          0xA7
#define UI_STATUS_SIGN_SECRET_LEN   22

static const unsigned char ui_statusSignSecretObf[UI_STATUS_SIGN_SECRET_LEN] =
{
	0xc8, 0xd4, 0xd7, 0x95, 0xc5, 0xc2, 0xf8, 0xd4, 0xd3, 0xc6, 0xd3, 0xd2, 0xd4,
	0xf8, 0xc3, 0xc2, 0xd1, 0xf8, 0xc8, 0xc9, 0xcb, 0xde
};

static void UI_StatusSign_DecodeSecret(char* out, int outSize)
{
	int     i;

	if (outSize < UI_STATUS_SIGN_SECRET_LEN + 1)
	{
		if (outSize > 0)
		{
			out[0] = '\0';
		}
		return;
	}

	for (i = 0; i < UI_STATUS_SIGN_SECRET_LEN; i++)
	{
		out[i] = (char)(ui_statusSignSecretObf[i] ^ UI_STATUS_SIGN_XOR);
	}
	out[UI_STATUS_SIGN_SECRET_LEN] = '\0';
}

void UI_StatusSign_CopyInfostringOnly(const char* status, char* out, int outSize)
{
	int     i;

	if (!out || outSize <= 0)
	{
		return;
	}
	out[0] = '\0';
	if (!status)
	{
		return;
	}

	for (i = 0; status[i] && status[i] != '\n' && i < outSize - 1; i++)
	{
		out[i] = status[i];
	}
	out[i] = '\0';
}

void UI_StatusSign_BuildPayload(
	const char* version,
	const char* flags,
	const char* sivj,
	const char* date,
	char* payload,
	int payloadSize)
{
	if (!version)
	{
		version = "";
	}
	if (!flags)
	{
		flags = "";
	}
	if (!sivj)
	{
		sivj = "";
	}
	if (!date)
	{
		date = "";
	}

	Com_sprintf(payload, payloadSize, "v=%s;f=%s;j=%s;d=%s", version, flags, sivj, date);
}

void UI_StatusSign_BuildPayloadFromInfo(const char* info, char* payload, int payloadSize)
{
	char    version[64];
	char    flags[64];
	char    sivj[64];
	char    date[64];

	Q_strncpyz(version, Info_ValueForKey(info, "version"), sizeof(version));
	Q_strncpyz(flags, Info_ValueForKey(info, "flags"), sizeof(flags));
	Q_strncpyz(sivj, Info_ValueForKey(info, "sivj"), sizeof(sivj));
	Q_strncpyz(date, Info_ValueForKey(info, "date"), sizeof(date));

	UI_StatusSign_BuildPayload(version, flags, sivj, date, payload, payloadSize);
}

void UI_StatusSign_ComputeModSig(const char* payload, char* sigHex, int sigHexSize)
{
	char                secret[UI_STATUS_SIGN_SECRET_LEN + 1];
	char                block[256];
	unsigned char       digest[16];
	int                 payloadLen;
	int                 secretLen;
	int                 total;

	if (!payload || !sigHex || sigHexSize < 33)
	{
		if (sigHex && sigHexSize > 0)
		{
			sigHex[0] = '\0';
		}
		return;
	}

	UI_StatusSign_DecodeSecret(secret, sizeof(secret));
	payloadLen = (int)strlen(payload);
	secretLen = (int)strlen(secret);
	total = payloadLen + secretLen;
	if (total >= (int)sizeof(block))
	{
		sigHex[0] = '\0';
		return;
	}

	memcpy(block, payload, payloadLen);
	memcpy(block + payloadLen, secret, secretLen);

	UI_MD5_Buffer((const unsigned char*)block, total, digest);
	UI_MD5_HexLower(digest, sigHex);
}

static qboolean UI_StatusSign_HexEqual(const char* a, const char* b)
{
	int     i;

	if (!a || !b)
	{
		return qfalse;
	}
	for (i = 0; i < 32; i++)
	{
		char    ca;
		char    cb;

		ca = a[i];
		cb = b[i];
		if (ca >= 'A' && ca <= 'F')
		{
			ca = (char)(ca - 'A' + 'a');
		}
		if (cb >= 'A' && cb <= 'F')
		{
			cb = (char)(cb - 'A' + 'a');
		}
		if (ca != cb)
		{
			return qfalse;
		}
	}
	return qtrue;
}

qboolean UI_StatusSign_VerifyInfo(
	const char* info,
	char* payloadOut,
	int payloadSize,
	char* computedSig,
	int computedSigSize)
{
	char            modSig[64];
	char            infostring[MAX_INFO_STRING];
	char            localPayload[128];
	char            localSig[33];
	const char*     parse;

	if (!info || !info[0])
	{
		return qfalse;
	}

	UI_StatusSign_CopyInfostringOnly(info, infostring, sizeof(infostring));
	parse = infostring;
	if (!parse[0])
	{
		return qfalse;
	}

	Q_strncpyz(modSig, Info_ValueForKey(parse, "mod_sig"), sizeof(modSig));
	if (!modSig[0])
	{
		return qfalse;
	}

	if (payloadOut && payloadSize > 0)
	{
		UI_StatusSign_BuildPayloadFromInfo(parse, payloadOut, payloadSize);
	}
	else
	{
		UI_StatusSign_BuildPayloadFromInfo(parse, localPayload, sizeof(localPayload));
		payloadOut = localPayload;
	}

	if (computedSig && computedSigSize >= 33)
	{
		UI_StatusSign_ComputeModSig(payloadOut, computedSig, computedSigSize);
	}
	else
	{
		UI_StatusSign_ComputeModSig(payloadOut, localSig, sizeof(localSig));
		computedSig = localSig;
	}

	if ((int)strlen(modSig) != 32)
	{
		return qfalse;
	}

	if ((int)strlen(computedSig) != 32)
	{
		return qfalse;
	}

	return UI_StatusSign_HexEqual(modSig, computedSig);
}
