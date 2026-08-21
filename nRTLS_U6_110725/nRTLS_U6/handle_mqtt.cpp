#include "handle_mqtt.h"
#include "DataBase.h"
#include "handle_SPIFFS.h"
#include "handle_com_regs.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "handle_spi_master.h"
#include "handle_wifi.h"
#include "iBeaconTest.h"
#include "uart_config.h"
#include "web_server.h"

#define MQTT_TIMEOUT_RESET 15 * 60000
// cmd_status_str cmd2_pss;
MQTT_Exchange_str MQTT_Exchange;

WiFiClient MQTTWifiClient;
// PubSubClient mqtt_client(MQTTWifiClient);

String TopicServer;
String TopicDevice;
static uint8_t *BUFFER_SEND;
volatile uint32_t Position_Buff_Wr_mqtt = 0;
volatile uint32_t Position_Buff_Rd_mqtt = 0;

#define bufferSize (MQTT_MAX_PACKET_SIZE * 5)

TimeOutEvent TimeCheck(10000);

String mqtt_communication_decode(String payload);
MqttHandle::MqttHandle() {
  this->mqtt_client = new PubSubClient(this->MQTTClient);
  this->MQTT_isConnected = false;
  BUFFER_SEND = (uint8_t *)heap_caps_malloc(bufferSize, MALLOC_CAP_DMA);
  // BUFFER_SEND = (uint8_t *)ps_malloc(bufferSize);
}
MqttHandle::~MqttHandle() {}

void mqtt_callback(char *topic, byte *payload, unsigned int length);

bool MqttHandle::mqtt_isconnect(void) { return this->MQTT_isConnected; }

void MqttHandle::mqtt_setup() {
  dbg_mqtt("Connect to server: %s, port %d", Config_Internet.MQTT.Server,
           Config_Internet.MQTT.Port);

  mqtt_client->setServer(Config_Internet.MQTT.Server,
                         Config_Internet.MQTT.Port);
  mqtt_client->setBufferSize(bufferSize);
  mqtt_client->setCallback(mqtt_callback);
  // mqtt_client->setKeepAlive(30); // Giữ kết nối: mỗi 30s gửi PING để broker không ngắt
  // mqtt_client->setSocketTimeout(2);

  //	boolean connect(const char *id, const char *user, const char *pass);

  /*key mặt định ban đầu*/
  // if (!strcmp(MQTT_Exchange.cmd13.TokenKey, ""))
  // 	strncpy(MQTT_Exchange.cmd13.TokenKey, MQTT_DEFAULT_KEY,
  // L_TOKEN_KEY_SIZE);
}

uint8_t dataHandler(String data, size_t len) {
  dbg_mqtt("Rx: %s", data.c_str());
  static uint32_t PackitID_cmp = 0;
  static uint8_t CMD_cmp = 0;
  DynamicJsonDocument djbco(2048);
  DeserializationError error = deserializeJson(djbco, data);
  if (error) {
    dbg_mqtt("deserializeJson() failed: %s", error.c_str());
    return 0;
  }

  // chỉ nên xóa 1 số biến cần xóa thôi
  // MQTT_Exchange.SerialID = 0x00; //Hieu
  MQTT_Exchange.PackitID = 0x00;
  MQTT_Exchange.CMD = 0x00;
  memset(MQTT_Exchange.Reply, 0x00, REPLY_SIZE);
  // memset((uint8_t *)&MQTT_Exchange, 0x00, sizeof(MQTT_Exchange_str));

  // header
  if (!djbco["SerialID"].isNull()) {
    MQTT_Exchange.SerialID = djbco["SerialID"]; // "test"
    dbg_mqtt("SerialID NOT NULL");
  } else if (!djbco["Serial_ID"].isNull()) {
    MQTT_Exchange.SerialID = djbco["Serial_ID"]; // "test"
    dbg_mqtt("Serial_ID NOT NULL");
  } else {
    debug_TCP("SerialID NULL");
    return 0; // Hieu 20240311
  }

  debug_TCP("MQTT_Exchange SerialID: %ld", MQTT_Exchange.SerialID);

  // if (!djbco["MessageID"].isNull())
  // MQTT_Exchange.PackitID = djbco["MessageID"]; // 2
  if (!djbco["CMD"].isNull())
    MQTT_Exchange.CMD = djbco["CMD"]; // 2
  if (!djbco["CMDServerID"].isNull())
    MQTT_Exchange.CMDServerID = djbco["CMDServerID"]; // 2
  if (!djbco["Reply"].isNull())
    strcpy(MQTT_Exchange.Reply, djbco["Reply"]);

  // dbg_mqtt("%s", MQTT_Exchange.Serial);

  // dbg_mqtt("MID %d", MQTT_Exchange.PackitID);
  // dbg_mqtt("cmd %d", MQTT_Exchange.CMD);

  /*------------------+++++++++++++++++------------------*/
  // if (strcmp(MQTT_Exchange.Serial, Config_Device.Device.Serial))
  // {
  // 	static uint8_t retry_key = 0;
  // 	if (retry_key++ >= 3)
  // 	{
  // 		retry_key = 0;
  // 		cmd1_lost_key(&MQTT_Exchange.cmd1);
  // 	}
  // 	return;
  // }

  /*Khống chế việc server gửi lặp lại liên tục*/
  // if (PackitID_cmp == MQTT_Exchange.PackitID && CMD_cmp == MQTT_Exchange.CMD)
  // {
  // 	data = "";
  // 	djbco[MQTT_REPLY] = "OK";
  // 	serializeJson(djbco, data);

  // 	// this->mqtt_send_data(1, MQTT_Exchange.cmd1.TokenKey, data);
  // 	return;
  // }

  PackitID_cmp = MQTT_Exchange.PackitID;
  CMD_cmp = MQTT_Exchange.CMD;

  switch (CMD_cmp) {

  case MQTT_CMD_TOKEN_KEY: {
    if (data.indexOf("\"" + String(MQTT_TOKEN_KEY) + "\"") > -1)
      strncpy(MQTT_Exchange.cmd1.TokenKey, djbco[MQTT_TOKEN_KEY],
              L_TOKEN_KEY_SIZE);
    break;
  }
  case MQTT_CMD_UPDATE_OTA: {
    dbg_mqtt("MQTT_CMD_UPDATE_OTA");
    if (!djbco["MD5"].isNull())
      strcpy(MQTT_Exchange.cmd2.MD5, djbco["MD5"]);
    // if (!djbco["HwVer"].isNull())
    //     strcpy(MQTT_Exchange.cmd2.Hardware, djbco["HwVer"]);
    // if (!djbco["FwVer"].isNull())
    //     strcpy(MQTT_Exchange.cmd2.Firmware, djbco["FwVer"]);
    if (!djbco["FileName"].isNull())
      strcpy(MQTT_Exchange.cmd2.FileName, djbco["FileName"]);
    if (!djbco["Path"].isNull())
      strcpy(MQTT_Exchange.cmd2.Path, djbco["Path"]);
    return MQTT_CMD_UPDATE_OTA;
  }

  case MQTT_CMD_RESET_ALL: {
    if (!djbco["Status"].isNull()) {
      MQTT_Exchange.cmd3.Reset_status = djbco["Status"];
      com_frame_t tx_frame;
      reset_t Reset;
      memset((uint8_t *)&tx_frame, 0, sizeof(tx_frame));
      tx_frame.header.type = write_ram;
      tx_frame.header.msk_regs = START_RESET;
      if (MQTT_Exchange.cmd3.Reset_status == 1) {
        Reset.timeout = 3000;
        Reset.reset = 1;
        memcpy((uint8_t *)tx_frame.data, &Reset, sizeof(Reset));
        tx_frame.header.check_crc =
            SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
        Serial1.write((uint8_t *)&tx_frame,
                      sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
        dbg_mqtt("set RESET ALL");
        return MQTT_CMD_RESET_ALL;
      }

      else if (MQTT_Exchange.cmd3.Reset_status == 2) {
        Reset.timeout = 3000;
        Reset.reset = 2;
        memcpy((uint8_t *)tx_frame.data, &Reset, sizeof(Reset));
        tx_frame.header.check_crc =
            SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
        Serial1.write((uint8_t *)&tx_frame,
                      sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
        dbg_mqtt("set RESET DW");
        return MQTT_CMD_RESET_ALL;
        // return 0;
      }

      else if (MQTT_Exchange.cmd3.Reset_status == 3) {
        Reset.timeout = 3000;
        Reset.reset = 3;
        memcpy((uint8_t *)tx_frame.data, &Reset, sizeof(Reset));
        tx_frame.header.check_crc =
            SPI_master.calcCRC(tx_frame.data, tx_frame.header.msk_regs.len);
        Serial1.write((uint8_t *)&tx_frame,
                      sizeof(tx_frame.header) + tx_frame.header.msk_regs.len);
        dbg_mqtt("set RESET DW");
        return MQTT_CMD_RESET_ALL;
        // return 0;
      }
    }
    break;
  }

  case MQTT_CMD_SERVER_DATA:
    /* code */
    break;

  case MQTT_CMD_SERVER_MQTT:
    /* code */
    break;

  case MQTT_CMD_SERVER_FTP:
    /* code */
    break;

  case MQTT_CMD_SERVER_NTP:
    /* code */
    break;

  case MQTT_CMD_TWO_WAY:

    if (!djbco["BaseID"].isNull()) {
      dbg_mqtt("MQTT_CMD_TWO_WAY");
      // ==== BLOCK CMD 8 when Broadcast TWR is active ====
      // BSS-TWR chiếm toàn bộ nRF52, DS-TWR sẽ bị timeout gây spam CMD 8
      if (g_beacon_cfg.enable_bcast_twr) {
        dbg_mqtt("CMD 8 BLOCKED: Broadcast TWR is active!");
        // Trả phản hồi BUSY ngay, không gửi xuống U7
        sendmqtt_distance(djbco["BaseID"], 0xFFFFFFFF);
        break;
      }
      MQTT_Exchange.cmd8_9_10_11.BaseID = djbco["BaseID"];
      Fag_mask_regs.FAG_TWO_WAY_RES = 2;
      return MQTT_CMD_TWO_WAY;
    } else {
      dbg_mqtt("BASE ID NOT NULL");
    }
    break;
    /* code */

  case MQTT_CMD_CONFIG_DW:
    /* code */
    break;

  case MQTT_CMD_SET_MASTER: {
    dbg_mqtt("MQTT_CMD_INTERVAL_MASTER");

    if (!djbco["isEnable"].isNull()) {
      dw_master_tdoa.enable = djbco["isEnable"];
      dbg_mqtt("I'm here 1 !!!");
    }

    if (!djbco["Dw_Pre_Tx"].isNull()) {
      dw_master_tdoa.time_prepare = djbco["Dw_Pre_Tx"];
      dbg_mqtt("I'm here 2!!!");
    }

    if (!djbco["DW_Interval"].isNull()) {
      dw_master_tdoa.interval = djbco["DW_Interval"];
      dbg_mqtt("I'm here 3 !!!");
    }

    Fag_mask_regs.FAG_MASTER_RES = 2;

    return MQTT_CMD_SET_MASTER;
  }

  case MQTT_CMD_ACC_SYNC_MASTER: {
    dbg_mqtt("MQTT_CMD_ACC_SYNC_MASTER");

    if (!djbco["isEnable"].isNull()) {
      master_access.enable = djbco["isEnable"];
      dbg_mqtt("isEnable is NOT NULL !!!");
    } else
      dbg_mqtt("isEnable is NULL !!!");

    if (!djbco["Serial_Master"].isNull()) {
      master_access.Serial = djbco["Serial_Master"];
      dbg_mqtt("Serial_Master is NOT NULL !!!");
    } else
      dbg_mqtt("Serial_Master is NULL !!!");

    if (!djbco["Timestamp"].isNull()) {
      master_access.Timestamp = (uint32_t)djbco["Timestamp"];
      dbg_mqtt("Timestamp is NOT NULL !!!");
    } else
      dbg_mqtt("Timestamp is NULL !!!");

    if (!djbco["Access"].isNull()) {
      uint8_t num = djbco["Access"];
      if (num == 1)
        Fag_mask_regs.FAG_MASTER_ACCESS_RES1 = 2;
      else if (num == 2)
        Fag_mask_regs.FAG_MASTER_ACCESS_RES2 = 2;
      else if (num == 3)
        Fag_mask_regs.FAG_MASTER_ACCESS_RES3 = 2;
      else if (num == 4)
        Fag_mask_regs.FAG_MASTER_ACCESS_RES4 = 2;

      dbg_mqtt("ACESS %d", num);
    } else
      dbg_mqtt("Access is NULL !!!");

    return MQTT_CMD_ACC_SYNC_MASTER;
  }

  case MQTT_CMD_STATUS_INTERNET:
    /* code */
    break;

  case MQTT_CMD_STATUS_DECAWAY:

    break;
  case MQTT_CMD_STATUS_BEACON:
    /* code */
    break;

  case MQTT_CMD_STATUS_HW:
    /* code */
    break;

  case MQTT_CMD_BEACON: {
    dbg_mqtt("MQTT_CMD_BEACON");

    if (!djbco["Active"].isNull())
      my_ibeacon.Beacon_config.active_Beacon = djbco["Active"];

    if (!djbco["HighMajor"].isNull())
      my_ibeacon.Beacon_config.hi_major = djbco["HighMajor"];

    if (!djbco["LowMajor"].isNull())
      my_ibeacon.Beacon_config.lo_major = djbco["LowMajor"];

    if (!djbco["HighMinor"].isNull())
      my_ibeacon.Beacon_config.hi_minor = djbco["HighMinor"];

    if (!djbco["LowMinor"].isNull())
      my_ibeacon.Beacon_config.lo_minor = djbco["LowMinor"];

    if (!djbco["Time"].isNull())
      my_ibeacon.Beacon_config.time = djbco["Time"];

    my_ibeacon.handle_beacon();
    return MQTT_CMD_BEACON;
  }

  case MQTT_CMD_BEACON_CONFIG: {
    dbg_mqtt("MQTT_CMD_BEACON_CONFIG");
    if (apply_beacon_config_json(djbco)) {
      dbg_mqtt("CMD20: apply OK, FAG_BEACON_RES=%d", Fag_mask_regs.FAG_BEACON_RES);
      return MQTT_CMD_BEACON_CONFIG;
    }
    dbg_mqtt("CMD20: apply_beacon_config_json FAILED!");
    break;
  }

  case MQTT_CMD_CONFIG_NET: {
    dbg_mqtt("MQTT_CMD_CONFIG_NET");
    // Tạm lưu thông số hiện tại
    String new_mac = String(Config_Internet.Ethernet.Mac);
    String new_host = String(Config_Device.Device.HostName);
    String new_devId = String(Config_Device.Device.SerialID);
    bool mac_or_host_changed = false;

    if (!djbco["EMAC"].isNull() && djbco["EMAC"].as<String>() != "") {
      new_mac = djbco["EMAC"].as<String>();
      if (new_mac != String(Config_Internet.Ethernet.Mac))
        mac_or_host_changed = true;
    }
    if (!djbco["HostName"].isNull() && djbco["HostName"].as<String>() != "") {
      new_host = djbco["HostName"].as<String>();
      if (new_host != String(Config_Device.Device.HostName))
        mac_or_host_changed = true;
    }
    if (!djbco["NewSerialID"].isNull() &&
        djbco["NewSerialID"].as<String>() != "") {
      new_devId = djbco["NewSerialID"].as<String>();
      if (new_devId != String(Config_Device.Device.SerialID))
        mac_or_host_changed = true;
    }

    if (mac_or_host_changed) {
      String cfg_cmd = "CFG NET=" + new_mac + "," + new_host + "," + new_devId;
      dbg_mqtt("[MQTT_CFG] Detecting Config changed! Sending: %s\n",
                    cfg_cmd.c_str());
      UartConfig::handleLine(cfg_cmd);
    }
    return MQTT_CMD_CONFIG_NET;
  }

  default:
    break;
  }

  return 0;
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  /*kiểm tra topic đúng của mình cần k*/
  // if (strncmp(TopicServer.c_str(), topic, TopicServer.length()))
  // 	return;

  // if (length > 1023)
  // 	return;
  dbg_mqtt("topic rx: %s", topic);
  dbg_mqtt("MQTT conn=%d, heap=%lu", Mqtt_Handle.mqtt_isconnect(), (unsigned long)ESP.getFreeHeap());

  /*xin vùng ram để thực hiện lấy dữ liệu*/
  // byte *point_payload = (byte *)ps_malloc(length + 50);
  // memset(point_payload, 0, length + 49);
  String dataPayload = String((char *)payload);
  dataPayload = dataPayload.substring(0, length);
  dbg_mqtt("data rx (len=%u): %s", length, dataPayload.c_str());

  // Copy the payload to the new buffer
  // memcpy(point_payload, payload, length);
  // String data = mqtt_communication_decode(String((char *)point_payload));

  uint8_t handler_result = dataHandler(dataPayload, length);
  if (handler_result == 0) {
    dbg_mqtt("dataHandler returned 0 → NO REPLY will be sent! CMD=%d", MQTT_Exchange.CMD);
  }
  switch (handler_result) {

  case MQTT_CMD_UPDATE_OTA: {
    dbg_mqtt("send ok");
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["FileName"] = MQTT_Exchange.cmd2.FileName;
    // doc["HwVer"] = MQTT_Exchange.cmd2.Hardware;
    // doc["FwVer"] = MQTT_Exchange.cmd2.Firmware;
    doc["Path"] = MQTT_Exchange.cmd2.Path;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    // dbg_mqtt("%s",_output.c_str());
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    dbg_mqtt("send complete");
    if (cmd2_handle(&MQTT_Exchange.cmd2)) {
      dbg_mqtt("OK");
      return;
    }

    break;
  }

  case MQTT_CMD_RESET_ALL: {
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    if (MQTT_Exchange.cmd3.Reset_status == 1)
      ESPRebootTo.ToEUpdate(1000);
    break;
  }

  case MQTT_CMD_TWO_WAY: {
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["BaseID"] = MQTT_Exchange.cmd8_9_10_11.BaseID;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    // FIX: Không gọi setConfigDW() từ Core 0 (MQTT callback) vì gây race condition
    // với Core 1 trên Serial1 và tx_frame buffer. FAG_TWO_WAY_RES = 2 đã được set
    // trong dataHandler(), Core 1 sẽ tự xử lý trong vòng lặp tiếp theo (~1ms).
    break;
  }

  case MQTT_CMD_SET_MASTER: {
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["isEnable"] = dw_master_tdoa.enable;
    doc["Dw_Pre_Tx"] = dw_master_tdoa.time_prepare;
    doc["DW_Interval"] = dw_master_tdoa.interval;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    SPI_master.setConfigDW();
    break;
  }

  case MQTT_CMD_ACC_SYNC_MASTER: {
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["isEnable"] = master_access.enable;
    doc["Serial_Master"] = master_access.Serial;
    doc["Timestamp"] = (uint32_t)master_access.Timestamp;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    // free(doc);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    SPI_master.setConfigDW();
    break;
  }

  case MQTT_CMD_BEACON: {
    DynamicJsonDocument doc(512);
    // doc["SerialID"] = MQTT_Exchange.SerialID; //Hieu  2024011
    doc["SerialID"] = Config_Device.Device.SerialID; // Hieu  2024011
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["Active"] = my_ibeacon.Beacon_config.active_Beacon;
    doc["HighMajor"] = my_ibeacon.Beacon_config.hi_major;
    doc["LowMajor"] = my_ibeacon.Beacon_config.lo_major;
    doc["HighMinor"] = my_ibeacon.Beacon_config.hi_minor;
    doc["LowMinor"] = my_ibeacon.Beacon_config.lo_minor;
    doc["Time"] = my_ibeacon.Beacon_config.time;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    // free(doc);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    break;
  }

  case MQTT_CMD_BEACON_CONFIG: {
    DynamicJsonDocument doc(512);
    doc["SerialID"] = Config_Device.Device.SerialID;
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    dbg_mqtt("CMD20 Reply publishing... len=%d", _output.length());
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    break;
  }

  case MQTT_CMD_CONFIG_NET: {
    DynamicJsonDocument doc(512);
    doc["SerialID"] = Config_Device.Device.SerialID;
    doc["CMDServerID"] = MQTT_Exchange.CMDServerID;
    doc["CMD"] = MQTT_Exchange.CMD;
    doc["Reply"] = "OK";
    String _output;
    serializeJson(doc, _output);
    Mqtt_Handle.send_data(TopicDevice.c_str(), _output.c_str());
    break;
  }

  default:
    break;
  }
  // Free the memory
  // free(point_payload);
}

// void MqttHandle::addBuffersendMQTT(uint8_t *Buf, uint16_t Length)
// {
//     for (uint16_t i = 0; i < Length; i++)
//     {
//         BUFFER_DMA[Position_Buff_Wr_mqtt] = Buf[i];
//         Position_Buff_Wr_mqtt++;
//         if (Position_Buff_Wr_mqtt > BuffDMA_Size)
//         {
//             Position_Buff_Wr_mqtt = 0;
//             // FagOverfull = true;
//             debug_TCP("bufffer DMA full");
//         }
//     }
// }

void MqttHandle::send_data(const char *topic, const char *data) {
  // dbg_mqtt("topic: %s",topic);
  // dbg_mqtt("data: %s",data);
  if (mqtt_client->connected()) {
    if (this->mqtt_client->publish(topic, data)) {
      MQTT_isConnected = true;
      dbg_mqtt("send ok");
    } else {
      MQTT_isConnected = false;
      dbg_mqtt("publish FAILED! topic=%s len=%d state=%d", topic,
               strlen(data), mqtt_client->state());
    }
  }

  else {
    MQTT_isConnected = false;
    uint16_t Length = strlen(data);
    char data1[Length + 1];
    memset(data1, 0, sizeof(data1));
    dbg_mqtt("len:%d/%lu", Length, Position_Buff_Wr_mqtt);
    memcpy(data1, data, Length);
    strcat(data1, "\n");
    dbg_mqtt("send add:%s-", data1);

    for (uint16_t i = 0; i < (Length + 1); i++) {
      BUFFER_SEND[Position_Buff_Wr_mqtt] = data1[i];
      Position_Buff_Wr_mqtt++;
      if (Position_Buff_Wr_mqtt > bufferSize) {
        Position_Buff_Wr_mqtt = 0;
        // FagOverfull = true;
        dbg_mqtt("bufffer mqtt full");
      }
      if (data1[i] == 10)
        break;
    }
  }
}

uint32_t MqttHandle::Buff_is_available(void) {
  uint32_t Num = 0;
  if (Position_Buff_Wr_mqtt > Position_Buff_Rd_mqtt)
    Num = Position_Buff_Wr_mqtt - Position_Buff_Rd_mqtt;
  else if (Position_Buff_Wr_mqtt < Position_Buff_Rd_mqtt) {
    Num =
        (uint32_t)(bufferSize - Position_Buff_Rd_mqtt) + Position_Buff_Wr_mqtt;
    // FagOverfull = false;
  }

  return Num;
}

uint8_t MqttHandle::readBufferSend(uint8_t *data, uint32_t Length) {
  if (!Length)
    return 0;

  memset(data, NULL, Length);

  for (uint32_t i = 0; i < Length; i++) {
    data[i] = BUFFER_SEND[Position_Buff_Rd_mqtt];
    Position_Buff_Rd_mqtt++;
    if (Position_Buff_Rd_mqtt > bufferSize)
      Position_Buff_Rd_mqtt = 0;
  }
  return 1;
}

void MqttHandle::mqtt_loop() {
  //	static TO_TypeDef timeout_reset;
  static uint8_t retry_report_lost_key = 0;
  bool connected_before_loop = mqtt_client->connected();
  mqtt_client->loop();
  bool connected_now = mqtt_client->connected();
  MQTT_isConnected = connected_now;
  if (connected_before_loop && !connected_now) {
    dbg_mqtt("mqtt connection lost, state=%d", mqtt_client->state());
  }
  // /*nếu chưa có connect internet thì bỏ qua hết*/
  if (handle_ethernet.wifi_isconnect() || handle_ethernet.lan_isconnect()) {
    // dbg_mqtt("inthernet ok");
  } else
    return;

  /*Kiểm tra subscribe tới server*/
  if (TimeCheck.ToEExpired()) {
    TimeCheck.ToEUpdate(5000);

    if (!mqtt_client->connected()) {
      dbg_mqtt("mqtt reconnect (was disconnected)");
      retry_report_lost_key += 1;
      // String buff = Default_Value.Serial + '_' + random(1, 255);
      // dbg_mqtt("Reconnectting.... to client id %s", buff.c_str());
      // dbg_mqtt("Reconnectting.... to client id %s", Default_Value.Serial);
      // dbg_mqtt("Connect to server: %s, port %d",
      // 		 FileConfig.ConfigFile.SERVER.ControllerServer,
      // 		 FileConfig.ConfigFile.SERVER.ControllerPort);
      /*Setup timeout*/
      // mqtt_client->setSocketTimeout(10);
      if (mqtt_client->connect(String(Config_Device.Device.SerialID).c_str(),
                               // if (mqtt_client.connect(buff.c_str(),
                               Config_Internet.MQTT.User,
                               Config_Internet.MQTT.Pass)) {
        dbg_mqtt("Connectted!");
        MQTT_isConnected = true;
        //				TO_Stop(&timeout_reset);
        /*topic trao đổi dữ liệu*/
        TopicServer = String(Config_Internet.MQTT.TopicServer) + "/" +
                      String(Config_Device.Device.SerialID);

        /*topic trao đổi dữ liệu*/
        TopicDevice = String(Config_Internet.MQTT.TopicDevice) + "/" +
                      String(Config_Device.Device.SerialID);

        /*subscribe topic trao đổi dữ liệu*/
        mqtt_client->subscribe(TopicServer.c_str());
        //  mqtt_client->subscribe(TopicDevice.c_str());

        UpdateConfig();
        // mqtt_client->subscribe("config");

        /*refesh report lost key*/
        retry_report_lost_key = 0;

        FlagcheckLed = true;
      }
      /*Nếu kết nối MQTT thất bại*/
      else {
        MQTT_isConnected = false;
        FlagcheckLed = true;
        //				TO_Start(&timeout_reset,
        // MQTT_TIMEOUT_RESET);
      }
    }

    else {

      uint16_t num = this->Buff_is_available();
      if (num) {
        dbg_mqtt("tx add send:%d", num);
        uint8_t databuff[num];
        memset(databuff, 0, num);
        this->readBufferSend(databuff, num);
        this->mqtt_client->publish(TopicDevice.c_str(), (char *)databuff);
        dbg_mqtt("%s", (char *)databuff);
      } else {
        UpdateConfig();
        FlagcheckLed = true; // Giống Code B: cập nhật LED mỗi 5s (trong TimeCheck), không spam
      }
    }

  }
}

void MqttHandle::mqtt_send_data(uint8_t cmd, char *AES_key, String Data) {
  dbg_mqtt("TX:%d-%s", cmd, Data.c_str());

  String mqtt_Data =
      cmd1_encryption(AES_key, (char *)Data.c_str(), Data.length());
  mqtt_client->publish(TopicDevice.c_str(), mqtt_Data.c_str());
}

String mqtt_communication_decode(String payload) {
  // if (payload.length() > 1024)
  // 	return;

  // Giải mã dữ liệu
  String data = "";
  data = cmd1_decryption(MQTT_Exchange.cmd1.TokenKey, (char *)payload.c_str(),
                         payload.length());

  // Nếu k đúng token key thì giải mã bằng default key
  if (data.indexOf("\"" + String(MQTT_SERIAL) + "\"") == -1) {
    data = cmd1_decryption(MQTT_DEFAULT_KEY, (char *)payload.c_str(),
                           payload.length());

    if (data.indexOf("\"" + String(MQTT_SERIAL) + "\"") == -1)
      cmd1_lost_key(&MQTT_Exchange.cmd1);
  }
  return data;
}

// ============================================================
// Gửi ISP3080 version qua MQTT CMD 24 mỗi 20 giây
// ============================================================
void mqtt_send_isp_version() {
  static unsigned long last_ms = 0;
  extern char g_isp_fw_version[10];
  extern char g_isp_hw_version[10];
  extern bool g_isp_version_received;

  if (!g_isp_version_received)
    return;
  if (!Mqtt_Handle.mqtt_isconnect())
    return;
  if (millis() - last_ms < 20000)
    return;
  last_ms = millis();

  DynamicJsonDocument djbco(256);
  djbco[MQTT_SERIAL] = Config_Device.Device.SerialID;
  djbco[MQTT_CMD] = MQTT_CMD_ISP_VERSION;
  djbco["ISP_FW"] = g_isp_fw_version;
  djbco["ISP_HW"] = g_isp_hw_version;
  extern volatile bool g_base_bcast_twr_active;
  djbco["BcastTWR"] = g_base_bcast_twr_active;
  djbco[MQTT_REPLY] = "";

  String data;
  serializeJson(djbco, data);
  // Không dùng mã hoá (mqtt_send_data), sử dụng thẳng MQTT Publish (send_data)
  Mqtt_Handle.send_data(TopicDevice.c_str(), data.c_str());
  // Serial.printf("\n[MQTT] ISP ver TX (UNENCRYPTED) => %s\n", data.c_str());
}

MqttHandle Mqtt_Handle;
