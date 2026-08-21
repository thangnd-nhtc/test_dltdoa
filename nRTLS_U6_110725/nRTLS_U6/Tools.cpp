#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Tools.h"

/*
- In chuoi Buff
- BCDStr: chuoi sau khi chuyen
- Buff: la chuoi Hex
- Size: chieu dai mang hex can chuyen sang string, nen mang string co chieu dai la 2*len + 1
- Case: 1. chuyen dang chu Hoa, 0. chuyen dang chu thuong
#define LOWER_CASE	0
#define UPPER_CASE	1
- Vi du: hex[]={0x12,0xAB,0xD3}, len=3, hoathuong=1 -> String="12ABD3"
BCDString(String,hex,3,UPPER_CASE);
*/
char *BCDString(char *BCDStr, uint8_t *Buff, uint8_t Size, uint8_t Case)
{
	uint8_t i, j = 0;
	for (i = 0; i < Size; i++)
	{
		if (Case == UPPER_CASE)
			sprintf(BCDStr + j, "%02X", Buff[i]);
		else
			sprintf(BCDStr + j, "%02x", Buff[i]);
		j += 2;
	}
	BCDStr[Size * 2] = '\0';
	return BCDStr;
}

uint16_t hexstr_to_char(char *chrs, const char *hexstr)
{
	int len = strlen(hexstr);

	if (len % 2 != 0)
		return 0; // Loi chieu dai chuoi
	int final_len = len / 2;
	for (int i = 0, j = 0; j < final_len; i += 2, j++)
		chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25;
	chrs[final_len] = '\0';

	return final_len;
}

uint16_t byteArrayToHexString(uint8_t *byte_array, int byte_array_len, char *hexstr, int hexstr_len)
{
	int off = 0;
	int i;

	for (i = 0; i < byte_array_len; i++)
	{
		off += snprintf(hexstr + off, hexstr_len - off, "%02x", byte_array[i]);
	}
	hexstr[off] = '\0';

	return off;
}
