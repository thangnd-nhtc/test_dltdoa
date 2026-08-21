#ifndef _TOOLS_H
#define _TOOLS_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "stdint.h"
#define LOWER_CASE 0
#define UPPER_CASE 1

	char *BCDString(char *BCDStr, uint8_t *Buff, uint8_t Size, uint8_t UpperCase);
	uint16_t hexstr_to_char(char *chrs, const char *hexstr);
	uint16_t byteArrayToHexString(uint8_t *byte_array, int byte_array_len, char *hexstr, int hexstr_len);

#ifdef __cplusplus
}
#endif
#endif
