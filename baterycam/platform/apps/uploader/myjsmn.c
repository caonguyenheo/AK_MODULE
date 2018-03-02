#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// json parsing
#include "jsmn.h"

#define FAILED    (-1)
#define SUCCESS   0

static int32_t
_jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

static int32_t
_json_get_string_value(const char *json, jsmntok_t *tok, const char *key, char *value, uint32_t valLen) {
  if (_jsoneq(json, tok, key) == 0 && tok[1].type == JSMN_STRING) {
    uint32_t len = tok[1].end-tok[1].start;
    // check len
    if (len >= valLen)
      return -1;
    strncpy(value, (char*)(json+ tok[1].start), len);
    (value)[len] = '\0';
    // UART_PRINT("%s: %s\n", key, *value);
    return 0;
  }
  return -1;
}

int32_t parseJson_customized(char *json,
  char *inputStr, char* outputStr, int outputStr_Len)
{
  int i;
  int r;
  jsmn_parser par;
  jsmntok_t tok[256]; /* We expect no more than 256 tokens */

  jsmn_init(&par);
  r = jsmn_parse(&par, json, strlen(json), tok, sizeof(tok)/sizeof(tok[0]));
  if (r < 0) {
    printf("Failed to parse JSON: %d\n", r);
    return -1;
  }
  // Assume the top-level element is an object 
  if (r < 1 || tok[0].type != JSMN_OBJECT) {
    printf("Object expected\n");
    return -1;
  }
  // Loop over all keys of the root object 
  for (i = 0; i < r; i++) {
      if (_json_get_string_value(json, &tok[i], inputStr, outputStr, outputStr_Len) == 0) {
          return 0;
      }
  }

  return -1;
}
