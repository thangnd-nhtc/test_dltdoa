#include "web_server.h"
#include "ArduinoJson.h"
#include "DataBase.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "define.h"
#include "handle_SPIFFS.h"
#include "handle_com_regs.h"
#include "handle_config.h"
#include "handle_ethernet.h"
#include "handle_spi_master.h"
#include "handle_wifi.h"
#include "iBeaconTest.h"
#include <ESPmDNS.h>
#include <FS.h>
// #include <WebServer.h>
#include <WiFi.h>
// #include <WiFiClient.h>

#include <ETH.h> // PTB add
#include <Update.h>
#include "OTA.h"

#include "uart_config.h"
// WebServer server(80);
// WebServer server(25123);
// WebServer server80(80);
AsyncWebServer server2(80);

Fag_mask_regs_t Fag_mask_regs;
master_access_t master_access;
master_t dw_master_tdoa;
master_t dw_master_twr;
anten_delay_t anten_delay_twr;
anten_delay_t anten_delay_tdoa;
dw_config_t dw_config_twr;
dw_config_t dw_config_tdoa;
dw_txconfig_t dw_txconfig_twr;
dw_txconfig_t dw_txconfig_tdoa;
dw_two_way_t two_way;
SerialID_t SerialID;
handle_led_status_t led_status_t;
beacon_cfg_t g_beacon_cfg;
String current_target_tag_id_str = "";
volatile bool g_base_bcast_twr_active = false;

// Trạng thái ACK BLE Fragment (cho trang Web polling)
// "IDLE" | "PENDING" | "ACKED" | "DONE"
String g_beacon_ack_status = "IDLE";
uint64_t g_beacon_ack_bits = 0;

// Luu ket qua Request Data (CMD 97) cho Web polling
volatile bool g_request_data_ready = false;
request_data_t g_last_request_data;

// Biến hỗ trợ OTA ISP3080
File isp3080_upload_file;
volatile bool start_isp3080_ota = false;

#define BCAST_TWR_STATE_FILE "/bcast_twr_state.bin"
#define BCAST_TWR_STATE_MAGIC 0x42545752UL // 'BTWR'
#define BCAST_TWR_STATE_VER 1

typedef struct
{
  uint32_t magic;
  uint8_t version;
  uint8_t enabled;
  uint16_t cfg_size;
  beacon_cfg_t cfg;
} bcast_twr_state_t;

static bool save_bcast_twr_state(bool enabled, const beacon_cfg_t *cfg)
{
  bcast_twr_state_t state;
  memset(&state, 0, sizeof(state));
  state.magic = BCAST_TWR_STATE_MAGIC;
  state.version = BCAST_TWR_STATE_VER;
  state.enabled = enabled ? 1 : 0;
  state.cfg_size = sizeof(beacon_cfg_t);

  if (enabled && cfg)
  {
    memcpy(&state.cfg, cfg, sizeof(beacon_cfg_t));
    state.cfg.val_id_mode = 5;
    state.cfg.enable_bcast_twr = 1;
    state.cfg.broadcast = 1;
  }

  File f = SPIFFS.open(BCAST_TWR_STATE_FILE, FILE_WRITE);
  if (!f)
  {
    debug_webserver("[BCAST_TWR] Failed to open state file for write");
    return false;
  }

  bool ok = (f.write((const uint8_t *)&state, sizeof(state)) == sizeof(state));
  f.flush();
  f.close();
  debug_webserver("[BCAST_TWR] Save full config: %d (%s)\n", enabled, ok ? "OK" : "FAIL");
  return ok;
}

static bool load_bcast_twr_state(beacon_cfg_t *cfg)
{
  if (!SPIFFS.exists(BCAST_TWR_STATE_FILE))
    return false;

  File f = SPIFFS.open(BCAST_TWR_STATE_FILE, FILE_READ);
  if (!f)
    return false;

  bcast_twr_state_t state;
  memset(&state, 0, sizeof(state));
  size_t n = f.read((uint8_t *)&state, sizeof(state));
  f.close();

  if (n != sizeof(state) || state.magic != BCAST_TWR_STATE_MAGIC ||
      state.version != BCAST_TWR_STATE_VER ||
      state.cfg_size != sizeof(beacon_cfg_t) || state.enabled != 1)
  {
    debug_webserver("[BCAST_TWR] State invalid/off");
    return false;
  }

  if (cfg)
  {
    memcpy(cfg, &state.cfg, sizeof(beacon_cfg_t));
    cfg->val_id_mode = 5;
    cfg->enable_bcast_twr = 1;
    cfg->broadcast = 1;
  }
  return true;
}

void restore_bcast_twr_after_boot()
{
  beacon_cfg_t saved_cfg;
  memset(&saved_cfg, 0, sizeof(saved_cfg));
  bool enabled = load_bcast_twr_state(&saved_cfg);
  g_base_bcast_twr_active = enabled;
  if (!enabled)
  {
    debug_webserver("[BCAST_TWR] Restore state: OFF");
    return;
  }

  // Khi U6 đã nhận ISP version nghĩa là U7/ISP đã sẵn sàng.
  // Lúc đó gửi lại gói cấu hình tối thiểu tương đương:
  // {"SerialID": Id base, "CMD":20, "enable_bcast_twr":true, "val_id_mode":5}
  memset(&g_beacon_cfg, 0, sizeof(g_beacon_cfg));
  g_beacon_cfg.val_id_mode = 5;
  g_beacon_cfg.enable_bcast_twr = 1;

  uint32_t temp_id = Config_Device.Device.SerialID;
  for (int i = 4; i >= 0; i--)
  {
    uint8_t two_digits = temp_id % 100;
    temp_id /= 100;
    g_beacon_cfg.SerialID[i] = ((two_digits / 10) << 4) | (two_digits % 10);
  }

  Fag_mask_regs.FAG_BEACON_RES = 2;
  debug_webserver("[BCAST_TWR] Restore CMD20: SerialID=%lu, mode=5, bcast_twr=1 -> schedule BEACON_CFG\n",
                (unsigned long)Config_Device.Device.SerialID);
}

// const char HTTP_HEAD_WEB[] PROGMEM = "<html lang=\"en\"><head><meta
// http-equiv=\"Content-Type\" content=\"text/html;	charset=utf-8\"><meta
// name=\"viewport\" content=\"width=device-width, initial-scale=1,
// user-scalable=no\"><title>{v}</title>"; const char HTTP_STYLE[] PROGMEM =
// "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;}
// input{width:100%;} body{text-align: center;font-family:verdana;}
// button{border:0;border-radius:0.3rem;background-color:#4CAF50;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}
// .q{float: right;width: 64px;text-align: right;} .l{background:
// url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\")
// no-repeat left center;background-size: 1em;}</style>";
// // Style button
// const char HTTP_STYLEB[] PROGMEM = "<style>.c{text-align: center;}
// div,input{padding:5px;font-size:1em;} input{width:100%;} body{text-align:
// center;font-family:verdana;}
// button{border:0;border-radius:0.3rem;background-color:#4CAF50;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}
// .q{float: right;width: 64px;text-align: right;} .l{background:
// url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\")
// no-repeat left center;background-size: 1em;}"
// 								   ".switch
// {position: relative;display: inline-block;width: 60px;height: 34px;}.switch
// input {display:none;}.slider {position: absolute;cursor: pointer;top: 0;left:
// 0;right: 0;bottom: 0;background-color: #ccc;-webkit-transition:
// .4s;transition: .4s;}.slider:before {position: absolute;content: \"\";height:
// 26px;width: 26px;left: 4px;bottom: 4px;background-color:
// white;-webkit-transition: .4s;transition: .4s;}input:checked + .slider
// {background-color: #2196F3;}input:focus + .slider {box-shadow: 0 0 1px
// #2196F3;}input:checked + .slider:before {-webkit-transform:
// translateX(26px);-ms-transform: translateX(26px);transform:
// translateX(26px);}/* Rounded sliders */.slider.round {border-radius:
// 34px;}.slider.round:before {border-radius: 50%;}</style>";

// const char HTTP_SCRIPT[] PROGMEM = "<script>function
// c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
// const char HTTP_HEAD_WEB_END[] PROGMEM = "</head><body><div
// style='text-align:left;display:inline-block;min-width:260px;'>";

// const char HTTP_ITEM[] PROGMEM = "<div><a href='#p'
// onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>"; const
// char HTTP_FORM_START[] PROGMEM = "<form method='post' action='wlansave'>";
// const char HTTP_FORM_PARAM[] PROGMEM = "<br><input id='s' name='n' length=32
// placeholder='SSID'><br><input id='p' name='p' length=64 type='password'
// placeholder='password'><br>"; const char HTTP_FORM_END[] PROGMEM =
// "<br><br><br><button type='submit'>save</button></form>"; const char
// HTTP_WLAN_REFRESH[] PROGMEM = "<br><div class=\"c\"><a
// href=\"/wlanconf\">Scan</a></div>"; const char HTTP_END[] PROGMEM =
// "</div></body></html>";

// AsyncWebServerRequest *request;

// bool WebAuthCheck(char *User, char *Pass)
// {
// 	// AsyncWebServerRequest *request;

// 	// if (!request->authenticate(User, Pass) &&
// !request->authenticate("admin", "2109"))
// 	// {
// 	// 	request->requestAuthentication();
// 	// 	return 0;
// 	// }
// 	return 1;
// }

// Helper convert "0125100000" (Hex-pair string) -> 5 bytes
void hexToBytes(String hex, uint8_t *bytes, int len)
{
  // Xóa sạch mảng trước khi ghi (đảm bảo các byte trống là 0)
  memset(bytes, 0, len);

  int strLen = hex.length();
  int byteIdx = len - 1; // Bắt đầu từ byte cuối cùng

  // Duyệt chuỗi từ cuối lên đầu, mỗi lần lấy 2 ký tự (1 byte)
  for (int i = strLen; i > 0; i -= 2)
  {
    if (byteIdx < 0)
      break; // Đã điền hết mảng bytes

    int startIdx = max(0, i - 2);
    String part = hex.substring(startIdx, i);
    bytes[byteIdx--] = (uint8_t)strtol(part.c_str(), NULL, 16);
  }
}

// Helper convert 5 bytes -> Hex-pair string
String bytesToHex(uint8_t *bytes, int len)
{
  String res = "";
  for (int i = 0; i < len; i++)
  {
    char buf[3];
    sprintf(buf, "%02X", bytes[i]);
    res += buf;
  }
  return res;
}

void base_config_get(AsyncWebServerRequest *request)
{
  debug_webserver("get baseconfig");
  DynamicJsonDocument doc(2048);
  doc["Serial"] = Config_Device.Device.Serial;
  doc["SerialID"] = Config_Device.Device.SerialID;
  doc["ServerMQTT"] = Config_Internet.MQTT.Server;
  // doc["CMD"] = MQTT_CMD_SERVER_DATA;
  // doc["CMDServerID"] = 0;
  // doc["Packit ID"] = 0;

  doc["PortMQTT"] = Config_Internet.MQTT.Port;
  doc["UserMQTT"] = Config_Internet.MQTT.User;
  doc["PassMQTT"] = Config_Internet.MQTT.Pass;
  doc["Topicserver"] = Config_Internet.MQTT.TopicServer;
  doc["Topicdevice"] = Config_Internet.MQTT.TopicDevice;
  doc["ServerTCP"] = Config_Internet.TCP.Server;
  doc["PortTCP"] = Config_Internet.TCP.Port;
  doc["ServerFTP"] = Config_Internet.FTP.Server;
  doc["PortFTP"] = Config_Internet.FTP.Port;
  doc["UserFTP"] = Config_Internet.FTP.User;
  doc["PassFTP"] = Config_Internet.FTP.Pass;
  doc["AuthUser"] = Config_Internet.Auth.User;
  doc["AuthPass"] = Config_Internet.Auth.Pass;
  doc["SSIDWifiSTA"] = Config_Internet.Wifi_STA.Ssid;
  doc["PassWifiSTA"] = Config_Internet.Wifi_STA.Pass;
  doc["SSIDWifiAP"] = Config_Internet.Wifi_AP.Ssid;
  doc["PassWifiAP"] = Config_Internet.Wifi_AP.Pass;

  char ip[20];
  char gw[20];
  char sn[20];
  char dns[20];

  if (eth_connected)
  {
    doc["ConnectionMode"] = "E";
    sprintf(ip, "%s", Config_Internet.Ethernet.Ip.toString().c_str());
    sprintf(gw, "%s", Config_Internet.Ethernet.Gw.toString().c_str());
    sprintf(sn, "%s", Config_Internet.Ethernet.Sn.toString().c_str());
    sprintf(dns, "%s", Config_Internet.Ethernet.Dns.toString().c_str());
    doc["IP"] = ip;
    doc["GW"] = gw;
    doc["Sn"] = sn;
    doc["DNS"] = dns;
  }

  else if (wifi_connected)
  {
    doc["ConnectionMode"] = "W";
    sprintf(ip, "%s", Config_Internet.Wifi_STA.Ip.toString().c_str());
    sprintf(gw, "%s", Config_Internet.Wifi_STA.Gw.toString().c_str());
    sprintf(sn, "%s", Config_Internet.Wifi_STA.Sn.toString().c_str());
    sprintf(dns, "%s", Config_Internet.Wifi_STA.Dns.toString().c_str());
    doc["IP"] = ip;
    doc["GW"] = gw;
    doc["Sn"] = sn;
    doc["DNS"] = dns;
  }
  else
  {
    doc["ConnectionMode"] = "N";
    doc["IP"] = "0.0.0.0";
    doc["GW"] = "0.0.0.0";
    doc["Sn"] = "0.0.0.0";
    doc["DNS"] = "0.0.0.0";
  }

  doc["EMAC"] = Config_Internet.Ethernet.Mac;
  doc["WMAC"] = Config_Internet.Wifi_STA.Mac;

  String output = "";
  serializeJson(doc, output);
  // free(doc);
  request->send(200, "text/json", output);
}

static bool saveBaseConfigAtomic(const String &data)
{
  static const char *tempPath = "/baseconfig.tmp";

  if (SPIFFS.exists(tempPath) && !SPIFFS.remove(tempPath))
  {
    debug_webserver("[BASE_CFG] Cannot remove stale temp file");
    return false;
  }

  File tempFile = SPIFFS.open(tempPath, FILE_WRITE);
  if (!tempFile)
  {
    debug_webserver("[BASE_CFG] Cannot open temp file for writing");
    return false;
  }

  const size_t expected = data.length();
  const size_t written = tempFile.write(
      reinterpret_cast<const uint8_t *>(data.c_str()), expected);
  tempFile.close();

  if (written != expected)
  {
    debug_webserver("[BASE_CFG] Incomplete write: %u/%u bytes",
                    static_cast<unsigned>(written),
                    static_cast<unsigned>(expected));
    SPIFFS.remove(tempPath);
    return false;
  }

  // Chỉ xóa file cũ sau khi file tạm đã được ghi đủ.
  if (SPIFFS.exists(BASECONFIG) && !SPIFFS.remove(BASECONFIG))
  {
    debug_webserver("[BASE_CFG] Cannot remove current config");
    SPIFFS.remove(tempPath);
    return false;
  }

  if (!SPIFFS.rename(tempPath, BASECONFIG))
  {
    debug_webserver("[BASE_CFG] Cannot commit temp file");
    SPIFFS.remove(tempPath);
    return false;
  }

  return true;
}

void base_config_post(AsyncWebServerRequest *request)
{
  debug_webserver("post baseconfig");
  String data = request->arg((size_t)0);
  debug_webserver("%s", data.c_str());
  // String data = request->arg(0);
  // for(int i=0;i<1;i++)

  // debug_webserver("ARG[%s]: %s\n", request->argName(i).c_str(),
  // request->arg(i).c_str());

  size_t size = data.length();
  DynamicJsonDocument doc(size + 1024);
  auto error = deserializeJson(doc, data);
  if (error)
  {
    debug_webserver("deserializeJson() failed: %s", error.c_str());
    request->send(404, "text/html", "Json Format Error" + data);
    return;
  }

  // Tạm lưu thông số để xem có cần đổi MAC / Hostname không
  bool mac_or_host_changed = false;
  String new_mac = String(Config_Internet.Ethernet.Mac);
  String new_host = String(Config_Device.Device.HostName);
  String new_devId = String(Config_Device.Device.SerialID);

  if (doc.containsKey("EMAC") && doc["EMAC"].as<String>() != "")
  {
    new_mac = doc["EMAC"].as<String>();
    if (new_mac != String(Config_Internet.Ethernet.Mac))
      mac_or_host_changed = true;
  }
  if (doc.containsKey("HostName") && doc["HostName"].as<String>() != "")
  {
    new_host = doc["HostName"].as<String>();
    if (new_host != String(Config_Device.Device.HostName))
      mac_or_host_changed = true;
  }
  if (doc.containsKey("SerialID"))
  {
    new_devId = doc["SerialID"].as<String>();
  }

  // Chỉ ghi EEPROM và đồng bộ peer khi SerialID thực sự thay đổi.
  // Web luôn gửi SerialID trong mọi lần Save; đồng bộ lại ID không đổi có thể
  // chặn request nhiều giây do UART retry/timeout.
  if (doc.containsKey("SerialID") && !mac_or_host_changed)
  {
    uint32_t devid = doc["SerialID"].as<uint32_t>();
    if (devid != Config_Device.Device.SerialID)
    {
      debug_webserver("SerialID changed: %lu -> %lu\n",
                      Config_Device.Device.SerialID, devid);
      saveDevIdRaw(devid); // Gọi hàm từ uart_config
      Config_Device.Device.SerialID = devid;
      pushAndConfirmPeerDeviceId(devid, /*doConfirm=*/true);
    }
  }

  if (!saveBaseConfigAtomic(data))
  {
    debug_webserver("[BASE_CFG] Update failed; previous config preserved");
    request->send(200, "text/plain", "Update FAILED: cannot save BaseConfig, old config preserved");
    return;
  }

  Config_Device.checkBaseConfig();

  Fag_mask_regs.FAG_SerialID_RES = 2;
  request->send(200, "text/html", "Update OK");

  // ✅ Kích hoạt đổi MAC, Hostname và ID qua thư viện UartConfig nếu có thay đổi
  // Lệnh CFG NET=... sẽ tự động lưu EEPROM, đồng bộ File và kích hoạt ESPRebootTo (Delay Reboot 1s)
  if (mac_or_host_changed)
  {
    String cfg_cmd = "CFG NET=" + new_mac + "," + new_host + "," + new_devId;
    debug_webserver("[WEB_CFG] Detecting MAC/Host changed! Sending: %s\n", cfg_cmd.c_str());
    UartConfig::handleLine(cfg_cmd);
  }

  // AsyncWebParameter* p = request->arg(0);
  // String data = request->arg(0);
  // Serial.printf("%s",request->arg(0).c_str());
}

void dw1000config_get(AsyncWebServerRequest *request)
{
  debug_webserver("get dw1000config");
  DynamicJsonDocument djbco(1024);
  // JsonObject& root = djbco.createObject();
  // dw_config_twr
  djbco["chan"] = dw_config_twr.chan;
  djbco["prf"] = dw_config_twr.prf;
  djbco["txPreambLength"] = dw_config_twr.txPreambLength;

  djbco["rxPAC"] = dw_config_twr.rxPAC;
  djbco["txCode"] = dw_config_twr.txCode;
  djbco["rxCode"] = dw_config_twr.rxCode;
  djbco["nsSFD"] = dw_config_twr.nsSFD;

  djbco["dataRate"] = dw_config_twr.dataRate;
  djbco["phrMode"] = dw_config_twr.phrMode;
  djbco["sfdTO"] = dw_config_twr.sfdTO;

  // char Buf[11];
  // snprintf(Buf,11,"0x%02X",dw_txconfig_tdoa.PGdly);
  // djbco["PGdly"] = Buf;
  // snprintf(Buf,11,"0x%08X",dw_txconfig_tdoa.power);

  // dw_txconfig_twr
  djbco["power"] = dw_txconfig_twr.power;
  djbco["PGdly"] = dw_txconfig_twr.PGdly;

  // djbco["power"] = Buf;
  djbco["isEnable"] = dw_master_twr.enable;
  djbco["DW_Interval"] = dw_master_twr.interval;
  djbco["Dw_Pre_Tx"] = dw_master_twr.time_prepare;

  djbco["Esp_Pre_Tx"] = 0;
  // doc["DW_Interval"] = 0;
  // doc["Dw_Pre_Tx"] = 1;
  djbco["Rx_Delay"] = 0;
  djbco["Rx_Timeout"] = 0;
  djbco["Tx_Delay"] = 0;
  djbco["AntenRx_Delay"] = anten_delay_twr.rx;
  djbco["AntenTx_Delay"] = anten_delay_twr.tx;

  // String json = "{\"chan\":1,\"prf\":1,\"txPreambLength\":4,\"rxPAC\":1,\"txCode\":1,\"rxCode\":4,\"nsSFD\":1,\
  // 					//							 \"dataRate\":1,\"phrMode\":0,\"sfdTO\":1,\"PGdly\":\"0x95\",\"power\":\"0x1F1F1F1F\",\
  // 					//							 \"Esp_Pre_Tx\":1,\"DW_Interval\":0,\"Dw_Pre_Tx\":1}";
  String json;
  serializeJson(djbco, json);
  // free(djbco);
  debug_webserver("%s", json.c_str());
  request->send(200, "text/json", json);
}

void dw1000config_post(AsyncWebServerRequest *request)
{
  debug_webserver("post dw1000config");
  String data = request->arg((size_t)1);
  debug_webserver("%s", data.c_str());
  size_t size = data.length();
  DynamicJsonDocument djbpo(size + 1000);
  auto error = deserializeJson(djbpo, data);
  if (error)
  {
    request->send(404, "text/html", "Json Format Error" + data);
    return;
  }

  // debug_webserver("%s",data.c_str());

  if (!djbpo["chan"].isNull())
    dw_config_tdoa.chan = djbpo["chan"];
  if (!djbpo["prf"].isNull())
    dw_config_tdoa.prf = djbpo["prf"];
  if (!djbpo["txPreambLength"].isNull())
    dw_config_tdoa.txPreambLength = djbpo["txPreambLength"];
  if (!djbpo["rxPAC"].isNull())
    dw_config_tdoa.rxPAC = djbpo["rxPAC"];
  if (!djbpo["txCode"].isNull())
    dw_config_tdoa.txCode = djbpo["txCode"];
  if (!djbpo["rxCode"].isNull())
    dw_config_tdoa.rxCode = djbpo["rxCode"];
  if (!djbpo["nsSFD"].isNull())
    dw_config_tdoa.nsSFD = djbpo["nsSFD"];
  if (!djbpo["dataRate"].isNull())
    dw_config_tdoa.dataRate = djbpo["dataRate"];
  if (!djbpo["phrMode"].isNull())
    dw_config_tdoa.phrMode = djbpo["phrMode"];
  if (!djbpo["sfdTO"].isNull())
    dw_config_tdoa.sfdTO = djbpo["sfdTO"];
  Fag_mask_regs.FAG_DW_CONFIG_RES = 2;

  // dw_txconfig_tdoa.PGdly =
  // (uint8_t)strtol(djbpo["PGdly"].as<char*>(),NULL,0); dw_txconfig_tdoa.power
  // = (uint32_t)strtol(djbpo["power"].as<char*>(),NULL,0);
  if (!djbpo["PGdly"].isNull())
    dw_txconfig_tdoa.PGdly = djbpo["PGdly"];
  if (!djbpo["power"].isNull())
    dw_txconfig_tdoa.power = djbpo["power"];
  Fag_mask_regs.FAG_DW_CONFIG_TX_RES = 2;

  if (!djbpo["AntenRx_Delay"].isNull())
  {
    anten_delay_tdoa.rx = djbpo["AntenRx_Delay"];
    Fag_mask_regs.FAG_DW_ANT_DELAY_RES = 2;
  }
  if (!djbpo["AntenTx_Delay"].isNull())
    anten_delay_tdoa.tx = djbpo["AntenTx_Delay"];

  if (!djbpo["isEnable"].isNull())
  {
    dw_master_tdoa.enable = djbpo["isEnable"];
    Fag_mask_regs.FAG_MASTER_RES = 2;

    if (!djbpo["Dw_Pre_Tx"].isNull())
      dw_master_tdoa.time_prepare = djbpo["Dw_Pre_Tx"];
    else
      dw_master_tdoa.time_prepare = 0;
    if (!djbpo["DW_Interval"].isNull())
      dw_master_tdoa.interval = djbpo["DW_Interval"];
    else
      dw_master_tdoa.interval = 0;
  }
  request->send(200, "text/html", "Update OK");
}

// void DHCPconfig_get(AsyncWebServerRequest *request)
// {
// 	debug_webserver("get DHCPconfig");
// 	DynamicJsonDocument doc(512);
// 	doc["DhcpMode"] = Config_Internet.DHCP_setting.DHCPEn;
// 	doc["IP"] = Config_Internet.DHCP_setting.Ip;
// 	doc["GW"] = Config_Internet.DHCP_setting.Gw;
// 	doc["Sn"] = Config_Internet.DHCP_setting.Sn;
// 	doc["DNS"] = Config_Internet.DHCP_setting.Dns;
// 	doc["EMAC"] = Config_Internet.Ethernet.Mac;
// 	doc["WMAC"] = Config_Internet.Wifi_STA.Mac;
// 	String output = "";
// 	serializeJson(doc, output);
// 	request->send(200, "text/json", output);
// }
// PTB custom
void DHCPconfig_get(AsyncWebServerRequest *request)
{
  debug_webserver("get DHCPconfig");

  // Cập nhật lại thông tin DHCP/Static trước khi trả về
  if (Config_Internet.DHCP_setting.DHCPEn == 1)
  {
    // DHCP Auto → lấy thông tin IP hiện tại từ stack Ethernet
    Config_Internet.Ethernet.Ip = ETH.localIP();
    Config_Internet.Ethernet.Gw = ETH.gatewayIP();
    Config_Internet.Ethernet.Sn = ETH.subnetMask();
    Config_Internet.Ethernet.Dns = ETH.dnsIP();

    strcpy(Config_Internet.DHCP_setting.Ip,
           Config_Internet.Ethernet.Ip.toString().c_str());
    strcpy(Config_Internet.DHCP_setting.Gw,
           Config_Internet.Ethernet.Gw.toString().c_str());
    strcpy(Config_Internet.DHCP_setting.Sn,
           Config_Internet.Ethernet.Sn.toString().c_str());
    strcpy(Config_Internet.DHCP_setting.Dns,
           Config_Internet.Ethernet.Dns.toString().c_str());

    debug_webserver("DHCP Mode: Auto");
    debug_webserver("IP:%s GW:%s SN:%s DNS:%s", Config_Internet.DHCP_setting.Ip,
                    Config_Internet.DHCP_setting.Gw,
                    Config_Internet.DHCP_setting.Sn,
                    Config_Internet.DHCP_setting.Dns);
  }
  else
  {
    debug_webserver("DHCP Mode: Manual");
    debug_webserver("IP:%s GW:%s SN:%s DNS:%s", Config_Internet.DHCP_setting.Ip,
                    Config_Internet.DHCP_setting.Gw,
                    Config_Internet.DHCP_setting.Sn,
                    Config_Internet.DHCP_setting.Dns);
  }

  // Đóng gói JSON trả về cho web client
  DynamicJsonDocument doc(512);
  doc["DhcpMode"] = Config_Internet.DHCP_setting.DHCPEn;
  doc["IP"] = Config_Internet.DHCP_setting.Ip;
  doc["GW"] = Config_Internet.DHCP_setting.Gw;
  doc["Sn"] = Config_Internet.DHCP_setting.Sn;
  doc["DNS"] = Config_Internet.DHCP_setting.Dns;
  doc["EMAC"] = Config_Internet.Ethernet.Mac;
  doc["WMAC"] = Config_Internet.Wifi_STA.Mac;

  String output;
  serializeJson(doc, output);

  // Trả về kết quả dạng JSON
  request->send(200, "application/json", output);
}

void DHCPconfig_post(AsyncWebServerRequest *request)
{
  debug_webserver("post DHCPconfig");
  String data = request->arg((size_t)0);
  debug_webserver("%s", data.c_str());
  size_t size = data.length();
  DynamicJsonDocument doc(1024);
  auto error = deserializeJson(doc, data);
  if (error)
  {
    debug_webserver("deserializeJson() failed: %s", error.c_str());
    request->send(404, "text/html", "Json Format Error" + data);
    return;
  }

  if (SPIFFS.exists(DHCPCONFIG) == true)
    SPIFFS.remove(DHCPCONFIG);
  _handle_SPIFFS.writeFile(DHCPCONFIG, (uint8_t *)data.c_str(), size);
  Config_Device.checkDHCPconfig();
}

void base_twr_get(AsyncWebServerRequest *request)
{
  debug_webserver("get base twr");
  String data = request->arg((size_t)0);
  debug_webserver("%s", data.c_str());
  MQTT_Exchange.cmd8_9_10_11.BaseID = atoi(data.c_str());
  Fag_mask_regs.FAG_TWO_WAY_RES = 2;
  SPI_master.setConfigDW();
  char m_distance[10];
  delay(3000);
  sprintf(m_distance, "{\"Twr\":%lu}", two_way.distance);
  request->send(200, "text/html", m_distance);
}

// void beconconfig_get(AsyncWebServerRequest *request)
// {
// 	debug_webserver("get beacon config");

// 	// Đọc dữ liệu từ file nếu có
// 	String dataFromFile = "";
// 	if (SPIFFS.exists(BEACONCONFIG))
// 	{
// 		dataFromFile = _handle_SPIFFS.readInformations(BEACONCONFIG);
// 	}

// 	// Nếu có dữ liệu từ file, parse và trả về
// 	if (dataFromFile.length() > 0)
// 	{
// 		DynamicJsonDocument doc(4096);
// 		auto error = deserializeJson(doc, dataFromFile);
// 		if (!error)
// 		{
// 			// Đảm bảo có đầy đủ 37 trường
// 			// Thêm SerialID nếu chưa có
// 			if (!doc.containsKey("SerialID"))
// 			{
// 				doc["SerialID"] = Config_Device.Device.SerialID;
// 			}
// 			// Thêm val_id_last nếu chưa có
// 			if (!doc.containsKey("val_id_last"))
// 			{
// 				doc["val_id_last"] =
// Config_Device.Device.SerialID;
// 			}

// 			String output = "";
// 			serializeJson(doc, output);
// 			request->send(200, "text/json", output);
// 			return;
// 		}
// 	}

// 	// Fallback: trả về đầy đủ 37 trường với giá trị mặc định
// 	DynamicJsonDocument doc(4096);

// 	// Timer/Send Tag (5 trường)
// 	doc["val_motion"] = JsonNull();
// 	doc["val_stand"] = JsonNull();
// 	doc["val_sleep1"] = JsonNull();
// 	doc["val_sleep2"] = JsonNull();
// 	doc["val_sleep3"] = JsonNull();

// 	// Sleep Mode (3 trường)
// 	doc["val_mode1"] = JsonNull();
// 	doc["val_mode2"] = JsonNull();
// 	doc["val_mode3"] = JsonNull();

// 	// Motion (3 trường)

// 	// Battery (4 trường)
// 	doc["val_batt_default"] = JsonNull();
// 	doc["val_batt_high"] = JsonNull();
// 	doc["val_batt_inc"] = JsonNull();
// 	doc["val_batt_dec"] = JsonNull();

// 	// UART/Charge/LED/Airplane (6 trường)
// 	doc["val_uart_led"] = JsonNull();
// 	doc["val_charge_noise"] = JsonNull();
// 	doc["val_charge_update"] = JsonNull();
// 	doc["val_led_blink"] = JsonNull();
// 	doc["val_led_blink_ms"] = JsonNull();
// 	doc["val_airplane"] = JsonNull();

// 	// ID Configuration (3 trường)
// 	doc["val_id_last"] = Config_Device.Device.SerialID;
// 	doc["SerialID"] = Config_Device.Device.SerialID;
// 	doc["val_id_new"] = JsonNull();
// 	doc["val_id_mode"] = JsonNull();

// 	// UWB/DWT Configuration (13 trường)
// 	doc["uwb_chan"] = JsonNull();
// 	doc["uwb_plen"] = JsonNull();
// 	doc["uwb_pac"] = JsonNull();
// 	doc["uwb_txcode"] = JsonNull();
// 	doc["uwb_rxcode"] = JsonNull();
// 	doc["uwb_sfdtype"] = JsonNull();
// 	doc["uwb_datarate"] = JsonNull();
// 	doc["uwb_phrmode"] = JsonNull();
// 	doc["uwb_phrrate"] = JsonNull();
// 	doc["uwb_sfdto"] = JsonNull();
// 	doc["uwb_stsmode"] = JsonNull();
// 	doc["uwb_stslen"] = JsonNull();
// 	doc["uwb_pdoa"] = JsonNull();

// 	String output = "";
// 	serializeJson(doc, output);
// 	request->send(200, "text/json", output);
// }

void beconconfig_get(AsyncWebServerRequest *request)
{
  debug_webserver("Get beacon config");

  DynamicJsonDocument doc(4096);
  // 2. Fallback lấy dữ liệu từ struct nếu không có file
  doc["val_motion"] = g_beacon_cfg.val_motion;
  doc["val_stand"] = g_beacon_cfg.val_stand;
  doc["val_sleep1"] = g_beacon_cfg.val_sleep1;
  doc["val_sleep2"] = g_beacon_cfg.val_sleep2;
  doc["val_sleep3"] = g_beacon_cfg.val_sleep3;

  doc["val_mode1"] = g_beacon_cfg.val_mode1;
  doc["val_mode2"] = g_beacon_cfg.val_mode2;
  doc["val_mode3"] = g_beacon_cfg.val_mode3;

  doc["val_batt_default"] = g_beacon_cfg.val_batt_default;
  doc["val_batt_high"] = g_beacon_cfg.val_batt_high;
  doc["val_batt_inc"] = g_beacon_cfg.val_batt_inc;
  doc["val_batt_dec"] = g_beacon_cfg.val_batt_dec;

  // Chỉ lấy giá trị thực tế trong struct g_beacon_cfg
  doc["val_id_last"] = bytesToHex(g_beacon_cfg.val_id_last, 5);
  doc["SerialID"] = bytesToHex(g_beacon_cfg.SerialID, 5);
  doc["val_id_new"] = bytesToHex(g_beacon_cfg.val_id_new, 5);
  doc["val_id_mode"] = g_beacon_cfg.val_id_mode;
  doc["val_request"] = g_beacon_cfg.val_request;
  doc["val_id_enable"] = (g_beacon_cfg.val_id_change == 1);
  doc["broadcast"] = (g_beacon_cfg.broadcast == 1);
  doc["enable_bcast_twr"] = (g_beacon_cfg.enable_bcast_twr == 1);
  doc["charge_tx"] = g_beacon_cfg.charge_tx;                   // 0: ignore, 1: OFF, 2: ON
  doc["ota_enable"] = g_beacon_cfg.ota_enable;                 // 0: ignore, 1: ENABLE OTA
  doc["sys_config_default"] = g_beacon_cfg.sys_config_default; // 0: ignore, 1: reset to default
  doc["sleep_enable"] = g_beacon_cfg.sleep_enable;             // 0: ignore, 1: DISABLE, 2: ENABLE

  doc["uwb_chan"] = g_beacon_cfg.uwb_chan;
  doc["uwb_plen"] = g_beacon_cfg.uwb_plen;
  doc["uwb_pac"] = g_beacon_cfg.uwb_pac;
  doc["uwb_txcode"] = g_beacon_cfg.uwb_txcode;
  doc["uwb_rxcode"] = g_beacon_cfg.uwb_rxcode;
  doc["uwb_sfdtype"] = g_beacon_cfg.uwb_sfdtype;
  doc["uwb_datarate"] = g_beacon_cfg.uwb_datarate;
  doc["uwb_phrmode"] = g_beacon_cfg.uwb_phrmode;
  doc["uwb_phrrate"] = g_beacon_cfg.uwb_phrrate;
  doc["uwb_sfdto"] = g_beacon_cfg.uwb_sfdto;
  doc["uwb_stsmode"] = g_beacon_cfg.uwb_stsmode;
  doc["uwb_stslen"] = g_beacon_cfg.uwb_stslen;
  doc["uwb_pdoa"] = g_beacon_cfg.uwb_pdoa;

  // Bổ sung SerialID nếu trong file bị thiếu

  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

// void beconconfig_post(AsyncWebServerRequest *request)
// {
// 	debug_webserver("post beacon config");
// ... logic cũ ...
// }

void beconconfig_post(AsyncWebServerRequest *request)
{
  debug_webserver("Post beacon config: Starting update...");

  if (request->args() == 0)
  {
    request->send(400, "text/html", "Error: No data received");
    return;
  }

  // 1. Lấy dữ liệu từ Request bằng tham chiếu (tránh copy String)
  // Fix lỗi ambiguous overload bằng cách cast (size_t)0
  const String &data = request->hasArg("beaconConfig")
                           ? request->arg("beaconConfig")
                           : request->arg((size_t)0);

  size_t size = data.length();
  if (size == 0)
  {
    request->send(400, "text/html", "Error: Empty data");
    return;
  }

  debug_webserver("Beacon POST size: %d | Heap: %d\n", size, ESP.getFreeHeap());

  // Use Heap for JsonDocument - 4096 bytes is safe
  DynamicJsonDocument *docPtr = new DynamicJsonDocument(4096);
  if (!docPtr)
  {
    debug_webserver("Failed to allocate memory for JSON");
    request->send(500, "text/html", "Server Memory Error");
    return;
  }
  DynamicJsonDocument &djbpo = *docPtr;

  DeserializationError error = deserializeJson(djbpo, data);

  if (error)
  {
    debug_webserver("JSON Parse Fail: %s\n", error.c_str());
    request->send(400, "text/html", "Json Format Error");
    delete docPtr;
    return;
  }

  if (apply_beacon_config_json(djbpo))
  {
    request->send(200, "text/html", "Update OK");
  }
  else
  {
    request->send(500, "text/html", "Update Failed");
  }

  delete docPtr; // Comment out to test if delete causes crash
}

bool apply_beacon_config_json(DynamicJsonDocument &djbpo)
{
  // Reset toàn bộ struct trước khi parse JSON mới
  // Chỉ những field có trong JSON mới sẽ được gán giá trị != 0
  // Từ đó ISP3080 chỉ phát fragment cho field thực sự có trong JSON
  memset(&g_beacon_cfg, 0, sizeof(beacon_cfg_t));
  g_beacon_cfg.val_id_mode = 0xFE; // 0xFE: No change (Prevent accidental State_Default trigger)
  current_target_tag_id_str = "";  // Reset target ID string

  // --- Timer/Send Tag ---
  if (djbpo.containsKey("val_motion"))
    g_beacon_cfg.val_motion = djbpo["val_motion"].as<uint16_t>();
  if (djbpo.containsKey("val_stand"))
    g_beacon_cfg.val_stand = djbpo["val_stand"].as<uint16_t>();
  if (djbpo.containsKey("val_sleep1"))
    g_beacon_cfg.val_sleep1 = djbpo["val_sleep1"].as<uint16_t>();
  if (djbpo.containsKey("val_sleep2"))
    g_beacon_cfg.val_sleep2 = djbpo["val_sleep2"].as<uint16_t>();
  if (djbpo.containsKey("val_sleep3"))
    g_beacon_cfg.val_sleep3 = djbpo["val_sleep3"].as<uint16_t>();
  // --- Sleep Mode ---
  if (djbpo.containsKey("val_mode1"))
    g_beacon_cfg.val_mode1 = djbpo["val_mode1"].as<uint8_t>();
  if (djbpo.containsKey("val_mode2"))
    g_beacon_cfg.val_mode2 = djbpo["val_mode2"].as<uint8_t>();
  if (djbpo.containsKey("val_mode3"))
    g_beacon_cfg.val_mode3 = djbpo["val_mode3"].as<uint8_t>();
  // --- Battery ---
  if (djbpo.containsKey("val_batt_default"))
    g_beacon_cfg.val_batt_default = djbpo["val_batt_default"].as<uint16_t>();
  if (djbpo.containsKey("val_batt_high"))
    g_beacon_cfg.val_batt_high = djbpo["val_batt_high"].as<uint16_t>();
  if (djbpo.containsKey("val_batt_inc"))
    g_beacon_cfg.val_batt_inc = djbpo["val_batt_inc"].as<uint16_t>();
  if (djbpo.containsKey("val_batt_dec"))
    g_beacon_cfg.val_batt_dec = djbpo["val_batt_dec"].as<uint16_t>();

  // --- ID Configuration ---
  if (djbpo.containsKey("val_id_last"))
  {
    current_target_tag_id_str = djbpo["val_id_last"].as<String>();
    hexToBytes(current_target_tag_id_str, g_beacon_cfg.val_id_last, 5);
  }
  if (djbpo.containsKey("SerialID"))
    hexToBytes(djbpo["SerialID"].as<String>(), g_beacon_cfg.SerialID, 5);
  if (djbpo.containsKey("val_id_new"))
    hexToBytes(djbpo["val_id_new"].as<String>(), g_beacon_cfg.val_id_new, 5);
  if (djbpo.containsKey("val_id_enable"))
    g_beacon_cfg.val_id_change = djbpo["val_id_enable"].as<bool>() ? 1 : 0;
  if (djbpo.containsKey("broadcast"))
    g_beacon_cfg.broadcast = djbpo["broadcast"].as<bool>() ? 1 : 0;
  if (djbpo.containsKey("enable_bcast_twr"))
  {
    g_beacon_cfg.enable_bcast_twr = djbpo["enable_bcast_twr"].as<bool>() ? 1 : 0;
    g_base_bcast_twr_active = (g_beacon_cfg.enable_bcast_twr == 1);
  }
  // --- CHARGE_TX (0x40) ---
  if (djbpo.containsKey("charge_tx"))
    g_beacon_cfg.charge_tx = djbpo["charge_tx"].as<uint8_t>(); // 1=OFF, 2=ON
  // --- OTA (0x4001) ---
  if (djbpo.containsKey("ota_enable"))
    g_beacon_cfg.ota_enable = djbpo["ota_enable"].as<uint8_t>(); // 1=ENABLE
  // --- SYS_CONFIG_DEFAULT (0x4002) ---
  if (djbpo.containsKey("sys_config_default"))
    g_beacon_cfg.sys_config_default = djbpo["sys_config_default"].as<uint8_t>(); // 1=reset to default
  // --- SLEEP (0x4003) ---
  if (djbpo.containsKey("sleep_enable"))
    g_beacon_cfg.sleep_enable = djbpo["sleep_enable"].as<uint8_t>(); // 1=DISABLE, 2=ENABLE
  if (djbpo.containsKey("val_id_mode"))
    g_beacon_cfg.val_id_mode = djbpo["val_id_mode"].as<uint8_t>();
  if (djbpo.containsKey("val_request"))
    g_beacon_cfg.val_request = djbpo["val_request"].as<uint8_t>();
  // --- UWB Configuration ---
  if (djbpo.containsKey("uwb_chan"))
    g_beacon_cfg.uwb_chan = djbpo["uwb_chan"].as<uint8_t>();
  if (djbpo.containsKey("uwb_plen"))
    g_beacon_cfg.uwb_plen = djbpo["uwb_plen"].as<uint8_t>();
  if (djbpo.containsKey("uwb_pac"))
    g_beacon_cfg.uwb_pac = djbpo["uwb_pac"].as<uint8_t>();
  if (djbpo.containsKey("uwb_txcode"))
    g_beacon_cfg.uwb_txcode = djbpo["uwb_txcode"].as<uint8_t>();
  if (djbpo.containsKey("uwb_rxcode"))
    g_beacon_cfg.uwb_rxcode = djbpo["uwb_rxcode"].as<uint8_t>();
  if (djbpo.containsKey("uwb_sfdtype"))
    g_beacon_cfg.uwb_sfdtype = djbpo["uwb_sfdtype"].as<uint8_t>();
  if (djbpo.containsKey("uwb_datarate"))
    g_beacon_cfg.uwb_datarate = djbpo["uwb_datarate"].as<uint8_t>();
  if (djbpo.containsKey("uwb_phrmode"))
    g_beacon_cfg.uwb_phrmode = djbpo["uwb_phrmode"].as<uint8_t>();
  if (djbpo.containsKey("uwb_phrrate"))
    g_beacon_cfg.uwb_phrrate = djbpo["uwb_phrrate"].as<uint8_t>();
  if (djbpo.containsKey("uwb_sfdto"))
    g_beacon_cfg.uwb_sfdto = djbpo["uwb_sfdto"].as<uint16_t>();
  if (djbpo.containsKey("uwb_stsmode"))
    g_beacon_cfg.uwb_stsmode = djbpo["uwb_stsmode"].as<uint8_t>();
  if (djbpo.containsKey("uwb_stslen"))
    g_beacon_cfg.uwb_stslen = djbpo["uwb_stslen"].as<uint8_t>();
  if (djbpo.containsKey("uwb_pdoa"))
    g_beacon_cfg.uwb_pdoa = djbpo["uwb_pdoa"].as<uint8_t>();
  // --- MAC Address (5 bytes hex string, VD: "1023ABCDEF") ---
  if (djbpo.containsKey("mac_address"))
  {
    current_target_tag_id_str = djbpo["mac_address"].as<String>();
    hexToBytes(current_target_tag_id_str, g_beacon_cfg.mac_address, 5);
  }

  // --- Read Apply Config To Base Flag ---
  if (djbpo.containsKey("apply_config_to_base"))
  {
    g_beacon_cfg.apply_config_to_base = djbpo["apply_config_to_base"].as<bool>() ? 1 : 0;
  }

  if (g_base_bcast_twr_active)
  {
    g_beacon_cfg.val_id_mode = 5;
    g_beacon_cfg.enable_bcast_twr = 1;
    g_beacon_cfg.broadcast = 1;
  }
  save_bcast_twr_state(g_base_bcast_twr_active, &g_beacon_cfg);

  Fag_mask_regs.FAG_BEACON_RES = 2;

  // Reset trang thai ACK cho trang web polling
  g_beacon_ack_status = "PENDING";
  g_beacon_ack_bits = 0;
  g_request_data_ready = false; // Reset de khong bi dinh data cu

  return true;
}

void WebServerInit(void)
{
  // server2.serveStatic("/", SPIFFS, "/"); // Comment lại dòng này để tránh bypass auth
  server2.onNotFound([](AsyncWebServerRequest *request)
                     {
                       String path = request->url();
                       debug_webserver("url:%s", path.c_str());
                       Eth_TimeCheck.ToEDisable();

                       // Yêu cầu đăng nhập cho tất cả các request (trừ favicon nếu muốn)
                       if (path != "/favicon.icon")
                       {
                         if (!request->authenticate(Config_Internet.Auth.User,
                                                    Config_Internet.Auth.Pass))
                         {
                           return request->requestAuthentication();
                         }
                       }

                       if (path.endsWith("/"))
                         path += "home.htm";

                       // Kiểm tra file có tồn tại trong SPIFFS không trước khi gửi
                       if (SPIFFS.exists(path))
                       {
                         String contentType = getContentType(path);
                         request->send(SPIFFS, path, contentType);
                       }
                       else
                       {
                         request->send(404, "text/plain", "Not Found");
                       }
                     });

  //  server2.serveStatic("/baseconfig.htm", SPIFFS, "/baseconfig.htm");
  server2.on("/checkinfor", HTTP_GET, [](AsyncWebServerRequest *request)
             {
               char infor[50];
               sprintf(infor, "{\"SerialID\":%lu}", Config_Device.Device.SerialID);
               request->send(200, "text/plain", infor);
             });

  server2.on("/baseconfig", HTTP_GET, base_config_get);
  server2.on("/baseconfig", HTTP_POST, base_config_post);
  server2.on("/dw1000", HTTP_GET, dw1000config_get);
  server2.on("/dw1000", HTTP_POST, dw1000config_post);
  server2.on("/DHCPconfig", HTTP_GET, DHCPconfig_get);
  server2.on("/DHCPconfig", HTTP_POST, DHCPconfig_post);
  server2.on("/beaconconfig", HTTP_GET, beconconfig_get);
  server2.on("/beaconconfig", HTTP_POST, beconconfig_post);

  // --- API Polling trang thai ACK BLE Fragment cho Web ---
  server2.on("/beacon_ack_status", HTTP_GET, [](AsyncWebServerRequest *request)
             {
               char buf[128];
               char status_hex[20];
               sprintf(status_hex, "%08X%08X", (uint32_t)(g_beacon_ack_bits >> 32), (uint32_t)(g_beacon_ack_bits & 0xFFFFFFFF));
               bool is_done = (g_beacon_ack_bits & (1ULL << 63)) != 0;
               sprintf(buf, "{\"status\":\"%s\",\"bits\":\"%s\",\"done\":%s}",
                       g_beacon_ack_status.c_str(), status_hex, is_done ? "true" : "false");
               request->send(200, "application/json", buf);
             });

  // --- API Polling ket qua Request Data (CMD 97) cho Web auto-fill ---
  server2.on("/beacon_request_data", HTTP_GET, [](AsyncWebServerRequest *request)
             {
               if (!g_request_data_ready)
               {
                 request->send(200, "application/json", "{\"ready\":false}");
                 return;
               }
               DynamicJsonDocument doc(1024);
               doc["ready"] = true;

               JsonObject timer = doc.createNestedObject("Timer");
               timer["Motion"] = g_last_request_data.timer.send_tag_motion;
               timer["Stand"] = g_last_request_data.timer.send_tag_stand;
               timer["Sleep1"] = g_last_request_data.timer.send_tag_sleep1;
               timer["Sleep2"] = g_last_request_data.timer.send_tag_sleep2;
               timer["Sleep3"] = g_last_request_data.timer.send_tag_sleep3;
               timer["Mode1"] = g_last_request_data.timer.sleep_mode1;
               timer["Mode2"] = g_last_request_data.timer.sleep_mode2;
               timer["Mode3"] = g_last_request_data.timer.sleep_mode3;
               timer["BattDef"] = g_last_request_data.timer.batt_default;
               timer["BattHigh"] = g_last_request_data.timer.batt_high;
               timer["BattInc"] = g_last_request_data.timer.batt_increase;
               timer["BattDec"] = g_last_request_data.timer.batt_decrease;

               JsonObject config = doc.createNestedObject("Config");
               config["Chan"] = g_last_request_data.config.uwb_chan;
               config["Plen"] = g_last_request_data.config.uwb_plen;
               config["PAC"] = g_last_request_data.config.uwb_pac;
               config["TxCode"] = g_last_request_data.config.uwb_txcode;
               config["RxCode"] = g_last_request_data.config.uwb_rxcode;
               config["SFDType"] = g_last_request_data.config.uwb_sfdtype;
               config["DataRate"] = g_last_request_data.config.uwb_datarate;
               config["PHRMode"] = g_last_request_data.config.uwb_phrmode;
               config["PHRRate"] = g_last_request_data.config.uwb_phrrate;
               config["SFDTO"] = g_last_request_data.config.uwb_sfdto;
               config["STSMode"] = g_last_request_data.config.uwb_stsmode;
               config["STSLen"] = g_last_request_data.config.uwb_stslen;
               config["PDOA"] = g_last_request_data.config.uwb_pdoa;

               doc["State"] = g_last_request_data.current_state;

               JsonObject id_tag = doc.createNestedObject("ID_Tag");
               char id_hex[12];
               sprintf(id_hex, "%02X%02X%02X%02X%02X",
                       g_last_request_data.id_tag.id[0], g_last_request_data.id_tag.id[1],
                       g_last_request_data.id_tag.id[2], g_last_request_data.id_tag.id[3],
                       g_last_request_data.id_tag.id[4]);
               id_tag["ID"] = id_hex;

               String output;
               serializeJson(doc, output);
               g_request_data_ready = false; // Reset sau khi web da doc
               request->send(200, "application/json", output);
             });
  server2.on("/twr", HTTP_GET, base_twr_get);
  // server2.on("/twr", HTTP_POST, base_twr_post);

  // --- API XỬ LÝ NHẬN FILE OTA CHO ISP3080 ---
  server2.on(
      "/update_isp", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        bool ok = !start_isp3080_ota; // Nếu chưa bắt đầu -> lỗi upload
        if (isp3080_upload_file)
        {
          isp3080_upload_file.close();
        }
        if (ok)
        {
          request->send(200, "text/html", "Upload OK! Tự chuyển trang... <script>setTimeout(function(){window.location.href='/update_isp.htm'}, 3000);</script>");
          // Khi upload xong file, ta bật cờ báo hiệu cho main loop tiến hành đẩy sang U7
          start_isp3080_ota = true;
        }
        else
        {
          request->send(500, "text/html", "Upload Lỗi! <a href='/update_isp.htm'>Thử lại</a>");
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          debug_webserver("[ISP_OTA] Bat dau nhan file: %s\n", filename.c_str());
          if (SPIFFS.exists("/isp3080.bin"))
          {
            SPIFFS.remove("/isp3080.bin");
          }
          isp3080_upload_file = SPIFFS.open("/isp3080.bin", FILE_WRITE);
          start_isp3080_ota = false; // reset cờ
        }

        if (isp3080_upload_file && len > 0)
        {
          isp3080_upload_file.write(data, len);
        }

        if (final)
        {
          if (isp3080_upload_file)
          {
            isp3080_upload_file.close();
          }
          debug_webserver("[ISP_OTA] Nhan file hoan tat, kich thuoc: %u bytes\n", index + len);
        }
      });

  // --- API OTA U6: Nạp trực tiếp firmware vào ESP32 Main (U6) ---
  server2.on(
      "/update_u6", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        bool ok = !Update.hasError();
        request->send(200, "text/plain", ok ? "U6 OTA OK! Dang khoi dong lai..." : "U6 OTA LOI!");
        if (ok)
        {
          delay(1000);
          ESP.restart();
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          debug_webserver("[U6_WEB_OTA] Bat dau nhan firmware: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          {
            Update.printError(Serial);
          }
        }
        if (Update.isRunning())
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }
        if (final)
        {
          if (Update.end(true))
          {
            debug_webserver("[U6_WEB_OTA] Nap thanh cong! Tong: %u bytes\n", index + len);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  // --- API SPIFFS U6: Nạp SPIFFS image vào ESP32 Main (U6) ---
  server2.on(
      "/update_spiffs", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        bool ok = !Update.hasError();
        request->send(200, "text/plain", ok ? "SPIFFS OK! Dang khoi dong lai..." : "SPIFFS OTA LOI!");
        if (ok)
        {
          delay(1000);
          ESP.restart();
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          debug_webserver("[U6_SPIFFS_OTA] Bat dau nhan SPIFFS image: %s\n", filename.c_str());
          // U_SPIFFS = 100 (command để ghi vào phân vùng SPIFFS)
          if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
          {
            Update.printError(Serial);
          }
        }
        if (Update.isRunning())
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }
        if (final)
        {
          if (Update.end(true))
          {
            debug_webserver("[U6_SPIFFS_OTA] Nap SPIFFS thanh cong! Tong: %u bytes\n", index + len);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  // --- API OTA U7: Lưu file vào SPIFFS rồi kích hoạt luồng OTA UART cũ ---
  // Lưu ý: Dự án này dùng SPIFFS thay cho SD card (handle_sdcard.cpp đã mod)
  extern String g_u7_ota_status;

  server2.on("/ota_u7_status", HTTP_GET, [](AsyncWebServerRequest *request)
             { request->send(200, "text/plain", g_u7_ota_status); });

  server2.on(
      "/update_u7", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        if (Update_info.DW_update)
        {
          g_u7_ota_status = "IN_PROGRESS";
          request->send(200, "text/plain", "U7 OTA: File da luu. Dang nap qua UART...");
        }
        else
        {
          g_u7_ota_status = "FAILED";
          request->send(500, "text/plain", "U7 OTA LOI: Khong luu duoc file!");
        }
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        static File u7_upload_file;
        if (!index)
        {
          debug_webserver("[U7_WEB_OTA] Bat dau nhan firmware: %s\n", filename.c_str());
          // Xóa file cũ nếu tồn tại (dùng SPIFFS, không phải SD)
          if (SPIFFS.exists("/firmware_dw.bin"))
          {
            SPIFFS.remove("/firmware_dw.bin");
          }
          u7_upload_file = SPIFFS.open("/firmware_dw.bin", FILE_WRITE);
        }
        if (u7_upload_file && len > 0)
        {
          u7_upload_file.write(data, len);
        }
        if (final)
        {
          if (u7_upload_file)
          {
            u7_upload_file.close();
          }
          debug_webserver("[U7_WEB_OTA] Luu file xong: %u bytes\n", index + len);
          // Kích hoạt luồng OTA U7 cũ (giống như khi FTP download xong)
          Update_info.DW_update = true;
          otaled_flag = ota_send_init;
        }
      });

  server2.begin();
  // SPIFFS.begin(true);
}

// void Webserverloop()
// {
// 	server.handleClient();
// 	// server2.handleClient();
// }

String getContentType(String filename)
{
  //	if (server.hasArg("download"))
  //		return "application/octet-stream";
  if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

// bool handleFileRead(String path)
// {
// 	// DEBUGLOG_WEBSERVER("\r\nhandleFileRead: %s", path.c_str());
// 	if (path.endsWith("/"))
// 		path += "home.htm";
// 	String contentType = getContentType(path);
// 	String pathWithGz = path + ".gz";
// 	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
// 	{
// 		if (SPIFFS.exists(pathWithGz))
// 			path += ".gz";
// 		File file = SPIFFS.open(path, "r");
// 		size_t lenfile = file.size();

// 		size_t sent = server.streamFile(file, contentType);
// 		debug_webserver("size file %lu -- size send %lu",lenfile,sent );
// 		if(lenfile == sent)
// 		return true;

// 		file.close();

// 	}
// 	return false;
// }

// void returnFail(String msg)
// {
// 	server.send(500, "text/plain", msg + "\r\n");
// }

// void handleFileList()
// {
// 	// WifiConfig.WifiDelayReInit(600);
// 	if (!server.hasArg("dir"))
// 	{
// 		returnFail("BAD ARGS");
// 		return;
// 	}
// 	if (!SPIFFS.begin(true))
// 		return;
// 	String path = server.arg("dir");
// 	if (path != "/" && !SPIFFS.exists((char *)path.c_str()))
// 	{
// 		returnFail("BAD PATH");
// 		return;
// 	}
// 	File dir = SPIFFS.open((char *)path.c_str());
// 	path = String();
// 	if (!dir.isDirectory())
// 	{
// 		dir.close();
// 		returnFail("NOT DIR");
// 		return;
// 	}
// 	dir.rewindDirectory();

// 	String output = "[";
// 	for (int cnt = 0; true; ++cnt)
// 	{
// 		File entry = dir.openNextFile();
// 		if (!entry)
// 			break;

// 		if (cnt > 0)
// 			output += ',';

// 		output += "{\"type\":\"";
// 		output += (entry.isDirectory()) ? "dir" : "file";
// 		output += "\",\"name\":\"";
// 		// Ignore '/' prefix
// 		output += entry.name() + 1;
// 		output += "\"";
// 		output += "}";
// 		entry.close();
// 	}
// 	output += "]";
// 	server.send(200, "text/json", output);
// 	debug_webserver("%s", output.c_str());
// 	dir.close();
// }
