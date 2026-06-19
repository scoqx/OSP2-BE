#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_EMOJI_PATH "gfx/emoji"
#define CG_EMOJI_CONFIG_NAME "gfx/emoji/emoji.cfg"
#define CG_EMOJI_HASH_SIZE 512

typedef struct emojiTable_s emojiTable_t;

struct emojiTable_s
{
	emoji_t emoji;
	emojiTable_t* next;
	emojiTable_t* prev;
};

static  emojiTable_t* emojiTable[CG_EMOJI_HASH_SIZE];
static qboolean emojiLoaded = qfalse;
static qboolean emojiOverlayActive = qfalse;
static int emojiOverlayStartTime = 0;
static int emojiOverlayDuration = 120000;
static int emojiOverlayCols = 1;
static int emojiOverlayRows = 0;
static int emojiOverlayDisplayCount = 0;
static int emojiOverlayTotalCount = 0;
static int emojiOverlayStartIndex = 0;
static int emojiOverlayPage = 1;
static int emojiOverlayTotalPages = 1;
static int emojiOverlayWindowHeight = 0;
static unsigned int emojiOverlayHash = 0u;
static const char* emojiOverlayNames[CG_EMOJI_HASH_SIZE];

static void CG_OSPEmojiTokenToPlainName(const char* token, char* out, int outSize)
{
	int tokenLen;
	int copyLen;

	if (!token || !out || outSize <= 0)
	{
		return;
	}

	tokenLen = strlen(token);
	if (tokenLen >= 3 && token[0] == EMOJI_CHAR && token[tokenLen - 1] == EMOJI_CHAR)
	{
		copyLen = tokenLen - 2;
		if (copyLen >= outSize)
		{
			copyLen = outSize - 1;
		}
		memcpy(out, token + 1, copyLen);
		out[copyLen] = '\0';
	}
	else
	{
		Q_strncpyz(out, token, outSize);
	}
}

static int CG_OSPEmojiOverlayCollectAndSort(void)
{
	int i;
	int j;
	int emojiCount;
	int maxEmojiCount;
	emojiTable_t* node;

	emojiCount = 0;
	maxEmojiCount = sizeof(emojiOverlayNames) / sizeof(emojiOverlayNames[0]);
	for (i = 0; i < CG_EMOJI_HASH_SIZE; ++i)
	{
		for (node = emojiTable[i]; node; node = node->next)
		{
			if (emojiCount >= maxEmojiCount)
			{
				break;
			}
			emojiOverlayNames[emojiCount++] = node->emoji.name;
		}
	}

	for (i = 0; i < emojiCount - 1; ++i)
	{
		for (j = i + 1; j < emojiCount; ++j)
		{
			if (Q_stricmp(emojiOverlayNames[i], emojiOverlayNames[j]) > 0)
			{
				const char* tmp;
				tmp = emojiOverlayNames[i];
				emojiOverlayNames[i] = emojiOverlayNames[j];
				emojiOverlayNames[j] = tmp;
			}
		}
	}

	return emojiCount;
}

static unsigned int CG_OSPEmojiOverlayHash(int startIndex, int emojiCount)
{
	/* FNV-1a 32-bit for canonical sorted emoji names list */
	unsigned int hash;
	int i;
	const unsigned char* p;
	char plainName[64];

	hash = 2166136261u;
	for (i = 0; i < emojiCount; ++i)
	{
		CG_OSPEmojiTokenToPlainName(emojiOverlayNames[startIndex + i], plainName, sizeof(plainName));
		for (p = (const unsigned char*)plainName; *p; ++p)
		{
			hash ^= *p;
			hash *= 16777619u;
		}
		/* Separator to avoid concatenation ambiguity */
		hash ^= (unsigned int)'\n';
		hash *= 16777619u;
	}

	return hash;
}

static void CG_OSPEmojiHashToHex(unsigned int hash, char* out, int outSize)
{
	static const char hex[] = "0123456789ABCDEF";
	int i;

	if (!out || outSize < 9)
	{
		return;
	}

	for (i = 0; i < 8; ++i)
	{
		int shift;
		unsigned int nibble;
		shift = (7 - i) * 4;
		nibble = (hash >> shift) & 0xF;
		out[i] = hex[nibble];
	}
	out[8] = '\0';
}

void CG_OSPEmojiList_f(void)
{
	int argc;
	int requestedPage;
	int emojiCount;
	int pageCount;
	int pageStartIndex;
	int maxPageItems;
	int totalPages;
	int cols;
	int rows;
	const int maxRows = 24;
	const int fixedCols = 4;
	const int defaultMaxPageItems = 80;

	argc = trap_Argc();
	requestedPage = 1;
	if (argc >= 2)
	{
		requestedPage = atoi(CG_Argv(1));
		if (requestedPage < 1)
		{
			requestedPage = 1;
		}
	}

	if (emojiOverlayActive && argc < 2)
	{
		emojiOverlayActive = qfalse;
		return;
	}

	CG_OSPLoadEmoji();

	emojiCount = CG_OSPEmojiOverlayCollectAndSort();
	if (emojiCount <= 0)
	{
		emojiOverlayActive = qfalse;
		return;
	}

	maxPageItems = maxRows * fixedCols;
	if (defaultMaxPageItems < maxPageItems)
	{
		maxPageItems = defaultMaxPageItems;
	}

	totalPages = (emojiCount + maxPageItems - 1) / maxPageItems;
	if (requestedPage > totalPages)
	{
		requestedPage = totalPages;
	}
	if (emojiOverlayActive && argc >= 2 && requestedPage == emojiOverlayPage)
	{
		emojiOverlayActive = qfalse;
		return;
	}

	pageStartIndex = (requestedPage - 1) * maxPageItems;
	pageCount = emojiCount - pageStartIndex;
	if (pageCount > maxPageItems)
	{
		pageCount = maxPageItems;
	}

	cols = fixedCols;
	rows = (pageCount + cols - 1) / cols;

	emojiOverlayDisplayCount = pageCount;
	if (rows > maxRows)
	{
		emojiOverlayDisplayCount = maxRows * cols;
		rows = maxRows;
	}

	emojiOverlayStartIndex = pageStartIndex;
	emojiOverlayTotalCount = emojiCount;
	emojiOverlayPage = requestedPage;
	emojiOverlayTotalPages = totalPages;
	emojiOverlayCols = cols;
	emojiOverlayRows = rows;
	emojiOverlayWindowHeight = 48 + rows * 14;
	emojiOverlayHash = CG_OSPEmojiOverlayHash(0, emojiCount);
	emojiOverlayStartTime = cg.time;
	emojiOverlayActive = qtrue;
}

void CG_OSPDrawEmojiListOverlay(void)
{
	const float x = 18.0f;
	const float y = 72.0f;
	const float w = 604.0f;
	const float contentLeft = x + 8.0f;
	const float lineYStart = y + 24.0f;
	const float lineStep = 14.0f;
	float footerY;
	vec4_t bgColor;
	float colWidth;
	char hashHex[9];
	const int flags = DS_HLEFT | DS_EMOJI | DS_PROPORTIONAL;
	const int flagsRight = DS_HRIGHT | DS_EMOJI | DS_PROPORTIONAL;
	int elapsedMs;
	int remainingMs;
	int remainingSec;
	int row;

	if (!emojiOverlayActive)
	{
		return;
	}

	if (cg.time - emojiOverlayStartTime > emojiOverlayDuration)
	{
		emojiOverlayActive = qfalse;
		return;
	}

	elapsedMs = cg.time - emojiOverlayStartTime;
	remainingMs = emojiOverlayDuration - elapsedMs;
	if (remainingMs < 0)
	{
		remainingMs = 0;
	}
	remainingSec = (remainingMs + 999) / 1000;
	CG_OSPEmojiHashToHex(emojiOverlayHash, hashHex, sizeof(hashHex));
	Vector4Copy(cgs.beEmojiListBgColor, bgColor);
	bgColor[3] = cg_emojiListOpaque.value;

	CG_FillRect(x, y, w, (float)emojiOverlayWindowHeight, bgColor);
	CG_OSPDrawGradientFrame(x, y, w, (float)emojiOverlayWindowHeight, 1, 12, 0.005f, 0.05f, 3);

	CG_OSPDrawStringNew(
	    contentLeft, y + 6.0f,
	    va("^7Emoji list total ^2%d^7. Set (^2%s^7)", emojiOverlayTotalCount, hashHex),
	    colorWhite,
	    colorBlack,
	    8.0f, 12.0f,
	    (int)w,
	    flags,
	    cgFontIndices.emojiList,
	    NULL, NULL, NULL
	);
	CG_OSPDrawStringNew(
	    x + w - 8.0f, y + 6.0f,
	    va("^7Page ^2%d^7/^2%d^7", emojiOverlayPage, emojiOverlayTotalPages),
	    colorWhite,
	    colorBlack,
	    8.0f, 12.0f,
	    (int)(x + w - 8.0f),
	    flagsRight,
	    cgFontIndices.emojiList,
	    NULL, NULL, NULL
	);

	colWidth = (w - 16.0f) / (float)emojiOverlayCols;

	for (row = 0; row < emojiOverlayRows; ++row)
	{
		int col;

		for (col = 0; col < emojiOverlayCols; ++col)
		{
			int index;
			char plainName[48];
			char itemText[96];
			float colX;

			index = row + col * emojiOverlayRows;
			if (index >= emojiOverlayDisplayCount)
			{
				continue;
			}
			index += emojiOverlayStartIndex;

			CG_OSPEmojiTokenToPlainName(emojiOverlayNames[index], plainName, sizeof(plainName));
			Com_sprintf(itemText, sizeof(itemText), "%s ^7- ^2%s^7", emojiOverlayNames[index], plainName);
			colX = contentLeft + col * colWidth;

			CG_OSPDrawStringNew(
			    colX, lineYStart + row * lineStep,
			    itemText,
			    colorWhite,
			    colorBlack,
			    8.0f, 12.0f,
			    (int)colWidth,
			    flags,
			    cgFontIndices.emojiList,
			    NULL, NULL, NULL
			);
		}
	}

	footerY = y + (float)emojiOverlayWindowHeight - 14.0f;
	CG_OSPDrawStringNew(
	    contentLeft, footerY,
	    va("^7Timer: ^2%ds^7 | ^3/emojiList <page>^7 | repeat ^3/emojiList^7 to close",
	       remainingSec),
	    colorWhite,
	    colorBlack,
	    7.0f, 10.0f,
	    (int)w,
	    flags,
	    cgFontIndices.emojiList,
	    NULL, NULL, NULL
	);
}

const emoji_t* CG_OSPGetEmoji(const char* name)
{
	char emoji_name[MAX_STRING_CHARS];
	const char* second_colon;
	int emoji_len;
	emojiTable_t* target;

	// If first character not EMOJI_CHAR it is not emoji
	if (name[0] != EMOJI_CHAR)
	{
		return NULL;
	}

	second_colon = name + 1;
	while (*second_colon && *second_colon != EMOJI_CHAR)
	{
		second_colon++;
	}

	// Not emoji if there is no second colon or no text
	if (*second_colon != EMOJI_CHAR || second_colon - name == 1)
	{
		return NULL;
	}

	emoji_len = second_colon - name + 1;

	// Emoji name too long
	if (emoji_len > MAX_STRING_CHARS - 1)
	{
		return NULL;
	}

	memcpy(emoji_name, name, emoji_len);

	emoji_name[emoji_len] = 0;

	{
		int li;

		for (li = 0; li < emoji_len; li++)
		{
			char c;

			c = emoji_name[li];
			if (c >= 'A' && c <= 'Z')
			{
				emoji_name[li] = c - 'A' + 'a';
			}
		}
	}

	target = emojiTable[Com_GenerateHashValue(emoji_name, CG_EMOJI_HASH_SIZE)];

	while (target && Q_stricmp(emoji_name, target->emoji.name))
	{
		target = target->next;
	}

	return target ? &target->emoji : NULL;
}

void CG_OSPAddEmoji(const char* name)
{
	/* store in the hash table */
	emojiTable_t** node;
	emojiTable_t* new_emoji;
	char shadername[MAX_NAME_LENGTH];

	new_emoji = (emojiTable_t*)Z_Malloc(sizeof(emojiTable_t));
	OSP_MEMORY_CHECK(new_emoji);

	Q_strncpyz(shadername, va("%s/%s", CG_EMOJI_PATH, name), MAX_NAME_LENGTH);
	Q_strncpyz(new_emoji->emoji.name, va("%c%s%c", EMOJI_CHAR, name, EMOJI_CHAR), MAX_NAME_LENGTH);

	Q_strlwr(new_emoji->emoji.name);

	new_emoji->emoji.shader = trap_R_RegisterShader(shadername);
	if (!new_emoji->emoji.shader)
	{
		CG_Printf("Couldn't load emoji token: \"%s\" shader name: \"%s\"\n", name, shadername);
		Z_Free(new_emoji);
		return;
	}

	new_emoji->emoji.len = strlen(new_emoji->emoji.name);

	node = &emojiTable[Com_GenerateHashValue(new_emoji->emoji.name, CG_EMOJI_HASH_SIZE)];
	if (*node == NULL)
	{
		*node = new_emoji;
	}
	else
	{
		//collision
		while ((*node)->next)
		{
			node = &(*node)->next;
		}
		new_emoji->prev = *node;
		(*node)->next = new_emoji;
	}
}

static const char* CG_OSPStringFindIfChar(const char* begin, const char* end, qboolean(*pred)(char string))
{
	while (begin < end)
	{
		if (pred(*begin))
		{
			return begin;
		}
		++begin;
	}
	return end;
}

static const char* CG_OSPStringFindIfNotChar(const char* begin, const char* end, qboolean(*pred)(char string))
{
	while (begin < end)
	{
		if (!pred(*begin))
		{
			return begin;
		}
		++begin;
	}
	return end;
}

static qboolean CG_OSPEmojiIsCharAllowed(char c)
{
	return (c >= 'a' && c <= 'z')
	       || (c >= 'A' && c <= 'Z')
	       || (c >= '0' && c <= '9')
	       ||  c == '_'
	       ||  c == '-'
	       ;

}

void CG_OSPLoadEmoji(void)
{
	char text[16384];
	const char* begin;
	const char* end;
	char emoji_name[MAX_INFO_VALUE];
	fileHandle_t filehandle;
	int file_size;

	if (emojiLoaded)
	{
		return;
	}

	emojiLoaded = qtrue;

	file_size = trap_FS_FOpenFile(CG_EMOJI_CONFIG_NAME, &filehandle, 0);
	if (file_size <= 0)
	{
		CG_Printf("\n*** ERROR: Could not open emoji config \"%s\"\n", CG_EMOJI_CONFIG_NAME);
	}
	else if (file_size >= 16384)
	{
		CG_Printf("\n*** Emoji config file \"%s\" too big! (%d bytes, max = %d)\n", CG_EMOJI_CONFIG_NAME, file_size, 16383);
		trap_FS_FCloseFile(filehandle);
	}
	else if (filehandle)
	{
		trap_FS_Read(text, file_size, filehandle);
		trap_FS_FCloseFile(filehandle);
		/* Split file content to strings */
		begin = &text[0];
		end = &text[file_size];
		while (begin < end)
		{
			const char* name_begin;
			const char* name_end;
			int name_size;

			name_begin = CG_OSPStringFindIfChar(begin, end, CG_OSPEmojiIsCharAllowed);
			name_end = CG_OSPStringFindIfNotChar(name_begin, end, CG_OSPEmojiIsCharAllowed);

			name_size = name_end - name_begin;
			if (name_size > 0)
			{
				memcpy(emoji_name, name_begin, name_size);
				emoji_name[name_size] = 0;
				CG_OSPAddEmoji(emoji_name);
			}
			begin = name_end;
		}
	}
}

#define CG_EMOJI_SORTED_MAX 512

static const emoji_t* emojiSorted[CG_EMOJI_SORTED_MAX];
static int emojiSortedCount;
static qboolean emojiSortedBuilt;

static void CG_OSPEmojiBuildSortedList(void)
{
	int i;
	int j;
	emojiTable_t* node;

	if (emojiSortedBuilt)
	{
		return;
	}

	emojiSortedBuilt = qtrue;
	emojiSortedCount = 0;

	for (i = 0; i < CG_EMOJI_HASH_SIZE; ++i)
	{
		for (node = emojiTable[i]; node; node = node->next)
		{
			if (emojiSortedCount >= CG_EMOJI_SORTED_MAX)
			{
				return;
			}
			emojiSorted[emojiSortedCount++] = &node->emoji;
		}
	}

	for (i = 0; i < emojiSortedCount - 1; ++i)
	{
		for (j = i + 1; j < emojiSortedCount; ++j)
		{
			if (Q_stricmp(emojiSorted[i]->name, emojiSorted[j]->name) > 0)
			{
				const emoji_t* tmp;
				tmp = emojiSorted[i];
				emojiSorted[i] = emojiSorted[j];
				emojiSorted[j] = tmp;
			}
		}
	}
}

int CG_OSPEmojiSortedCount(void)
{
	if (!emojiLoaded)
	{
		CG_OSPLoadEmoji();
	}
	CG_OSPEmojiBuildSortedList();
	return emojiSortedCount;
}

const emoji_t* CG_OSPEmojiSortedAt(int index)
{
	if (index < 0 || index >= CG_OSPEmojiSortedCount())
	{
		return NULL;
	}
	return emojiSorted[index];
}

int CG_OSPEmojiPrefixMatch(const char* plainPrefix, const emoji_t** outList, int maxOut)
{
	int i;
	int count;
	int prefixLen;
	char plain[64];
	const emoji_t* em;

	if (!outList || maxOut < 1)
	{
		return 0;
	}

	count = 0;
	prefixLen = plainPrefix ? strlen(plainPrefix) : 0;

	for (i = 0; i < CG_OSPEmojiSortedCount() && count < maxOut; ++i)
	{
		em = emojiSorted[i];
		CG_OSPEmojiTokenToPlainName(em->name, plain, sizeof(plain));
		if (prefixLen > 0 && Q_stricmpn(plain, plainPrefix, prefixLen))
		{
			continue;
		}
		outList[count++] = em;
	}

	return count;
}



