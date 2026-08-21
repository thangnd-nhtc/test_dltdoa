// NetBIOS.cpp — NBNS responder with NB (0x20) & NBSTAT (0x21) support
#include "NetBIOS.h"

extern "C" {
  #include <lwip/netif.h>
  #include "esp_system.h"  // esp_read_mac(...)
}

#define NBNS_PORT             137
#define NBNS_MAX_HOSTNAME_LEN 32

// ===== Wire-level packet “views” (packed) =====
typedef struct {
  uint16_t id;
  uint8_t  flags1;
  uint8_t  flags2;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
  // then: QNAME (first-level encoded), 0x00
  //       QTYPE(2), QCLASS(2)
} __attribute__((packed)) nbns_hdr_t;

// ===== Utilities =====
static inline uint16_t rd16_be(const uint8_t* p) { return (uint16_t(p[0])<<8) | p[1]; }

// Decode “first-level encoded” NB name (RFC 1002) into 15-char string (uppercase already)
void NetBIOS::_getnbname(const char *nbname, char *name, uint8_t maxlen) {
  // nbname points to 32 ASCII chars (each nibble encoded as 'A' + nibble)
  // caller provides maxlen ≥ 15
  uint8_t b;
  uint8_t c = 0;

  while ((*nbname) && (c < maxlen)) {
    b = (uint8_t((*nbname++) - 'A') << 4);
    if (*nbname) {
      b |= uint8_t((*nbname++) - 'A');
    }
    if (!b || b == ' ') break;
    name[c++] = char(b);
  }
  name[c] = 0;
}

// ===== Build NB (0x20) POSITIVE NAME RESPONSE =====
void NetBIOS::_build_nb_name_answer(const uint8_t* qraw, size_t qlen, const IPAddress local_ip, uint8_t* out, size_t& outlen) {
  // qraw: pointer to beginning of NBNS header of the query
  // Build response mirroring NAME/QNAME from query, type=0x20, class=1
  const nbns_hdr_t* qh = (const nbns_hdr_t*)qraw;

  // locate QNAME in query
  const uint8_t* p = (const uint8_t*)(qraw + sizeof(nbns_hdr_t));
  if (p >= qraw + qlen) { outlen = 0; return; }

  // QNAME is length=0x20 + 32 chars, then 0x00
  // In ESP Arduino lib, some senders include a 'name_len' byte in custom structs,
  // but wire format is: 0x20 + 32, then 0x00.
  // Here, we will COPY all bytes from the first length byte until and including the 0x00 terminator.
  // Then append QTYPE(2), QCLASS(2) — but since it's an answer RR, these fields are in RR too.

  // Find end of QNAME (the 0x00 terminator)
  const uint8_t* qname = p;
  const uint8_t* qend  = p;
  // guard: skip label len (should be 0x20) + 32-name + 0x00
  if (qname >= qraw + qlen) { outlen = 0; return; }
  // simple scan to 0x00 with bounds
  while (qend < qraw + qlen && *qend != 0x00) ++qend;
  if (qend >= qraw + qlen) { outlen = 0; return; }
  // Now qend points to 0x00
  qend += 1; // include terminator
  if (qend + 4 > qraw + qlen) { outlen = 0; return; } // need QTYPE+QCLASS present

  const uint8_t* qtype_ptr  = qend;
  const uint8_t* qclass_ptr = qend + 2;
  const uint16_t qtype  = rd16_be(qtype_ptr);
  const uint16_t qclass = rd16_be(qclass_ptr);
  (void)qclass;

  // Start building answer
  uint8_t* w = out;

  // Header
  // ID
  _wr16_be(w, qh->id); w += 2;
  // FLAGS: QR=1(0x80), Opcode=0, AA=1(0x04), TC=0, RD=0 ; RA=0, Z=0, RCODE=0
  *w++ = 0x85;  // flags1
  *w++ = 0x00;  // flags2
  // QDCOUNT=0
  _wr16_be(w, 0); w += 2;
  // ANCOUNT=1
  _wr16_be(w, 1); w += 2;
  // NSCOUNT=0, ARCOUNT=0
  _wr16_be(w, 0); w += 2;
  _wr16_be(w, 0); w += 2;

  // Answer RR: NAME (copy QNAME exactly as sent)
  memcpy(w, qname, (size_t)(qend - qname)); w += (qend - qname);

  // TYPE, CLASS (mirror request’s class=1)
  _wr16_be(w, 0x0020); w += 2;  // NB (0x20)
  _wr16_be(w, 0x0001); w += 2;  // IN

  // TTL
  _wr32_be(w, 300); w += 4;

  // RDLENGTH = 6 (FLAGS(2) + ADDR(4))
  _wr16_be(w, 6); w += 2;

  // RDATA
  // FLAGS (node type etc.) — keep 0x0000 (unique, B-node)
  _wr16_be(w, 0x0000); w += 2;

  // IP address: choose correct local interface per netmask (like your original code)
  uint32_t addr_be = (uint32_t)local_ip; // default
  // Try to find a netif whose masked network matches packet iface
  for (auto nif = netif_list; nif; nif = nif->next) {
    uint32_t nip = ip4_addr_get_u32(netif_ip4_addr(nif));
    uint32_t nmk = ip4_addr_get_u32(netif_ip4_netmask(nif));
    // if this interface is up and has IP
    if (nip != 0 && nmk != 0) {
      addr_be = nip;
      // break;  // optional: first valid iface
    }
  }
  _wr32_be(w, addr_be); w += 4;

  outlen = (size_t)(w - out);
}

// ===== Build NBSTAT (0x21) NAME STATUS RESPONSE =====
void NetBIOS::_build_nbstat_answer(const uint8_t* qraw, size_t qlen, uint8_t* out, size_t& outlen) {
  const nbns_hdr_t* qh = (const nbns_hdr_t*)qraw;

  // Locate QNAME region of query to mirror it back
  const uint8_t* p = (const uint8_t*)(qraw + sizeof(nbns_hdr_t));
  if (p >= qraw + qlen) { outlen = 0; return; }
  const uint8_t* qname = p;
  const uint8_t* qend  = p;
  while (qend < qraw + qlen && *qend != 0x00) ++qend;
  if (qend >= qraw + qlen) { outlen = 0; return; }
  qend += 1; // include 0x00

  // RDATA for NBSTAT:
  // [1]  NUM_NAMES = 1
  // [18] NAME ENTRY: 15 chars padded ' ', SUFFIX(1), FLAGS(2)
  // [6]  UNIT_ID (MAC)
  // [46] STATISTICS (zero-filled minimal compatible block)
  uint8_t rdata[128];
  uint8_t* r = rdata;

  // 1) NUM_NAMES
  *r++ = 1;

  // 2) NAME ENTRY (18 bytes)
  char nbname[16]; memset(nbname, ' ', sizeof(nbname));
  // Use our configured name (≤15 uppercase)
  String n = _name;
  if (n.length() > 15) n = n.substring(0, 15);
  memcpy(nbname, n.c_str(), n.length());
  // 15 chars
  memcpy(r, nbname, 15); r += 15;
  // suffix
  *r++ = 0x00; // workstation service
  // flags
  _wr16_be(r, 0x0000); r += 2; // unique, active

  // 3) UNIT ID (MAC)
  uint8_t mac[6] = {0};
  // Prefer ETH MAC; if not available, fall back to WiFi STA
  if (esp_read_mac(mac, ESP_MAC_ETH) != ESP_OK) {
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
  }
  memcpy(r, mac, 6); r += 6;

  // 4) STATISTICS block (46 bytes) — zero-filled is fine for Windows
  memset(r, 0x00, 46); r += 46;

  const uint16_t rdata_len = (uint16_t)(r - rdata);

  // Build response header + answer
  uint8_t* w = out;

  // Header
  _wr16_be(w, qh->id); w += 2;
  *w++ = 0x85;  // QR=1, AA=1
  *w++ = 0x00;  // RA=0, RCODE=0
  _wr16_be(w, 0); w += 2; // QDCOUNT=0
  _wr16_be(w, 1); w += 2; // ANCOUNT=1
  _wr16_be(w, 0); w += 2; // NSCOUNT=0
  _wr16_be(w, 0); w += 2; // ARCOUNT=0

  // NAME (mirror query's QNAME exactly)
  memcpy(w, qname, (size_t)(qend - qname)); w += (qend - qname);

  // TYPE=0x0021 (NBSTAT), CLASS=IN(1)
  _wr16_be(w, 0x0021); w += 2;
  _wr16_be(w, 0x0001); w += 2;

  // TTL
  _wr32_be(w, 300); w += 4;

  // RDLENGTH
  _wr16_be(w, rdata_len); w += 2;

  // RDATA
  memcpy(w, rdata, rdata_len); w += rdata_len;

  outlen = (size_t)(w - out);
}

// ===== Packet handler =====
void NetBIOS::_onPacket(AsyncUDPPacket &packet) {
  const uint8_t* data = packet.data();
  size_t len = packet.length();
  if (len < sizeof(nbns_hdr_t)) return;

  const nbns_hdr_t* qh = (const nbns_hdr_t*)data;

  // Only process QUERY (QR=0)
  if (qh->flags1 & 0x80) return;

  // Locate question fields (QNAME + QTYPE + QCLASS)
  const uint8_t* p = data + sizeof(nbns_hdr_t);
  if (p >= data + len) return;

  // Walk QNAME until terminator 0x00
  const uint8_t* qname = p;
  const uint8_t* qend  = p;
  while (qend < data + len && *qend != 0x00) ++qend;
  if (qend >= data + len) return;
  qend += 1; // include 0x00

  if (qend + 4 > data + len) return; // need QTYPE/QCLASS
  const uint16_t qtype  = rd16_be(qend);
  // const uint16_t qclass = rd16_be(qend + 2);

  // Decode asked name (for 0x20 we verify match; for 0x21 we can answer regardless)
  char asked[NBNS_MAX_HOSTNAME_LEN + 1] = {0};
  if ((qname + 1 + 32) <= data + len) {
    // first-level encoded string starts after the length byte
    _getnbname((const char*)(qname + 1), asked, 15);
  }

  // Decide answer type
  uint8_t resp[512];
  size_t  rlen = 0;

  if (qtype == 0x0021) {
    // NBSTAT: respond with node status table (works with nbtstat -A)
    _build_nbstat_answer(data, len, resp, rlen);
  } else if (qtype == 0x0020) {
    // NB name query: only answer if name matches ours
    if (_name.equalsIgnoreCase(asked)) {
      // choose best local IP to report
      IPAddress lip = packet.localIP();
      // Prefer interface that matches the network
      for (auto nif = netif_list; nif; nif = nif->next) {
        uint32_t nip = ip4_addr_get_u32(netif_ip4_addr(nif));
        uint32_t nmk = ip4_addr_get_u32(netif_ip4_netmask(nif));
        uint32_t maskedip = nip & nmk;
        uint32_t maskedin = (uint32_t)lip & nmk;
        if (nip && nmk && maskedip == maskedin) { lip = IPAddress(nip); break; }
      }
      _build_nb_name_answer(data, len, lip, resp, rlen);
    }
  } else {
    // other types: ignore
    return;
  }

  if (rlen > 0) {
    _udp.writeTo(resp, rlen, packet.remoteIP(), NBNS_PORT);
  }
}

// ===== Public APIs =====
NetBIOS::NetBIOS() {}
NetBIOS::~NetBIOS() { end(); }

bool NetBIOS::begin(const char *name) {
  if (!name || !*name) return false;
  _name = String(name);
  _name.toUpperCase();
  if (_name.length() > 15) _name = _name.substring(0, 15);

  if (_udp.connected()) return true;

  _udp.onPacket(
    [](void *arg, AsyncUDPPacket &packet) {
      ((NetBIOS *)(arg))->_onPacket(packet);
    },
    this
  );
  return _udp.listen(NBNS_PORT);
}

void NetBIOS::end() {
  if (_udp.connected()) {
    _udp.close();
  }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_NETBIOS)
NetBIOS NBNS;
#endif
