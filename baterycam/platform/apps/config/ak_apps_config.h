#ifndef _AK_APPS_CONFIG_H_
#define _AK_APPS_CONFIG_H_

/*----------------------------------------------------------------------------*/
/*                            Include                                         */
/*----------------------------------------------------------------------------*/
#include "apps_log.h"

#include "ak_thread.h"
#include "ak_global.h"
#include "ak_sd_card.h"

#include "command.h"
#include "ak_ao.h"
#include "ak_ai.h"
#include "ak_common.h"
#include "anyka_types.h"
#include "kernel.h"
#include "ak_vi.h"
#include "ak_venc.h"
#include "drv_gpio.h"
#include "sys_common.h"
//#include "sdcard.h"

// #include "aec_interface.h" // for AEC

// #include "mem_api.h"
#include "libc_mem.h"

#include "config_model.h"

#define AK_PLATFORM                         (1)
/*----------------------------------------------------------------------------*/
/*                            Define for System                               */
/*----------------------------------------------------------------------------*/
typedef enum system_mode_t
{
    E_SYS_MODE_NORMAL = 0,
    E_SYS_MODE_FTEST,
    E_SYS_MODE_FWUPGRADE,
    E_SYS_MODE_MAX
}ENUM_SYS_MODE_t;

enum SPI_PACKET_TYPE {
    CMD_VERSION                 = 0x01,
    CMD_UDID                    = 0x02,
    DATA_STREAMING              = 0x03,
    CMD_STATUS                  = 0x04,
    CMD_GET_PACKET              = 0x05,
    CMD_RESET                   = 0x06,
    CMD_TCP                     = 0x07,
    CMD_UDP                     = 0x08,
    CMD_PING                    = 0x09,
    CMD_GET_RESP                = 0x0A,
    CMD_REQUEST                 = 0x0B,
    CMD_RESPONSE                = 0x0C,
    STREAM_ACK                  = 0x0D,
    SPI_CMD_FLICKER             = 0x0E,
    SPI_CMD_FLIP                = 0x0F,
    SPI_CMD_VIDEO_BITRATE       = 0x10,
    SPI_CMD_VIDEO_FRAMERATE     = 0x11,
    SPI_CMD_VIDEO_RESOLUTION    = 0x12,
    SPI_CMD_AES_KEY             = 0x13,
    SPI_CMD_GET_CDS             = 0x14,
    SPI_CMD_FWUPGRADE           = 0x15,
    SPI_CMD_MOTION_UPLOAD       = 0x16,
    SPI_CMD_NTP                 = 0x17,
    SPI_UPGRADE_GET_VERSION     = 0x18,
    SPI_CMD_PLAY_DINGDONG       = 0x19,
    SPI_CMD_UDID                = 0x1A,
    SPI_CMD_TOKEN               = 0x1B,
    SPI_CMD_URL                 = 0x1C,
    SPI_CMD_ACK_P2P_SD_STREAM   = 0x1D, // ACK for file transfer
    SPI_CMD_FILE_TRANSFER       = 0x1E, // for file transfer
    SPI_CMD_PLAY_VOICE          = 0x1F,
    SPI_CMD_PLAY_CUT_AK_POWER   = 0x20,
    SPI_CMD_VOLUME              = 0x21, //change volume
    SPI_CMD_FACTORY_TEST        = 0x24,
    MAX_PACKET_TYPE
};

typedef struct time_debug_upload
{
	unsigned long first_i_frame;
	unsigned long take_snapshot;
	unsigned long start_uploader;
	unsigned long add_event_done;
	unsigned long upload_snapshot_done;
}TIME_DEBUG_UPLOAD_t;


#define CC_SEND_SPI_PACKET_MASK     0x80


#define CFG_DEBUG_MAX_POTENTIAL             (1)
// #define CFG_USE_SCHEDULER                   (1)

// #define CFG_P2P_RESEND_PACKET_BY_PACKET         (1)
// #define CFG_P2P_RESEND_IFRAME         (1)

#define CFG_DEBUG_BANDWIDTH                 (1)

#define TASK_STACK_SIZE 			4096


#define CFG_MAIN_TASK_PRIO 				10
#define CFG_CMD_TASK_PRIO 				10

#if 1
#define CFG_SPI_TASK_PRIO	 			60      //1
#define CFG_VIDEO_TASK_PRIO 			50
#define CFG_AUDIO_TASK_PRIO 			55
#define CFG_SPI_CMD_TASK_PRIO 			50
#define CFG_SCHEDULER_TASK_PRIO         70      // x
#define CFG_INT_TASK_TEST_PRIO			70      // x
#define CFG_DBLTC_TASK_PRIO	 			20
#define CFG_FTEST_TASK_PRIO	 			60      // x
#define CFG_TALKBACK_TASK_PRIO          65
#define CFG_PACKET_QUEUE_TASK_PRIO      40
#define CFG_SNAPSHOT_TASK_PRIO          40
#define CFG_FWUPG_TASK_PRIO             40
#define CFG_FLV_RECORDER_TASK_PRIO      40

#else
#define CFG_SPI_TASK_PRIO	 			10      //1
#define CFG_VIDEO_TASK_PRIO 			10
#define CFG_AUDIO_TASK_PRIO 			10
#define CFG_SPI_CMD_TASK_PRIO 			10
#define CFG_SCHEDULER_TASK_PRIO         10      // x
#define CFG_INT_TASK_TEST_PRIO			10      // x
#define CFG_DBLTC_TASK_PRIO	 			10
#define CFG_FTEST_TASK_PRIO	 			10      // x
#define CFG_TALKBACK_TASK_PRIO          10
#define CFG_PACKET_QUEUE_TASK_PRIO      10
#define CFG_SNAPSHOT_TASK_PRIO          10
#define CFG_FWUPG_TASK_PRIO             10
#endif


#define VIDEO_TASK_DELAY                10   //ms
#define VIDEO_TASK_NOFRAME              6

// 64ms x 2byte x 8000 sample/1000 ms = 1024 bytes ~512 samples
// 
#define AUDIO_TASK_DELAY                15 // This delay must be less 15ms
#define AUDIO_TASK_NOFRAME              5

#define TALKBACK_TASK_DELAY             10
#define TALKBACK_TASK_NOFRAME           5

#define SYS_ENCODE_JPEG                (1)

#define SYS_USE_DINGDONG_WAV_1S         (1)

#define SYS_MAX_LENGTH_KEY              (64)

#define SYS_MOTIONUPLOAD_ID                   (1)
#define SYS_DIR_NAME_PREFIX             "DoorBell-T"      //"DoorBell-T002CD1AB0"
#define SYS_DIR_NAME_CLIP_SUFFIX        "clip"
#define SYS_DIR_NAME_SNAPSHOT_SUFFIX    "snapshot"
#define SYS_EVENT_CODE                  (6)


#define SYS_COOLDOWN_PUSH               10000 // ms

#define SYS_CDS_THRESOLD                (150)
#define SYS_CDS_THRESOLD_ON             (640)
#define SYS_CDS_THRESOLD_OFF            (600)
#define SYS_LED_PIN_CONTROL             (47)



#define SYS_UPDATE_INFO_CLIP_WAIT_TIMEOUT (17000)
/*----------------------------------------------------------------------------*/
/*                            Define for SDcard                               */
/*----------------------------------------------------------------------------*/
// #define CFG_DEBUG_VIDEO_WITH_SDCARD     (1)
// #define CFG_DEBUG_AUDIO_WITH_SDCARD     (1)
#define CFG_MONITOR_SPEED_SEND_DATA_INTERVAL        (1000) // 10 seconds
#define CFG_MONITOR_VIDEO_INTERVAL          CFG_MONITOR_SPEED_SEND_DATA_INTERVAL
#define CFG_MONITOR_AUDIO_INTERVAL          CFG_MONITOR_SPEED_SEND_DATA_INTERVAL
#define CFG_MONITOR_SPI_INTERVAL            (10*CFG_MONITOR_SPEED_SEND_DATA_INTERVAL)
#define CFG_MONITOR_SPI_SEND_DUMMY          (2*CFG_MONITOR_SPEED_SEND_DATA_INTERVAL)

/*----------------------------------------------------------------------------*/
/*                            Define for SPI                                  */
/*----------------------------------------------------------------------------*/
#define MAX_SPI_PACKET_LEN              1024
#define SPI_TRAN_SIZE                   MAX_SPI_PACKET_LEN
#define SPI_UPLOAD_RAW_MAXLEN           (MAX_SPI_PACKET_LEN-10)

#define SPI_RB_MAX_LEN                  (128*1024)
//#define SPI_DISABLE_MUTEX               (1)

// #define SPI_CC3200_READ_INT				23

#define SPI_CC3200_TRIGGER_INT          20
#define SPI_AK_TRIGGER_INT              23

#define SPI_TRANSACTION_INTERVAL		2 // ms

/*----------------------------------------------------------------------------*/
/*                            Define for Video                                */
/*----------------------------------------------------------------------------*/
#define CFG_DISPLAY_VIDEO_720P          1
#define CFG_DISPLAY_VIDEO_VGA           2
#define CFG_DISPLAY_VIDEO_INDEX         CFG_DISPLAY_VIDEO_720P // CFG_DISPLAY_VIDEO_VGA    //0:720P 1:720P 2:VGA

#define CFG_DISPLAY_SRC_WIDTH_MAX       1280
#define CFG_DISPLAY_SRC_HEIGHT_MAX      720

#define CFG_DISPLAY_SRC_WIDTH_VGA       640
#define CFG_DISPLAY_SRC_HEIGHT_VGA      360

/*
Issue low memory: Decrease len of buffer H264
*/
// #define CFG_MAX_FRAME_LEN               (CFG_DISPLAY_SRC_WIDTH_MAX*CFG_DISPLAY_SRC_HEIGHT_MAX*2)
#define CFG_MAX_FRAME_LEN               (200*1024)

#define CFG_MAX_H264_OUT_BUF_COUNT      (4)

#define CFG_BITRATE_KBPS                (300)
#define CFG_BITRATE_MBPS                (CFG_BITRATE_KBPS/1000)

#define CFG_VIDEO_FRAME_PER_SECOND      (12)
#define CFG_VIDEO_GOP                   (12) // (4)

#define CFG_VIDEO_MIN_QP                30
#define CFG_VIDEO_MAX_QP                35

#define VIDEO_MONITOR_BITRATE_INTERVAL		10  // seconds

/*----------------------------------------------------------------------------*/
/*                            Define for Audio                                */
/*----------------------------------------------------------------------------*/

/*  by the way, please also note that FM_EN and AK_EN are inverted. 
    FM_EN = low enable. SPK_EN = high enable. 
*/

// #ifdef DOORBELL_TINKLE820
// #define SPK_ENABLE                      (0)
// #define SPK_DISABLE                     (1)
// #else
#define SPK_ENABLE                      (1)
#define SPK_DISABLE                     (0)
// #endif

#define FM_ENABLE                       (0)
#define FM_DISABLE                      (1)

// #define CFG_DEBUG_ECHO_CANCELLATION     (1)
// must be fine tune acording to CFG_AUDIO_DURATION
#define AEC_FRAME_LEN                       (128)
#define AEC_FRAME_SIZE                      (AEC_FRAME_LEN * 2)
#define MIC_ONEBUF_SIZE                     (AEC_FRAME_SIZE * 4)

#define CFG_AUDIO_SAMPLE_RATE           (16000)
#define CFG_AUDIO_SAMPLE_BITS           (16)
#define CFG_AUDIO_CHANNEL_NUM           AUDIO_CHANNEL_MONO //(1)
#define CFG_AUDIO_GAIN                  (7)  // 0~7
//#define CFG_AUDIO_FRAME_SIZE            (512)

// #define CFG_AUDIO_BUF_SPI_OUT           (20*1024)
// #define CFG_AUDIO_DURATION              (56)//(64)//(32) ms
#define CFG_AUDIO_DURATION              (64) // ms
#define CFG_AUDIO_FRAME_SIZE            (CFG_AUDIO_DURATION * 8 * 2)

#define CFG_AUDIO_BUF_SPI_OUT           (CFG_AUDIO_FRAME_SIZE + 32) // for header SPI + data

#define CFG_AUDIO_G722                   (1)

#define CFG_AUDIO_MIC_GAIN_DEFAULT				(0)

#define CFG_AUDIO_LENGTH_SEND					(512) // 16 packet/second

/*----------------------------------------------------------------------------*/
/*                            Define for Talkback                             */
/*----------------------------------------------------------------------------*/
// #define CFG_DEBUG_TALKBACK_WITH_SDCARD      (1)


// #define CFG_TALKBACK_FAST_PLAY              (1)

#define CFG_TALKBACK_USE_RINGBUFFER             (1)
#define CFG_TALKBACK_G722                   (1)

#define CFG_TALKBACK_SAMPLE_RATE           (16000)
#define CFG_TALKBACK_SAMPLE_BITS           (16)
#define CFG_TALKBACK_CHANNEL_NUM           AUDIO_CHANNEL_MONO //(1)
#define CFG_TALKBACK_SPEAKER_VL            (0x05)

#define CFG_TALKBACK_BUF_MAX                (4*1024)
#define CFG_TALKBACK_RINGBUF_MAX            (4*1024)

#define CFG_PLAY_MUSIC_BUF_SIZE             (4*1024)

/*----------------------------------------------------------------------------*/
/*                            Define for Resend Video                         */
/*----------------------------------------------------------------------------*/
// #define CFG_DISABLE_RESEND                      (1)

// #define CFG_RESEND_VIDEO                        (1)
#define CFG_RESEND_MAX_PACKET_SAVE              (100) // ~3 seconds
#define CFG_RESEND_MAX_TIMEOUT                  (500)   // milisecond

/*----------------------------------------------------------------------------*/
/*                            Define for SPI Command                          */
/*----------------------------------------------------------------------------*/
#define CFG_SPI_COMMAND_ENABLE                  (1)
/*
Issue low memory: Decrease len of buffer command spi
*/
#define SPI_CMD_RB_LEN                          (16*1024)

/*----------------------------------------------------------------------------*/
/*                            Define for FIRMWARE UPGRADE                     */
/*----------------------------------------------------------------------------*/
#define SPI_FWUPGRADE_BUFFER_LEN                          (64*1024)

#define SPI_FW_TYPE_CMD                          0x15

/* Byte control */
#define FWUPG_TYPE_CMD_OFFSET                   0
#define FWUPG_CONTROL_OFFSET                    1
#define FWUPG_M2S_SEND_POS_OFFSET               2

#define SPI_FW_CTRL_INIT                            0x00
#define SPI_FW_CTRL_UPGRADING                       0x01
#define SPI_FW_CTRL_POSITION                        0x02
#define SPI_FW_CTRL_DONE_UPGRADING                  0x03
#define SPI_FW_CTRL_STOP_UPGRADING                  0x04
#define SPI_FW_CTRL_HASHING                         0x05

#define FWUPG_CTRL_HASH_TYPE_OFFSET             4
#define FWUPG_CTRL_HASH_TYPE_LEN                1 // byte
#define FWUPG_CTRL_HASH_TYPE_MD5                0x01
#define FWUPG_CTRL_HASH_TYPE_SHA1               0x02

#define FWUPG_CTRL_HASH_DIGEST_OFFSET           (FWUPG_CTRL_HASH_TYPE_OFFSET+1)

/*----------------------------------------------------------------------------*/
/*                            Define for MOTION UPLOAD                        */
/*----------------------------------------------------------------------------*/
#define UPLOADER_RUN_ON_AK_PLAT                                 (1)
#define UPLOADER_DEBUG                                          (1)
// #define UPLOADER_ENABLE                                     (1)
/*
Issue low memory: Decrease len of buffer upload and p2p+sd_stream
*/
#define SPI_UPLOADER_BUFFER_LEN                             (40*1024)
#define SPI_P2PSDSTREAM_BUFFER_LEN                          (32*1024)

#define SNAPSHOT_STATE_INIT                         (0)
#define SNAPSHOT_STATE_RUNNING                      (1)
#define SNAPSHOT_STATE_DONE                         (2)
#define SNAPSHOT_STATE_WAIT                         (3)

#define UPLOADER_STATE_INIT                         (0)
#define UPLOADER_STATE_RUNNING                      (1)
#define UPLOADER_STATE_DONE                         (2)

#define RECORD_STATE_INIT                         (0)
#define RECORD_STATE_RUNNING                      (1)
#define RECORD_STATE_DONE                         (2)

#define UPLOADER_USE_SSL                            (1)
#define UPLOADER_USE_SSL_HTTP_UPLOAD                (1)

/*----------------------------------------------------------------------------*/
/*                            Define for P2P SDCARD STREAMING                 */
/*----------------------------------------------------------------------------*/
#define P2P_SDCARD_STREAMING_DEBUG                              (1)
#define P2P_SD_STREAM_EXT                                       "flv"
#define P2P_SD_STREAM_PACKET_MAX_RAW                            (999)
#define P2P_SD_STREAM_MAX_PACKET_BACKUP                         (336)
#define P2P_SD_STREAM_PACKET_MAX_TIMEOUT                               (1300)   // milisecond
#define P2P_SD_STREAM_ACK_MAX_TIMEOUT                               (7000)   // milisecond


#define SPICMD_TYPE_VOICE_BEEP                 0x01
#define SPICMD_TYPE_VOICE_JINGLE               0x02
#define SPICMD_TYPE_ZEROPADDING                0xFF

#define SPICMD_TYPE_EN_VOICE_FAIL              0x11
#define SPICMD_TYPE_EN_VOICE_SUCCESS           0x12
#define SPICMD_TYPE_EN_VOICE_TIMEOUT           0x13

#define SPICMD_TYPE_EN_VOICE_SORRYNOTFREETOMEETNOW    0x14
#define SPICMD_TYPE_EN_VOICE_YOUCANLEAVEIT            0x15
#define SPICMD_TYPE_EN_VOICE_WElCOMECOMINGOVERNOW     0x16

//#ifdef DOORBELL_TINKLE820
//#else
#define SPICMD_TYPE_EN_VOICE_HOWCANIHELP              0x17
#define SPICMD_TYPE_EN_VOICE_SORRYNOTATHOME           0x18
#define LEN_HOWCANIHELP              18966
#define LEN_SORRYNOTATHOME           12088
//#endif

#define SPICMD_TYPE_EN_VOICE_WHOAREYOULOOKINGFOR      0x19
#define SPICMD_TYPE_EN_VOICE_UNABLETOCONNECT          0x1A
#define SPICMD_TYPE_EN_VOICE_WARNINGWIFIISWEAK        0x1B

#define SPICMD_TYPE_CH_VOICE_FAIL              0x21
#define SPICMD_TYPE_CH_VOICE_SUCCESS           0x22
#define SPICMD_TYPE_CH_VOICE_TIMEOUT           0x23

/*----------------------------------------------------------------------------*/
/*                            Define for Model                                */
/*----------------------------------------------------------------------------*/

#ifdef DOORBELL_TINKLE820
#define MODEL_ID_STR                      "003"         // Tinkle820
#define LOCAL_VERSION_STR                 "Tinkle820FM_01.01.15_invert_pin_spk_fm_enable"
#define TALKBACK_2WAY                      1
#define RECORDING_SDCARD                   1
#define SDCARD_ENABLE                      0
#define SYS_RECORD_DURATION_TIME           15

#else
#define MODEL_ID_STR                      "002"         // LaoHu Doorbell
#define LOCAL_VERSION_STR                 "LHDoorbell_01.00.54_newlib_ak_39xx_100ms"
#define TALKBACK_2WAY                      0
#define RECORDING_SDCARD                   1
#define SDCARD_ENABLE                      1
#define SYS_RECORD_DURATION_TIME           15

#endif


#define CK_STATE_SD_RECORD_START          (1)
#define CK_STATE_SD_RECORD_DONE           (2)
#define SPICMD_AK_CUT_PW                  0x20

#define SPICMD_TYPE_CH_VOICE_SORRYNOTFREETOMEETNOW    0x24
#define SPICMD_TYPE_CH_VOICE_YOUCANLEAVEIT            0x25
#define SPICMD_TYPE_CH_VOICE_WElCOMECOMINGOVERNOW     0x26
#define SPICMD_TYPE_CH_VOICE_HOWCANIHELP              0x27
#define SPICMD_TYPE_CH_VOICE_SORRYNOTATHOME           0x28

#define SPICMD_TYPE_CH_VOICE_CONNECTED                0x29
#define SPICMD_TYPE_CH_VOICE_UNABLETOCONNECT          0x2A
#define SPICMD_TYPE_CH_VOICE_WARNINGWIFIISWEAK        0x2B

//#define LEN_DING      12330
//#define LEN_SUCCESS   23246
//#define LEN_FAIL      19218
//#define LEN_TIMEOUT   22942

#define LEN_DING      6986
#define LEN_SUCCESS   28496
#define LEN_FAIL      26632
#define LEN_TIMEOUT   27326
#define LEN_PROGRESS  35444
#define LEN_PAIRING   31272



#define LEN_CHSUCCESS    13996
#define LEN_CHFAIL       13686
#define LEN_CHTIMEOUT    14686


//#define LEN_SORRYNOTFREETOMEETNOW    41696
#define LEN_SORRYNOTFREETOMEETNOW    56068
#define LEN_YOUCANLEAVEIT            45302
#define LEN_WElCOMECOMINGOVERNOW     46868
#define LEN_WHOAREYOULOOKINGFOR      31026

#define LEN_CH_SORRYNOTFREETOMEETNOW    46770
#define LEN_CH_YOUCANLEAVEIT            37734
#define LEN_CH_WElCOMECOMINGOVERNOW     34088
#define LEN_CH_HOWCANIHELP              41066
#define LEN_CH_SORRYNOTATHOME           42218

#define LEN_EN_CONNECTED                18852
#define LEN_EN_UNABLETOCONNECT          24704
#define LEN_EN_WARNINGWIFIISWEAK        46856
#define LEN_BEEP                        6244

#endif /* _GM_CONFIG_H_ */
