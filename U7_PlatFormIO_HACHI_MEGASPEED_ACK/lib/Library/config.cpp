#include "config.h"
#include <EEPROM.h>
#include "handle_decawave.h"

extern DwHandle Handle_Dw;

namespace Config
{

    static bool eeprom_open = false;

    // ------------------------------------
    // Khởi tạo EEPROM (gọi 1 lần trong setup)
    // ------------------------------------
    bool begin()
    {
        if (!eeprom_open)
        {
            eeprom_open = EEPROM.begin(EEPROM_SIZE);
        }
        return eeprom_open;
    }

    // ------------------------------------
    // Kết thúc (hiếm khi cần, nhưng nên có)
    // ------------------------------------
    void end()
    {
        if (eeprom_open)
        {
            EEPROM.end();
            eeprom_open = false;
        }
    }

    // ------------------------------------
    // Đọc device_id từ EEPROM
    // ------------------------------------
    uint32_t getDeviceID()
    {
        uint32_t id = 0;
        EEPROM.get(EEPROM_ADDR_DEVICE_ID, id);
        return id;
    }

    // ------------------------------------
    // Ghi device_id vào EEPROM
    // ------------------------------------
    void setDeviceID(uint32_t id, bool commit_now)
    {
        EEPROM.put(EEPROM_ADDR_DEVICE_ID, id);
        if (commit_now)
            EEPROM.commit();
        // In log ra Serial
        dbg_main("[EEPROM] Saved DeviceID: 0x%08lX (%lu)\n", id, id);
        if (Handle_Dw.begin())
        {
            dbg_main("%s", "[DW3000] Initialization OK ✅");
        }
        else
        {
            dbg_main("%s", "[DW3000] Initialization FAILED ❌");
        }
    }

    // ------------------------------------
    // Kiểm tra device_id hợp lệ
    // ------------------------------------
    bool isDeviceIDValid()
    {
        uint32_t id = 0;
        EEPROM.get(EEPROM_ADDR_DEVICE_ID, id);
        if (id == 0xFFFFFFFFu || id == 0x00000000u)
        {
            return false;
        }
        return true;
    }

} // namespace Config
