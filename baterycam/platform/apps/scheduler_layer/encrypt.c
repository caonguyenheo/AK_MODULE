/*
* Copyright (C) 2014 NxComm Pte. Ltd.
*
* This unpublished material is proprietary to NxComm.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission
* of NxComm.
*/

/* FILE   : encrypt.c
 * AUTHOR : leon
 * DATE   : Mar 24, 2015
 * DESC   : Contain all handle about encrypt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encrypt.h"
#include "ak_apps_config.h"

/******************************************************************
                          STATIC VARIABLE
 ******************************************************************/
static char                 encryptCBCMode[16] = {'c', 'b', 'c', 0};
static char                 encryptECBMode[16] = {'e', 'c', 'b', 0};

/******************************************************************
                          STATIC FUNCTION
 ******************************************************************/


/******************************************************************
                          PUBLIC FUNCTION
 ******************************************************************/

/*
 * PURPOSE : Initialize AES encrypt engine
 * INPUT   : [key]     - AES key
 * OUTPUT  : None
 * RETURN  : AES engine pointer
 * DESCRIPT: None
 */
void*
AESEngineInit(void* key)
    {
    int             i = 0;
    int             ret = 0;
    tEncryptEngine  *engine = NULL;

    printf("%s entered!\n", __func__);
    if(key == NULL){
        printf("key null!\n");
        return NULL;
    }
    // engine = (tEncryptEngine*)calloc(1, sizeof(tEncryptEngine));
    engine = (tEncryptEngine*)malloc(sizeof(tEncryptEngine));
    printf("allocate engine! 0x%p\n", engine);
    if(engine != NULL)
        {
        for(i = 0; i < eAESCountType; i++)
            {
            /* Init for mode CBC */
            ret = aes_set_key(&(engine->cbc[i].ctx), key, _AES_MAX_KEY_SIZE_, encryptCBCMode);
            engine->cbc[i].size = 1024;
            // engine->cbc[i].data = (char*)calloc(1, sizeof(char) * engine->cbc[i].size);
            engine->cbc[i].data = (char*)malloc(sizeof(char) *engine->cbc[i].size);
            memset(engine->cbc[i].data, 0x00, sizeof(char) *engine->cbc[i].size);
            if(ret)
                {
                engine->isInit = 0;
                free(engine);
                printf("Test point 1 return NULL!\n");
                return NULL;
                }

            /* Init for mode ECB */
            ret = aes_set_key(&(engine->ecb[i].ctx), key, _AES_MAX_KEY_SIZE_, encryptECBMode);
            engine->ecb[i].size = 32;
            // engine->ecb[i].data = (char*)calloc(1, sizeof(char) * engine->cbc[i].size);
            engine->ecb[i].data = (char*)malloc(sizeof(char) * engine->ecb[i].size);
            memset(engine->ecb[i].data, 0x00, sizeof(char) * engine->ecb[i].size);
            if(ret)
                {
                engine->isInit = 0;
                free(engine);
                return NULL;
                }
            }
        for(i = 0; i < _AES_IV_SIZE_; i++)
            engine->iv[i] = 0;
        engine->isInit = 1;
        }
    return engine;
    }

/*
 * PURPOSE : De-Initialize AES encrypt engine
 * INPUT   : [engine] - AES engine pointer
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: This function does not free engine pointer
 */
void
AESEngineDeInit(void* engine)
    {
    int             i = 0;
    tEncryptEngine *pEngine = NULL;

    if(engine == NULL)
        return;
    pEngine = (tEncryptEngine*)engine;
    for(i = 0; i < eAESCountType; i++)
        {
        /* Deinit for mode CBC */
        if(pEngine->cbc[i].data)
            {
            free(pEngine->cbc[i].data);
            pEngine->cbc[i].data = NULL;
            }
        /* Deinit for mode ECB */
        if(pEngine->ecb[i].data)
            {
            free(pEngine->ecb[i].data);
            pEngine->ecb[i].data = NULL;
            }
        }
    }

/*
 * PURPOSE : Random string
 * INPUT   : [size] - Length of string
 * OUTPUT  : [str]  - Random string
 * RETURN  : None
 * DESCRIPT: None
 */
void
stringRandom(void* str, uint32_t size)
    {
    int     i = 0;
    uint8_t *data = (uint8_t*)str;

    if(str == NULL)
        return;
    else
        data = (uint8_t*)str;

    for(i = 0; i < size; i++)
        //data[i] = 35 + random()%91; // for linux
        data[i] = 35 + rand()%91;       // for RTOS
    data[i] = '\0';
    }

/*
 * PURPOSE : Convert a string into hex string
 * INPUT   : [str]  - string want to convert
 *           [size] - length of string
 * OUTPUT  : None
 * RETURN  : Pointer to new string
 * DESCRIPT: User must free returned pointer after use
 */
void*
string2hex(void* str, uint32_t size)
    {
    int     i;
    char    *hexStr = NULL;
    char    *data = NULL;

    if(str == NULL)
        return NULL;
    else
        data = (char*)str;

    // hexStr = (char*)calloc(1, sizeof(char)* size * 2 + 1);
    hexStr = (char*)malloc( sizeof(char)* size * 2 + 1);
    if(hexStr == NULL)
        return NULL;

    for(i = 0; i < size; i++)
        sprintf(hexStr + i*2, "%02x", data[i]);
    return hexStr;
    }

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
Encrypt_AES_Padding(void* engine, void* dataIn, uint32_t iSize, uint32_t* oSize)
    {
    tEncryptEngine *pEngine = NULL;

    /* Check input params */
    if(engine == NULL || dataIn == NULL)
        {
        *oSize = 0;
        return NULL;
        }
    pEngine = (tEncryptEngine*)engine;

    /* Compute size */
    if((iSize % _AES_BLK_SIZE_) != 0)
        (*oSize) =  ((iSize / 16) + 1) * 16;
    else
        (*oSize) = iSize + 16;

    /* Prepare buffer  */
    if(pEngine->cbc[eAESEncryptType].size < (*oSize))
        {
        pEngine->cbc[eAESEncryptType].data = (void*)realloc(pEngine->cbc[eAESEncryptType].data, sizeof(uint8_t) * (*oSize + 16));
        if(pEngine->cbc[eAESEncryptType].data != NULL)
            pEngine->cbc[eAESEncryptType].size = (*oSize);
        }

    /* Encrypt */
    if(pEngine->cbc[eAESEncryptType].data != NULL)
        {
        memset(pEngine->cbc[eAESEncryptType].data, 0, pEngine->cbc[eAESEncryptType].size);
        memcpy(pEngine->cbc[eAESEncryptType].data, dataIn, iSize);
        aes_encrypt(&(pEngine->cbc[eAESEncryptType].ctx), pEngine->iv, pEngine->cbc[eAESEncryptType].data, *(oSize));
        }
    return pEngine->cbc[eAESEncryptType].data;
    }

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
Decrypt_AES_Padding(void* engine, void* dataIn, uint32_t iSize, uint32_t* oSize)
    {
    tEncryptEngine *pEngine = NULL;

    /* Check input params */
    if(engine == NULL || dataIn == NULL)
        {
        *oSize = 0;
        return NULL;
        }
    pEngine = (tEncryptEngine*)engine;

    /* Compute size */
    if((iSize % _AES_BLK_SIZE_) != 0)
        (*oSize) =  ((iSize / 16) + 1) * 16;
    else
        (*oSize) = iSize;

    /* Prepare buffer  */
    if(pEngine->cbc[eAESDecryptType].size < (*oSize))
        {
        pEngine->cbc[eAESDecryptType].data = (void*)realloc(pEngine->cbc[eAESDecryptType].data, sizeof(uint8_t) * (*oSize + 16));
        if(pEngine->cbc[eAESDecryptType].data != NULL)
            pEngine->cbc[eAESDecryptType].size = (*oSize);
        }

    /* Encrypt */
    if(pEngine->cbc[eAESDecryptType].data != NULL)
        {
        memset(pEngine->cbc[eAESDecryptType].data, 0, pEngine->cbc[eAESDecryptType].size);
        memcpy(pEngine->cbc[eAESDecryptType].data, dataIn, iSize);
        aes_decrypt(&(pEngine->cbc[eAESDecryptType].ctx), pEngine->iv, pEngine->cbc[eAESDecryptType].data, *(oSize));
        }
    return pEngine->cbc[eAESDecryptType].data;
    }

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
Encrypt_AES_NonPadding(void* engine, void* data, uint32_t size)
    {
    tEncryptEngine *pEngine = NULL;

    /* Check input params */
    if(engine == NULL || data == NULL) {
        printf(" Encrypt_AES_NonPadding      1\n");
        return;
    }
    if(size % _AES_BLK_SIZE_ != 0) {
        printf(" Encrypt_AES_NonPadding      2\n");
        return;
    }

    /* Encrypt */
    pEngine = (tEncryptEngine*)engine;
    if(size == 0 || size > pEngine->ecb[eAESEncryptType].size) {
        printf(" Encrypt_AES_NonPadding      3: size=%d, ecb.size=%d\n", size, pEngine->ecb[eAESEncryptType].size);
        return;
    }
    aes_encrypt(&(pEngine->ecb[eAESEncryptType].ctx), pEngine->iv, data, size);
    }

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
Decrypt_AES_NonPadding(void* engine, void* data, uint32_t size)
    {
    tEncryptEngine *pEngine = NULL;

    /* Check input params */
    if(engine == NULL || data == NULL)
        return;
    if(size % _AES_BLK_SIZE_ != 0)
        return;

    /* Encrypt */
    pEngine = (tEncryptEngine*)engine;
    if(size == 0 || size > pEngine->ecb[eAESEncryptType].size)
        return;
    aes_decrypt(&(pEngine->ecb[eAESDecryptType].ctx), pEngine->iv, data, size);
    }
