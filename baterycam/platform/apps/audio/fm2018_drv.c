/**
  ******************************************************************************
  * File Name          : fm2018_drv.c
  * Description        : This file contains API control FM2018
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 NXCOMM PTE LTD
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <string.h>
#include "fm2018_drv.h"
#include "ak_apps_config.h"

#include "drv_gpio.h"
/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

#define FM2018_IRQ_ANA_PIN                  (0)
#define FM2018_SHI_PIN                      (31)
#define FM2018_PWD_EN                       (42)

static char *help[]={
	"fm switch mode 1: analog and 2: dsp",
	"usage:fmsw 0 or fmsw 1\n"
};


static FM_PARAM_CFG_t g_fm2018_380_config[FM2018_MAX_CONFIG] =
{
    {FM2018_START_ADDRESS, FM2018_START_DATA},
    {FM_ADDR_01, FM_DATA_01},
    {FM_ADDR_02, FM_DATA_02},
    {FM_ADDR_03, FM_DATA_03},
    {FM_ADDR_04, FM_DATA_04},
    {FM_ADDR_05, FM_DATA_05},

    {FM_ADDR_06, FM_DATA_06},
    {FM_ADDR_07, FM_DATA_07},
    {FM_ADDR_08, FM_DATA_08},
    {FM_ADDR_09, FM_DATA_09},
    {FM_ADDR_10, FM_DATA_10},

    {FM_ADDR_11, FM_DATA_11},
    {FM_ADDR_12, FM_DATA_12},
    {FM_ADDR_13, FM_DATA_13},
    {FM_ADDR_14, FM_DATA_14},
    {FM_ADDR_15, FM_DATA_15},

    {FM_ADDR_16, FM_DATA_16},
    {FM_ADDR_17, FM_DATA_17},
    {FM_ADDR_18, FM_DATA_18},
    {FM_ADDR_19, FM_DATA_19},
    {FM_ADDR_20, FM_DATA_20},

    {FM_ADDR_21, FM_DATA_21},
    {FM_ADDR_22, FM_DATA_22},
    {FM_ADDR_23, FM_DATA_23},
    {FM_ADDR_24, FM_DATA_24},
    {FM_ADDR_25, FM_DATA_25},

    {FM_ADDR_26, FM_DATA_26},
    {FM_ADDR_27, FM_DATA_27},
    {FM_ADDR_28, FM_DATA_28},
    {FM_ADDR_29, FM_DATA_29},
    {FM_ADDR_30, FM_DATA_30},

    {FM_ADDR_31, FM_DATA_31},
    {FM_ADDR_32, FM_DATA_32},
    {FM_ADDR_33, FM_DATA_33},
    {FM_ADDR_34, FM_DATA_34},
    {FM_ADDR_35, FM_DATA_35},

    {FM_ADDR_36, FM_DATA_36},
    {FM_ADDR_37, FM_DATA_37},
    {FM_ADDR_38, FM_DATA_38},
    {FM_ADDR_39, FM_DATA_39},
    {FM_ADDR_40, FM_DATA_40},

    {FM_ADDR_41, FM_DATA_41},
    {FM_ADDR_42, FM_DATA_42},
    {FM_ADDR_43, FM_DATA_43},
    {FM_ADDR_44, FM_DATA_44},
    // {FM_ADDR_45, FM_DATA_45}, // very bad
    // {FM_ADDR_46, FM_DATA_46},

    {FM2018_END_ADDRESS, FM2018_END_DATA}
};

/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/
static unsigned short swap16bit(unsigned short value)
{
    unsigned short ret = 0;
    ret = (value >> 8 ) & 0xFF; // shift byte high to low
    ret |= (value << 8) & 0xFF00; // shift byte low to high
    return ret;
}

static int conv_struct_proc_to_tran(PROC_INST_t *pProcInst, TRAN_INST_t *pTranInst)
{
    int ret = 0;
    unsigned short tmp;
    unsigned char *p = (unsigned char *)&tmp;
    if(!pProcInst || !pTranInst)
    {
        return -1;
    }
    tmp = swap16bit(pProcInst->sync);
    pTranInst->sync[0] = *p;
    pTranInst->sync[1] = *(p + 1);
    pTranInst->cmd = pProcInst->cmd;
    tmp = swap16bit(pProcInst->addr);
    pTranInst->addr[0] = *p;
    pTranInst->addr[1] = *(p + 1);
    tmp = swap16bit(pProcInst->data);
    pTranInst->data[0] = *p;
    pTranInst->data[1] = *(p + 1);
    return ret;
}
static int conv_struct_tran_to_proc(PROC_INST_t *pProcInst, TRAN_INST_t *pTranInst)
{
    int ret = 0;
    if(!pProcInst || !pTranInst)
    {
        return -1;
    }
    pProcInst->sync = pTranInst->sync[0] & 0xFF;
    pProcInst->sync |= (pTranInst->sync[1] << 8) & 0xFF00;
    pProcInst->cmd = pTranInst->cmd;
    pProcInst->addr = pTranInst->addr[0] & 0xFF;
    pProcInst->addr |= (pTranInst->addr[1] << 8) & 0xFF00;
    pProcInst->data = pTranInst->data[0] & 0xFF;
    pProcInst->data |= (pTranInst->data[1] << 8) & 0xFF00;
    return ret;
}


/*
* Open device and init handle
*/
int fm2018_init(void **ppHandle)
{
    int iRet = 0;
   
    *ppHandle = ak_drv_i2c_open(FM2018_380_DEVICE_ADDRESS, 1);
    ak_print_normal("%s *ppHandle: %p-%x\n", __func__, *ppHandle, *ppHandle);
    if(*ppHandle == NULL)
    {
        iRet = -1;
        ak_print_error_ex("Open I2C failed!\n");
    }
    return iRet;
}

/*
* Close device and deinit handle
*/
int fm2018_deinit(void **ppHandle)
{
    void *pHandle = *ppHandle;
    if(pHandle != NULL)
    {
        ak_drv_i2c_close(pHandle);
    }
    return 0;
}

/*
* Write a unsigned short value to a memory or register
* The first byte 0xC0, device address.
* Instruction contains 7 bytes
* 0xFCF3: sync word - 2 bytes
* 0x3B: Command Entry byte - 1 byte
* 2 bytes address
* 2 bytes data
*/
int fm2018_write_reg(void *pHandle, unsigned short u16AddrRegWr, 
                                unsigned short u16DataWr)
{
    int ret = 0;
    PROC_INST_t process_instruct;
    TRAN_INST_t transfer_instruct;
    unsigned char *pData =  (unsigned char *)&transfer_instruct.cmd;
    
    if(pHandle == NULL)
    {
        return -1;
    }
    // fill data to structure
    process_instruct.sync = FM2018_SYNC_WORD;
    process_instruct.cmd = FM2018_CMD_WRITE;
    process_instruct.addr = u16AddrRegWr;
    process_instruct.data = u16DataWr;
    ret = conv_struct_proc_to_tran(&process_instruct, &transfer_instruct);
    if(ret != 0)
    {
        return -2;
    }
    // send data
    ret = ak_drv_i2c_write(pHandle, process_instruct.sync, pData, 5);
    return ret;
}
/*
* Write instructions to array memory/ register
*/
int fm2018_write_array_reg(void *pHandle, FM_PARAM_CFG_t sParamConfig);

/*
* Read a unsigned short value from a memory or register
*/
int fm2018_read_reg(void *pHandle, unsigned short u16AddrRegRd, 
                                unsigned short *u16DataRd)
{
    int ret = 0;
    TRAN_READ_t transfer_read;
    unsigned short *pTemp = (unsigned short *)&transfer_read.ind[0];
    unsigned char *pData =  (unsigned char *)&transfer_read;
    unsigned char data_read[2];

    if(pHandle == NULL)
    {
        return -1;
    }
    // printf("Addr: 0x%04x ", u16AddrRegRd);
    // 0x37
    transfer_read.cmd = FM2018_CMD_READ_MEM;
    *pTemp = swap16bit(u16AddrRegRd);
    // printf("Cmd37 %02X %02X%02X ", *pData, *(pData + 1), *(pData + 2));
    ret = ak_drv_i2c_write(pHandle, FM2018_SYNC_WORD, pData, 3);
    if(ret != 0)
    {
        return -2;
    }
    // 0x60 25
    transfer_read.cmd = FM2018_CMD_READ_REG;
    transfer_read.ind[0] = FM2018_DATA_PORT_LOW;
    // printf("Cmd25 %02X %02X%02X ", *pData, *(pData + 1), *(pData + 2));
    ret = ak_drv_i2c_write(pHandle, FM2018_SYNC_WORD, pData, 2);
    if(ret != 0)
    {
        return -3;
    }
    ret = ak_drv_i2c_read(pHandle, u16AddrRegRd, data_read, 2);
    if(ret != 0)
    {
        return -4;
    }
    *u16DataRd = (unsigned short )(data_read[0] & 0xFF); // byte low
    // printf("DL %02X%02X ", data_read[0], data_read[1]);
    // 0x60 26
    transfer_read.cmd = FM2018_CMD_READ_REG;
    transfer_read.ind[0] = FM2018_DATA_PORT_HIGH;
    // printf("Cmd26 %02X %02X%02X ", *pData, *(pData + 1), *(pData + 2));
    ret = ak_drv_i2c_write(pHandle, FM2018_SYNC_WORD, pData, 2);
    if(ret != 0)
    {
        return -5;
    }
    ret = ak_drv_i2c_read(pHandle, u16AddrRegRd, data_read, 2);
    if(ret != 0)
    {
        return -6;
    }
    *u16DataRd |= (unsigned short )(data_read[0]<<8 & 0xFF00); // byte high
    // printf("Dh %02X%02X\n", data_read[0], data_read[1]);
    return ret;
}
/*
* Read array memory or register
*/
int fm2018_read_array_reg(void *pHandle, FM_PARAM_CFG_t *psParamConfig);


/*
* Dump all config FM2018
*/
int fm2018_dump_all_config(void *handle)
{
    int ret = 0;
    unsigned short u16ReadValue = 0;
    unsigned short u16Addr = 0;
    unsigned short u16Data = 0;
    int i = 0;

    // loop, read config and write
    for(i = 0; i < FM2018_MAX_CONFIG; i++)
    {
        u16Addr = g_fm2018_380_config[i].addr;
        u16Data = g_fm2018_380_config[i].data;

        // read data before config
        ret = fm2018_read_reg(handle, u16Addr, &u16ReadValue);
        if(ret == 0)
        {
            ak_print_normal("[%d] A:0x%04X D:0x%04X\n", \
                i, u16Addr, u16ReadValue);
        }
        else
        {
            ak_print_error("Cannot READ FM2018 [%d] Addr: %04X (ret %d)\n", \
                                         i, u16Addr, ret);
        }
        ak_sleep_ms(50);
    }
    return ret;
}
/*
* Config FM2018 for Cinatic
*/
int fm2018_config_param(void)
{
    int ret = 0;
    void *handle = NULL;
    unsigned short u16ReadValue = 0;
    unsigned short u16Addr = 0;
    unsigned short u16Data = 0;
    int i = 0;

    // open device
    ret = fm2018_init(&handle);
    if(NULL == handle){
        ak_print_error("Cannot open I2C (0xC0)!\n");
        return -1;
    }

    // debug
    // fm2018_dump_all_config(handle);

    // loop, read config and write
    for(i = 0; i < FM2018_MAX_CONFIG; i++)
    {
        u16Addr = g_fm2018_380_config[i].addr;
        u16Data = g_fm2018_380_config[i].data;
        // write config
        ret = fm2018_write_reg(handle, u16Addr, u16Data);
        if(ret != 0)
        {
            ak_print_error("Cannot write FM2018 [%d] Addr: %04X (ret %d)\n", \
                                                i, u16Addr, ret);
            break;
        }
        else
        {
            // confirm data
            ret = fm2018_read_reg(handle, u16Addr, &u16ReadValue);
            //ak_print_normal("[%02d] Addr: 0x%04X-0x%04X (Read: 0x%04X) (ret %d)\n", \
            //                    i, u16Addr, u16Data, u16ReadValue, ret);
        }
        ak_sleep_ms(20);
    }
    ak_print_normal("Config FM2018 done!\n");
    // close device
    ret = fm2018_deinit(&handle);
    return ret;
}

/*
* Config FM2018 for Cinatic
*/
int fm2018_ftest_config_param(void)
{
    int ret = 0;
    void *handle = NULL;
    unsigned short u16ReadValue = 0;
    unsigned short u16Addr = 0;
    unsigned short u16Data = 0;
    int i = 0;

    // open device
    ret = fm2018_init(&handle);
    if(NULL == handle){
        ak_print_error("Cannot open I2C (0xC0)!\n");
        return -1;
    }

    // debug
    fm2018_dump_all_config(handle);

    // loop, read config and write
    for(i = 0; i < FM2018_MAX_CONFIG; i++)
    {
        u16Addr = g_fm2018_380_config[i].addr;
        u16Data = g_fm2018_380_config[i].data;
        // write config
        ret = fm2018_write_reg(handle, u16Addr, u16Data);
        if(ret != 0)
        {
            ak_print_error("Cannot write FM2018 [%d] Addr: %04X (ret %d)\n", \
                                                i, u16Addr, ret);
            break;
        }
        else
        {
            // confirm data
            ret = fm2018_read_reg(handle, u16Addr, &u16ReadValue);
            ak_print_normal("[%02d] Addr: 0x%04X-0x%04X (Read: 0x%04X) (ret %d)\n", \
                                i, u16Addr, u16Data, u16ReadValue, ret);
        }
        ak_sleep_ms(20);
    }
    ak_print_normal("Ftest Config FM2018 done!\n");
    // close device
    ret = fm2018_deinit(&handle);
    return ret;
}


/*---------------------------------------------------------------------------*/
// test structure
static void test_structure(void)
{
    PROC_INST_t process_instruct;
    TRAN_INST_t transfer_instruct;
    unsigned char *p = (unsigned char *)&process_instruct;
    int i = 0;

    printf("---------------------------------------\n");
    process_instruct.sync = 0xFCF3;
    process_instruct.cmd = 0x3B;
    process_instruct.addr = 0x1E30;
    process_instruct.data = 0x1234;
    fm2018_dump_data(p, sizeof(PROC_INST_t));
    // convert
    conv_struct_proc_to_tran(&process_instruct, &transfer_instruct);
    p = (unsigned char *)&transfer_instruct;
    printf("TRAN_INST_t: ");
    fm2018_dump_data(p, sizeof(TRAN_INST_t));
    printf("---------------------------------------\n");
    for(i = 0; i < sizeof(TRAN_INST_t); i++)
    {
        *(p + i) = i + 8;
    }
    printf("Init data: ");
    fm2018_dump_data(p, sizeof(TRAN_INST_t));
    conv_struct_tran_to_proc(&process_instruct, &transfer_instruct);
    printf("PROC_INST_t: sync: %04X cmd %02X addr %04X data %04X\n", \
        process_instruct.sync, process_instruct.cmd, process_instruct.addr, process_instruct.data);    
    printf("---------------------------------------\n");
}

/*
0x1E39 is set to 1 by default, which presets it to low-to-high edge trigger
*/
int fm2018_analog_communication_mode(void)
{
    int ret = 0;
    gpio_set_pin_as_gpio(FM2018_IRQ_ANA_PIN);
    gpio_set_pin_dir(FM2018_IRQ_ANA_PIN, 1);    // 1 means output;
    gpio_set_pin_level(FM2018_IRQ_ANA_PIN, 1);
    return ret; 
}
/*
0x1E39 is set to 1 by default, which presets it to low-to-high edge trigger
So maybe this function is not effect
*/
int fm2018_dsp_mode(void)
{
    int ret = 0;
    gpio_set_pin_as_gpio(FM2018_IRQ_ANA_PIN);
    gpio_set_pin_dir(FM2018_IRQ_ANA_PIN, 1);    // 1 means output;
    gpio_set_pin_level(FM2018_IRQ_ANA_PIN, 0);
    return ret; 
}


void fm2018_dump_data(unsigned char *data, int length)
{
    int i = 0;
    unsigned char str[256];
    memset(str, 0x00, sizeof(str));

    for(i = 0; i < length; i++)
    {
        sprintf(str + strlen(str), "%02X ", *(data + i));
    }
    printf("Dump: %s\n", str);  

}

void fm2018_drv_set_pin_irq_analog(int logic)
{
	gpio_set_pin_level(FM2018_IRQ_ANA_PIN, logic);
}

int fm2018_power_down_init_gpio(void)
{
    int ret = 0;
    gpio_set_pin_as_gpio(FM2018_PWD_EN);
    gpio_set_pin_dir(FM2018_PWD_EN, 1);    // 1 means output;
    gpio_set_pin_level(FM2018_PWD_EN, 1);
    return ret; 
}


int fm2018_pwd_active_high(void)
{
    int ret = 0;
    gpio_set_pin_level(FM2018_PWD_EN, 1);
    return ret; 
}

int fm2018_pwd_active_low(void)
{
    int ret = 0;
    gpio_set_pin_level(FM2018_PWD_EN, 0);
    return ret; 
}
/*
    by the way, please also note that FM_EN and AK_EN are inverted. 
    FM_EN = low enable. SPK_EN = high enable.
*/
int fm2018_pwd_active(void)
{
    int ret = 0;
    gpio_set_pin_level(FM2018_PWD_EN, FM_ENABLE);
    return ret; 
}

int fm2018_pwd_deactive(void)
{
    int ret = 0;
    gpio_set_pin_level(FM2018_PWD_EN, FM_DISABLE);
    return ret; 
}

int fm2018_shi_init_gpio(void)
{
    int ret = 0;
    gpio_set_pin_as_gpio(FM2018_SHI_PIN);
    gpio_set_pin_dir(FM2018_SHI_PIN, 1);    // 1 means output;
    gpio_set_pin_level(FM2018_SHI_PIN, 1);
    return ret;
}


// int fm2018_read_data(void *handle, unsigned short reg_addr_rd, unsigned short *rd_value)
// {
//     int ret = 0;
//     unsigned short reg_addr = 0xFCF3;
//     unsigned char data_cmdrx37[16] = {0x37, 0x1E, 0x3E, };
//     unsigned char data_cmdrx6025[16] = {0x60, 0x25, };
//     unsigned char data_cmdrx6026[16] = {0x60, 0x26, };
//     unsigned char data_read[2];

//     if(handle == NULL || rd_value == NULL)
//     {
//         ak_print_error("Param handle | rd_value NULL!\n");
//         return -1;
//     }

//     // byte high
//     data_cmdrx37[1] = (unsigned char)((reg_addr_rd >> 8) & 0xFF);
//     // byte low
//     data_cmdrx37[2] = (unsigned char)((reg_addr_rd >> 0) & 0xFF);
//     // ak_print_normal("reg: 0x%04X H:0x%02X L:0x%02X - ", reg_addr_rd, \
//     //                             data_cmdrx37[1], data_cmdrx37[2]);

//     // 0x37
//     ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx37, 3);
//     // printf("(read37) ret = %d ", ret);

//     // 0x60 25
//     ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx6025, 2);
//     ret = ak_drv_i2c_read(handle, reg_addr_rd, data_read, 2);
//     *rd_value = (unsigned short )(data_read[0] & 0xFF); // byte low
//     // printf("(read25) ret = %d ", ret);

//     // 0x60 26
//     ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx6026, 2);
//     ret = ak_drv_i2c_read(handle, reg_addr_rd, data_read, 2);
//     // printf("(read26) ret = %d\n", ret);
//     *rd_value |= (unsigned short )(data_read[0]<<8 & 0xFFFF); // byte high
//     printf("RD: reg: 0x%04X  value: 0x%04X\n", reg_addr_rd, *rd_value);
//     return ret;
// }

// int fm2018_config_data(void)
// {
//     int ret = 0;
//     int i = 0;
//     void *handle = NULL;
//     FM_TRANSACTION_WRITE_t sPacket;
//     unsigned char *p_cmd = (unsigned char *)&sPacket;
//     unsigned short *p_regaddr = (unsigned short *)&(sPacket.cmd.inst.sync[0]);
//     unsigned char *p_data = (unsigned char *)&(sPacket.cmd.inst.cmd_entry);

//     unsigned short read_value, reg;

//     // test_structure();

//     handle = ak_drv_i2c_open(0xC0);
//     if(NULL == handle){
//         printf("Cannot open I2C (0xC0)!\n");
//         return NULL;
//     }

//     // if(fm2018_init(&handle) != 0)
//     // {
//     //     ret = -1;
//     // }
//     // else
//     // {
//         ak_print_normal("%s pHandle: %p-%x\n", __func__, handle, handle);
//         ak_print_normal("Size of FM_INSTRUCTION_t: %d\n", sizeof(FM_INSTRUCTION_t));
//         ak_print_normal("Size of FM_TRANSACTION_WRITE_t: %d\n", sizeof(FM_TRANSACTION_WRITE_t));
//         sPacket.cmd.inst.sync[0] = (FM2018_SYNC_WORD>>8) & 0xFF;
//         sPacket.cmd.inst.sync[1] = (FM2018_SYNC_WORD>>0) & 0xFF;
//         sPacket.cmd.inst.cmd_entry = FM2018_CMD_WRITE;

//         for(i = 0; i < FM2018_MAX_CONFIG; i++)
//         {
//             // init data
//             sPacket.cmd.inst.addr[0] = (g_fm2018_380_config[i].addr >>8) & 0xFF;
//             sPacket.cmd.inst.addr[1] = (g_fm2018_380_config[i].addr >>0) & 0xFF;
//             sPacket.cmd.inst.data[0] = (g_fm2018_380_config[i].data >> 8) & 0xFF;
//             sPacket.cmd.inst.data[1] = (g_fm2018_380_config[i].data >> 0) & 0xFF;
//             // send data
//             // dumpdata
//             printf(">>>Cmd[%d] ", i);
//             fm2018_dump_data(p_cmd, sizeof(FM_TRANSACTION_WRITE_t));
//             // printf("%04X %02X %02X %02X %02X %02X ", *p_regaddr, p_data[0], p_data[1], p_data[2], p_data[3], p_data[4]);
//             ret = ak_drv_i2c_write(handle, 0xFCF3, p_data, 5);
//             // printf("ak_drv_i2c_write ret = %d\n", ret);

//             if(i != (FM2018_MAX_CONFIG-1)){
//                 // read to confirm
//                 reg = sPacket.cmd.inst.addr[0] << 8;
//                 reg |= sPacket.cmd.inst.addr[1] << 0;
//                 // printf("reg: %04X\n", reg);
//                 fm2018_read_data(handle, reg, &read_value);
//             }
//             ak_sleep_ms(100);
//         }
//     //     ret = fm2018_deinit(&handle);
//     // }
//     ak_drv_i2c_close(handle);

//     return ret;
// }

void* fm2018_main(void *arg)
{
    int ret = 0;
    // /* Test FM2018 */
    // void *handle = NULL;
    // unsigned short reg_addr = 0xFCF3;
    // unsigned char data_tx[16] = {0x3B, 0x1E, 0x3E, 0x12, 0x34, };
    // unsigned char data_tx_end[16] = {0x3B, 0x1E, 0x3A, 0x00, 0x00, };

    // unsigned char data_rx[16] = {0x37, 0x1E, 0x3E, };
    // unsigned char data_cmdrx37[16] = {0x37, 0x1E, 0x3E, };
    // unsigned char data_cmdrx6025[16] = {0x60, 0x25, };
    // unsigned char data_cmdrx6026[16] = {0x60, 0x26, };
    // unsigned char data_recv1[16];
    // unsigned char data_recv2[16];
    // unsigned short reg_addr_rd = 0x1E3E;

    // handle = ak_drv_i2c_open(0xC0);
    // if(NULL == handle){
    //     printf("Cannot open I2C (0xC0)!\n");
    //     return NULL;
    // }
    // ret = ak_drv_i2c_write(handle, reg_addr, data_tx, 5);
    // printf("I2C write 1E3E ret = %d\n", ret);
    // // ret = ak_drv_i2c_write(handle, reg_addr, data_tx_end, 5);
    // // printf("I2C write 1E3A ret = %d\n", ret);

    // // read-------
    // memset(data_recv1, 0x00, sizeof(data_recv1));
    // memset(data_recv2, 0x00, sizeof(data_recv2));

    // // 0x37
    // ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx37, 3);
    // printf("I2C read37 ret = %d\n", ret);

    // // 0x60 25
    // ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx6025, 2);
    // ret = ak_drv_i2c_read(handle, reg_addr_rd, data_recv1, 2);
    // printf("I2C read25 ret = %d\n", ret);
    // // 0x60 26
    // ret = ak_drv_i2c_write(handle, reg_addr, data_cmdrx6026, 2);
    // ret = ak_drv_i2c_read(handle, reg_addr_rd, data_recv2, 2);
    // printf("I2C read26 ret = %d\n", ret);

    // printf("Data byte low: ");
    // fm2018_dump_data(data_recv1, 5);
    // printf("Data byte high: ");
    // fm2018_dump_data(data_recv2, 5);
    // ak_drv_i2c_close(handle);


    // ak_sleep_ms(1000);

    // // test
    // fm2018_config_data();
    ret = fm2018_config_param();
    return NULL;
}


void * fm2018_test_on_fly(int argc, char **args)
{
	// toggle pin IRQ_ANA
	printf("%s start! argc: %d\r\n", __func__, argc);
	
    // check parameter and turn on /off pin IRQ_ANA
    if(strcmp("1",args[0])== 0)
	{
		fm2018_drv_set_pin_irq_analog(1);
		ak_print_error_ex("SET MODE ANALOG\r\n");
	}
	else if(strcmp("0",args[0])== 0)
	{
		fm2018_drv_set_pin_irq_analog(0);
		ak_print_error_ex("SET MODE DIGITAL\r\n");
	}
	else
	{
		ak_print_error_ex(

"Command is invalid\r\n");
		ak_print_error_ex(

"Args[0]: %s\r\n", args[0]);
	}
	
	return NULL;
}

static int fm2018_test(void) 
{
    ak_pthread_t id;

    // ak_login_all_filter();
	// ak_login_all_encode();
	// ak_login_all_decode();


    ak_thread_create(&id, fm2018_main, 0, 32*1024, 10);
    ak_print_normal("FM driver was working!\n");
    return 0;
}

static int cmd_fm2018_reg(void)
{
    cmd_register("fmsw", fm2018_test_on_fly, help);
    return 0;
}
cmd_module_init(cmd_fm2018_reg)

/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

