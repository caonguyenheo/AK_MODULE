/**
  ******************************************************************************
  * File Name          : json_handle.c
  * Description        : This file contain functions for build json
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 NXCOMM PTE LTD
  
  *
  ******************************************************************************
  */


/*
********************************************************************************
*                            INCLUDED FILES                                    *
********************************************************************************
*/
#include <string.h>
#include "json.h"
#include "json_handle.h"
#include "connect.h"

/*
********************************************************************************
*                            DEFINITIONS                                       *
********************************************************************************
*/

static char *g_astrKeySearch[] =
{
    //KEY_DATA_RESPONSE,
    //KEY_STORAGE_ID,
    //KEY_STORAGE_DATA,
    KEY_X_AMZ_DATE,                 // 1
    KEY_X_AMZ_ALGORITHM,            // 2
    KEY_X_AMZ_META_UUID,            // 3
    KEY_SNAPSHOT_X_AMZ_SIGNATURE,   // 4
    KEY_CLIP_X_AMZ_SIGNATURE,       // 5
    KEY_X_AMZ_CREDENTIAL,           // 6
    KEY_SNAPSHOT_URL,               // 7
    KEY_ACL,                        // 8
    KEY_X_AMZ_SERVER_SIDE_ENCRYPT,  // 9
    KEY_SNAPSHOT_POLICY,            // 10
    KEY_LOG_URL,                    // 11
    KEY_CLIP_URL,                   // 12
    KEY_CLIP_POLICY,                // 13
    KEY_SUCCESS_ACTION_REDIRECT,    // 14
    KEY_KEY_PATH                    // 15
    //KEY_MESSAGE,
    //KEY_STATUS
};


/*
********************************************************************************
*                   GLOBAL FUNCTIONS/PROCEDURES                                *
********************************************************************************
*/

/*
* Function Name  : add_element_json
* Description    : build a string json with key and value for root
* Input          : key: string of key
* Input          : value: string of value
* Ouput          : output string json root
* Return         :  -1 if parameter is invalid
*                   -2 if mkstring json failed
*                   0 if success
*/
static int add_string_element_json(JsonNode *root, char *key, char *value)
{
    JsonNode *node = NULL;
    // char *key_json = NULL;
    // int len_key = 0;
    if(!root || !key || !value){
        dzlog_error("Argument null!");
        return -1;
    }
    node  = json_mkstring(value);
    if(!node){
        return -2;
    }
    json_append_member(root, key, node);
    // len_key = strlen(key);
    // key_json = (char *)malloc(len_key + 2);
    // if(!key_json){
    //     return -2;
    // }

    // *(key_json + 0) = FIRST_CHAR_CODE;
    // *(key_json + 1) = SECOND_CHAR_CODE;
    // memcpy(key_json + 2, key, len_key);
    // json_append_member(root, key_json, node);
    return 0;
}

static int add_number_element_json(JsonNode *root, char *key, double value)
{
    JsonNode *node = NULL;
    //char *key_json = NULL;
    //int len_key = 0;
    if(!root || !key){
        dzlog_error("Argument null!");
        return -1;
    }
    node  = json_mknumber(value);
    if(!node){
        return -2;
    }
    json_append_member(root, key, node);
    // len_key = strlen(key);
    // key_json = (char *)malloc(len_key + 2);
    // if(!key_json){
    //     return -2;
    // }

    // *(key_json + 0) = FIRST_CHAR_CODE;
    // *(key_json + 1) = SECOND_CHAR_CODE;
    // memcpy(key_json + 2, key, len_key);
    // json_append_member(root, key_json, node);
    return 0;
}

/*
* Function Name  : add_registration_id
* Description    : build json for registration_id from udid
* Input          : char udid
* Ouput          : output string json root
* Return         :  -1 if parameter is invalid
*                   -2 if mkstring json failed
*                   0 if success
*/
static int add_registration_id(JsonNode *root, char *udid)
{
    int ret = 0;
    if(!root || !udid){
        dzlog_error("%s parameter invalid.", __func__);
        return -1;
    }
    // TODO: check length of udid or so...on
    ret = add_string_element_json(root, KEY_REGISTRATION_ID, udid); 
    return ret;
}

/*
* Function Name  : add_alert_type
* Description    : build json for alert type (motion)
* Input          : int alert type
* Ouput          : output string json root
* Return         :  -1 if parameter is invalid
*                   0 if success
*/
static int add_alert_type(JsonNode *root, double alert_type)
{
    int ret = 0;
    JsonNode *node = NULL;

    if(!root){
        dzlog_error("%s parameter invalid.", __func__);
        return -1;
    }
    // TODO: check alert type motion (4)
    ret = add_number_element_json(root, KEY_ALERT_TYPE, alert_type); 
    return ret;
}

/*
* Function Name  : add_auth_token
* Description    : build json for add_auth_token
* Input          : char type of alert
* Ouput          : output string json root
* Return         :  -1 if parameter is invalid
*                   -2 if mkstring json failed
*                   0 if success
*/
static int add_auth_token(JsonNode *root, char *auth_token)
{
    int ret = 0;
    if(!root || !auth_token){
        dzlog_error("%s parameter invalid.", __func__);
        return -1;
    }
    // TODO: Get token from server
    ret = add_string_element_json(root, KEY_AUTH_TOKEN, auth_token); 
    return ret;
}

/*
* Build json for get event_add
*/
/*
* Function Name  : build_input_json_post_event_add
* Description    : build input json for POST method EVENT ADD
* Input          : 
* Ouput          :
* Return         :  -1 if parameter is invalid
*                   -2 if mkstring json failed
*                   0 if success
*/
char * build_input_json_post_event_add(char *udid, char *alert_type, 
                                                    char *auth_token)
{
    int rc = 0;
    JsonNode * root;
    char * res = NULL;
    root = json_mkobject(); 

    /* UDID */
    rc = add_registration_id(root, VALUE_REG_ID);
    if (rc != 0){
        dzlog_error("Error adding fw version param");
        goto done;
    }

    /* ALERT TYPE */
    rc = add_alert_type(root, VALUE_ALERT_TYPE);
    if (rc != 0){
        dzlog_error("Error attributes param");
        goto done;
    }

    /* AUTHENTICATION TOKEN */
    rc = add_auth_token(root, VALUE_AUTH_TOKEN);
    if (rc != 0){
        dzlog_error("Error locations param");
        goto done;
    }
    res = json_encode(root);
done:
    /* Remove allocated memory */
    json_delete(root); 
    return res;
}

char *jh_find_brace(char *str)
{
    char *ret = NULL;
    if(!str){
        return NULL;
    }
    ret = strstr(str, "{");
    return ret;
}

/*
* Program need to call this function to get main data
*/
JsonNode *jh_get_json_node_coredata(char *str_json_response)
{
    JsonNode *j_data = NULL;
    JsonNode *json_dec = NULL;

    if(!str_json_response){
        return NULL;
    }
    json_dec = json_decode(str_json_response);
    if(!json_dec){
        debug("json_dec null!");
        return NULL;
    }
    debug("Json Decode OK!");
    // parse key "data"
    j_data = json_find_member(json_dec, KEY_DATA_RESPONSE);
    if(!j_data){
        return NULL;
    }
    debug("Find data!");
    j_data = json_find_member(j_data, KEY_STORAGE_DATA);
    if(!j_data){
        return NULL;
    }
    debug("Find storage_data!");
    return j_data;
}

/*
* Wrapper: find info by searching json
*/
int jh_find_data_by_key(JsonNode *json_core_data, char *key, char *info)
{
    int ret = 0;
    JsonNode *jtmp = NULL;
    if(!json_core_data || !key || !info){
        debug("Argument null!");
        return -1;
    }
    debug("json_find_member %s\r\n", key);
    jtmp = json_find_member(json_core_data, key);
    if(!jtmp){
        debug("Cannot find %s\r\n", key);
        return -1;
    }
    memcpy(info, jtmp->string_, strlen(jtmp->string_));
    return ret;
}



int jh_parse_response(char *json_string)
{
    int ret = 0;
    int i = 0;
    JsonNode *json_dec = NULL;
    JsonNode *j_data = NULL;
    JsonNode *j_data_parse = NULL;
    JsonNode *jtmp = NULL;

    if(!json_string){
        return -1;
    }
    debug("The 1st element: %c", *json_string);
    json_dec = json_decode(json_string);
    if(!json_dec){
        debug("ERROR: json_decode failed!");
        return -2;
    }
    // parse key "data"
    j_data = json_find_member(json_dec, KEY_DATA_RESPONSE);
    if(!j_data){
        return -3;
    }
    debug("Find data!");

    // parse key "storage_id"
    // j_data = json_find_member(j_data, KEY_STORAGE_ID);
    // if(!j_data){
    //     return -4;
    // }
    // debug("Find storage_id!");

    // parse key "storage_data"
    j_data = json_find_member(j_data, KEY_STORAGE_DATA);
    if(!j_data){
        return -5;
    }
    debug("Find storage_data!");

    #if 1
    for(i = 0; i < JH_SIZE_ARRAY_KEY; i++){
        j_data_parse = j_data;
        jtmp = json_find_member(j_data_parse, g_astrKeySearch[i]);
        if(!jtmp){
            dzlog_info("Cannot find [Key]%s", g_astrKeySearch[i]);
        }
        else{
            printf("\x1b[32m[KEY]:%s\x1b[0m\n\x1b[34m[VAL]\x1b[0m %s\r\n", g_astrKeySearch[i], jtmp->string_);
        }
    }
    #endif
    return ret;
}

/*
********************************************************************************
*                               END OF FILES                                   *
********************************************************************************
*/
