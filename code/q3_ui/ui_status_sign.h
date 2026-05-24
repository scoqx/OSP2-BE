/*
===========================================================================
ui_status_sign.h — mod_sig check (matches q3-status-emulator status_sign.py)
===========================================================================
*/

#ifndef UI_STATUS_SIGN_H
#define UI_STATUS_SIGN_H

void UI_StatusSign_CopyInfostringOnly(const char* status, char* out, int outSize);

void UI_StatusSign_BuildPayload(
	const char* version,
	const char* flags,
	const char* sivj,
	const char* date,
	char* payload,
	int payloadSize);

void UI_StatusSign_BuildPayloadFromInfo(const char* info, char* payload, int payloadSize);

void UI_StatusSign_ComputeModSig(const char* payload, char* sigHex, int sigHexSize);

qboolean UI_StatusSign_VerifyInfo(const char* info, char* payloadOut, int payloadSize, char* computedSig, int computedSigSize);

#endif /* UI_STATUS_SIGN_H */
