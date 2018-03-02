/* FILE   : encrypt.h
 * AUTHOR : leon
 * DATE   : Mar 24, 2015
 * DESC   : Contain all handle about encrypt
 */

#ifndef ENCRYPT_H_
#define ENCRYPT_H_
#include <stddef.h> /* for size_t */
#include <stdint.h>
#include "aes_sw.h"

#include "ak_apps_config.h"


#define _AES_KEY_MAX_BYTES_LEN_   16  //bytes
#define _AES_MAX_KEY_SIZE_        128 //bit
#define _AES_IV_SIZE_             16
#define _AES_BLK_SIZE_            16

/******************************************************************
                          TYPE DEFINE
 ******************************************************************/

typedef enum eAESOpType
    {
    eAESEncryptType = 0,
    eAESDecryptType,
    eAESCountType,
    }eAESOpType;

typedef struct tAESContext
    {
    void        *data;
    size_t      size;
    aes_context ctx;
    }tAESContext;

typedef struct tEncryptEngine
    {
    tAESContext cbc[eAESCountType];
    tAESContext ecb[eAESCountType];
    int         isInit;
    uint8_t     iv[_AES_IV_SIZE_];
    //unsigned char     iv[_AES_IV_SIZE_];
    }tEncryptEngine;


/*
 * PURPOSE : Initialize AES encrypt engine
 * INPUT   : [key]     - AES key
 * OUTPUT  : None
 * RETURN  : AES engine pointer
 * DESCRIPT: None
 */
void*
AESEngineInit(void* key);

/*
 * PURPOSE : De-Initialize AES encrypt engine
 * INPUT   : [engine] - AES engine pointer
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: This function does not free engine pointer
 */
void
AESEngineDeInit(void* engine);

/*
 * PURPOSE : Encrypt AES data CBC mode with padding
 * INPUT   : [engine]  - AES engine pointer
 *           [dataIn]  - data need to encrypt
 *           [iSize]   - size of data input
 * OUTPUT  : [oSize]   - size of data encrypted
 * RETURN  : Pointer to encrypted data
 * DESCRIPT: Wrapper function
 */
void*
Encrypt_AES_Padding(void* engine, void* dataIn, uint32_t iSize, uint32_t* oSize);

/*
 * PURPOSE : Decrypt AES data CBC mode with padding
 * INPUT   : [engine]  - AES engine pointer
 *           [dataIn]  - data need to decrypt
 *           [iSize]   - size of data input
 * OUTPUT  : [oSize]   - size of data decrypted
 * RETURN  : Pointer to decrypted data
 * DESCRIPT: Wrapper function
 */
void*
Decrypt_AES_Padding(void* engine, void* dataIn, uint32_t iSize, uint32_t* oSize);

/*
 * PURPOSE : Encrypt AES data ECB mode without padding
 * INPUT   : [engine]  - AES engine pointer
 *           [data]    - data need to encrypt
 *           [size]    - size of data input
 * OUTPUT  : [data]    - Encrypted data
 * RETURN  : None
 * DESCRIPT: Input size must be block of 16
 */
void
Encrypt_AES_NonPadding(void* engine, void* data, uint32_t size);

/*
 * PURPOSE : Decrypt AES data ECB mode without padding
 * INPUT   : [engine]  - AES engine pointer
 *           [data]    - data need to decrypt
 *           [size]    - size of data input
 * OUTPUT  : [data]    - Decrypted data
 * RETURN  : None
 * DESCRIPT: Input size must be block of 16
 */
void
Decrypt_AES_NonPadding(void* engine, void* data, uint32_t size);

/*
 * PURPOSE : Convert a string into hex string
 * INPUT   : [str]  - string want to convert
 *           [size] - length of string
 * OUTPUT  : None
 * RETURN  : Pointer to new string
 * DESCRIPT: User must free returned pointer after use
 */
void*
string2hex(void* str, uint32_t size);

/*
 * PURPOSE : Random string
 * INPUT   : [size] - Length of string
 * OUTPUT  : [str]  - Random string
 * RETURN  : None
 * DESCRIPT: None
 */
void
stringRandom(void* str, uint32_t size);

#endif /* ENCRYPT_H_ */
