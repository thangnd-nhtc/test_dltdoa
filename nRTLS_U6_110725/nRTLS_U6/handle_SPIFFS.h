#ifndef HANDLESPI_H
#define HANDLESPI_H

#include "Arduino.h"
#include "Update.h"
#include "SPIFFS.h"
#include <FS.h>
#include "define.h"

#define cUpdateFWDebug  mUpdateFWDebug
#define ROLLBACK "/rollBack.bin"
#define FWOTA    "/FW_DW.bin"
#define FWOTA    "/FW_MAIN.bin"



 
class handle_SPIFFS
{
public:
    handle_SPIFFS(/* args */);
    ~handle_SPIFFS();

    uint8_t beginSPIFFS();
    String readInformations(char* namefile);
    uint8_t startUpdateFW(const char * path);
    uint8_t writeFile(char* filename,uint8_t *buf, size_t size);
    bool create_file(char *path);
    void append_file(char *path, uint8_t *message, size_t size);
private:

    /* data */
    uint8_t performUpdateFW(Stream &updateSource, size_t updateSize);

    UpdateClass Update;
};

extern handle_SPIFFS _handle_SPIFFS;

#endif
