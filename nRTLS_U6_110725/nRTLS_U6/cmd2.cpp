
#include "DataBase.h"
#include "OTA.h"
#include "handle_mqtt.h"
#include "handle_config.h"


typedef enum
{
	cmd2_compare_equal,
	cmd2_compare_big,
	cmd2_compare_small,
} cmd2_compare_flag;

bool cmd2_check_version(cmd2_compare_flag flag, char *version_dev, char *version_cmp)
{
	/*nếu là so sánh bằng*/
	if (flag == cmd2_compare_equal)
	{
		// cmd2_debug("compare equal %s - %s", version_dev, version_cmp);
		if (strcmp(version_dev, version_cmp))
			return false;
		return true;
	}
	/*nếu không phải so sánh bằng*/
	double ver_dev = 0;
	double ver_cmp = 0;
	ver_dev = atof(version_dev);
	ver_cmp = atof(version_cmp);

	/*nếu là so sánh lớn*/
	if (flag == cmd2_compare_big)
	{
		// cmd2_debug("compare big %02.02f - %02.02f", ver_dev, ver_cmp);
		if (ver_dev < ver_cmp)
			return true;
		return false;
	}
	/*nếu là so sánh bes*/
	if (flag == cmd2_compare_small)
	{
		// cmd2_debug("compare small %02.02f - %02.02f", ver_dev, ver_cmp);
		if (ver_dev > ver_cmp)
			return true;
		return false;
	}

	return true;
}

bool cmd2_handle(cmd2_str *data)
{
	cmd2_debug("%s", data->MD5);
	cmd2_debug("%s", data->Firmware);
	cmd2_debug("%s", data->Hardware);
	cmd2_debug("%s", data->FileName);
	cmd2_debug("%s", data->Path);
	char filename[100];
	memcpy(&filename,data->FileName, strlen(data->FileName));
	char * p;
	p = strtok(filename, "_");
	if(p != NULL)
	  p = strtok(NULL, "_");
	if(p != NULL) 
	{
		
		char board[2];
		sprintf(&board[0],"%s",p);
		cmd2_debug("%c\n", board[0]);
		if(board[0] == 'I')
		OTA_Main_update(data->FileName, data->Path, data->MD5);
		else if(board[0] == 'D')
		OTA_DW_update(data->FileName, data->Path, data->MD5);
		return true;
    }
	
	return false;
}

String report_UpdateOTA(bool complete)
{
	DynamicJsonDocument doc(512);
	//doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
	//doc["SerialID"] = Config_Device.Device.SerialID; //Hieu  2024011
	// doc["MessageID"] = MQTT_Exchange.PackitID;
	doc["SerialID"] = Config_Device.Device.SerialID; //Hieu  2024011
	doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
	doc["CMD"] = MQTT_Exchange.CMD;
	doc["FileName"] = MQTT_Exchange.cmd2.FileName;
	// doc["HwVer"] = MQTT_Exchange.cmd2.Hardware;
	// doc["FwVer"] = MQTT_Exchange.cmd2.Firmware;
	doc["Path"] = MQTT_Exchange.cmd2.Path;
	if(complete)
	doc["Reply"] = "Update_complete";
	else
	doc["Reply"] = "Update_fail";

	String _output;
    serializeJson(doc, _output);
	Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());

	return _output;
}
