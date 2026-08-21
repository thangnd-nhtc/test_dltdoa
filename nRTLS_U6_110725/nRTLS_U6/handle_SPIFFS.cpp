#include "handle_SPIFFS.h"
#include "WString.h"
#include "ArduinoJson.h"

handle_SPIFFS::handle_SPIFFS(/* args */)
{
}

handle_SPIFFS::~handle_SPIFFS()
{
}

uint8_t handle_SPIFFS::beginSPIFFS()
{
    uint8_t ret;
    ret = SPIFFS.begin(true);
    if(ret)
    debug_SPIFFS("begin SPIFFS OK");
    else
    debug_SPIFFS("An Error has occurred while mounting SPIFFS");
    return ret;
}

String handle_SPIFFS::readInformations(char* namefile) // đọc thông tin file config 
{
    String data = "";
    File dataConfig  = SPIFFS.open(namefile,FILE_READ);
    if(dataConfig.isDirectory())
    {
        debug_SPIFFS("Error, read config is not a file %s", namefile);
    }
    else
    {
        data = dataConfig.readString();
    }
    dataConfig.close();
    return data;
}

uint8_t handle_SPIFFS::startUpdateFW(const char * path) //update FW
{
    uint8_t ret = 0;
    File updateBin  = SPIFFS.open(path,FILE_READ);
    if(updateBin){
      if(updateBin.isDirectory())
      {
         debug_SPIFFS("Error, update.bin is not a file");
         updateBin.close();
         return 0;
      }
    }
    size_t updateSize = updateBin.size();
    debug_SPIFFS("size :%d",updateSize);
    // debug_SPIFFS("data:");
    // while (updateBin.available()) 
    // {
    //     Serial.write(updateBin.read());
    // }

    if (updateSize > 0) 
    {
         debug_SPIFFS("Try to start update");
         ret = this->performUpdateFW(updateBin, updateSize);
    }
    else 
    {
        debug_SPIFFS("Error, file is empty");
        return 0;
    }
    
    updateBin.close();
    SPIFFS.end();
    return ret;
}

uint8_t handle_SPIFFS::writeFile(char* filename, uint8_t *buf, size_t size) // ghi dữ liệu vào file OTA
{
    // Serial.printf("Writing file: %s\r\n", path);
    uint8_t ret = 1;
    File file = SPIFFS.open(filename, FILE_APPEND);
    if(!file){
        debug_SPIFFS("- failed to open file for writing");
        ret = 0;
    }
    else
    {
        uint8_t retry = 0;
        while (file.write(buf, size) != size && retry < 3)
        {
            delay(1);
            retry++;
        }
    }
    file.flush();
    file.close();

    return ret;
}

// perform the actual update from a given stream
uint8_t handle_SPIFFS::performUpdateFW(Stream &updateSource, size_t updateSize)  // xử lý update OTA
{
    uint8_t ret = 0;
   if (Update.begin(updateSize)) {      
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
         debug_SPIFFS("Written %d successfully" ,written);
      }
      else {
         debug_SPIFFS("Written only : %d - %d .Retry?",written,updateSize);
      }
      if (Update.end()) {
            debug_SPIFFS("OTA done!");  
         if (Update.isFinished()) {
            debug_SPIFFS("Update successfully completed. Rebooting.");
            ret = 1;
         }
         else {
            debug_SPIFFS("Update not finished? Something went wrong!");
         }
      }
      else {
         debug_SPIFFS("Error Occurred. Error #: %d" , Update.getError());
      }

   }
   else
   {
      debug_SPIFFS("Not enough space to begin OTA");
   }

   return ret;
}


bool handle_SPIFFS::create_file(char *path)
{

	/*Nếu SD chưa sẵng sàng thì bỏ qua*/
	if (this->beginSPIFFS() == false)
		return false;

	/*bật cờ báo bận*/
	// while (sd_is_busy_f == true || sd_is_busy_to.ToERemain())
		// ;
	// sd_is_busy_f = true;

	debug_SPIFFS("create file %s", path);

	File file = SPIFFS.open(path, FILE_WRITE);
	if (!file)
	{
		debug_SPIFFS("Failed to create file %s", path);
		file.close();
		/*tắt cờ báo bận*/
		// sd_is_busy_f = false;
		return false;
	}

	file.close();
	/*tắt cờ báo bận*/
	// sd_is_busy_f = false;
	return true;
}



handle_SPIFFS _handle_SPIFFS;
