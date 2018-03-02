/*
 * Copyright (C) 2017 Cinatic
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* FILE   : rec.c
 * AUTHOR : leon
 * DATE   : Sept 11, 2017
 * DESC   : Main task for taking care record
 */

#include "flv_rec_os.h"
#include "flv_engine.h"
#include "ak_video.h"
#include "sdcard.h"
#include "ak_apps_config.h"
#include "file_on_mem.h"
#include "config_model.h"

typedef enum eRecStatus
    {
    eRecStatIdle,
    eRecStatRecording,
    eRecStatExited,
    eRecStatCount,
    }eRecStatus;

typedef struct tStreamMgrRec
    {
    FILE    *pFile;
    int     maxDur;
    int     curDur;
    int     quitFlag;
    long    startRecTs;
    int     stopRecord;
    char    filename[256];
    eRecStatus stat;
    flv_rec_task_id tid;
    flv_rec_mutex lock;
    flv_rec_cond cond;
    }tStreamMgrRec;

static tStreamMgrRec gStreamMgrRec = {0, 0, 0};
extern int g_sd_update;
static void *
StreamMgrRecordingHdl(void *d)
    {
    int iErrCode = 0;
    int dur = 0;
    int totalSize = 0;
    tStreamMgrRec* ctx = (tStreamMgrRec*)d;

    while(!(ctx->quitFlag))
        {
        ENTER_CRITICAL(ctx->lock);
        mylog_info("Waiting for command\n");
        ctx->stat = eRecStatIdle;
        LEAVE_CRITICAL(ctx->lock);

        WAIT_COND(ctx->cond);

        if(ctx->quitFlag)
            break;

        mylog_info("Start recording %s with duration %d seconds\n", ctx->filename, ctx->maxDur/1000);
        /* Start to record */
        ctx->curDur = -1;
        ctx->stat = eRecStatRecording;
        ctx->stopRecord = 0;
        iErrCode = 0;
        totalSize = 0;
        ctx->startRecTs = FLVMuxerGetTimeStamp();
        while(!(ctx->stopRecord))
            {
            if(ctx->pFile)
                {
                /* Need to write header and first I-Frame */
                if(ctx->curDur == -1)
                    {
                    iErrCode = FLVEngineWriteHeader(ctx->pFile, 0);
                    if(iErrCode < 0)
                        {
                        msleep(500);
                        continue;
                        }
                    totalSize += iErrCode;
                    mylog_info("Write header size %d\n", iErrCode);
                    ctx->curDur = 0;
                    }

                dur = 0;
                iErrCode = FLVEngineWriteToFile(ctx->pFile, ctx->curDur == 0, ctx->maxDur - ctx->curDur, &dur);
                // mylog_info("FLVEngineWriteToFile return %d. cur dur %d\n", iErrCode, dur);
                if(iErrCode < 0)
                    break;
                totalSize += iErrCode;
                if(dur > 0)
                    ctx->curDur += dur;
                if(ctx->curDur >= ctx->maxDur)
                    break;

                /* Check system time for make sure duration is not over */
                if ((FLVMuxerGetTimeStamp() - ctx->startRecTs) >= (ctx->maxDur + 500))
                    {
                    mylog_info("System time is over %d-%d=%d. Stop recording\n",
                               FLVMuxerGetTimeStamp(), ctx->startRecTs,
                               FLVMuxerGetTimeStamp() - ctx->startRecTs);
                    break;
                    }
                }
            }

        if(iErrCode > 0)
            iErrCode = 0;

        //fflush(ctx->pFile);
        f_mem_flush(ctx->pFile);
        //fclose(ctx->pFile);
        f_mem_close(ctx->pFile);
        g_sd_update = CK_STATE_SD_RECORD_DONE;
        mylog_info("$$$$$$$$$$$$$$$$$$$$$$$$$$ CLOSE ############################");//nguyen_cao add log
        ctx->pFile = NULL;
        mylog_info("%s duration %d seconds (%u). Size %d\n",
                   ctx->filename, ctx->curDur/1000, ctx->curDur, totalSize);
        //FlvRetimestampExternalCall(ctx->filename);
        mylog_info("TO DO : Update duration and file size\n");
        thirdparty_record_done(ctx->curDur/1000, ctx->filename, totalSize, iErrCode);
        }
    ctx->stat = eRecStatExited;
    DEL_CRITICAL(ctx->lock);
    DEL_COND(ctx->cond);
    return 0;
    }


/*
 * PURPOSE : Start record FLV Task
 * INPUT   : [stacksize] - Task stack size
 *           [pri]       - Task priority
 * OUTPUT  : None
 * RETURN  : Task ID of this task
 * DESCRIPT: None
 */
int
StreamMgrRecordingRun(int stacksize, int pri)
    {
    size_t          stksize = 32*1024;

    if(stacksize)
        stksize = stacksize;
    memset(&gStreamMgrRec, 0, sizeof(gStreamMgrRec));
    NEW_CRITICAL(gStreamMgrRec.lock);
    NEW_COND(gStreamMgrRec.cond);
    return FLVRecorderTaskCreate(&(gStreamMgrRec.tid), StreamMgrRecordingHdl, (void*)(&gStreamMgrRec), stksize, pri);
    }

/*
 * PURPOSE : Record FLV to file
 * INPUT   : [filename] - File name
 *           [duration] - Maximum duration
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
StreamMgrRecordingStart(char* filename, int duration)
    {
    int i = 0;
    int iRetryTime = 5;
    int iStat = -1;

    if(gStreamMgrRec.stat != eRecStatIdle)
        return -1;

    /* We are in middle of recording */
    if(gStreamMgrRec.pFile != NULL)
        return -2;

#ifndef DOORBELL_TINKLE820
    // ak_mount_fs(1, 0, "a:");
    if(sdcard_get_status() != E_SDCARD_STATUS_MOUNTED )
    {
        sdcard_mount();
    }
    else
    {
        ak_print_normal("SDcard was mounted! Ready!\r\n");
    }
#endif
    //gStreamMgrRec.pFile = fopen(filename, "wb");
    gStreamMgrRec.pFile = f_mem_open(filename, "wb");
    if(gStreamMgrRec.pFile == NULL)
        return -3;
    gStreamMgrRec.maxDur = duration * 1000;
    snprintf(gStreamMgrRec.filename, sizeof(gStreamMgrRec.filename) - 1, "%s", filename);

    /* Signal waiting Task */
    for(i = 0; i < iRetryTime; i++)
        {
        ENTER_CRITICAL(gStreamMgrRec.lock);
        iStat = gStreamMgrRec.stat;
        LEAVE_CRITICAL(gStreamMgrRec.lock);
        if(iStat != eRecStatIdle)
            {
            mylog_error("Recording is still busy\n");
            msleep(200);
            continue;
            }
        else
            {
            SIGNAL_COND(gStreamMgrRec.cond);
            return 0;
            }
        }

    /* Recording is still busy. Clean all */
    mylog_error("Something wrong. Clean resource\n");
    if(gStreamMgrRec.pFile)
        {
        //fflush(gStreamMgrRec.pFile);
        f_mem_flush(gStreamMgrRec.pFile);
        //fclose(gStreamMgrRec.pFile);
        f_mem_close(gStreamMgrRec.pFile);
        }
    gStreamMgrRec.maxDur = 0;
    memset(gStreamMgrRec.filename, 0, sizeof(gStreamMgrRec.filename));
    return -4;
    }

/*
 * PURPOSE : Stop all recording is in progress
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : 0 if success
 * DESCRIPT: None
 */
int
StreamMgrRecordingStop()
    {
    gStreamMgrRec.stopRecord = 1;
    return 0;
    }

/*
 * PURPOSE : Exit record FLV Task
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : None
 * DESCRIPT: None
 */
void
StreamMgrRecordingExit()
    {
    gStreamMgrRec.quitFlag = 1;
    SIGNAL_COND(gStreamMgrRec.cond);
    }

/*
 * PURPOSE : Get status of recording process
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : status
 * DESCRIPT: None
 */
int
StreamMgrRecordingStatus()
    {
    return gStreamMgrRec.stat;
    }
