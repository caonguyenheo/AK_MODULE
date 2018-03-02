/** Copyright (c) 2015 Cvisionhk */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#include <endian.h>
#if __BYTE_ORDER == __BIG_ENDIAN
#define MD5_WORDS_BIGENDIAN
#endif

#include "md5.h"

#ifndef MD5_WORDS_BIGENDIAN
#define md5_byteReverse(pucBuf, len)	/* Nothing */
#else
void md5_byteReverse(unsigned char *pucBuf, unsigned longs);

#ifndef ASM_MD5
/*
 * Note: this code is harmless on little-endian machines.
 */

void md5_byteReverse(unsigned char *pucBuf, unsigned longs)
    {
    ui32_t ui32Val;
    do
        {
        ui32Val = (ui32_t) ((unsigned) pucBuf[3] << 8 | pucBuf[2]) << 16 |
                  ((unsigned) pucBuf[1] << 8 | pucBuf[0]);
        *(ui32_t *) pucBuf = ui32Val;
        pucBuf += 4;
        }
    while (--longs);
    }
#endif
#endif

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void MD5Init(struct MD5Context *psMd5Ctx)
    {
    psMd5Ctx->buf[0] = 0x67452301;
    psMd5Ctx->buf[1] = 0xefcdab89;
    psMd5Ctx->buf[2] = 0x98badcfe;
    psMd5Ctx->buf[3] = 0x10325476;

    psMd5Ctx->bits[0] = 0;
    psMd5Ctx->bits[1] = 0;
    }

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void MD5Update(struct MD5Context *psMd5Ctx, unsigned char const *buf, unsigned len)
    {
    ui32_t ui32t;

    /* Update bitcount */

    ui32t = psMd5Ctx->bits[0];
    if ((psMd5Ctx->bits[0] = ui32t + ((ui32_t) len << 3)) < ui32t)
        psMd5Ctx->bits[1]++;		/* Carry from low to high */
    psMd5Ctx->bits[1] += len >> 29;

    ui32t = (ui32t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (ui32t)
        {
        unsigned char *p = (unsigned char *) psMd5Ctx->in + ui32t;

        ui32t = 64 - ui32t;
        if (len < ui32t)
            {
            memcpy(p, buf, len);
            return;
            }
        memcpy(p, buf, ui32t);
        md5_byteReverse(psMd5Ctx->in, 16);
        MD5Transform(psMd5Ctx->buf, (ui32_t *) psMd5Ctx->in);
        buf += ui32t;
        len -= ui32t;
        }
    /* Process data in 64-byte chunks */

    while (len >= 64)
        {
        memcpy(psMd5Ctx->in, buf, 64);
        md5_byteReverse(psMd5Ctx->in, 16);
        MD5Transform(psMd5Ctx->buf, (ui32_t *) psMd5Ctx->in);
        buf += 64;
        len -= 64;
        }

    /* Handle any remaining bytes of data. */

    memcpy(psMd5Ctx->in, buf, len);
    }

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void MD5Final(unsigned char digest[16], struct MD5Context *psMd5Ctx)
    {
    unsigned cnt;
    unsigned char *puc;

    /* Compute number of bytes mod 64 */
    cnt = (psMd5Ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    puc = psMd5Ctx->in + cnt;
    *puc++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    cnt = 64 - 1 - cnt;

    /* Pad out to 56 mod 64 */
    if (cnt < 8)
        {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset(puc, 0, cnt);
        md5_byteReverse(psMd5Ctx->in, 16);
        MD5Transform(psMd5Ctx->buf, (ui32_t *) psMd5Ctx->in);

        /* Now fill the next block with 56 bytes */
        memset(psMd5Ctx->in, 0, 56);
        }
    else
        {
        /* Pad block to 56 bytes */
        memset(puc, 0, cnt - 8);
        }
    md5_byteReverse(psMd5Ctx->in, 14);

    /* Append length in bits and transform */
    ((ui32_t *) psMd5Ctx->in)[14] = psMd5Ctx->bits[0];
    ((ui32_t *) psMd5Ctx->in)[15] = psMd5Ctx->bits[1];

    MD5Transform(psMd5Ctx->buf, (ui32_t *) psMd5Ctx->in);
    md5_byteReverse((unsigned char *) psMd5Ctx->buf, 4);
    memcpy(digest, psMd5Ctx->buf, 16);
    memset(psMd5Ctx, 0, sizeof(psMd5Ctx));	/* In case it's sensitive */
    }

#ifndef ASM_MD5

/* The four core functions - MD5_F1 is optimized somewhat */

/* #define MD5_F1(x, y, z) (x & y | ~x & z) */
#define MD5_F1(x, y, z) (z ^ (x & (y ^ z)))
#define MD5_F2(x, y, z) MD5_F1(z, x, y)
#define MD5_F3(x, y, z) (x ^ y ^ z)
#define MD5_F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5_STEP_PROCESS(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void MD5Transform(ui32_t ui32Buf[4], ui32_t const ui32In[16])
    {
    register ui32_t a, b, c, d;

    a = ui32Buf[0];
    b = ui32Buf[1];
    c = ui32Buf[2];
    d = ui32Buf[3];

    MD5_STEP_PROCESS(MD5_F1, a, b, c, d, ui32In[0] + 0xd76aa478, 7);
    MD5_STEP_PROCESS(MD5_F1, d, a, b, c, ui32In[1] + 0xe8c7b756, 12);
    MD5_STEP_PROCESS(MD5_F1, c, d, a, b, ui32In[2] + 0x242070db, 17);
    MD5_STEP_PROCESS(MD5_F1, b, c, d, a, ui32In[3] + 0xc1bdceee, 22);
    MD5_STEP_PROCESS(MD5_F1, a, b, c, d, ui32In[4] + 0xf57c0faf, 7);
    MD5_STEP_PROCESS(MD5_F1, d, a, b, c, ui32In[5] + 0x4787c62a, 12);
    MD5_STEP_PROCESS(MD5_F1, c, d, a, b, ui32In[6] + 0xa8304613, 17);
    MD5_STEP_PROCESS(MD5_F1, b, c, d, a, ui32In[7] + 0xfd469501, 22);
    MD5_STEP_PROCESS(MD5_F1, a, b, c, d, ui32In[8] + 0x698098d8, 7);
    MD5_STEP_PROCESS(MD5_F1, d, a, b, c, ui32In[9] + 0x8b44f7af, 12);
    MD5_STEP_PROCESS(MD5_F1, c, d, a, b, ui32In[10] + 0xffff5bb1, 17);
    MD5_STEP_PROCESS(MD5_F1, b, c, d, a, ui32In[11] + 0x895cd7be, 22);
    MD5_STEP_PROCESS(MD5_F1, a, b, c, d, ui32In[12] + 0x6b901122, 7);
    MD5_STEP_PROCESS(MD5_F1, d, a, b, c, ui32In[13] + 0xfd987193, 12);
    MD5_STEP_PROCESS(MD5_F1, c, d, a, b, ui32In[14] + 0xa679438e, 17);
    MD5_STEP_PROCESS(MD5_F1, b, c, d, a, ui32In[15] + 0x49b40821, 22);

    MD5_STEP_PROCESS(MD5_F2, a, b, c, d, ui32In[1] + 0xf61e2562, 5);
    MD5_STEP_PROCESS(MD5_F2, d, a, b, c, ui32In[6] + 0xc040b340, 9);
    MD5_STEP_PROCESS(MD5_F2, c, d, a, b, ui32In[11] + 0x265e5a51, 14);
    MD5_STEP_PROCESS(MD5_F2, b, c, d, a, ui32In[0] + 0xe9b6c7aa, 20);
    MD5_STEP_PROCESS(MD5_F2, a, b, c, d, ui32In[5] + 0xd62f105d, 5);
    MD5_STEP_PROCESS(MD5_F2, d, a, b, c, ui32In[10] + 0x02441453, 9);
    MD5_STEP_PROCESS(MD5_F2, c, d, a, b, ui32In[15] + 0xd8a1e681, 14);
    MD5_STEP_PROCESS(MD5_F2, b, c, d, a, ui32In[4] + 0xe7d3fbc8, 20);
    MD5_STEP_PROCESS(MD5_F2, a, b, c, d, ui32In[9] + 0x21e1cde6, 5);
    MD5_STEP_PROCESS(MD5_F2, d, a, b, c, ui32In[14] + 0xc33707d6, 9);
    MD5_STEP_PROCESS(MD5_F2, c, d, a, b, ui32In[3] + 0xf4d50d87, 14);
    MD5_STEP_PROCESS(MD5_F2, b, c, d, a, ui32In[8] + 0x455a14ed, 20);
    MD5_STEP_PROCESS(MD5_F2, a, b, c, d, ui32In[13] + 0xa9e3e905, 5);
    MD5_STEP_PROCESS(MD5_F2, d, a, b, c, ui32In[2] + 0xfcefa3f8, 9);
    MD5_STEP_PROCESS(MD5_F2, c, d, a, b, ui32In[7] + 0x676f02d9, 14);
    MD5_STEP_PROCESS(MD5_F2, b, c, d, a, ui32In[12] + 0x8d2a4c8a, 20);

    MD5_STEP_PROCESS(MD5_F3, a, b, c, d, ui32In[5] + 0xfffa3942, 4);
    MD5_STEP_PROCESS(MD5_F3, d, a, b, c, ui32In[8] + 0x8771f681, 11);
    MD5_STEP_PROCESS(MD5_F3, c, d, a, b, ui32In[11] + 0x6d9d6122, 16);
    MD5_STEP_PROCESS(MD5_F3, b, c, d, a, ui32In[14] + 0xfde5380c, 23);
    MD5_STEP_PROCESS(MD5_F3, a, b, c, d, ui32In[1] + 0xa4beea44, 4);
    MD5_STEP_PROCESS(MD5_F3, d, a, b, c, ui32In[4] + 0x4bdecfa9, 11);
    MD5_STEP_PROCESS(MD5_F3, c, d, a, b, ui32In[7] + 0xf6bb4b60, 16);
    MD5_STEP_PROCESS(MD5_F3, b, c, d, a, ui32In[10] + 0xbebfbc70, 23);
    MD5_STEP_PROCESS(MD5_F3, a, b, c, d, ui32In[13] + 0x289b7ec6, 4);
    MD5_STEP_PROCESS(MD5_F3, d, a, b, c, ui32In[0] + 0xeaa127fa, 11);
    MD5_STEP_PROCESS(MD5_F3, c, d, a, b, ui32In[3] + 0xd4ef3085, 16);
    MD5_STEP_PROCESS(MD5_F3, b, c, d, a, ui32In[6] + 0x04881d05, 23);
    MD5_STEP_PROCESS(MD5_F3, a, b, c, d, ui32In[9] + 0xd9d4d039, 4);
    MD5_STEP_PROCESS(MD5_F3, d, a, b, c, ui32In[12] + 0xe6db99e5, 11);
    MD5_STEP_PROCESS(MD5_F3, c, d, a, b, ui32In[15] + 0x1fa27cf8, 16);
    MD5_STEP_PROCESS(MD5_F3, b, c, d, a, ui32In[2] + 0xc4ac5665, 23);

    MD5_STEP_PROCESS(MD5_F4, a, b, c, d, ui32In[0] + 0xf4292244, 6);
    MD5_STEP_PROCESS(MD5_F4, d, a, b, c, ui32In[7] + 0x432aff97, 10);
    MD5_STEP_PROCESS(MD5_F4, c, d, a, b, ui32In[14] + 0xab9423a7, 15);
    MD5_STEP_PROCESS(MD5_F4, b, c, d, a, ui32In[5] + 0xfc93a039, 21);
    MD5_STEP_PROCESS(MD5_F4, a, b, c, d, ui32In[12] + 0x655b59c3, 6);
    MD5_STEP_PROCESS(MD5_F4, d, a, b, c, ui32In[3] + 0x8f0ccc92, 10);
    MD5_STEP_PROCESS(MD5_F4, c, d, a, b, ui32In[10] + 0xffeff47d, 15);
    MD5_STEP_PROCESS(MD5_F4, b, c, d, a, ui32In[1] + 0x85845dd1, 21);
    MD5_STEP_PROCESS(MD5_F4, a, b, c, d, ui32In[8] + 0x6fa87e4f, 6);
    MD5_STEP_PROCESS(MD5_F4, d, a, b, c, ui32In[15] + 0xfe2ce6e0, 10);
    MD5_STEP_PROCESS(MD5_F4, c, d, a, b, ui32In[6] + 0xa3014314, 15);
    MD5_STEP_PROCESS(MD5_F4, b, c, d, a, ui32In[13] + 0x4e0811a1, 21);
    MD5_STEP_PROCESS(MD5_F4, a, b, c, d, ui32In[4] + 0xf7537e82, 6);
    MD5_STEP_PROCESS(MD5_F4, d, a, b, c, ui32In[11] + 0xbd3af235, 10);
    MD5_STEP_PROCESS(MD5_F4, c, d, a, b, ui32In[2] + 0x2ad7d2bb, 15);
    MD5_STEP_PROCESS(MD5_F4, b, c, d, a, ui32In[9] + 0xeb86d391, 21);

    ui32Buf[0] += a;
    ui32Buf[1] += b;
    ui32Buf[2] += c;
    ui32Buf[3] += d;
    }

#endif
