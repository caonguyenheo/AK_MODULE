#include <string.h>
#include "url_parser.h"

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static int requestParser(char *req, url_token *parsed_req, int *num_parsed_req, int max_token);
static char *endString(char *str);

//*****************************************************************************
//                      FUNCTION IMPLEMENT
//*****************************************************************************
int parse_url(char *req, url_parser *parsed_url) {
	int num_parsed_req = 0;
	int ret_val;
	if (req == NULL || parsed_url == NULL) {
		ret_val = -1;
		goto exit;
	}
	ret_val = requestParser(req, parsed_url->parsed_token, &num_parsed_req, MAX_TOKEN_NUM);
	if (ret_val < 0) {
		goto exit;
	}
	parsed_url->num_token = num_parsed_req;
	parsed_url->pChUrl = req;

exit:
	return ret_val;
}

static int requestParser(char *req, url_token *parsed_req, int *num_parsed_req, int max_token) {
	char *pChReq = NULL;
	char *pEqualChar = NULL;
	char *pTokenChar = NULL;

	char *pChStart = NULL;
	char *pChStop = NULL;

	int ret_val = 0;

//	UART_PRINT("DEBUG_REQUEST_PARSER  requestParser\r\n");

	/* Check input */
	if (req == NULL || parsed_req == NULL) {
		ret_val = -1;
		goto exit;
	}
//	UART_PRINT("DEBUG_REQUEST_PARSER  requestParser passed input\r\n");

	/* Parser */
	pChReq = req;
	*num_parsed_req = 0;
	while (pChReq != NULL && pChReq != '\0' && *num_parsed_req < max_token) {

//		UART_PRINT("DEBUG_REQUEST_PARSER  requestParser parse num=%d\r\n", *num_parsed_req);
		// Find Token Char
		pTokenChar = strstr(pChReq, PARAMS_SPLITTER);

		// Cannot find "&" char check if there is exist "=" char
		if (pTokenChar == NULL) {
			pEqualChar = strstr(pChReq, PARAMS_EQUAL);
			// Cannot find "=" char
			if (pEqualChar == NULL) {
				//UART_PRINT("DEBUG_REQUEST_PARSER  requestParser failed\r\n");
				ret_val = -1;
				goto exit;
			}
			// UART_PRINT("DEBUG_REQUEST_PARSER  requestParser the last one\r\n");
			// Key from begin to equal char
			parsed_req[*num_parsed_req].key.pHead = pChReq;
			parsed_req[*num_parsed_req].key.pTail = pEqualChar;
			// Value the rest of string
			parsed_req[*num_parsed_req].value.pHead = pEqualChar + 1;
			parsed_req[*num_parsed_req].value.pTail = (char *)endString(pEqualChar);
			(*num_parsed_req)++;
			goto exit;
		}

		pEqualChar = strstr(pChReq, PARAMS_EQUAL);

		// This is an invalid case, cannot find any "=" char
		if (pEqualChar == NULL) {
			//UART_PRINT("DEBUG_REQUEST_PARSER  requestParser cannot find = case\r\n");
			ret_val = -1;
			goto exit;
		}
		// This is an invalid case, current token lack of "=" char, continue with next token
		if (pEqualChar > pTokenChar) {
			//UART_PRINT("DEBUG_REQUEST_PARSER  requestParser find = but invalid case\r\n");
			goto next_token;
		}

		//UART_PRINT("DEBUG_REQUEST_PARSER  requestParser normal case\r\n");

		// Key from begin to equal char
		parsed_req[*num_parsed_req].key.pHead = pChReq;
		parsed_req[*num_parsed_req].key.pTail = pEqualChar;
		// Value from equal char +1 to token char
		parsed_req[*num_parsed_req].value.pHead = pEqualChar + 1;
		parsed_req[*num_parsed_req].value.pTail = pTokenChar;
		// Increase number of parsed token
		(*num_parsed_req)++;

next_token:
		pChReq = pTokenChar + 1;
	}

exit:
	return 0;
}

static char *endString(char *str) {
	char * pStr = NULL;
	pStr = str;
	while (*pStr != '\0') {
		pStr++;
	}
	return pStr;
}

int getValueFromKey(const url_parser *parsed_url, const char *inKey, char *outVal, int buf_sz) {
	int i;
	int strLen;
	int valueLen;
	int numToken;
	const url_token *inToken;

	inToken = parsed_url->parsed_token;
	numToken = parsed_url->num_token;
	for (i = 0; i < numToken; i++) {
		strLen = inToken[i].key.pTail - inToken[i].key.pHead;
		if (strLen == strlen(inKey)) {
			if (memcmp(inToken[i].key.pHead, inKey, strLen) == 0) {
				valueLen = inToken[i].value.pTail - inToken[i].value.pHead;
				if (valueLen > buf_sz) {
					return ERROR_OUT_OF_BUFFER;
				}
				memcpy(outVal, inToken[i].value.pHead, valueLen);
				outVal[inToken[i].value.pTail - inToken[i].value.pHead] = '\0';
				return ERROR_NONE;
			}
		}
	}

	return ERROR_UNKNOWN;
}

