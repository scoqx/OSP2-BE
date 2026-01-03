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
// cg_teamtasks_crypto.c -- team task pack/unpack functions

#include "cg_local.h"

/*
 * Generate encryption key from clientID, model and headmodel
 * Uses simple hash combination for key generation
 */
static int CG_GenerateCryptoKey(int clientID)
{
	int key;
	clientInfo_t* ci;
	int headModelHash;
	int i;
	const char* headModelName;

	ci = &cgs.clientinfo[clientID];
	if (!ci->infoValid)
	{
		/* Fallback if client info not available */
		return clientID * 0x4A3B2C1D;
	}

	/* Hash original head model name (not the current one, which may be changed) */
	/* Use model only as a small addition, clientID is the main factor */
	headModelName = ci->originalHeadModel[0] != '\0' ? ci->originalHeadModel : ci->headModelName;
	headModelHash = 0;
	for (i = 0; headModelName[i] != '\0' && i < MAX_QPATH; i++)
	{
		headModelHash = (headModelHash * 31) + (int)headModelName[i];
	}

	/* Combine: clientID is PRIMARY, model hash is only a small addition */
	/* This ensures different clients have different keys even with same model */
	key = clientID;
	key = (key << 16) | ((key >> 16) ^ headModelHash);
	key = key ^ (headModelHash << 8);

	/* Ensure key is non-zero and has good bit distribution */
	if (key == 0)
	{
		key = 0x12345678;
	}

	return key;
}

/*
 * Permutation schemes for data scrambling based on clientID
 * Each scheme rearranges coordinates X, Y, Z in different order
 * toClient and templateID stay in fixed positions for simplicity
 */
static int CG_PermutePackData(int toClient, int templateID, int x, int y, int z, int scheme)
{
	int packed;
	int coord1;
	int coord2;
	int coord3;

	/* Permute coordinates based on scheme */
	/* 6 possible permutations of 3 coordinates */
	switch (scheme & 0x07)
	{
		case 0:  /* X, Y, Z */
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
		case 1:  /* X, Z, Y */
			coord1 = x;
			coord2 = z;
			coord3 = y;
			break;
		case 2:  /* Y, X, Z */
			coord1 = y;
			coord2 = x;
			coord3 = z;
			break;
		case 3:  /* Y, Z, X */
			coord1 = y;
			coord2 = z;
			coord3 = x;
			break;
		case 4:  /* Z, X, Y */
			coord1 = z;
			coord2 = x;
			coord3 = y;
			break;
		case 5:  /* Z, Y, X */
			coord1 = z;
			coord2 = y;
			coord3 = x;
			break;
		case 6:  /* X, Y, Z (same as 0, but different scheme number for variety) */
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
		case 7:  /* X, Z, Y (same as 1, but different scheme number for variety) */
			coord1 = x;
			coord2 = z;
			coord3 = y;
			break;
		default:
			coord1 = x;
			coord2 = y;
			coord3 = z;
			break;
	}

	/* Standard packing: toClient, templateID (5 bits for 0-31), coord1, coord2, coord3 */
	/* Use 5 bits for templateID to support values 0-31 (max enum value) */
	packed = ((toClient & 0x01) << 31) |
	         ((templateID & 0x1F) << 26) |
	         ((coord1 & 0x7F) << 19) |
	         ((coord2 & 0x7F) << 12) |
	         ((coord3 & 0x7F) << 5);

	return packed;
}

/*
 * Extract coordinates from packed data (bits 19, 12, 5)
 */
static void CG_ExtractCoordinates(int packed, int* coord1, int* coord2, int* coord3)
{
	*coord1 = (packed >> 19) & 0x7F;
	*coord2 = (packed >> 12) & 0x7F;
	*coord3 = (packed >> 5) & 0x7F;
}

/*
 * Reverse permutation to extract data
 */
static void CG_PermuteUnpackData(int packed, int scheme, int* toClient, int* templateID, int* x, int* y, int* z)
{
	int coord1;
	int coord2;
	int coord3;

	/* Extract coordinates based on scheme */
	switch (scheme & 0x07)
	{
		case 0:  /* X, Y, Z */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
		case 1:  /* X, Z, Y */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord3;
			*z = coord2;
			break;
		case 2:  /* Y, X, Z */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord2;
			*y = coord1;
			*z = coord3;
			break;
		case 3:  /* Y, Z, X */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord3;
			*y = coord1;
			*z = coord2;
			break;
		case 4:  /* Z, X, Y */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord2;
			*y = coord3;
			*z = coord1;
			break;
		case 5:  /* Z, Y, X */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord3;
			*y = coord2;
			*z = coord1;
			break;
		case 6:  /* X, Y, Z (same as 0) */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
		case 7:  /* X, Z, Y (same as 1) */
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord3;
			*z = coord2;
			break;
		default:
			CG_ExtractCoordinates(packed, &coord1, &coord2, &coord3);
			*x = coord1;
			*y = coord2;
			*z = coord3;
			break;
	}

	/* Standard extraction: toClient, templateID (5 bits for 0-31) */
	*toClient = (packed >> 31) & 0x01;
	*templateID = (packed >> 26) & 0x1F;
}

int CG_OSPTeamTaskPack(int toClient, int templateID,
                       int x, int y, int z)
{
	int packed;
	int cryptoKey;
	int encrypted;
	int senderClientID;
	int senderIDPacked;
	int verificationBit;
	int scheme;

	/* Get sender's client ID */
	senderClientID = cg.clientNum;

	/* Select permutation scheme based on clientID (8 different schemes) */
	/* Use complex non-linear selection to make it unpredictable */
	scheme = ((senderClientID * 17) ^ (senderClientID >> 2) ^ (senderClientID << 3)) & 0x07;

	/* Pack data with permutation */
	packed = CG_PermutePackData(toClient, templateID, x, y, z, scheme);

	/* Generate encryption key from sender's clientID and model info */
	cryptoKey = CG_GenerateCryptoKey(senderClientID);

	/* Apply XOR encryption to data bits (bits 31-5) */
	encrypted = packed & 0xFFFFFFE0;
	encrypted = encrypted ^ (cryptoKey & 0xFFFFFFE0);

	/* Pack senderClientID with cyclic shift +52, keep within 4 bits (0-15) to fit in bits 4-0 */
	senderIDPacked = ((senderClientID + 52) & 0x0F);
	senderIDPacked = senderIDPacked ^ ((cryptoKey >> 5) & 0x0F);

	/* Calculate parity/verification bit from encrypted data and senderID */
	/* This adds extra protection - bit 4 is XOR of key bits and data */
	verificationBit = ((cryptoKey >> 11) ^ (cryptoKey >> 17) ^
	                   (encrypted >> 11) ^ (encrypted >> 17) ^
	                   senderIDPacked) & 0x01;

	/* Combine: bit 4 = verification, bits 3-0 = senderID */
	return encrypted | (verificationBit << 4) | senderIDPacked;
}

void CG_OSPTeamTaskUnpack(int packed, int senderClientID, int* toClient, int* templateID,
                          int* x, int* y, int* z)
{
	int encrypted;
	int senderIDPacked;
	int cryptoKey;
	int decrypted;
	int expectedVerificationBit;

	/* Extract verification bit and obfuscated senderID */
	int verificationBit;
	int scheme;
	verificationBit = (packed >> 4) & 0x01;
	senderIDPacked = packed & 0x0F;

	/* Get encrypted data (bits 31-5) */
	encrypted = packed & 0xFFFFFFE0;

	/* If senderClientID is valid, verify it matches */
	if (senderClientID >= 0 && senderClientID < MAX_CLIENTS && cgs.clientinfo[senderClientID].infoValid)
	{
		int expectedSenderIDPacked;
		cryptoKey = CG_GenerateCryptoKey(senderClientID);

		/* Reconstruct expected senderIDPacked using same logic as packing */
		/* This is the correct way to verify sender, since multiple clientIDs can map to same value after & 0x0F */
		expectedSenderIDPacked = ((senderClientID + 52) & 0x0F);
		expectedSenderIDPacked = expectedSenderIDPacked ^ ((cryptoKey >> 5) & 0x0F);

		/* Verify expected senderIDPacked matches received */
		if (expectedSenderIDPacked != senderIDPacked)
		{
			/* SenderID mismatch - REJECT data */
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}

		/* Verify verification bit matches expected */
		expectedVerificationBit = ((cryptoKey >> 11) ^ (cryptoKey >> 17) ^
		                           (encrypted >> 11) ^ (encrypted >> 17) ^
		                           senderIDPacked) & 0x01;
		if (verificationBit != expectedVerificationBit)
		{
			/* Verification bit mismatch - REJECT data */
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}
	}
	else
	{
		/* Sender was unknown/invalid - try to find correct sender */
		int i;
		int found;
		found = 0;
		for (i = 0; i < MAX_CLIENTS && !found; i++)
		{
			int expectedSenderIDPacked;
			if (!cgs.clientinfo[i].infoValid)
			{
				continue;
			}
			cryptoKey = CG_GenerateCryptoKey(i);

			/* Reconstruct expected senderIDPacked using same logic as packing */
			/* This is the correct way to verify sender, since multiple clientIDs can map to same value after & 0x0F */
			expectedSenderIDPacked = ((i + 52) & 0x0F);
			expectedSenderIDPacked = expectedSenderIDPacked ^ ((cryptoKey >> 5) & 0x0F);

			/* Check if this clientID produces the same senderIDPacked */
			if (expectedSenderIDPacked == senderIDPacked)
			{
				/* Verify verification bit matches */
				int expectedVerificationBit;
				expectedVerificationBit = ((cryptoKey >> 11) ^ (cryptoKey >> 17) ^
				                           (encrypted >> 11) ^ (encrypted >> 17) ^
				                           senderIDPacked) & 0x01;
				if (verificationBit == expectedVerificationBit)
				{
					/* Found correct sender with matching verification */
					senderClientID = i;
					found = 1;
				}
			}
		}

		if (!found)
		{
			/* No valid sender found - REJECT data */
			*toClient = 0;
			*templateID = 0;
			*x = 0;
			*y = 0;
			*z = 0;
			return;
		}
	}

	/* Decrypt data with verified key */
	decrypted = encrypted ^ (cryptoKey & 0xFFFFFFE0);

	/* Select permutation scheme based on clientID (must match packing scheme) */
	scheme = ((senderClientID * 17) ^ (senderClientID >> 2) ^ (senderClientID << 3)) & 0x07;

	/* Extract data using reverse permutation */
	CG_PermuteUnpackData(decrypted, scheme, toClient, templateID, x, y, z);
}


