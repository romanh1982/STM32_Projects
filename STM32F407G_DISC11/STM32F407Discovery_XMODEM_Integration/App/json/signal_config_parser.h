/*
 * signal_config_parser.h
 *
 *  Created on: Jun 26, 2025
 *      Author: roman
 */

#ifndef JSON_SIGNAL_CONFIG_PARSER_H_
#define JSON_SIGNAL_CONFIG_PARSER_H_

int32_t parse_and_validate_signal_config(const char *json_str, const char *cmd_name, JsonParsedSigGenPar_HandlType_t *config);

#endif /* JSON_SIGNAL_CONFIG_PARSER_H_ */
