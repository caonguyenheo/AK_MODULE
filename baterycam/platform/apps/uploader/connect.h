#ifndef CONNECT_H
#define CONNECT_H

#include <netdb.h> // on linux
#include <unistd.h>

// #include <buffer.h>
#include "buffer.h"
#include "dbg.h"
#include "upload_main.h"

#define RECV_SIZE 1024
#define BUF_SIZE  RECV_SIZE + 1

#define HTTP_TEST_ON_LINUX              1
//#define SERVER_LOCAL                      1
#define HTTP_UPLOAD_MAX_FILENAME_LEN (255+1)

#ifdef HTTP_TEST_ON_LINUX
#define dzlog_info          debug
#define dzlog_debug         debug
#define dzlog_warn          debug
#define dzlog_error         debug
#endif


/*---------------------------------------------------------------------------*/
#define CON_FILE_EXTENTION_SNAPSHOT         ".jpg"
#define CON_FILE_EXTENTION_CLIP             ".mp4"

#define CON_CONNECTION_TYPE                 "keep-alive"
#define CON_SIZE_KEY_SNAP                   (10)
#define CON_SIZE_KEY_CLIP                   (10)
// #define CON_MAC_ADDRESS                     "74DAEA813043"
#define CON_MAC_ADDRESS                     "F4B85ECD1AB0"
#define POSTFIELD_POLICY                    "Policy"

#ifdef SERVER_CEPHS3
#define POSTFIELD_SIGNATURE                 "Signature"
#else
#define POSTFIELD_SIGNATURE                 "X-Amz-Signature"
#endif

#define POSTFIELD_META_TAG                  "x-amz-meta-tag"
#define POSTFIELD_NAME_CONTENT_TYPE_SNAP        "Content-Type"
#define POSTFIELD_VALUE_CONTENT_TYPE_SNAP       "image/jpeg"

#define POSTFIELD_NAME_CONTENT_TYPE_CLIP        "Content-Type"
#define POSTFIELD_VALUE_CONTENT_TYPE_CLIP       "video/mp4"

#define POSTFILED_STR_CACHE_CONTROL             "Cache-Control: max-age=0\r\n"
#define POSTFILED_STR_ORIGIN                    "Origin: null\r\n"
#define POSTFILED_STR_UPGRADE                   "Upgrade-Insecure-Requests: 1\r\n"

#define POST_REQUEST_HEADER_SIZE            (3072)
#define POSTFIELD_POST_X                    "POST %s HTTP/1.1\r\n"
#define POSTFIELD_HOST_X                    "Host: %s\r\n"
#define POSTFIELD_CONNECTION_X              "Connection: %s\r\n"
#define POSTFIELD_CONTENT_LENGTH_X          "Content-Length: %d\r\n"
#define POSTFIELD_USER_AGENT_X              "User-Agent: %s\r\n"
#define POSTFIELD_CONTENT_TYPE_FORM_DATA_X            \
            "Content-Type: multipart/form-data; boundary=" BOUNDARY_PREFIX_FIRST "%s\r\n"
//#define POSTFIELD_ACCEPT                    "Accept: */*\r\n"
#define POSTFIELD_ACCEPT                    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
#define POSTFIELD_ACCEPT_CODING             "Accept-Encoding: gzip, deflate\r\n"
#define POSTFIELD_ACCEPT_LANGUAGE           \
                                    "Accept-Language: en-US,en;q=0.8\r\n\r\n"

#define POSTFIELD_CONTENT_DISPOSITION_X     \
                        "Content-Disposition: form-data; name=\"%s\"\r\n\r\n"

#define POSTFIELD_CONTENT_DISPOSITION_VALUE                         "%s\r\n"

/* For snapshot */
#define POSTFIELD_CONTENT_DISPOSITION_FILE  \
        "Content-Disposition: form-data; name=\"file\"; filename=\"%s.jpg\"\r\n"
#define POSTFIELD_CONTENT_TYPE_SNAPSHOT         "Content-Type: image/jpeg\r\n\r\n"
/* For clip */
#define POSTFIELD_CONTENT_DISPOSITION_FILE_CLIP  \
        "Content-Disposition: form-data; name=\"file\"; filename=\"%s.mp4\"\r\n"
#define POSTFIELD_CONTENT_TYPE_CLIP         "Content-Type: application/octet-stream\r\n\r\n"

#define POSTFIELD_CONTENT_DISPOSITION_KEYPATH_VALUE_CLIP       "%s/%s/${filename}\r\n"


#define BOUNDARY_PREFIX_FIRST                     "----"
#define BOUNDARY_PREFIX                     "------"
#define BOUNDARY_SUFFIX                     "--"

#define POSTFIELD_BOUNDARY_FIRST                  BOUNDARY_PREFIX_FIRST "%s\r\n"
#define POSTFIELD_BOUNDARY                  BOUNDARY_PREFIX "%s\r\n"
#define POSTFIELD_BOUNDARY_END              BOUNDARY_PREFIX "%s" BOUNDARY_SUFFIX "\r\n"

#define FIELD_SUBMIT                        "submit"

#ifndef SERVER_CEPHS3
#define BOUNDARY_HARD_CODE                  "WebKitFormBoundarytDB7BtAb3nX61Q"
#else
#define BOUNDARY_HARD_CODE                  "WebKitFormBoundaryx4BTAJG6yMrwlwvy"
#endif

#define BOUNDARY_SIZE_STR                   (48)

#define CHECK_ERROR(R)                      do{ \
                                                if(R < 0){ \
                                                return -1; } \
                                            }while(0)


#define STR_UPLOAD_SNAP_OK_HEAD         "HTTP/1.1"
#define STR_UPLOAD_SNAP_OK_CONTENT      "No Content"
#define STR_UPLOAD_SNAP_OK_LOCATION     "Location:"  

#define HTTP_STT_NUMBER                                 10
#define HTTP_STT_CODE_SUCCESS_200_OK                    "200"
#define HTTP_STT_CODE_SUCCESS_201_CREATED               "201"
#define HTTP_STT_CODE_SUCCESS_202_ACCEPTED              "202"
#define HTTP_STT_CODE_SUCCESS_203_NON_AUTH              "203"
#define HTTP_STT_CODE_SUCCESS_204_NO_CONTENT            "204"
#define HTTP_STT_CODE_SUCCESS_205_RESET_CONTENT         "205"
#define HTTP_STT_CODE_SUCCESS_206_PARTIAL_CONTENT       "206"
#define HTTP_STT_CODE_SUCCESS_207_MULTI_STATUS          "207"
#define HTTP_STT_CODE_SUCCESS_208_ALREADY_REPORTED      "208"
#define HTTP_STT_CODE_SUCCESS_226_IM_USED               "226"

#define END_PATTERN_PARSE_SUCCESS_INFO      "&"
#define CON_NUMBER_OF_PATTERN               4
#define CON_SIZE_STRING_ELEMENT_INFO        256
#define PATTERN_STR_EVENT_ID                "event_id="
#define PATTERN_STR_UPDATE_DATE             "update_date="
#define PATTERN_STR_TOKEN                   "token="
#define PATTERN_STR_REG_ID                  "registration_id="
#define PATTERN_STR_FILE_NAME               "file_name="
#define PATTERN_STR_FILE_TYPE               "file_type="



typedef struct CON_HEADER_SNAP{
    char *req_path;
    char *host_name;
    char *connect_type; // Keep-Alive or Close
    char *usr_agent;
    char *boundary;
    int total_length;
}CON_HEADER_SNAP_t;

typedef struct CON_TAIL_SNAP{
    char *boundary;
    char *submit;
}CON_TAIL_SNAP_t;

typedef struct CON_UPLOAD_SUCCESS_INFO{
    char *event_id;
    char *udid;
    char *update_date;
    char *file_name;
    char *token;
    int file_type;  //file type 0: none, 1 :snap, 2:clip, 3:log
}CON_UPLOAD_SUCCESS_INFO_t;
/*---------------------------------------------------------------------------*/

// #ifdef HTTP_TEST_ON_LINUX
// int make_connection(struct addrinfo *res);
// #else
// int init_connection(char *hostname, char *port);
// #endif
int init_connection(char *hostname, char *port, int protocol);


int make_request(int sockfd, char *hostname, char *request_path);
int fetch_response(int sockfd, Buffer **response, int recv_size);

char *
build_header_post_request(char *hostname, char *request_path,
                                char *sFileName, unsigned int u32LenFile);
char *
build_body_post_request(char *sFileName, char *data, int i32Len);
char *
build_tail_post_request(void);

char *con_find_json_response(char *response);
char *build_request_upload_snapshot(char *req_path, char *host_name, char *response, int *len_stream, char* l_filename, char* l_file_ext, char* g_generated_filename, int g_generated_filename_len);

int uploading_parse_repsonse(char *response, int *http_status_code, char *location);
/*
* After getting response form http POST event add and upload snapshot, 
* this function is needed to call
* This will parse and store [success_action_redirect]
*/
int parse_info_success_upload(char *response, int type);
int make_request_event_upload_snap(int sockfd, char *hostname, char *request_path, char *port);
int send_event_success_parse_repsonse(char *response);


#endif //CONNECT_H
