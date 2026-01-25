/**
 * @file json_utils.c
 * @brief Utilities for parsing simple JSON key-value and array pairs using JSMN.
 *
 * This module provides basic functions to:
 *  - Parse a uint16_t and uint32_t value from a JSON object.
 *  - Parse an array of uint32_t or uint16_t values from a JSON object.
 *  - Match a token string against a key.
 *
 * Note:
 *  - Only flat (non-nested) JSON objects are supported.
 *  - Assumes key names and values are simple (no complex structures).
 */

#include <string.h>
#include <stdlib.h>
#include "jsmn.h"
#include "json_utils.h"


/**
 * @brief Compare a JSON token against a string.
 *
 * @param[in] js   Pointer to JSON string.
 * @param[in] tok  Pointer to token to compare.
 * @param[in] s    Null-terminated string to match.
 *
 * @return true if token matches string, false otherwise.
 */
bool json_token_streq(const char *js, const jsmntok_t *tok, const char *s) {
    return (int)strlen(s) == (tok->end - tok->start) &&
           strncmp(js + tok->start, s, (size_t)(tok->end - tok->start)) == 0;
}

/**
 * @brief Parse a uint16_t value from a JSON key.
 *
 * @param[in]  js       Pointer to JSON string.
 * @param[in]  tokens   Array of parsed tokens.
 * @param[in]  tokcount Number of tokens.
 * @param[in]  key      Key name to search for.
 * @param[out] out      Pointer to output uint16_t variable.
 *
 * @return true if the key is found and parsed successfully, false otherwise.
 */
bool json_parse_u16(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint16_t *out) {
    for (int i = 1; i < tokcount; i++) {
        if (tokens[i].type == JSMN_STRING && json_token_streq(js, &tokens[i], key)) {
            jsmntok_t *val_tok = &tokens[i + 1];

            // Buffer for value extraction (ensure null-termination)
            char buf[16];
            int len = val_tok->end - val_tok->start;

            if (len >= (int)sizeof(buf)) return false;  // Value too long
            strncpy(buf, js + val_tok->start, (size_t)len);
            buf[len] = '\0';

            *out = (uint16_t)atoi(buf);
            return JSON_PARSE_OK;
        }
    }
    return JSON_PARSE_KEY_NOT_FOUND;
}

/**
 * @brief Parse a uint32_t value from a JSON key.
 *
 * @param[in]  js       Pointer to JSON string.
 * @param[in]  tokens   Array of parsed tokens.
 * @param[in]  tokcount Number of tokens.
 * @param[in]  key      Key name to search for.
 * @param[out] out      Pointer to output uint32_t variable.
 *
 * @return true if the key is found and parsed successfully, false otherwise.
 */
bool json_parse_u32(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint32_t *out) {
    for (int i = 1; i < tokcount; i++) {
        if (tokens[i].type == JSMN_STRING && json_token_streq(js, &tokens[i], key)) {
            jsmntok_t *val_tok = &tokens[i + 1];

            char buf[20];  // uint32_t max value is 10 digits, 20 gives enough margin
            int len = val_tok->end - val_tok->start;

            if (len >= (int)sizeof(buf)) return false;  // Buffer too small
            strncpy(buf, js + val_tok->start, (size_t)len);
            buf[len] = '\0';

            *out = (uint32_t)strtoul(buf, NULL, 10);
            return JSON_PARSE_OK;
        }
    }
    return JSON_PARSE_KEY_NOT_FOUND;
}


/**
 * @brief Parse an array of uint32_t values from a JSON key.
 *
 * @param[in]  js         Pointer to JSON string.
 * @param[in]  tokens     Array of parsed tokens.
 * @param[in]  tokcount   Number of tokens.
 * @param[in]  key        Key name to search for.
 * @param[out] out        Output array for parsed uint32_t values.
 * @param[in]  max_len    Maximum number of elements to parse.
 * @param[out] parsed_len Number of successfully parsed elements.
 *
 * @return true if parsing succeeds, false otherwise.
 */
bool json_parse_array_u32(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint32_t *out, size_t max_len, size_t *parsed_len) {
    *parsed_len = 0;
    for (int i = 1; i < tokcount; i++) {
        if (tokens[i].type == JSMN_STRING && json_token_streq(js, &tokens[i], key)) {
            jsmntok_t *arr_tok = &tokens[i + 1];

            if (arr_tok->type != JSMN_ARRAY) return false;

            int count = arr_tok->size;
            if ((size_t)count > max_len) return false;  // Exceeds output buffer size

            for (int j = 0; j < count; j++) {
                jsmntok_t *val_tok = &tokens[i + 2 + j];

                char buf[16];
                int len = val_tok->end - val_tok->start;
                if (len >= (int)sizeof(buf)) return false;

                strncpy(buf, js + val_tok->start, (size_t)len);
                buf[len] = '\0';

                out[j] = (uint32_t)strtoul(buf, NULL, 10);
            }
            *parsed_len = (size_t)count;
            return JSON_PARSE_OK;
        }
    }
    return JSON_PARSE_KEY_NOT_FOUND;
}

/**
 * @brief Parse an array of uint16_t values from a JSON key.
 *
 * @param[in]  js         Pointer to JSON string.
 * @param[in]  tokens     Array of parsed tokens.
 * @param[in]  tokcount   Number of tokens.
 * @param[in]  key        Key name to search for.
 * @param[out] out        Output array for parsed uint16_t values.
 * @param[in]  max_len    Maximum number of elements to parse.
 * @param[out] parsed_len Number of successfully parsed elements.
 *
 * @return true if parsing succeeds, false otherwise.
 */
bool json_parse_array_u16(const char *js, jsmntok_t *tokens, int tokcount, const char *key, uint16_t *out, size_t max_len, size_t *parsed_len) {
    *parsed_len = 0;
    for (int i = 1; i < tokcount; i++) {
        if (tokens[i].type == JSMN_STRING && json_token_streq(js, &tokens[i], key)) {
            jsmntok_t *arr_tok = &tokens[i + 1];

            if (arr_tok->type != JSMN_ARRAY) return false;

            int count = arr_tok->size;
            if ((size_t)count > max_len) return false;

            for (int j = 0; j < count; j++) {
                jsmntok_t *val_tok = &tokens[i + 2 + j];

                char buf[16];
                int len = val_tok->end - val_tok->start;
                if (len >= (int)sizeof(buf)) return false;

                strncpy(buf, js + val_tok->start, (size_t)len);
                buf[len] = '\0';

                out[j] = (uint16_t)atoi(buf);
            }
            *parsed_len = (size_t)count;
            return JSON_PARSE_OK;
        }
    }
    return JSON_PARSE_KEY_NOT_FOUND;
}
