#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <IPAddress.h>
#include "define.h"
#include <TimeOutEvent.h>
/* 
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN DEFINE_ETH_POWER_PIN

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE DEFINE_ETH_TYPE

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR DEFINE_ETH_ADDR

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN DEFINE_ETH_MDC_PIN

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN DEFINE_ETH_MDIO_PIN

extern volatile bool eth_connected;
extern volatile bool wifi_connected;
extern TimeOutEvent Eth_TimeCheck;

class handleEthernet
{
public:
    handleEthernet(/* args */);
    ~handleEthernet();

    void eth_setup();
    bool lan_isconnect();
    bool wifi_isconnect();
    void eth_loop();

private:
    /* data */

};

extern handleEthernet handle_ethernet;

#endif
