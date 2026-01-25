/*
 * parse_utils.c
 *
 *  Created on: May 15, 2025
 *      Author: Roman
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "parse_utils.h"
/**
 * @brief  Advance the input string pointer past any leading commas or spaces.
 * @param  str_ptr  Pointer to a C‐string pointer. On return, *str_ptr points
 *                  to the first character in the string that is neither a
 *                  comma (',') nor a space (' ').
 *
 * Example:
 *   const char *cmd = ",,  ,FOO=1";
 *   skip_leading_commas_spaces(&cmd);
 *   // now cmd points at 'F' in "FOO=1"
 */
void skip_leading_commas_spaces(const char **str_ptr)
{
    /* While the character pointed to by *str_ptr is a comma or space… */
    while (**str_ptr == ',' || **str_ptr == ' ') {
        /* …advance the pointer by one to skip it */
        (*str_ptr)++;
    }
}

/**
 * @brief   Search the full input string for a key=value pair and extract a 32-bit integer.
 * @param   input       Null-terminated input string to search (unchanged during call)
 * @param   param_cfg   Pattern, min/max range, and output pointer
 * @return  true if found and valid; false if key not found or value out of range
 *
 * This function does NOT modify the input pointer. It scans the string for the given key,
 * parses the following value if found, validates the range, and writes it to *out.
 */
bool parse_u16_param_from_string(const char *input, const ParseParamU16_t *param_cfg)
{
    const char *inputPtrCopy = input;
    size_t key_len = strlen(param_cfg->pattern);

    while (*inputPtrCopy) {
        /* Match the pattern (e.g. "NUM_TONES=") */
        if (strncmp(inputPtrCopy, param_cfg->pattern, key_len) == 0) {
            inputPtrCopy += key_len;

            uint16_t val = 0;
            if (!(*inputPtrCopy >= '0' && *inputPtrCopy <= '9')) return false;

            while (*inputPtrCopy >= '0' && *inputPtrCopy <= '9') {
                val = val * 10 + (uint16_t)(*inputPtrCopy - '0');
                inputPtrCopy++;
            }

            if (val < param_cfg->min_val || val > param_cfg->max_val) return false;

            *(param_cfg->out) = val;
            return true;
        }

        /* Skip ahead to next comma or end */
        while (*inputPtrCopy && *inputPtrCopy != ',') inputPtrCopy++;
        if (*inputPtrCopy == ',') inputPtrCopy++;
    }

    return false; // pattern not found
}

/**
 * @brief   Search the input string for a key=val1;val2;... and parse an array of uint16_t values.
 * @param   input            Null-terminated input string to scan (remains unmodified).
 * @param   key              Parameter key to match (e.g. "FREQS=" or "AMPS=").
 * @param   out_array        Pointer to destination array where parsed integers will be stored.
 * @param   expected_count   Expected number of integers in the list.
 * @return  true if the key was found, and exactly expected_count valid integers were parsed;
 *          false if the key was not found or the format/count was invalid.
 *
 * This function does not advance or modify the input string pointer, allowing multiple
 * independent extractions from a single source command string.
 */
bool parse_array_u16_from_string(const char *input, const char *key, uint16_t *out_array, uint16_t expected_count)
{
    const char *inputPtrCopy = input;
    size_t key_len = strlen(key);

    while (*inputPtrCopy) {
        /* Look for the key pattern (e.g. "FREQS=") */
        if (strncmp(inputPtrCopy, key, key_len) == 0) {
            inputPtrCopy += key_len;

            for (uint16_t i = 0; i < expected_count; i++) {
            	uint16_t val = 0;

                /* Ensure current character is a digit */
            	if (!isdigit((unsigned char)*inputPtrCopy)) return false;

                /* Parse numeric value */
                while (isdigit((unsigned char)*inputPtrCopy)) {
                    val = val * 10 + (*inputPtrCopy - '0');
                    inputPtrCopy++;
                }

                out_array[i] = val;

                /* Expect a semicolon between values, but not after the last */
                if (i < expected_count - 1) {
                    if (*inputPtrCopy != ';') {
                        return false;
                    }
                    inputPtrCopy++;  // skip the semicolon
                }
            }

            /* Success: all values parsed */
            return true;
        }

        /* Skip to the next comma-delimited token */
        while (*inputPtrCopy && *inputPtrCopy != ',') {
            inputPtrCopy++;
        }
        if (*inputPtrCopy == ',') {
            inputPtrCopy++;
        }
    }

    /* Pattern not found */
    return false;
}

/**
 * @brief   Search the input string for a key=val1;val2;... and parse an array of uint32_t values.
 * @param   input            Null-terminated input string to scan (remains unmodified).
 * @param   key              Parameter key to match (e.g. "FREQS=" or "AMPS=").
 * @param   out_array        Pointer to destination array where parsed integers will be stored.
 * @param   expected_count   Expected number of integers in the list.
 * @return  true if the key was found, and exactly expected_count valid integers were parsed;
 *          false if the key was not found or the format/count was invalid.
 *
 * This function does not advance or modify the input string pointer, allowing multiple
 * independent extractions from a single source command string.
 */
bool parse_array_u32_from_string(const char *input, const char *key, uint32_t *out_array, uint16_t expected_count)
{
    const char *inputPtrCopy = input;
    size_t key_len = strlen(key);

    while (*inputPtrCopy) {
        /* Look for the key pattern (e.g. "FREQS=") */
        if (strncmp(inputPtrCopy, key, key_len) == 0) {
            inputPtrCopy += key_len;

            for (uint16_t i = 0; i < expected_count; i++) {
            	uint32_t val = 0;
            	if (!isdigit((unsigned char)*inputPtrCopy)) return false;


                /* Parse numeric value */
                while (isdigit((unsigned char)*inputPtrCopy)) {
                    val = val * 10 + (*inputPtrCopy - '0');
                    inputPtrCopy++;
                }

                out_array[i] = val;

                /* Expect a semicolon between values, but not after the last */
                if (i < expected_count - 1) {
                    if (*inputPtrCopy != ';') {
                        return false;
                    }
                    inputPtrCopy++;  // skip the semicolon
                }
            }

            /* Success: all values parsed */
            return true;
        }

        /* Skip to the next comma-delimited token */
        while (*inputPtrCopy && *inputPtrCopy != ',') {
            inputPtrCopy++;
        }
        if (*inputPtrCopy == ',') {
            inputPtrCopy++;
        }
    }

    /* Pattern not found */
    return false;
}

