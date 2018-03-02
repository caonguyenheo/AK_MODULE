/**
  ******************************************************************************
  * File Name          : fwupgrade.c
  * Description        :
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 CINATIC LTD COMPANY
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <string.h>
#include "fwupgrade.h"

#include "ak_apps_config.h"
#include "drv_gpio.h"
#include "ak_apps_config.h"
#include "command.h"
#include "sys_common.h"
#include "ak_common.h"
#include "ak_partition.h"
#include "kernel.h"
#include "md5.h"
#include "main_ctrl.h"
#include "spi_transfer.h"

#include "ak_drv_wdt.h"

// #include "version.h"
/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define fwupg_swap_byte_16(x) \
((unsigned int)((((unsigned int)(x) & 0x00ff) <<  8) | \
                (((unsigned int)(x) & 0xff00) >>  8)))


#define fwupg_swap_byte_32(x) \
((unsigned int)((((unsigned int)(x) & 0x000000ffUL) << 24) | \
         (((unsigned int)(x) & 0x0000ff00UL) <<  8) | \
         (((unsigned int)(x) & 0x00ff0000UL) >>  8) | \
         (((unsigned int)(x) & 0xff000000UL) >> 24)))


/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
int fwupgrade_read_partition(char *pcNamePartition, char *pBuf, unsigned long *pLen)
{
    int iRet = 0;
    void *handle =NULL;
    unsigned long ulSizeParti = 0;
    if(pcNamePartition == NULL|| pBuf == NULL || pLen == NULL)
    {
        ak_print_error_ex("Parameter is NULL!");
        return -1;
    }
    ak_print_normal("%s.\r\n", ak_partition_get_version());
    ak_print_normal_ex("%s\r\n", pcNamePartition);
    handle = ak_partition_open((const char *)pcNamePartition);
    if(handle == NULL)
	{
		ak_print_error_ex("write parti open err!\n");
        iRet = -2;
	}
    else
    {
        // get size of partition
        ulSizeParti = ak_partition_get_dat_size(handle);
        ak_print_normal("ulSizeParti: %u\n", ulSizeParti);
        if(ulSizeParti > 0)
        {
            iRet = ak_partition_read(handle, pBuf, ulSizeParti);
            if(iRet != ulSizeParti)
            {
                ak_print_error_ex("Reading partition was failed! (ret: %d)", iRet);
                iRet = -4;
            }
            else
            {
                *pLen = ulSizeParti;
                ak_print_normal_ex("Read partition %s OK, ret = %d, len: %u\n", pcNamePartition, iRet, ulSizeParti);
                iRet = 0; // it means Read OK
            }
        }
        else
        {
            ak_print_error_ex("Get size partition failed!\n");
            iRet = -3;
        }
        ak_print_normal("Close partition : %s\r\n", pcNamePartition);
        ak_partition_close(handle);
    }
    return iRet;
}

int fwupgrade_write_partition(char *pcNamePartition, char *pBuf, unsigned long ulLen)
{
    int iRet = 0;
    void *handle =NULL;

    if(pcNamePartition == NULL|| pBuf == NULL)
    {
        ak_print_error_ex("Parameter is NULL!");
        return -1;
    }
    if(ulLen <= 0)
    {
        ak_print_error_ex("Length data <= 0!");
        return -1;
    }
    else
    {
        ak_print_normal("Prepare write: %d bytes!\n", ulLen);
    }
    handle = ak_partition_open(pcNamePartition);
    if(handle == NULL)
	{
		ak_print_error("\nwrite parti open err!\n");
        iRet = -2;
	}
    else
    {
        iRet = ak_partition_write(handle, pBuf, ulLen);
        ak_print_normal_ex("Partition write with ret %d\n", iRet);
        if(iRet < -1)
		{
			ak_print_error_ex("\n update err!\n");
			iRet = -3;
        }
        else
        {
            ak_print_normal_ex("partition write OK!\n");
            iRet = 0;
        }
        ak_partition_close(handle);
    }
    return iRet;
}

int fwupgrade_finish_update(void)
{
    char acUpFlagData[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x44, 0x6f, 0x6f, \
                             0x72, 0x62, 0x65, 0x6c, 0x6c, 0x30, 0x30, 0x32};
    // const char acPartiName[] = "UPFLAG";
    char acPartiName[] = "UPFLAG";
    int iRet = 0;

    iRet = fwupgrade_write_partition(acPartiName, acUpFlagData, 16);
    if(iRet != 0)
    {
        ak_print_error_ex("Write partition %s failed (%d)!", acPartiName, iRet);
    }
    return iRet;
}

/*
* idea check partition upflag, if it is not exist , don't allow upgrade
* return 0 if UPFLAG exist
* return -1 if UPFLAG is not exist
*/
int fwupgrade_check_model(void)
{
    int iRet = 0;
    void *handle = NULL;
    char acPartiName[] = "UPFLAG";
   
    handle = ak_partition_open(acPartiName);
    if(handle == NULL)
	{
		ak_print_error("\nOpen upflag failed!\n");
        iRet = -1;
	}
    else
    {
        ak_partition_close(handle);
    }
    return iRet;
}

int fwupgrade_erase_upflag(void)
{
    int iRet = 0;
    char acUpFlagData[16] = {0x00, };
    char acPartiName[] = "UPFLAG";

    memset(acUpFlagData, 0x01, 16);

    iRet = fwupgrade_write_partition(acPartiName, acUpFlagData, 16);
    if(iRet != 0)
    {
        ak_print_error_ex("Write partition %s failed (%d)!", acPartiName, iRet);
    }
    return iRet;
}

int fwupgrade_burn_image_kernel(char *bin, unsigned int length)
{
    int iRet = 0;
    char acPartiName[] = "KERNEL";

    // TODO: check size of partition before burning image
    unsigned long ulSizeParti = 0;

    iRet = fwupgrade_write_partition(acPartiName, bin, length);
    if(iRet != 0)
    {
        ak_print_error_ex("Writing partition %s failed! (%d)", acPartiName, iRet);
    }
    return iRet;
}

int fwupgrade_kernel_ota(char *bin, unsigned int length)
{
    int iRet = 0;
    if(bin == NULL || length <= 0)
    {
        return -1;
    }
    // erase upflag
    iRet = fwupgrade_erase_upflag();
    // burn
    if(iRet != 0)
    {
        printf("%s Erase UPFLAG failed!\n", __func__);
        iRet = -2;
    }
    else
    {
        iRet = fwupgrade_burn_image_kernel(bin, length);
        if(iRet != 0)
        {
            printf("%s burn failed!\n", __func__);
            iRet = -3;
        }
        else
        {
            // finish update upflag
            iRet = fwupgrade_finish_update();
            if(iRet != 0)
            {
                printf("%s burn finish but upflag failed!\n", __func__);
                iRet = -4;
            }
        }
    }
    return iRet;
}


void fwupgrade_main(void)
{
    int iRet = 0;
    // const char acPartiName[] = "KERNBK";
    char acPartiName[] = "KERNEL";
    char *pcBuffer = NULL;
    unsigned long ulLength = 0;

    pcBuffer = (char *)malloc(FWUPGRADE_SIZE_BUF_READ);
    if(pcBuffer == NULL)
    {
        ak_print_error_ex("Cannot malloc buffer read partition!\n");
        return;
    }
    iRet = fwupgrade_read_partition(acPartiName, pcBuffer, &ulLength);
    // iRet = fwupgrade_read_partition("KERNEL", pcBuffer, &ulLength);
    if(iRet == 0)
    {
        iRet = fwupgrade_erase_upflag();
        if(iRet == 0)
        {
            // write to partition
            ak_sleep_ms(100);
            iRet = fwupgrade_write_partition(acPartiName, pcBuffer, ulLength);
            if(iRet != 0)
            {
                ak_print_error_ex("Writing partition %s failed! (%d)", acPartiName, iRet);
            }
            else
            {
                ak_print_normal("Write partition %s OK!\n", acPartiName);
                iRet = fwupgrade_finish_update();
                ak_print_normal("Update finish (%d)!", iRet);
                sys_reset();
            }
        }
        else
        {
            ak_print_error("Erase UPFLAG failed!\r\n");
        }
    }
    else
    {
        ak_print_error_ex("Reading partition %s failed! (%d)", acPartiName, iRet);
    }
}


/*---------------------------------------------------------------------------*/
/*                           FIRMWARE UPGRADE                                */
/*---------------------------------------------------------------------------*/
static char *g_pcFirmwareBinary;
static unsigned int g_iLengthBinary = 0;
static FWUPG_BINARY_t g_sFwupg_BinaryInfo;

static int fwupg_parse_packet(FWUPG_INFO_t **ppPacketInfo, unsigned char *pPacket)
{
    int iRet = 0;
    FWUPG_INFO_t *pInfo;
    if(ppPacketInfo == NULL || pPacket == NULL)
    {
        ak_print_error("%s ppPacketInfo or pPacket null\r\n", __func__);
        return -1;
    }
    pInfo = (FWUPG_INFO_t *)pPacket;
    ak_print_normal("type: %02X\n", pInfo->header.type);
    ak_print_normal("control: %02X\n", pInfo->header.control);
    ak_print_normal("len_packet: %04X (%u)\n", pInfo->header.len_packet, \
                                                pInfo->header.len_packet);
    ak_print_normal("len_binary: %08X (%u)\n", pInfo->header.len_binary, \
                                                pInfo->header.len_binary);
    ak_print_normal("position: %08X\n", pInfo->header.position);
    ak_print_normal("data: %p\n", pInfo->data);
    *ppPacketInfo = pInfo;
    return iRet;
}

static void fwupg_dump_structure_binary(FWUPG_BINARY_t *pBin)
{
    if(pBin != NULL){
        ak_print_normal("length_bin: %u, curr_len: %u\n", pBin->length_bin, pBin->current_length);
        ak_print_normal("start: %p, end: %p, work: %p, pos: %p\n", \
                pBin->start_point, pBin->end_point, pBin->work_point, pBin->position_point);
        ak_print_normal("hash: %p, len_key: %u\n", pBin->hash_key, pBin->length_key);
    }
}
/*
* copy data to buffer
* update length
* return position
*/
static int fwupg_save_data(char *pRecv, FWUPG_BINARY_t *pBinInfo)
{
    int iRet = 0;
    FWUPG_INFO_t *pPacketInfo;
    char *pPositionSave = NULL;
    unsigned long ulPosRecv = 0;
    int iLen = 0;
    char *pData = NULL;
    
    if(pRecv == NULL || pBinInfo == NULL || g_pcFirmwareBinary == NULL)
    {
        return -1;
    }
    pPacketInfo = (FWUPG_INFO_t *)pRecv;
    iLen = fwupg_swap_byte_16(pPacketInfo->header.len_packet) - 8;
    ulPosRecv = fwupg_swap_byte_32(pPacketInfo->header.position);
    pPositionSave = g_pcFirmwareBinary + pBinInfo->current_length;
    if(pBinInfo->current_length != ulPosRecv)
    {
        ak_print_error("Wrong position!\n");
        return -2;
    }              
    pData = pRecv + 12; // byte 12th
    memcpy(pPositionSave, pData, iLen);
    // update length binary
    pBinInfo->length_bin = fwupg_swap_byte_32(pPacketInfo->header.len_binary);
    // update current length
    pBinInfo->current_length += iLen;
    // update pointer point to buffer
    pBinInfo->start_point = g_pcFirmwareBinary;
    pBinInfo->work_point = pPositionSave + iLen;
    // *pBinInfo->position_point = fwupg_swap_byte_32(pPacketInfo->header.position); // assign pointer equal integer
    return iRet;
}

// for binary flow
static int fwupg_update_position_resp(char *pRecv, char *pResp, FWUPG_BINARY_t *pBinInfo)
{
    int iRet = 0;
    unsigned long ulPosition = 0;
    if(pRecv == NULL || pResp == NULL || pBinInfo == NULL)
    {
        return -1;
    }
    // byte command ID
    // *(pResp + FWUPG_TYPE_CMD_OFFSET) = (*pRecv)&(~CC_SEND_SPI_PACKET_MASK);
    *(pResp + FWUPG_TYPE_CMD_OFFSET) = SPI_FW_TYPE_CMD;
    // byte control
    *(pResp + FWUPG_CONTROL_OFFSET) = SPI_FW_CTRL_POSITION;

    // byte position
    *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 0) = (char )(((pBinInfo->current_length & 0xFF000000) >> 24) & 0xFF);
    *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 1) = (char )(((pBinInfo->current_length & 0x00FF0000) >> 16) & 0xFF);
    *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 2) = (char )(((pBinInfo->current_length & 0x0000FF00) >> 8) & 0xFF);
    *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 3) = (char )(((pBinInfo->current_length & 0x000000FF) >> 0) & 0xFF);

    // printf("%02X%02X %02X%02X%02X%02X\n", *pResp, *(pResp+1), \
    //                                 *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 0), \
    //                                 *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 1), \
    //                                 *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 2), \
    //                                 *(pResp + FWUPG_M2S_SEND_POS_OFFSET + 3) );
    return iRet;
}

// send stop upgrading or upgrading done
static int fwupg_update_notification(char *pRecv, char *pResp, fw_etype_notify_t eType)
{
    int iRet = 0;
    char cTypeNotify = 0;
    if(pRecv == NULL || pResp == NULL)
    {
        return -1;
    }
    switch(eType)
    {
        case E_FW_TYPE_NOTIFY_STOP_UPGRADING:
            cTypeNotify = SPI_FW_CTRL_STOP_UPGRADING;
            break;
        case E_FW_TYPE_NOTIFY_DONE_UPGRADING:
            cTypeNotify = SPI_FW_CTRL_DONE_UPGRADING;
            break;
        default:
            cTypeNotify = SPI_FW_CTRL_STOP_UPGRADING;
            iRet = -2;
            break;
    }
    // byte command ID
    *(pResp + FWUPG_TYPE_CMD_OFFSET) = *pRecv;
    // byte control
    *(pResp + FWUPG_CONTROL_OFFSET) = cTypeNotify;
    return iRet;
}


static int fwupg_validate_bin(char *bin, unsigned int binlen, char *digest)
{
    int iRet = 0;
    int iNum = 0;
    unsigned int uiLast = 0;
    int i = 0;
    const int BUDGET = 4096;
    MD5_CTX sMd5Ctx;
    char acDigest[16] = {0x00, };
    char acDebug[128] = {0x00, };   

    if(bin == NULL || digest == NULL)
    {
        printf("Parameter binary or digest null!\r\n");
        iRet = -1;
    }
    else
    {
        memset(acDigest, 0x00, sizeof(acDigest));
        MD5Init(&sMd5Ctx);
        iNum = binlen/BUDGET;
        uiLast = binlen - iNum*BUDGET;
        for(i = 0; i < iNum; i++)
        {
            MD5Update(&sMd5Ctx, (unsigned char *)(bin + i*BUDGET), BUDGET);
        }
        MD5Update(&sMd5Ctx, (unsigned char const*)(bin + i*BUDGET), uiLast);
        MD5Final(acDigest, &sMd5Ctx);
        for(i = 0; i < sizeof(acDigest); i++)
        {
            *(digest + i) = acDigest[i];
            sprintf(acDebug +  strlen(acDebug), "%02X", acDigest[i]);
        }
        printf("Digest: %s\r\n", acDebug);
    }
    return iRet;
}

static char char_to_hex(char c)
{
    char ret_char = 0;
    if(c >= 0x30 && c <= 0x39)
    {
        ret_char = c - 0x30;
    }
    else if(c >= 'a' && c <= 'f')
    {
        ret_char = c - 0x57;
    }
    else if(c >= 'A' && c <= 'F')
    {
        ret_char = c - 0x37;
    }
    else
    {
        ret_char = 0xFF;
    }
    return ret_char;
}
static int string_to_hex(char *str, char *hex, int len_str)
{
    int iRet = 0;
    char temp = 0;
    int i = 0;
    if(str == NULL || hex == NULL || len_str <= 0)
    {
        return -1;
    }
    for(i = 0; i < len_str; i++)
    {
        temp = 0;
        temp = (char)((char_to_hex(str[2*i]) << 4 ) & 0xF0);
        temp |= (char)(char_to_hex(str[2*i + 1]) & 0x0F);
        hex[i] = temp;
        printf("%02x ", hex[i]);
    }
    return iRet;
}
/*---------------------------------------------------------------------------*/

int action_init_recv(char *pRecv, char *pResp);
int action_recvbin_recv(char *pRecv, char *pResp);
int action_verify_recv(char *pRecv, char *pResp);
// int action_finished_burning(char *pRecv, char *pResp);

fw_eState_t e_current_state = E_FW_STATE_INIT;
fw_eEvent_t e_new_event = E_FW_EVENT_RECV_PACKET;
int (* state_table[E_FW_MAX_STATE][E_FW_MAX_EVENT])(char *pRecv, char *pResp) =
{
    { action_init_recv}, // procedures for state init
    { action_recvbin_recv}, // procedures for state receive binary
    { action_verify_recv} // procedures for state verify
    // { action_finished_burning} // procedures for state burn
};

static int convert_structure_fwupg_info_bigendian(FWUPG_INFO_t *pInfoInput,
                                        FWUPG_INFO_t *pInfoOutput)
{
    int iRet = 0;
    if(pInfoInput == NULL || pInfoOutput == NULL)
    {
        return -1;
    }
    pInfoOutput->header.type = pInfoInput->header.type;
    pInfoOutput->header.control = pInfoInput->header.control;
    pInfoOutput->header.len_packet = fwupg_swap_byte_16(pInfoInput->header.len_packet);
    pInfoOutput->header.len_binary = fwupg_swap_byte_32(pInfoInput->header.len_binary);
    pInfoOutput->header.position = fwupg_swap_byte_32(pInfoInput->header.position);
    pInfoOutput->data = pInfoInput->data;
    return iRet;
}

int action_init_recv(char *pRecv, char *pResp)
{
    int iRet = 0;
    unsigned int uiSizeBin = 0;
    FWUPG_INFO_t *pInfo;
    int length_data = 0;

    error_check(pRecv);
    error_check(pResp);
    // receive first packet, get length binary
    pInfo = (FWUPG_INFO_t *)pRecv;

    
    uiSizeBin = fwupg_swap_byte_32(pInfo->header.len_binary);
    length_data = fwupg_swap_byte_16(pInfo->header.len_packet) - 4 - 4; // 4 bytes BIN_LEN, 4 byte Position
    printf("uiSizeBin: %u, len_packet: %d\n", uiSizeBin, length_data);
    // allocate memory to receive binary
    if(uiSizeBin <= 0)
    {
        iRet = -1;
        printf("CC3200 send packet with invalid length!\n");
    }
    else
    {
        g_sFwupg_BinaryInfo.length_bin = uiSizeBin;
        printf("Malloc buffer!\r\n");
        g_pcFirmwareBinary = (char *)malloc(uiSizeBin*sizeof(char));
        if(g_pcFirmwareBinary == NULL)
        {
            printf("%s cannot malloc buffer receive data!\n", __func__);
            iRet = -2;
        }
        else
        {
            g_sFwupg_BinaryInfo.start_point = g_pcFirmwareBinary;
            printf("Malloc buffer firmware OK! g_pcFirmwareBinary: %p, pRecv: %p\r\n", g_pcFirmwareBinary, pRecv);
            iRet = fwupg_save_data(pRecv, &g_sFwupg_BinaryInfo);
            if(iRet != 0)
            {
                if(iRet == -1){
                    printf("%s Please check input parameter!\n", __func__);
                }
                fwupg_update_position_resp(pRecv, pResp, &g_sFwupg_BinaryInfo);
                iRet = -3;
            }
            else
            {
                // stop streaming by break loop task video, audio, talkback.
                vSysModeSet(E_SYS_MODE_FWUPGRADE);
                fwupg_update_position_resp(pRecv, pResp, &g_sFwupg_BinaryInfo);
                // change current state to E_FW_STATE_RECV_BIN
                e_current_state = E_FW_STATE_RECV_BIN;
            }
        }
    }
    return iRet;
}
int action_recvbin_recv(char *pRecv, char *pResp)
{
    int iRet = 0;
    error_check(pRecv);
    error_check(pResp);
    // receive data
    if(g_pcFirmwareBinary == NULL)
    {
        printf("%s buffer null\n", __func__);
        iRet = -2;
        e_current_state = E_FW_STATE_INIT;
    }
    else
    {
        // update size of binary which received
        iRet = fwupg_save_data(pRecv, &g_sFwupg_BinaryInfo);
        if(iRet != 0)
        {
            if(iRet == -1){
                printf("%s Please check input parameter!\n", __func__);
            }
            // TODO: need to send stop upgrading because error
            fwupg_update_position_resp(pRecv, pResp, &g_sFwupg_BinaryInfo);
            iRet = -3;
        }
        else
        {
            fwupg_update_position_resp(pRecv, pResp, &g_sFwupg_BinaryInfo);
            // stop streaming by break loop task video, audio, talkback.
            vSysModeSet(E_SYS_MODE_FWUPGRADE);
            // change current state to E_FW_STATE_VERIFY if done receive
            if(g_sFwupg_BinaryInfo.current_length == g_sFwupg_BinaryInfo.length_bin)
            {
                g_iLengthBinary = g_sFwupg_BinaryInfo.current_length;
                e_current_state = E_FW_STATE_VERIFY;
            }
        }
    }
    return iRet;
}

static int fwupg_send_notify(char *pResp)
{
    int ret = 0;
    int i = 0;
    if(pResp)
    {
        for(i = 0; i < 10; i++)
        {
            ret = spi_fwupgrade_send(pResp, MAX_SPI_PACKET_LEN);
            ak_sleep_ms(100);
        }
    }
    else
    {
        printf("Response NULL!\r\n");
        ret = -1;
    }
    return ret;
}

static void fwupg_reboot_wdt(void)
{
    unsigned int fd = 0;
    unsigned int feed_time = 100;
    //start watchdog timer
    printf("watchdog reboot!\r\n");
    fd = ak_drv_wdt_open(feed_time);    
	if(0 != fd)
	{
		ak_print_error("open watchdog fail!\n");
		return ;
	}
	printf("watchdog stopped\n");
    ak_print_normal("\r\nOK\r\n");
}
int action_verify_recv(char *pRecv, char *pResp)
{
    int iRet = 0;
    FWUPG_HASHING_PACKET_t *pHashingInfo = NULL;
    int iLenHasing = 0;
    int i = 0;
    char acDigest[16] = {0x00, };
    char acDigestCCSent[16] = {0x00, };
    error_check(pRecv);
    error_check(pResp);

    pHashingInfo = (FWUPG_HASHING_PACKET_t *)pRecv;
    // receive hashing key
    iLenHasing = fwupg_swap_byte_16(pHashingInfo->len_packet) - FWUPG_CTRL_HASH_TYPE_LEN;
    printf("Length hashing: %d\n", iLenHasing);
    // malloc buffer to store digest
    if(g_sFwupg_BinaryInfo.hash_key == NULL)
    {
        g_sFwupg_BinaryInfo.hash_key = (unsigned char *)malloc(iLenHasing*sizeof(unsigned char));
        if(g_sFwupg_BinaryInfo.hash_key == NULL)
        {
            printf("Malloc buffer digest failed!\n");
            iRet = -2;
        }
    }

    if(iRet == 0)
    {
        // copy digest from packet to buffer
        memcpy(g_sFwupg_BinaryInfo.hash_key, \
            pRecv + FWUPG_CTRL_HASH_DIGEST_OFFSET, \
            iLenHasing);
        // verify
        if(fwupg_validate_bin(g_pcFirmwareBinary, \
                g_sFwupg_BinaryInfo.current_length, \
                acDigest) != 0)
        {
            printf("Generate digest failed!\n");
            iRet = -3;
            fwupg_update_notification(pRecv, pResp, E_FW_TYPE_NOTIFY_STOP_UPGRADING);
            // e_current_state = E_FW_STATE_FINISHED_BURNING;
            fwupg_send_notify(pResp);
            fwupg_reboot_wdt();
        }
        else
        {
            string_to_hex(g_sFwupg_BinaryInfo.hash_key, acDigestCCSent, 16);
            for(i = 0; i < 16; i++)
            {
                printf("[%d]%02X-%02X\n", i, acDigest[i], acDigestCCSent[i]);
                if(acDigest[i] != acDigestCCSent[i])
                {
                    iRet = -4;
                }
            }
            if(iRet == 0)
            {
                iRet = fwupgrade_kernel_ota(g_pcFirmwareBinary, \
                                        g_sFwupg_BinaryInfo.current_length);
                
                printf("Upgrade %d -> reboot\r\n", iRet);
                fwupg_update_notification(pRecv, pResp, E_FW_TYPE_NOTIFY_DONE_UPGRADING);
                // e_current_state = E_FW_STATE_FINISHED_BURNING;
                fwupg_send_notify(pResp);
                for(i=0; i < 5; i++)
                {
                    fwupg_send_notify(pResp);
                    ak_sleep_ms(200);
                }
                // sys_reset();
                fwupg_reboot_wdt();
            }
        }
        // change current state to E_FW_STATE_BURN if fw integrity
    }
    return iRet;
}

// int action_finished_burning(char *pRecv, char *pResp)
// {
//     // only send notification, then reboot
//     fwupg_update_notification(pRecv, pResp, E_FW_TYPE_NOTIFY_DONE_UPGRADING);
//     e_current_state = E_FW_STATE_FINISHED_BURNING;
//     // reboot
// }

fw_eEvent_t fw_get_new_event(void)
{
    fw_eEvent_t ret;
    // first packet ->
    ret = E_FW_STATE_INIT;
    return ret; 
}

static void fw_dump_data(char *data)
{
    FWUPG_INFO_t *pInfo;
    pInfo = (FWUPG_INFO_t *)data;
    printf("Typ:%02X CTL:%02X L:%04X B:%08X P:%08X\n", \
        pInfo->header.type, pInfo->header.control, \
        fwupg_swap_byte_16(pInfo->header.len_packet), \
        fwupg_swap_byte_32(pInfo->header.len_binary), 
        fwupg_swap_byte_32(pInfo->header.position));
}

static int fw_process_data(char *pDataRecv, char *pDataSend)
{
	int iRet = 0;
	int i = 0;

	if(!pDataRecv || !pDataSend)
	{
		return -1;
	}
	
	return iRet;
}

static int fwupg_send_version(char *pDataSend)
{
    int iRet = 0;
    // int i = 0;
    // if(pDataSend == NULL)
    // {
    //     return -1;
    // }
    // printf("%s ver: %s\r\n", __func__, fw_version);
    // *(pDataSend + 0) = 0x18;
    // *(pDataSend + 1) = fw_version[7];
    // *(pDataSend + 2) = fw_version[8];
    // *(pDataSend + 3) = fw_version[10];
    // *(pDataSend + 4) = fw_version[11];
    // *(pDataSend + 5) = fw_version[13];
    // *(pDataSend + 6) = fw_version[14];
    // *(pDataSend + 7) = '\0';

    // for(i = 0; i < 8; i++)
    // {
    //     printf("%02X ", *(pDataSend + i));
    // }
    // printf("\r\n");

    return iRet;
}

/*
* Thread firmware upgrade process data from CC3200
*/
void *fwupgrade_task(void *arg)
{
	int ret = 0;
	char *pBufRecv = NULL;
	char *pResponse = NULL;
	int iLengthResponse = 0;
	int cnt = 0;
    int i = 0;
    
	pBufRecv = (char *)malloc(MAX_SPI_PACKET_LEN);
	if(pBufRecv == NULL)
	{
		dzlog_error("%s Cannot allocate memory for pBufRecv\n", __func__);
		goto exit;
	}
	pResponse = (char *)malloc(MAX_SPI_PACKET_LEN);
	if(pResponse == NULL)
	{
		dzlog_error("%s Cannot allocate memory for pResponse\n", __func__);
		goto exit;
    }

    // fwupg_send_version(pResponse);
    // for(i = 0; i < 3; i++)
    // {
    //     ret = spi_fwupgrade_send(pResponse, MAX_SPI_PACKET_LEN);
    //     ak_print_error("AK has just sent version to CC3200!\r\n");
    //     ak_sleep_ms(100);
    // }

    if(fwupgrade_check_model() != 0)
    {
        ak_print_error("UPFLAG partition is not exist!\n");
        ak_print_error("THIS MODEL IS NOT ALLOWED UPGRADING\n");
        goto exit;
    }
    else
    {
        ak_print_normal("Start thread upgrading!\r\n");
    }

	while(1)
	{
		// receive data from spi
		ret = spi_fwupgrade_receive(pBufRecv, MAX_SPI_PACKET_LEN);
		if(ret < 0)
		{
			dzlog_error("%s spi cmd recv failed!\n", __func__);
		}
		else if(ret > 0)
		{
            dzlog_info("%s (%d)(%u/%u): ", __func__, e_current_state, \
                    g_sFwupg_BinaryInfo.current_length, g_sFwupg_BinaryInfo.length_bin);
			fw_dump_data(pBufRecv);
            ret = state_table[e_current_state][e_new_event](pBufRecv, pResponse);
            // printf("FS: %02X %02X %02X\n", *pResponse, *(pResponse+1), *(pResponse+5));
            if(ret != 0)
            {
                //TODO: send stop upgrading to CC3200
                ret = fwupg_update_notification(pBufRecv, pResponse, E_FW_TYPE_NOTIFY_STOP_UPGRADING);
                
                fwupg_update_position_resp(pBufRecv, pResponse, &g_sFwupg_BinaryInfo);
                ret = spi_fwupgrade_send(pResponse, MAX_SPI_PACKET_LEN);
                // reboot
                sys_reset();
            }
            else
            {
                // if there is not response data, send dummy to spi
                // set postion which has just received to pResp
                fwupg_update_position_resp(pBufRecv, pResponse, &g_sFwupg_BinaryInfo);
                ret = spi_fwupgrade_send(pResponse, MAX_SPI_PACKET_LEN);

            }
		}
		else
		{
            if(e_current_state != E_FW_STATE_INIT)
            {
                ak_sleep_ms(10);
                fwupg_update_position_resp(pBufRecv, pResponse, &g_sFwupg_BinaryInfo);
                ret = spi_fwupgrade_send(pResponse, MAX_SPI_PACKET_LEN);
            }
            else
            {
                ak_sleep_ms(10);
            }
		}
    }
exit:
    if(pBufRecv)
    {
        free(pBufRecv);
    }
    if(pResponse)
    {
	    free(pResponse);
    }
	ak_thread_exit();
	return NULL;
}







/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

