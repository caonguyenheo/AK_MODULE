#ifndef __URL_PARSER_H__
#define __URL_PARSER_H__

// ERROR_CODE
#define      ERROR_NONE                 0
#define      ERROR_UNKNOWN              -1
#define      ERROR_OUT_OF_BUFFER        -2

#define      PARAMS_SPLITTER            "&"
#define      PARAMS_EQUAL               "="

#define MAX_TOKEN_NUM                   10
#define URL_BUFFER_SIZE                 256


typedef struct url_segment_t {
    char *pHead;
    char *pTail;
}url_segment;

typedef struct url_token_t {
    url_segment key;
    url_segment value;
}url_token;

typedef struct url_parser_t {
    int num_token;
    char *pChUrl;
    url_token parsed_token[MAX_TOKEN_NUM];
} url_parser;

int parse_url(char *req, url_parser *parsed_url);
int getValueFromKey(const url_parser *parsed_url, const char *inKey, char *outVal, int buf_sz);
#endif // __URL_PARSER_H__
