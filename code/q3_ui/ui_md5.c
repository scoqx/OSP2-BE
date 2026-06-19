/*
===========================================================================
ui_md5.c — RFC 1321 MD5 (C89)
===========================================================================
*/

#include "ui_local.h"
#include "ui_md5.h"

typedef struct
{
	unsigned int    state[4];
	unsigned int    count[2];
	unsigned char   buffer[64];
} ui_md5_ctx_t;

static void UI_MD5_Transform(unsigned int state[4], const unsigned char block[64]);

static void UI_MD5_Encode(unsigned char* output, const unsigned int* input, unsigned int len)
{
	unsigned int    i;
	unsigned int    j;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

static void UI_MD5_Decode(unsigned int* output, const unsigned char* input, unsigned int len)
{
	unsigned int    i;
	unsigned int    j;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[i] = ((unsigned int)input[j]) |
		            (((unsigned int)input[j + 1]) << 8) |
		            (((unsigned int)input[j + 2]) << 16) |
		            (((unsigned int)input[j + 3]) << 24);
	}
}

static void UI_MD5_InitCtx(ui_md5_ctx_t* ctx)
{
	ctx->count[0] = 0;
	ctx->count[1] = 0;
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
}

static void UI_MD5_UpdateCtx(ui_md5_ctx_t* ctx, const unsigned char* input, unsigned int inputLen)
{
	unsigned int    i;
	unsigned int    index;
	unsigned int    partLen;

	index = (unsigned int)((ctx->count[0] >> 3) & 0x3f);

	if ((ctx->count[0] += ((unsigned int)inputLen << 3)) < ((unsigned int)inputLen << 3))
	{
		ctx->count[1]++;
	}
	ctx->count[1] += ((unsigned int)inputLen >> 29);

	partLen = 64 - index;

	if (inputLen >= partLen)
	{
		memcpy(&ctx->buffer[index], input, partLen);
		UI_MD5_Transform(ctx->state, ctx->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64)
		{
			UI_MD5_Transform(ctx->state, &input[i]);
		}

		index = 0;
	}
	else
	{
		i = 0;
	}

	memcpy(&ctx->buffer[index], &input[i], inputLen - i);
}

static unsigned char ui_md5Padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void UI_MD5_FinalCtx(unsigned char digest[16], ui_md5_ctx_t* ctx)
{
	unsigned char   bits[8];
	unsigned int    index;
	unsigned int    padLen;

	UI_MD5_Encode(bits, ctx->count, 8);

	index = (unsigned int)((ctx->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	UI_MD5_UpdateCtx(ctx, ui_md5Padding, padLen);
	UI_MD5_UpdateCtx(ctx, bits, 8);
	UI_MD5_Encode(digest, ctx->state, 16);
}

#define UI_MD5_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define UI_MD5_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define UI_MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define UI_MD5_I(x, y, z) ((y) ^ ((x) | (~z)))
#define UI_MD5_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define UI_MD5_FF(a, b, c, d, x, s, ac) \
	{ (a) += UI_MD5_F((b), (c), (d)) + (x) + (unsigned int)(ac); \
	  (a) = UI_MD5_ROTATE_LEFT((a), (s)); (a) += (b); }
#define UI_MD5_GG(a, b, c, d, x, s, ac) \
	{ (a) += UI_MD5_G((b), (c), (d)) + (x) + (unsigned int)(ac); \
	  (a) = UI_MD5_ROTATE_LEFT((a), (s)); (a) += (b); }
#define UI_MD5_HH(a, b, c, d, x, s, ac) \
	{ (a) += UI_MD5_H((b), (c), (d)) + (x) + (unsigned int)(ac); \
	  (a) = UI_MD5_ROTATE_LEFT((a), (s)); (a) += (b); }
#define UI_MD5_II(a, b, c, d, x, s, ac) \
	{ (a) += UI_MD5_I((b), (c), (d)) + (x) + (unsigned int)(ac); \
	  (a) = UI_MD5_ROTATE_LEFT((a), (s)); (a) += (b); }

static void UI_MD5_Transform(unsigned int state[4], const unsigned char block[64])
{
	unsigned int    a;
	unsigned int    b;
	unsigned int    c;
	unsigned int    d;
	unsigned int    x[16];

	UI_MD5_Decode(x, block, 64);

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	UI_MD5_FF(a, b, c, d, x[0], 7, 0xd76aa478);
	UI_MD5_FF(d, a, b, c, x[1], 12, 0xe8c7b756);
	UI_MD5_FF(c, d, a, b, x[2], 17, 0x242070db);
	UI_MD5_FF(b, c, d, a, x[3], 22, 0xc1bdceee);
	UI_MD5_FF(a, b, c, d, x[4], 7, 0xf57c0faf);
	UI_MD5_FF(d, a, b, c, x[5], 12, 0x4787c62a);
	UI_MD5_FF(c, d, a, b, x[6], 17, 0xa8304613);
	UI_MD5_FF(b, c, d, a, x[7], 22, 0xfd469501);
	UI_MD5_FF(a, b, c, d, x[8], 7, 0x698098d8);
	UI_MD5_FF(d, a, b, c, x[9], 12, 0x8b44f7af);
	UI_MD5_FF(c, d, a, b, x[10], 17, 0xffff5bb1);
	UI_MD5_FF(b, c, d, a, x[11], 22, 0x895cd7be);
	UI_MD5_FF(a, b, c, d, x[12], 7, 0x6b901122);
	UI_MD5_FF(d, a, b, c, x[13], 12, 0xfd987193);
	UI_MD5_FF(c, d, a, b, x[14], 17, 0xa679438e);
	UI_MD5_FF(b, c, d, a, x[15], 22, 0x49b40821);

	UI_MD5_GG(a, b, c, d, x[1], 5, 0xf61e2562);
	UI_MD5_GG(d, a, b, c, x[6], 9, 0xc040b340);
	UI_MD5_GG(c, d, a, b, x[11], 14, 0x265e5a51);
	UI_MD5_GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
	UI_MD5_GG(a, b, c, d, x[5], 5, 0xd62f105d);
	UI_MD5_GG(d, a, b, c, x[10], 9, 0x02441453);
	UI_MD5_GG(c, d, a, b, x[15], 14, 0xd8a1e681);
	UI_MD5_GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
	UI_MD5_GG(a, b, c, d, x[9], 5, 0x21e1cde6);
	UI_MD5_GG(d, a, b, c, x[14], 9, 0xc33707d6);
	UI_MD5_GG(c, d, a, b, x[3], 14, 0xf4d50d87);
	UI_MD5_GG(b, c, d, a, x[8], 20, 0x455a14ed);
	UI_MD5_GG(a, b, c, d, x[13], 5, 0xa9e3e905);
	UI_MD5_GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
	UI_MD5_GG(c, d, a, b, x[7], 14, 0x676f02d9);
	UI_MD5_GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

	UI_MD5_HH(a, b, c, d, x[5], 4, 0xfffa3942);
	UI_MD5_HH(d, a, b, c, x[8], 11, 0x8771f681);
	UI_MD5_HH(c, d, a, b, x[11], 16, 0x6d9d6122);
	UI_MD5_HH(b, c, d, a, x[14], 23, 0xfde5380c);
	UI_MD5_HH(a, b, c, d, x[1], 4, 0xa4beea44);
	UI_MD5_HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
	UI_MD5_HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
	UI_MD5_HH(b, c, d, a, x[10], 23, 0xbebfbc70);
	UI_MD5_HH(a, b, c, d, x[13], 4, 0x289b7ec6);
	UI_MD5_HH(d, a, b, c, x[0], 11, 0xeaa127fa);
	UI_MD5_HH(c, d, a, b, x[3], 16, 0xd4ef3085);
	UI_MD5_HH(b, c, d, a, x[6], 23, 0x04881d05);
	UI_MD5_HH(a, b, c, d, x[9], 4, 0xd9d4d039);
	UI_MD5_HH(d, a, b, c, x[12], 11, 0xe6db99e5);
	UI_MD5_HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
	UI_MD5_HH(b, c, d, a, x[2], 23, 0xc4ac5665);

	UI_MD5_II(a, b, c, d, x[0], 6, 0xf4292244);
	UI_MD5_II(d, a, b, c, x[7], 10, 0x432aff97);
	UI_MD5_II(c, d, a, b, x[14], 15, 0xab9423a7);
	UI_MD5_II(b, c, d, a, x[5], 21, 0xfc93a039);
	UI_MD5_II(a, b, c, d, x[12], 6, 0x655b59c3);
	UI_MD5_II(d, a, b, c, x[3], 10, 0x8f0ccc92);
	UI_MD5_II(c, d, a, b, x[10], 15, 0xffeff47d);
	UI_MD5_II(b, c, d, a, x[1], 21, 0x85845dd1);
	UI_MD5_II(a, b, c, d, x[8], 6, 0x6fa87e4f);
	UI_MD5_II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
	UI_MD5_II(c, d, a, b, x[6], 15, 0xa3014314);
	UI_MD5_II(b, c, d, a, x[13], 21, 0x4e0811a1);
	UI_MD5_II(a, b, c, d, x[4], 6, 0xf7537e82);
	UI_MD5_II(d, a, b, c, x[11], 10, 0xbd3af235);
	UI_MD5_II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
	UI_MD5_II(b, c, d, a, x[9], 21, 0xeb86d391);

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}

void UI_MD5_Buffer(const unsigned char* data, int len, unsigned char digest[16])
{
	ui_md5_ctx_t    ctx;

	UI_MD5_InitCtx(&ctx);
	if (data && len > 0)
	{
		UI_MD5_UpdateCtx(&ctx, data, (unsigned int)len);
	}
	UI_MD5_FinalCtx(digest, &ctx);
}

void UI_MD5_HexLower(const unsigned char digest[16], char* hexOut)
{
	static const char hex[] = "0123456789abcdef";
	int             i;

	for (i = 0; i < 16; i++)
	{
		hexOut[i * 2] = hex[(digest[i] >> 4) & 0xf];
		hexOut[i * 2 + 1] = hex[digest[i] & 0xf];
	}
	hexOut[32] = '\0';
}
