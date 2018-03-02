#ifndef MD5_H
#define MD5_H

typedef unsigned int ui32_t;
typedef unsigned char ui8_t;

struct MD5Context
    {
    ui32_t buf[4];
    ui32_t bits[2];
    ui8_t in[64];
    };

void MD5Init(struct MD5Context *psMd5Ctx);
void MD5Update(struct MD5Context *psMd5Ctx, unsigned char const *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(ui32_t buf[4], ui32_t const in[16]);

typedef struct MD5Context MD5_CTX;

#endif /* MD5_H */
