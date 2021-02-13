/*
 * config.h - General config file for MXETHControl
 */

#ifndef MXETHCONTROL_CONFIG_H
    #define MXETHCONTROL_CONFIG_H

    /*** Begin: WiFi Settings ***/
    #define CFG_WIFI_SSID "<changeme>"
    #define CFG_WIFI_PASSWORD "<changeme>"
    /*** End: WiFi Settings ***/

    // Set serial output verbosity
    //#define MXDEBUG                  // to enable debugging output, does not include MXINFO
    #define MXINFO                   // to enable info output
    //#define MXDEBUG_TIME           // to enable timing debugging output

    /*** Begin: Firmware Update settings ***/
    // if defined check/apply for firmware update via HTTP at every wake up/reboot, adds ~400ms
    //#define HTTP_OTA_FW_UPD
    // if defined do an HTTP firmware check/update, when triggered by an
    // MQTT MXETHControl/<MAC>/set/update "1" message
    #define MQTT_HTTP_OTA_FW_UPD
    // The MX ETH Control device checks that folder for two files based on its MAC address
    // firmware binary  - <deviceName>.bin     , e.g. MXETHControl807D3A78FF16.bin
    // firmware version - <deviceName>.version , e.g. MXETHControl807D3A78FF16.version
    #define CFG_FW_BASE_URL "http://192.168.199.10/MXETHControl/firmware/"
    // Firmware version, should match the changelog.txt and is used for OTA firmware
    // updates
    #define CFG_FW_VERSION "2.6"
    /*** End: Firmware Update settings ***/

    /*** Begin: MQTT settings ***/
    #define CFG_MQTT_SERVER {192, 168, 199, 79}  // andromeda
    #define CFG_MQTT_PORT 1883                   // default non ssl port is 1883
    #define CFG_MQTT_USER "<changeme>"
    #define CFG_MQTT_PASSWORD "<changeme>"
    /*** End: MQTT settings ***/

    /*** Begin: PIN settings ***/
    /*
    ### Pinout
    # ESP8266       WemosD1Mini    RFM69(H)CW   CW-Pin#   HCW-Pin#
    # GPIO15/SS/CS  D8             NSS/CS       7         5
    # GPIO14/SCLK   D5             SCK/CLK      6         4
    # GPIO4         D2             DIO0         9         14        # Interrupt
    # GPIO12/MISO   D6             MISO         8         2
    # GND           G              GND          3         1
    # GND           G              GND          14        8
    # GND           G              GND          N/A       10       
    # GPIO13/MOSI   D7             MOSI         5         3
    # GPIO0         D3             Reset        13        6         # Hard Reset of RFM module
    #               3V3            VCC          2         13
    #                              Ant          1         9         # Antenna
    */
    #define CFG_RF69_IRQ_PIN          4   // I first tried GPIO2 but that caused the Wemos to reset
    #define CFG_RF69_SPI_CS           15
    #define CFG_RF69_ISRFM69HW        true   // is required for RFM69H(C)W otherwise TX does not work
    #define CFG_RF69_POWERLEVEL       31     //min:0 - max:31, transmission power
    /* A manual reset of the RFM69CW is possible even for applications in which
       VDD cannot be physically disconnected. The RESET pin should be pulled high
       for a hundred microseconds, and then released. The user should then wait
       for 5 ms before using the module.
    */
    #define CFG_RF69_RST_PIN          0
    /*** End: PIN settings ***/

    /*** Begin: misc settings ***/
    #define CFG_ETH200MAXPACKETSIZE 10 // should be max 10 bytes
    // number of times a packet is transmitted, should take ~6s, the thermostats don't
    // listen all the time it looks like they are checking only every ~5s. At least
    // the window sensor and the remote control send for ~6-8s their commands repeatedly.
    #define CFG_ETH200NUMPACKETSENDREPEATS 330 // 350,400 cause sometimes the thermostat to activate twice
    // max command bytes:
    //  remote control: 2
    //  windows sensor: 1
    #define CFG_ETH200MAXCMDS 2
    // number of thermostats to "create"
    #define CFG_ETH200NUMTHERMOSTATS 7  // 1 to 255
    // number of groups of thermostats to "create"
    #define CFG_ETH200NUMGROUPS 5       // 1 to 255

    // size of messages array to handle parallel incoming messages
    #define CFG_MESSAGES_SIZE 5
    // size of mqttCmds array to handle parallel incoming MQTT Cmds
    #define CFG_MQTTCMDS_SIZE 30
    // duration in seconds during which a message is received and how long we should wait before
    // sending it to MQTT. So if any external tool is reacting to that message and sending a new
    // command to the MXETHControl device we are sure we don't start sending if we still receive something.
    // "Remote Control" and "Windows Sensor" send their packages continuously for ~10 seconds
    #define CFG_MESSAGE_DELAY 13

    /*
      duration that the ESP uses as delay(x) at the end of each loop.
      It's a tradeoff between
        longer delay  - reduced power consumption
        shorter delay - faster response times (catches more packages)

      anyway, debug package output reduces the number of catched packages from ~170 -> ~26
      100ms, reduces power consumption from 88mA -> 40mA, with debug output enabled, reduces received packages 26 -> 14
      500ms, reduces power consumption from 88mA -> 32mA, with debug output enabled, reduces received packages 26 -> 5
    */
    #define CFG_ESP_LOOP_DELAY 100  // in ms
    //#define CFG_ESP_LOOP_DELAY 500

    // the duration that needs to expire after a (last) packet we got before updating
    // the MQTT status/state topic from "receiving" to "listening"
    // need to be a factor x longer than CFG_ESP_LOOP_DELAY
    #define CFG_STATE_RECEIVING_MAX_TIME 500  // in ms
    /*** End: misc settings ***/
#endif //MXETHCONTROL_CONFIG_H
