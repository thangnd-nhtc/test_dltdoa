#ifndef _ESP_AES_H
#define _ESP_AES_H
#include <stdint.h>


void HW_AES_CBC128_2B_encryption(uint8_t *Data_Out, uint8_t *Data_In);
void HW_AES_CBC128_2B_decryption(uint8_t *Data_Out, uint8_t *Data_In);

uint16_t HW_AES_CBC128_encryption(uint8_t *Out, const uint8_t *In, uint16_t Size, uint8_t *Key);
uint16_t HW_AES_CBC128_decryption(uint8_t *Out, const uint8_t *In, uint16_t Size, uint8_t *Key);

void HW_EAS_Key_Update(char *Key);
void HW_AES_encryption(char *Data_Out, const uint8_t *Data_In);
void HW_AES_decryption(uint8_t *Data_Out, const char *Data_In);
void HW_AES_encryption_toHex(uint8_t *Data_Out, uint8_t *Data_In, int Size_In);
void HW_AES_decryption_toHex(uint8_t *Data_Out, uint8_t *Data_In);

#endif
