#ifndef JSON_UTILS_H_
#define JSON_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define JSMN_HEADER
#include "jsmn.h"  // declares only


#define MAX_JSON_TOKENS 64  // Maximum number of JSON tokens expected per command


typedef enum {
    JSON_PARSE_OK = 0,         /**< Key found and value parsed successfully */
    JSON_PARSE_KEY_NOT_FOUND,  /**< Key not present in JSON */
    JSON_PARSE_INVALID_FORMAT, /**< Value present but could not be parsed */
    JSON_PARSE_VALUE_TOO_LONG  /**< Value string too long for buffer */
} JsonParseStatus_t;

bool json_token_streq(const char *js, const jsmntok_t *tok, const char *s);
bool json_parse_u16(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint16_t *out);
bool json_parse_u32(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint32_t *out);
bool json_parse_array_u32(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint32_t *out, size_t max_len, size_t *parsed_len);
bool json_parse_array_u16(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint16_t *out, size_t max_len, size_t *parsed_len);

#endif // JSON_UTILS_H_
