/*
 * parse_utils.h
 *
 *  Created on: May 15, 2025
 *      Author: Roman
 */

#ifndef UTILS_PARSE_UTILS_PARSE_UTILS_H_
#define UTILS_PARSE_UTILS_PARSE_UTILS_H_



typedef struct {
    const char *pattern;
    uint16_t min_val;
    uint16_t max_val;
    uint16_t *out;
} ParseParamU16_t;

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
void skip_leading_commas_spaces(const char **str_ptr);

/**
 * @brief   Search the full input string for a key=value pair and extract a 32-bit integer.
 * @param   input       Null-terminated input string to search (unchanged during call)
 * @param   param_cfg   Pattern, min/max range, and output pointer
 * @return  true if found and valid; false if key not found or value out of range
 *
 * This function does NOT modify the input pointer. It scans the string for the given key,
 * parses the following value if found, validates the range, and writes it to *out.
 */
bool parse_u16_param_from_string(const char *input, const ParseParamU16_t *param_cfg);

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
bool parse_array_u16_from_string(const char *input, const char *key, uint16_t *out_array, uint16_t expected_count);

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
bool parse_array_u32_from_string(const char *input, const char *key, uint32_t *out_array, uint16_t expected_count);


#endif /* UTILS_PARSE_UTILS_PARSE_UTILS_H_ */
