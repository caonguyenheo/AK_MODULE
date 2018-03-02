//#include "drv_api.h"

#include "anyka_types.h"

#include "fs.h"
#include "print.h"

#include "tcard_dev_drv.h"

typedef struct
{
    void *   pHandle;
    int           bIsMount;

	unsigned char             driverCnt;
	unsigned char             firstDrvNo;
}T_SD_PARM,*T_pSD_PARM;

static T_SD_PARM   gSdParam={NULL,0,0,false};



static unsigned long sd_read(T_PMEDIUM medium, unsigned char *buf,unsigned long BlkAddr, unsigned long BlkCnt)
{
    unsigned long ret = 0;

	if (sd_dev_read_block(gSdParam.pHandle, BlkAddr, buf, BlkCnt) )
		ret = BlkCnt;
		
	return ret;
}


static unsigned long sd_write(T_PMEDIUM medium, const unsigned char* buf, unsigned long BlkAddr, unsigned long BlkCnt)
{
	unsigned long ret;
	
    if (sd_dev_write_block(gSdParam.pHandle, BlkAddr, buf, BlkCnt))
    {
        ret = BlkCnt;
    }

    return ret;
}



void set_sd_mount_times(void)
{
	gSdParam.bIsMount = 1;
}

bool mount_sd()
{
    DRIVER_INFO driver;
    unsigned long capacity = 0;
    unsigned long BytsPerSec = 0;
    unsigned char firstNo = 0;
    unsigned char drvCnt = 0;    
    bool ret = false;

    if (gSdParam.bIsMount > 0)
    {
        printk("sd have already mounted!\n");
        //return true;
    }
    else
    {
	    
		gSdParam.pHandle = sd_dev_drv_init();

		if (gSdParam.pHandle ==NULL)
			return false;

	    
		sd_dev_get_info(gSdParam.pHandle , &capacity, &BytsPerSec);

	    if (capacity !=0)
	    {
	        driver.nBlkCnt     = capacity;
	        driver.nBlkSize  = BytsPerSec;
	        driver.nMainType = MEDIUM_SD;
	        driver.nSubType  = USER_PARTITION;
	        driver.fRead     = sd_read;
	        driver.fWrite     = sd_write;
	         firstNo = FS_MountMemDev(&driver, &drvCnt, (unsigned char)-1);
	        mini_delay(20);
	    }
	    
	    printk("[MntSd]:FirstDrvNo = %d Count = %d\n",firstNo,drvCnt);
	    if (0XFF == firstNo)
	    {        
	        printk("sd mount faile\n");
	        sd_free(gSdParam.pHandle);
	        return false;
	    }

	    gSdParam.firstDrvNo = firstNo;
	    gSdParam.driverCnt  = drvCnt;
		
	}
	gSdParam.bIsMount++;
    return true;
	
}

bool unmount_sd()
{
    unsigned long i;
    bool mntRet = false;


    if (0 == gSdParam.bIsMount )
    {
		printf("sd don't mount , need't unmount\n");
        return true;
    }
    else
    {
    	gSdParam.bIsMount--;
		if(0 == gSdParam.bIsMount )
		{
		    for (i = 0; i<gSdParam.driverCnt; i++)
		    {
		        mntRet = FS_UnMountMemDev(i + gSdParam.firstDrvNo);
		        printf("SD UnMount:driver ID:%d,UnMount State:%d\n" \
		                                    ,i + gSdParam.firstDrvNo,mntRet);
		    }		    
		    sd_dev_drv_release(gSdParam.pHandle);
		    memset(&gSdParam ,0,sizeof(gSdParam));

		    printk("sd unmount all finish\n");
		} 
		else
		{
		    printk("sd unmount ok\n");
		}
    }
    return true;
}




