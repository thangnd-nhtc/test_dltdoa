#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hwcrypto/aes.h>
#include "ESP_aes.h"
#include "Tools.h"
#include "define.h"

// Number of bytes to make up a 128 bit key (16 * 8 = 128)
#define KEY_SIZE 16
#define BLOCK_AES_SIZE KEY_SIZE

#define KEY_DEFAULT "6e68746332313039"	/*nhtc2109*/
#define AES_KEY_ZERO "6e68746332313039" /*nhtc2109*/

static char AES_Key[] = "6e68746332313039";

uint8_t initVector[16] = {0};
uint8_t iv_zero[16] = {0};

void HW_EAS_Key_Update(char *Key)
{
	memset(AES_Key, 0, sizeof(AES_Key));
	memcpy(AES_Key, Key, KEY_SIZE);
}

void HW_EAS_GetKey_Present(uint8_t *Key)
{
	memcpy(Key, AES_Key, KEY_SIZE);
}

void HW_EAS_Key_Default(void)
{
	memset(AES_Key, 0, sizeof(AES_Key));
	memcpy(AES_Key, (uint8_t *)KEY_DEFAULT, KEY_SIZE);
}

/* Hàm mã hóa AES CBC 2Block(32bytes).
 * Giá trị trả về: số bytes đã mã hóa.
 * */
void HW_AES_CBC128_2B_encryption(uint8_t *Data_Out, uint8_t *Data_In)
{
	esp_aes_context aes_ctx_2b;

	esp_aes_init(&aes_ctx_2b);
	esp_aes_setkey(&aes_ctx_2b, (const uint8_t *)AES_KEY_ZERO, KEY_SIZE * 8);
	memset(iv_zero, 0, sizeof(iv_zero));
	esp_aes_crypt_cbc(&aes_ctx_2b, ESP_AES_ENCRYPT, 2 * BLOCK_AES_SIZE, iv_zero, Data_In, Data_Out);
	esp_aes_free(&aes_ctx_2b);
}

/* Hàm giải mã AES CBC 2Block(32bytes).
 * Giá trị trả về: số bytes đã giải mã.
 * */
void HW_AES_CBC128_2B_decryption(uint8_t *Data_Out, uint8_t *Data_In)
{
	esp_aes_context aes_ctx_2b;

	esp_aes_init(&aes_ctx_2b);
	esp_aes_setkey(&aes_ctx_2b, (const uint8_t *)AES_KEY_ZERO, KEY_SIZE * 8);
	memset(iv_zero, 0, sizeof(iv_zero));
	esp_aes_crypt_cbc(&aes_ctx_2b, ESP_AES_DECRYPT, 2 * BLOCK_AES_SIZE, iv_zero, Data_In, Data_Out);
	esp_aes_free(&aes_ctx_2b);
}

/* Hàm mã hóa AES CBC. 
 * Giá trị trả về: số bytes đã mã hóa.
 * */
uint16_t HW_AES_CBC128_encryption(uint8_t *Data_Out, const uint8_t *Data_In, uint16_t size_in, uint8_t *Key)
{
	esp_aes_context aes_ctx;
	uint8_t block_num;

	esp_aes_init(&aes_ctx);
	esp_aes_setkey(&aes_ctx, Key, KEY_SIZE * 8);

	// Tinh so khoi du lieu can ma hoa
	if ((size_in % BLOCK_AES_SIZE) != 0)
		block_num = (size_in / BLOCK_AES_SIZE) + 1;
	else
		block_num = (size_in / BLOCK_AES_SIZE);

	esp_aes_crypt_cbc(&aes_ctx, ESP_AES_ENCRYPT, block_num * BLOCK_AES_SIZE, initVector, Data_In, Data_Out);
	esp_aes_free(&aes_ctx);

	return block_num * BLOCK_AES_SIZE;
}

/* Hàm giải mã AES CBC.
 * Giá trị trả về: số bytes đã giải mã.
 * */
uint16_t HW_AES_CBC128_decryption(uint8_t *Data_Out, const uint8_t *Data_In, uint16_t size_in, uint8_t *Key)
{
	esp_aes_context aes_ctx;
	uint8_t block_num;

	esp_aes_init(&aes_ctx);
	esp_aes_setkey(&aes_ctx, Key, KEY_SIZE * 8);
	// Tinh so khoi du lieu can ma hoa
	if ((size_in % BLOCK_AES_SIZE) != 0)
		block_num = (size_in / BLOCK_AES_SIZE) + 1;
	else
		block_num = (size_in / BLOCK_AES_SIZE);

	esp_aes_crypt_cbc(&aes_ctx, ESP_AES_DECRYPT, block_num * BLOCK_AES_SIZE, initVector, Data_In, Data_Out);
	esp_aes_free(&aes_ctx);

	return block_num * BLOCK_AES_SIZE;
}

/* Ham ma hoa du lieu*/
void HW_AES_encryption(char *Data_Out, const uint8_t *Data_In)
{
	uint8_t aes_buf[256] = {0};
	uint16_t aes_len = 0;
	char str_hex[512] = {0};
	uint16_t str_len = 0;
	/* Do dai du lieu can ma hoa */
	str_len = strlen((char *)Data_In);
	/* Ma hoa du lieu dau vao */
	aes_len = HW_AES_CBC128_encryption(aes_buf, Data_In, str_len, (uint8_t *)AES_Key);
	/* Chuyen du lieu -> chuoi hexa */
	byteArrayToHexString(aes_buf, aes_len, str_hex, sizeof(str_hex));
	/* Xuat du lieu da ma hoa */
	memcpy(Data_Out, str_hex, strlen(str_hex));
}

/* Ham ma hoa du lieu*/
void HW_AES_decryption(uint8_t *Data_Out, const char *Data_In)
{
	char aes_buf[512] = {0};
	uint16_t aes_len = 0;
	uint8_t arr_data[256] = {0};
	uint16_t arr_len = 0;

	/* Convert hexstring to char aes array */
	aes_len = hexstr_to_char(aes_buf, (char *)Data_In);
	/* Decryption */
	HW_AES_CBC128_decryption(arr_data, (const uint8_t *)aes_buf, aes_len, (uint8_t *)AES_Key);
	/* Xuat du lieu da ma hoa */
	memcpy((char *)Data_Out, (char *)arr_data, strlen((char *)arr_data));
}

/* Ham ma hoa du lieu va hex string*/
void HW_AES_encryption_toHex(uint8_t *Data_Out, uint8_t *Data_In, int Size_In)
{
	uint8_t aes_buf[256] = {0};
	char str_hex[512] = {0};

	/*Đóng gói theo từng block 16*/
	if ((Size_In % BLOCK_AES_SIZE) != 0)
		Size_In = ((Size_In / BLOCK_AES_SIZE) + 1) * BLOCK_AES_SIZE;

	/* Ma hoa du lieu dau vao */
	HW_AES_CBC128_2B_encryption(aes_buf, Data_In);
	/* Chuyen du lieu -> chuoi hexa */
	byteArrayToHexString(aes_buf, Size_In, str_hex, sizeof(str_hex));
	/* Xuat du lieu da ma hoa */
	memcpy(Data_Out, str_hex, strlen(str_hex));
}

/* Ham giai ma du lieu va hex string*/
void HW_AES_decryption_toHex(uint8_t *Data_Out, uint8_t *Data_In)
{
	uint16_t aes_len = 0;
	uint8_t aes_buf[512] = {0};
	uint8_t arr_data[256] = {0};

	/*Tìm ký tự kết thuc và loại bỏ*/
	char *len_end = strstr((const char *)Data_In, "\r\n");
	if (len_end != 0)
	{
		len_end[0] = 0;
	}
	/* Convert hexstring to char aes array */
	aes_len = hexstr_to_char((char *)aes_buf, (const char *)Data_In);
	/* Ma hoa du lieu dau vao */
	HW_AES_CBC128_2B_decryption(arr_data, aes_buf);
	/* Xuat du lieu da ma hoa */
	memcpy(Data_Out, (char *)arr_data, aes_len);
}
