/**
  ******************************************************************************
  * File Name          : json_handle.h
  * Description        : This file contains all config of json_handle 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
 
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __JSON_HANDLE_H__
#define __JSON_HANDLE_H__
#ifdef __cplusplus
 extern "C" {
#endif

/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <stdint.h> 
#include "upload_main.h"
/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/
/* Define for POST event/add, get info from cinatic to upload file */
// Trung: hardcode test

#define JH_SIZE_ARRAY_KEY       (15)

#define VALUE_REG_ID                "000002F4B85ECC61B0INEACK"
#define VALUE_ALERT_TYPE            1
#define VALUE_AUTH_TOKEN            "de2397ff6e2a3a5abcfd221ed5a6d412"

#define KEY_REGISTRATION_ID         "registration_id"
#define KEY_ALERT_TYPE              "alert"
#define KEY_AUTH_TOKEN              "auth_token"

/* Define key for response */
#define KEY_DATA_RESPONSE               "data"
#define KEY_STORAGE_ID                  "storage_id"
#define KEY_STORAGE_DATA                "storage_data"
#define KEY_X_AMZ_DATE                  "X-Amz-Date"
#define KEY_X_AMZ_ALGORITHM             "X-Amz-Algorithm"
#define KEY_X_AMZ_META_UUID             "x-amz-meta-uuid"
#ifdef SERVER_CEPHS3
#define KEY_SNAPSHOT_X_AMZ_SIGNATURE    "SnapShot-Signature"
#else
#define KEY_SNAPSHOT_X_AMZ_SIGNATURE    "SnapShot-X-Amz-Signature"
#endif
#define KEY_CLIP_X_AMZ_SIGNATURE        "Clip-X-Amz-Signature"
#define KEY_X_AMZ_CREDENTIAL            "X-Amz-Credential"
#define KEY_SNAPSHOT_URL                "snapshot_url"
#define KEY_ACL                         "acl"
#define KEY_X_AMZ_SERVER_SIDE_ENCRYPT   "x-amz-server-side-encryption"
#define KEY_SNAPSHOT_POLICY             "SnapShot-Policy"
#define KEY_LOG_URL                     "log_url"
#define KEY_CLIP_URL                    "clip_url"
#define KEY_CLIP_POLICY                 "Clip-Policy"
#define KEY_SUCCESS_ACTION_REDIRECT     "success_action_redirect"
#define KEY_KEY_PATH                    "key"
#define KEY_MESSAGE                     "message"
#define KEY_STATUS                      "status"
#define KEY_AWSACCESSKEYID              "AWSAccessKeyId"

#define FIRST_CHAR_CODE                 0x0A
#define SECOND_CHAR_CODE                0x09


/*
********************************************************************************
*                            GLOBAL PROTOTYPES                                 *
********************************************************************************
*/
char * build_input_json_post_event_add(char *udid, char *alert_type, 
                                                    char *auth_token);
JsonNode *jh_get_json_node_coredata(char *str_json_response);
int jh_find_data_by_key(JsonNode *json_core_data, char *key, char *info);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_HANDLE_H__ */
/*
********************************************************************************
*                           END OF FILES                                       *
********************************************************************************
*/

