
#include "DataBase.h"
#include "handle_config.h"
#include "ESP_aes.h"
/**
 * Chương trình xin token key
 * Khởi động lên chưa có token key thì gửi lên server xin key
 * Nhận lệnh từ server giải mã bằng token key đầu tiên
 * Sau 1h tự động xin token key lại (đổi key)
*/

volatile bool flag_key = false;
TimeOutEvent TO_ReSendTokenKey(5000);

bool cmd1_is_tokenkey(void)
{
	return flag_key;
}
bool cmd1_info(cmd1_str *data)
{
	if (data->TokenKey != NULL)
	{
		cmd1_debug("%s", data->TokenKey);
		flag_key = true;
		TO_ReSendTokenKey.ToEUpdate(60 * 60000);
		return true;
	}
	return false;
}
void cmd1_resend_version(void)
{
	flag_key = false;
	TO_ReSendTokenKey.ToEUpdate(5000);
	cmd1_debug("re-send version");
}
void cmd1_lost_key(cmd1_str *data)
{
	cmd1_debug("lost token key");
	strncpy(data->TokenKey, MQTT_DEFAULT_KEY, L_TOKEN_KEY_SIZE);
	flag_key = false;
	if (TO_ReSendTokenKey.ToEGetStatus() == false)
		TO_ReSendTokenKey.ToEUpdate(5000);
	else if (TO_ReSendTokenKey.ToERemain() > 5000)
		TO_ReSendTokenKey.ToEUpdate(5000);
}

String cmd1_check(void)
{
	DynamicJsonDocument djbco(1024);
	String string_data;

	if (TO_ReSendTokenKey.ToEExpired() == true)
	{
		cmd1_debug("Re-send token key");
		if (flag_key == false)
			TO_ReSendTokenKey.ToEUpdate(5000);
		else
			TO_ReSendTokenKey.ToEUpdate(60 * 60000);

		djbco[MQTT_RANDOM] = random(0, 255);
		djbco[MQTT_SERIAL] = Config_Device.Device.SerialID;
		djbco[MQTT_CMD] = MQTT_CMD_TOKEN_KEY;

		djbco[MQTT_TOKEN_KEY] = _cmd1_str.TokenKey; 
		djbco[MQTT_HW_INTERNET] = _cmd1_str.HWInternet;
		djbco[MQTT_FW_INTERNET] = _cmd1_str.FWInternet;
		djbco[MQTT_HW_DW] =_cmd1_str.HWDecawave;
		djbco[MQTT_FW_DW] = _cmd1_str.FWDecawave;

		djbco[MQTT_REPLY] = "";

		string_data = "";
		serializeJson(djbco, string_data);
		cmd1_debug("%s", string_data.c_str());
		return string_data;
	}
	return "";
}

String cmd1_encryption(char *AES_Key, char *Array, int LenArray)
{
	char Aes_Buf[1024];
	memset(Aes_Buf, 0, sizeof(Aes_Buf));

	cmd1_debug("encryp %s-%s", AES_Key, Array);
	HW_EAS_Key_Update(AES_Key);
	HW_AES_encryption_toHex((uint8_t *)Aes_Buf, (uint8_t *)Array, LenArray);
	strcat(Aes_Buf, "\r\n");
	// cmd_13_debug("encryp OK %s", Aes_Buf);
	return String(Aes_Buf);
}

String cmd1_decryption(char *AES_Key, char *Array, int LenArray)
{
	char Data_Buf[1024];
	memset(Data_Buf, 0, sizeof(Data_Buf));
	// cmd_13_debug("Length %d, decryp %s",LenArray, Array);

	/*PHẢI BỎ CÁI ĐUÔI /R/N*/
	if (!strncmp(&Array[LenArray - 2], "\r\n", 2))
	{
		memset((uint8_t *)&Array[LenArray - 2], 0, 2);
		LenArray = LenArray - 2;
	}

	HW_EAS_Key_Update(AES_Key);
	HW_AES_decryption_toHex((uint8_t *)Data_Buf, (uint8_t *)Array);
	cmd1_debug("decryp OK %s-%s", AES_Key, Data_Buf);

	return String(Data_Buf);
}
