/****************************************************************************
MX ETH Control

Runs on a Wemos D1 mini/ESP8266, and uses an RFM69CW module to capture
packages from ELV ETH 200 comfort "window sensors" and "remote controls"
and publishes them via MQTT.
It also subscribes to MQTT topics to control thermostat devices by
simulating one "remote control" per thermostat.

It also includes firmware OTA updates either checked at every boot or
triggered via MQTT.

Copyright 2020 mt-mrx <64284703+mt-mrx@users.noreply.github.com>

Based heavily (several parts just copied) on:
  Code snippets in: CRC Berechnung für ETH comfort 200
    Authors: Dietmar Weisser, Michael Walischewski (michel72), Jonas G.
    https://www.mikrocontroller.net/topic/172034
    Michael Walischewski -> https://www.mikrocontroller.net/topic/172034#3018165
    Jonas G. -> https://www.mikrocontroller.net/topic/172034#5729047
    License: not specified
  RFM69 library:
    https://rpi-rfm69.readthedocs.io/en/latest/index.html
    https://github.com/jkittley/RFM69
    https://github.com/LowPowerLab/RFM69
    License: gpl-3.0
  Decode and encode functions from:
    Copyright (c) 2018 by Michael Dreher
    https://github.com/nospam2000/urh-ETH_Comfort_decode_plugin
    License: you can either use the GNU General Public License v3.0 or the MIT license
  MQTT stuff based on:
    Florian Knodt - www.adlerweb.info
    https://github.com/adlerweb/ESP8266-BME280-Multi
    License: BSD 2-Clause License
  Firmware OTA update from:
    Erik H. Bakke - https://www.bakke.online
    https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/
    License: not specified
  PubSubClient-Library
    Nicholas O'Leary
    https://github.com/knolleary/pubsubclient
    License: MIT License
  isFloat() function
    SurferTim
    https://forum.arduino.cc/index.php?topic=209407.msg1538725#msg1538725
    License: not specified
  rssiToPercentage() function
    Marvin Roger
    https://github.com/homieiot/homie-esp8266/blob/develop/src/Homie/Utils/Helpers.cpp
    License: MIT License
*****************************************************************************
License
*****************************************************************************
This program is free software; you can redistribute it 
and/or modify it under the terms of the GNU General    
Public License as published by the Free Software       
Foundation; either version 3 of the License, or        
(at your option) any later version.                    
                                                        
This program is distributed in the hope that it will   
be useful, but WITHOUT ANY WARRANTY; without even the  
implied warranty of MERCHANTABILITY or FITNESS FOR A   
PARTICULAR PURPOSE. See the GNU General Public        
License for more details.                              
                                                       
Licence can be viewed at                               
http://www.gnu.org/licenses/gpl-3.0.txt

Please maintain this license information along with authorship
and copyright notices in any redistribution of this code
****************************************************************************/

#include <Arduino.h>
#include <config.h>            // project settings file
#include <SPI.h>               // for communication with RFM69
#include <ESP8266WiFi.h>       // for wifi access
#include <ESP8266HTTPClient.h> // for firmware OTA
#include <ESP8266httpUpdate.h> // for firmware OTA updates

#include <MXDebugUtils.h>      // for debugging function support

#include <MXPubSubClientWrapper.h>

#include <ETH200RFM69.h>

ETH200RFM69 radio(CFG_RF69_SPI_CS, CFG_RF69_IRQ_PIN, CFG_RF69_ISRFM69HW);

// for ETH200 packet analysis
enum deviceType_t {
  deviceTypeUnknown,
  RemoteControl = 0x10,
  WindowSensor = 0x20,
};
enum deviceCmd_t {
  deviceCmdUnknown,
  WindowOpened = 0x41,
  WindowClosed = 0x40,
  WindowOpenedBatLow = 0xC1,
  WindowClosedBatLow = 0xC0,
  DayMode = 0x42,
  NightMode = 0x43,
  SetTemp = 0x40,
};
enum batteryStatus_t {
  batteryStatusUnknown,
  ok,
  low,
};

// messages queue/buffer definition
struct message {
  uint8_t hasData = 0;            //if this message has data in it
  unsigned long receiveTime = 0;  //when this message was first received
  uint8_t packetSize = 0;         //different sensors have different packet sizes
  deviceType_t deviceType = deviceType_t::deviceTypeUnknown;
  deviceCmd_t deviceCmd = deviceCmd_t::deviceCmdUnknown;
  float tempOffset = 0.0;         // only used when deviceCmd_t::SetTemp
  batteryStatus_t batteryStatus = batteryStatus_t::batteryStatusUnknown; // only in use for deviceType_t::WindowSensor
  int16_t RSSI = 0;               // signal strength during packet reception
  uint16_t numPackets = 1;        // Number of packages with this content received

  // packet split in its parts:
  uint8_t counter = 0;        //counter as received from sensor
  uint8_t deviceTypeRaw = 0;  //device type as received from sensor
  uint32_t deviceID = 0;      //device ID as received from sensor, only lower 3 byte used
  uint8_t cmdRaw[CFG_ETH200MAXCMDS] = {0};       //cmd bytes, in use by remote control, window sensor
  uint16_t crc = 0;                              //2 byte CRC value
  uint8_t packet[CFG_ETH200MAXPACKETSIZE] = {0}; // complete raw packet
};
message messages[CFG_MESSAGES_SIZE];

// MQTT cmd queue definition
struct mqttCmd {
  uint8_t hasData = 0;            //if this message has data in it
  unsigned long receiveTime = 0;  //when this cmd was received
  String topic;                   //the topic we got
  String value;                   //the value/cmd for that topic
};
mqttCmd mqttCmds[CFG_MQTTCMDS_SIZE];

// firmware version
const char* fwVer = CFG_FW_VERSION;

// WiFi Configuration
const char* cfg_wifi_ssid = CFG_WIFI_SSID;
const char* cfg_wifi_password = CFG_WIFI_PASSWORD;

String getStrippedMAC(void) {
  String stripMac = String(WiFi.macAddress());
  stripMac.replace(":", "");
  return stripMac;
}

String getDeviceName(void) {
  String devName = "MXETHControl";
  devName += getStrippedMAC();
  return devName;
}

String getMqttRoot(void) {
  //mqtt_root should not start with /
  //https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/
  String newRoot = "MXETHControl/";
  newRoot += getStrippedMAC();
  return newRoot;
}

String strippedMAC = getStrippedMAC();
String deviceName = getDeviceName();
const char* userAgent = "MX ETH Control - HTTP(S)/MQTT";

#if defined(HTTP_OTA_FW_UPD) || defined(MQTT_HTTP_OTA_FW_UPD)
  // The MX ETH Control device checks that folder for two files based on its MAC address
  // firmware binary  - <deviceName>.bin     , e.g. MXETHControl807D3A78FF16.bin
  // firmware version - <deviceName>.version , e.g. MXETHControl807D3A78FF16.version
  // If the MX ETH Control device detects a string inside the .version file
  // that's different than fwVer, it performs an update.
  const char* fwBaseUrl = CFG_FW_BASE_URL;
#endif //defined(HTTP_OTA_FW_UPD) || defined(MQTT_HTTP_OTA_FW_UPD)

// MQTT Configuration
IPAddress mqtt_server(CFG_MQTT_SERVER); //Use IP address of mqtt server so we don't need DNS
const unsigned int mqtt_port = CFG_MQTT_PORT;
const char* mqtt_user = CFG_MQTT_USER;
const char* mqtt_pass = CFG_MQTT_PASSWORD;
String mqtt_root = getMqttRoot();

// Removed all the MQTT via SSL and fingerprint verification from previous project
// MXAmbienceSensor, it took up to an extra 1000ms to do a fingerprint verification.
// I think it's not worth it.

#define MQTT_TOPIC_GET "/get"
#define MQTT_TOPIC_GET_SIGNAL "/rssi"
#define MQTT_TOPIC_SET "/set"
#define MQTT_TOPIC_SET_RESET "/reset"
#define MQTT_TOPIC_SET_UPDATE "/update"
#define MQTT_TOPIC_SET_PING "/ping"
#define MQTT_TOPIC_SET_PONG "/pong"
#define MQTT_TOPIC_STATUS "/status"
#define MQTT_TOPIC_STATUS_ONLINE "/online"
#define MQTT_TOPIC_STATUS_HARDWARE "/hardware"
#define MQTT_TOPIC_STATUS_VERSION "/version"
#define MQTT_TOPIC_STATUS_IP "/ip"
#define MQTT_TOPIC_STATUS_MAC "/mac"
#define MQTT_TOPIC_STATUS_STATE "/state"

#define MQTT_PRJ_HARDWARE "MXETHControl"
#define MQTT_PRJ_VERSION fwVer

WiFiClient wifiClient;
MXPubSubClientWrapper mqttClient(wifiClient);

// reset the RFM69 by using the defined reset Pin
void resetRFM69() {
  MXDEBUG_PRINTLLN(F("Resetting RMF69 via reset pin."));
  pinMode(CFG_RF69_RST_PIN, OUTPUT);
  digitalWrite(CFG_RF69_RST_PIN, HIGH);
  delayMicroseconds(100); // according to datasheet during this time 10mA current can be drawn on VDD
  digitalWrite(CFG_RF69_RST_PIN, LOW);
  delay(10); // wait 10ms for RFM69 to be ready
}

/**
 *   Dump some information on startup.
 **/
void splashScreen() {
  for (int i = 0; i < 1; i++) MXINFO_PRINTLN("");
  MXINFO_PRINTLN(F("#############################################"));
  MXINFO_PRINT(F("# "));
  MXINFO_PRINT(userAgent);
  MXINFO_PRINT(F(" - v"));
  MXINFO_PRINTLN(fwVer);
  MXINFO_PRINT(F("# DeviceName: "));
  MXINFO_PRINTLN(deviceName);
  MXINFO_PRINTLN(F("#############################################"));
  for (int i = 0; i < 1; i++) MXINFO_PRINTLN("");
}

/**
 * WiFi signal strength to percentage conversion
 *  Marvin Roger
 *  https://github.com/homieiot/homie-esp8266/blob/develop/src/Homie/Utils/Helpers.cpp
 *  License: MIT License
 **/
uint8_t rssiToPercentage(int32_t rssi) {
  uint8_t quality;
  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }

  return quality;
}


// converts temperature given as uint8_t hex value to float
float convertHex2Temp(uint8_t hexTemp) {
  float temp = 0;
  if ((hexTemp > 0) && (hexTemp < 128)) {
    // 0x00 counting upwards for positive temperature change
    temp = (float)hexTemp * 0.5;
  } else {
    //0xFF and counting downwards for negative temperature change
    temp = ((float)(256 - hexTemp) * -0.5);
  }
  return temp; 
}

// converts temperature given as float to uint8_t hex value
uint8_t convertTemp2Hex(float temp) {
  uint8_t hexTemp = 0x00;
  if (temp > 0) {
    MXDEBUG_PRINTLLN(F("Got a positive temperature value."));
    hexTemp = (uint8_t)((uint8_t)(temp * 10) / 5);
  } else {
    MXDEBUG_PRINTLLN(F("Got a negative temperature value."));
    hexTemp = (uint8_t)(256 - (int8_t)(temp * 10) / -5);
  }
  MXDEBUG_PRINTL(F("Temperature as float: "));
  MXDEBUG_PRINTLN(String(temp,1));
  MXDEBUG_PRINTL(F("Temperature as hex  : "));
  MXDEBUG_PRINTLN(hexTemp);
  return hexTemp;
}

// Check if a String is a valid float.
// source: https://forum.arduino.cc/index.php?topic=209407.msg1538738#msg1538738
boolean isFloat(String str) {
  String tBuf;
  boolean decPt = false;
 
  if (str.charAt(0) == '+' || str.charAt(0) == '-') {
    tBuf = &str[1];
  } else {
    tBuf = str;
  }

  for (unsigned int x = 0; x < tBuf.length(); x++) {
    if (tBuf.charAt(x) == '.') {
      if (decPt) {
        return false;
      } else {
        decPt = true;
      }
    } else if (tBuf.charAt(x) < '0' || tBuf.charAt(x) > '9') {
      return false;
    }
  }
  return true;
}

// wrapper for radio.send to publish MQTT "status/state = sending" message and toggle LED
boolean send(uint8_t buffer[], uint8_t bufferSize, uint8_t numStuffedBits = 0) {
  digitalWrite(LED_BUILTIN, LOW); // turn builtin LED on
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "sending", false);
  
  boolean ret = radio.send(buffer, bufferSize, numStuffedBits);

  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "listening", false);
  digitalWrite(LED_BUILTIN, HIGH); // turn builtin LED off
  return ret;
}

// wrapper for radio.sendPacket to publish MQTT "status/state = sending" message and toggle LED
boolean sendPacket(uint8_t deviceType, uint32_t address, uint8_t cmd, uint8_t cmds[], uint8_t cmdsSize) {
  digitalWrite(LED_BUILTIN, LOW); // turn builtin LED on
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "sending", false);
  
  boolean ret = radio.sendPacket(deviceType, address, cmd, cmds, cmdsSize);

  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "listening", false);
  digitalWrite(LED_BUILTIN, HIGH); // turn builtin LED off
  return ret;
}

// Handles thermostat cmnds by reacting to MQTT topics and their cmd
// return true - if handled successfully
//        false - otherwise
boolean handleThermostatCmds(String topic, String cmd) {
  // since the position of the thermostat ID in the MQTT is fixed by using
  // fixed length MAC address and fixed length IDs, we hard code the substring
  // if anything changes in the topic syntax this needs to be changed as well
  // topic :[MXETHControl/BCDDC2248523/thermostat/010101/set/cmd]
  //                                              ^     ^ end=43
  //                                              | start=37
  String thermostatID = topic.substring(37, 43);
  String thermostatRoot = (String)mqtt_root + "/thermostat/" + thermostatID;
  uint32_t id = strtoul(thermostatID.c_str(), NULL, 16); // ID as 4 byte integer
  MXDEBUG_PRINTLLN("Got cmd for thermostat");
  MXDEBUG_PRINTLN("ID:  " + thermostatID + ", as int: " + id);
  MXDEBUG_PRINTLN("CMD: " + cmd);

  boolean cmdSent = false;

  if (cmd == "Ready") {
    MXINFO_PRINTLLN(F("Got Ready command."));
    // do nothing when we are ready
    return true;
  } else if (cmd == "") {
    MXINFO_PRINTLLN(F("Got empty command, ignoring it."));
    return true;
  } else if (cmd == "testWindowOpened") {
    MXINFO_PRINTLLN(F("Sending static WindowOpened test packet captured from Window Sensor."));

    MXDEBUG_PRINTLN(F("Test packet(decoded): (preamble) (sync) A0 20 01 4F 5E 41 BC 5D"));
    MXDEBUG_PRINTLN(F("Test packet(encoded): (        ) (    ) 05 04 80 F2 7A 82 3D BA"));
    uint8_t testData[8] = {0x05, 0x04, 0x80, 0xF2, 0x7A, 0x82, 0x3D, 0xBA};

    cmdSent = send(testData, sizeof(testData));
  } else if (cmd == "testWindowClosed") {
    MXINFO_PRINTLLN(F("Sending static WindowClosed test packet captured from Window Sensor."));

    MXDEBUG_PRINTLN(F("Test packet(decoded): (preamble) (sync) A5 20 01 4F 5E 40 B2 58"));
    MXDEBUG_PRINTLN(F("Test packet(encoded): (        ) (    ) A5 04 80 F2 7A 02 4D 1A"));
    uint8_t testData[8] = {0xA5, 0x04, 0x80, 0xF2, 0x7A, 0x02, 0x4D, 0x1A};

    cmdSent = send(testData, sizeof(testData));
  } else if (cmd == "Learn") {
    MXINFO_PRINTLLN(F("Got Learn command."));
    // 0x20 simulating Window Sensor
    // id thermostat id, (in reality the ID of this simulated Window Sensor,
    // 0x40, window close, I believe any command is correct in learning mode
    //                     the thermostat just listens for any packet from any device.
    // 0x00, no additional cmd
    cmdSent = sendPacket(deviceType_t::WindowSensor, id, deviceCmd_t::WindowClosed, {0x00}, 0);
  } else if (cmd == "WindowOpened") {
    MXINFO_PRINTLLN(F("Got WindowOpened command."));
    cmdSent = sendPacket(deviceType_t::WindowSensor, id, deviceCmd_t::WindowOpened, {0x00}, 0);
  } else if (cmd == "WindowClosed") {
    MXINFO_PRINTLLN(F("Got WindowClosed command."));
    cmdSent = sendPacket(deviceType_t::WindowSensor, id, deviceCmd_t::WindowClosed, {0x00}, 0);
  } else if (cmd == "DayMode") {
    MXINFO_PRINTLLN(F("Got DayMode command."));
    uint8_t cmds[1] = {0x00};
    cmdSent = sendPacket(deviceType_t::RemoteControl, id, deviceCmd_t::DayMode, cmds, 1);
  } else if (cmd == "NightMode") {
    MXINFO_PRINTLLN(F("Got NightMode command."));
    uint8_t cmds[1] = {0x00};
    cmdSent = sendPacket(deviceType_t::RemoteControl, id, deviceCmd_t::NightMode, cmds, 1);
  } else if (isFloat(cmd) && (cmd.toFloat() >= -9.5) && (cmd.toFloat() <= +29.5)) {
    MXINFO_PRINTLLN(F("Got Temperature command."));
    // set temperature
    if (cmd.startsWith("+") || cmd.startsWith("-")) {
      MXDEBUG_PRINTLLN(F("Got a signed temperature command, treating it as an offset."));
      uint8_t cmds[1];
      cmds[0] = convertTemp2Hex(cmd.toFloat());
      cmdSent = sendPacket(deviceType_t::RemoteControl, id, deviceCmd_t::SetTemp, cmds, 1);
    } else {
      MXINFO_PRINTLLN(F("Got unsigned Temperature command, treating it as an absolute value."));
      MXDEBUG_PRINTLN(F("First turning the temperature all the way down."));
      uint8_t cmds[1] = {0xCA}; // -30 C
      cmdSent = sendPacket(deviceType_t::RemoteControl, id, deviceCmd_t::SetTemp, cmds, 1);

      mqttClient.loop(); // refresh mqtt connection, since it will expire after ~10 seconds and
                         // sending two commands will take a while
      delay(1000);       // wait a moment before sending the next command

      MXDEBUG_PRINTLN(F("Second turning the temperature to the desired absolute temp."));
      // we have an offset of 5 C, which is the lowest temperature of the thermostats
      cmds[0] = convertTemp2Hex(cmd.toFloat() - 5);
      cmdSent = sendPacket(deviceType_t::RemoteControl, id, deviceCmd_t::SetTemp, cmds, 1);
    }
  } else {
    MXINFO_PRINTLLN(F("Got unknown CMD, ignoring it!"));
    MXINFO_PRINTLN("ID:  " + thermostatID + ", as int: " + id);
    MXINFO_PRINTLN("CMD: " + cmd);
    return false;
  }


  if (cmdSent == true) {
    // publish also raw packet
    String rawPacket = "";
    char rawHex[2] = "";
    for (uint8_t i = 0; i < radio.lastSentPacketSize; i++ ) {
      sprintf(rawHex, "%02X", radio.lastSentPacket[i]); // padding the hex values with leading 0
      rawPacket = rawPacket + rawHex + ((i < radio.lastSentPacketSize - 1)? " ":"");
    }
    mqttClient.publish(thermostatRoot + "/get/raw", rawPacket, false);
  }

  return true;
}

#if defined(HTTP_OTA_FW_UPD) || defined(MQTT_HTTP_OTA_FW_UPD)
/**
 * Checks for firmware updates from configured OTA URL.
 **/
  void checkForFWUpdates() {
    // Firmware update server and filename
    // e.g. fwBaseUrl = "http://192.168.x.y/MXETHControl/firmware/"
    // The MX ETH Control device checks that folder for two files based on its MAC address
    // firmware binary  - <deviceName>.bin     , e.g. MXETHControl807D3A78FF16.bin
    // firmware version - <deviceName>.version , e.g. MXETHControl807D3A78FF16.version
    String fwVerUrl = String(fwBaseUrl);
    fwVerUrl.concat(deviceName);
    fwVerUrl.concat(".version");

    MXINFO_PRINTLN(F(""));
    MXTIME_PRINT(F(""));
    MXINFO_PRINTLN(F("Checking for firmware updates."));
    MXINFO_PRINT(F("Firmware version URL: "));
    MXINFO_PRINTLN(fwVerUrl);

    // Download the version string
    HTTPClient httpClient;
    httpClient.begin(wifiClient, fwVerUrl);
    int httpCode = httpClient.GET();
    if ( httpCode == 200 ) {
      String fwVerNew = httpClient.getString(); // this adds a linebreak to the String
      fwVerNew.trim(); //remove any trailing whitespace/linebreak

      MXINFO_PRINT(F("Current firmware version  : "));
      MXINFO_PRINTLN(fwVer);
      MXINFO_PRINT(F("Available firmware version: "));
      MXINFO_PRINTLN(fwVerNew);
      MXDEBUG_PRINT(F("Compare firmware String.equals() result: "));
      MXDEBUG_PRINTLN(fwVerNew.equals(fwVer));

      if (fwVerNew != fwVer) {
        MXINFO_PRINTLN(F("New version available, preparing to update."));
        String fwBinUrl = String(fwBaseUrl);
        fwBinUrl.concat(deviceName);
        fwBinUrl.concat(".bin");
        MXINFO_PRINT(F("Firmware binary URL : "));
        MXINFO_PRINTLN(fwBinUrl);
        MXTIME_PRINT(F(""));

        // Start the actual update
        ESPhttpUpdate.rebootOnUpdate(true);
        t_httpUpdate_return ret = ESPhttpUpdate.update(wifiClient, fwBinUrl);
        switch (ret) {
          case HTTP_UPDATE_FAILED:
            MXINFO_PRINT(F("HTTP_UPDATE_FAILED Error ("));
            MXINFO_PRINT(ESPhttpUpdate.getLastError());
            MXINFO_PRINT(F("): "));
            MXINFO_PRINTLN(ESPhttpUpdate.getLastErrorString().c_str());
            break;
          case HTTP_UPDATE_NO_UPDATES:
            MXINFO_PRINTLN(F("HTTP_UPDATE_NO_UPDATES"));
            break;
          case HTTP_UPDATE_OK:
            MXINFO_PRINTLN(F("HTTP_UPDATE_OK"));
            break;
        }
      } else {
        MXINFO_PRINTLN(F("No new version available."));
      }
      MXTIME_PRINT(F(""));
    } else {
      MXINFO_PRINT(F("Firmware version check failed, got HTTP response code "));
      MXINFO_PRINTLN(httpCode);
    }
    MXTIME_PRINT("");
    httpClient.end();
  }
#endif //defined(HTTP_OTA_FW_UPD) || defined(MQTT_HTTP_OTA_FW_UPD)

void mqttReconnect() {
  // Only do 3 connection retries, if it's not working
  // we do it again after the next deep sleep phase
  for (int i=0; i<=2; i++) {
    if (mqttClient.connected()) {
      // if we are connected stop the retries
      break;
    }
    MXINFO_PRINT(F("MQTT try #"));
    MXINFO_PRINT(i);
    MXINFO_PRINTLN(F(" attempting connection."));
    MXTIME_PRINT(F(""));

    // Attempt to connect
    MXINFO_PRINT(F("MQTT connecting as client: "));
    MXINFO_PRINTLN(deviceName);

    String willTopic = mqtt_root; //need to initialize the variable first, otherwise we have garbage
    willTopic += MQTT_TOPIC_STATUS;
    willTopic += MQTT_TOPIC_STATUS_ONLINE;
    const char* willMessage = "0";
    
    MXDEBUG_PRINTL(F("Free Heap Size: "));
    MXDEBUG_PRINTLN(ESP.getFreeHeap());
    MXDEBUG_PRINTL(F("MQTT user: "));
    MXDEBUG_PRINTLN(mqtt_user);
    MXDEBUG_PRINTL(F("MQTT pass: "));
    MXDEBUG_PRINTLN(mqtt_pass);
    MXDEBUG_PRINTL(F("MQTT MQTT_MAX_PACKET_SIZE: "));
    MXDEBUG_PRINTLN(MQTT_MAX_PACKET_SIZE);
    MXDEBUG_PRINTL(F("MQTT willTopic: "));
    MXDEBUG_PRINTLN(willTopic);
    MXDEBUG_PRINTL(F("MQTT willMessage: "));
    MXDEBUG_PRINTLN(willMessage);

    if (mqttClient.connect(\
          deviceName.c_str(), mqtt_user, mqtt_pass, \
          willTopic.c_str(), 0, 1, willMessage)) {
          //the added willTopic,willQos,willRetain,willMessage parameters enable the server
          //to notify all subscribed clients that this sensor is online=0 (means offline) when
          //the server looses the connection to it
      MXINFO_PRINTLN(F("MQTT connected to broker."));
      yield();
      mqttClient.subscribe(((String)mqtt_root + MQTT_TOPIC_SET + "/#").c_str());
      // we also need to retain the updated "online" state, otherwise only the willMessage state of "0" is retained
      // basically if we retain the LWT message then we need to retain any updates to it as well
      mqttClient.publish((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_ONLINE, "1", true);
      yield();
      mqttClient.loop(); //give the ESP a chance to react to publish messages
      MXTIME_PRINT(F(""));
    } else {
      MXINFO_PRINT(F("MQTT connection failed, rc="));
      MXINFO_PRINT(mqttClient.state());
      MXINFO_PRINTLN(F(" trying again in 2 seconds"));
      // Wait 2 second before retrying
      delay(2000);
    }
  }
}

// add an incoming MQTT message to the MQTT cmd queue
// returns true if new message was inserted.
boolean pushMQTTCmdsQueue(mqttCmd cmd) {
  for (uint8_t i = 0; i < CFG_MQTTCMDS_SIZE; i++) {
    // search for first free queue slot
    if (mqttCmds[i].hasData == 0) {
      mqttCmds[i] = cmd;
      return true;
    }
  }
  // if we reach this point we searched the complete messages queue and
  // didn't find a free slot
  MXINFO_PRINTLLN(F("ERROR: Could not write MQTT Cmd into mqttCmds queue. All slots full"));
  return false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  MXTIME_PRINT(F(""));
  MXINFO_PRINT(F("MQTT Message arrived topic : ["));
  MXINFO_PRINT(topic);
  MXINFO_PRINTLN(F("] "));

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  MXINFO_PRINT(F("MQTT Message: "));
  MXINFO_PRINTLN(message);

  String topicStr = topic;
  String check = ((String)mqtt_root + MQTT_TOPIC_SET);

  if (topicStr.startsWith((String)check + MQTT_TOPIC_SET_RESET)) {
    mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "restarting", false);
    MXINFO_PRINTLLN(F("Received MQTT reset command!"));
    MXINFO_PRINTLLN(F("RFM69 reset."));
    resetRFM69();
    MXINFO_PRINTLLN(F("ESP restart."));
    #if defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)
      Serial.flush();
    #endif //defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)
    ESP.restart();
  }

  if (topicStr.startsWith((String)check + MQTT_TOPIC_SET_PING)) {
    MXINFO_PRINTLN(F("MQTT Ping ... replying with Pong"));
    mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_GET + MQTT_TOPIC_SET_PONG), message, false);
    return;
  }

  if (topicStr.startsWith((String)check + MQTT_TOPIC_SET_UPDATE)) {
    #ifdef MQTT_HTTP_OTA_FW_UPD
      mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "checkingOTA", false);
      MXINFO_PRINTLN(F("MQTT OTA Requested. Starting update via HTTP."));
      checkForFWUpdates();
      // if we reach this point no fw update was done and we continue listening
      mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "listening", false);
    #else
      MXINFO_PRINTLN("MQTT OTA Requested. But it is disabled in firmware via MQTT_HTTP_OTA_FW_UPD.");
    #endif //MQTT_HTTP_OTA_FW_UPD
  }

  if (topicStr.startsWith((String)mqtt_root + "/thermostat/")) {
    MXDEBUG_PRINTLLN(F("Got message on thermostat subscription topic. Pushing it into the queue."));
    mqttCmd msg;
    msg.hasData = 1;
    msg.receiveTime = millis();
    msg.topic = topicStr;
    msg.value = (String)message;
    pushMQTTCmdsQueue(msg);
  }
  return;
}

/**
   Establish WiFi-Connection

   If connection times out (threshold 50 sec)
   device will sleep for 5 minutes and will restart afterwards.
*/
void startWiFi() {
  delay(10);
  // We start by connecting to a WiFi network
  MXINFO_PRINTLN(F(""));
  MXINFO_PRINT(F("Connecting to SSID: "));
  MXINFO_PRINTLN(cfg_wifi_ssid);
  MXINFO_PRINT(F("Device Name: "));
  MXINFO_PRINTLN(deviceName);

  WiFi.mode(WIFI_STA); //Disable the built-in WiFi access point.
  WiFi.hostname(deviceName);
  WiFi.begin(cfg_wifi_ssid, cfg_wifi_password);
  MXINFO_PRINTLN(F("Using DHCP."));

  int tryCnt = 0;
  int waitAmount = 30;
  while (WiFi.status() != WL_CONNECTED) {
    yield();
    delay(waitAmount);
    MXINFO_PRINT(F("."));
    tryCnt++;

    if (tryCnt > 150) {
      MXINFO_PRINTLN(F(""));
      MXINFO_PRINTLN(F("Could not connect to WiFi. Restarting."));
      #if defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)
        Serial.flush();
      #endif //defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)
      ESP.restart();
    }
  }

  MXINFO_PRINTLN(F(""));
  MXINFO_PRINT(F("WiFi connected, after "));
  MXINFO_PRINT(tryCnt * waitAmount);
  MXINFO_PRINTLN(F("ms"));
  MXINFO_PRINT(F("WiFi signal: "));
  MXINFO_PRINT((String)rssiToPercentage(WiFi.RSSI()));
  MXINFO_PRINTLN(F("%"));
  MXINFO_PRINT(F("IP address : "));
  MXINFO_PRINTLN(WiFi.localIP().toString());
  MXINFO_PRINTLN(F(""));
}

// converts a valid packet into a message structure
message convertPacket2Message() {
  message msg;
  msg.hasData = 1;
  msg.receiveTime = millis();
  msg.packetSize = radio.DATALEN;
  msg.counter = radio.DATA[0];

  //deviceType
  if (radio.DATA[1] == deviceType_t::RemoteControl) {
    msg.deviceType = deviceType_t::RemoteControl;
  } else if (radio.DATA[1] == deviceType_t::WindowSensor) {
    msg.deviceType = deviceType_t::WindowSensor;
  }
  
  //deviceID
  msg.deviceID = msg.deviceID << 8 | radio.DATA[2];
  msg.deviceID = msg.deviceID << 8 | radio.DATA[3];
  msg.deviceID = msg.deviceID << 8 | radio.DATA[4];

  //deviceCmd
  msg.cmdRaw[0] = radio.DATA[5];
  if (msg.deviceType == deviceType_t::WindowSensor) {
    switch (msg.cmdRaw[0]) {
      // WindowSensor
      case deviceCmd_t::WindowClosed:
        msg.deviceCmd = deviceCmd_t::WindowClosed;
        msg.batteryStatus = batteryStatus_t::ok;
        break;
      case deviceCmd_t::WindowOpened:
        msg.deviceCmd = deviceCmd_t::WindowOpened;
        msg.batteryStatus = batteryStatus_t::ok;
        break;
      case deviceCmd_t::WindowClosedBatLow:
        msg.deviceCmd = deviceCmd_t::WindowClosed;
        msg.batteryStatus = batteryStatus_t::low;
        break;
      case deviceCmd_t::WindowOpenedBatLow:
        msg.deviceCmd = deviceCmd_t::WindowOpened;
        msg.batteryStatus = batteryStatus_t::low;
        break;
    }
  }
  if (msg.deviceType == deviceType_t::RemoteControl) {
    switch (msg.cmdRaw[0]) {
      //RemoteControl
      case deviceCmd_t::DayMode:
        msg.deviceCmd = deviceCmd_t::DayMode;
        break;
      case deviceCmd_t::NightMode:
        msg.deviceCmd = deviceCmd_t::NightMode;
        break;
      case deviceCmd_t::SetTemp:
        msg.tempOffset = convertHex2Temp(radio.DATA[6]);
        msg.deviceCmd = deviceCmd_t::SetTemp;
        break;
    }
  }
  
  // keep a raw copy inside the message struct
  for (uint8_t i = 0; i < msg.packetSize; i++) {
    // copy the full raw packet 
    msg.packet[i] = radio.DATA[i];
  }

  // also get the signal strength
  msg.RSSI = radio.RSSI;

  return msg;
}

// check if msg is already an element in messages, if not insert it
// returns true if new message was inserted, false if already existed.
boolean pushMessages(message msg) {
  for (uint8_t i = 0; i < CFG_MESSAGES_SIZE; i++) {
    if (messages[i].hasData) {
      if ((messages[i].deviceID == msg.deviceID) &&
          (messages[i].counter == msg.counter)) {
        // found the same message already in the messages queue
        // no need to continue
        MXDEBUG_PRINTLLN(F("Message already in messages queue. Incrementing numPackets"));
        messages[i].numPackets++;
        return false;
      }
    }
  }
  // if we reach this point we searched the complete messages queue and
  // didn't find the package, we need to insert it at the first free position
  for (uint8_t i = 0; i < CFG_MESSAGES_SIZE; i++) {
    if (!messages[i].hasData) {
      messages[i] = msg;
      MXDEBUG_PRINTLLN(F("New received message written into messages queue."));
      return true;
    }
  }
  // if we reach this point we didn't find the msg in the queue and couldn't
  // insert it new
  MXDEBUG_PRINTLLN(F("ERROR: Could not write message into messages queue."));
  return false;
}

// check the MQTT cmd queue and handles the "oldest" entry
// if no entry was found, returns false
// if it handled an entry returns true
boolean runMQTTCmdsQueue() {
  unsigned long oldestTime = 0;
  uint8_t oldestIndex = 0;
  uint8_t msgCounter = 0;
  for (uint8_t i = 0; i < CFG_MQTTCMDS_SIZE; i++) {
    // search for first free queue slot
    if (mqttCmds[i].hasData == 1) {
      msgCounter++;
      if (mqttCmds[i].receiveTime > oldestTime) {
        oldestTime = mqttCmds[i].receiveTime;
        oldestIndex = i;
      }
    }
  }

  if (oldestTime > 0) {
    MXINFO_PRINTLLN(F("Found MQTT cmd queue entry, handling it."));
    MXDEBUG_PRINTL(F("MQTT cmd queue entry timestamp: "));
    MXDEBUG_PRINTLN(mqttCmds[oldestIndex].receiveTime);
    MXDEBUG_PRINTL(F("MQTT cmd queue entries        : "));
    MXDEBUG_PRINTLN(msgCounter);
    handleThermostatCmds(mqttCmds[oldestIndex].topic, mqttCmds[oldestIndex].value);
    // cleanup MQTT message queue entry
    mqttCmd tmp;
    mqttCmds[oldestIndex] = tmp;
    return true;
  }

  // if we reach this point we searched the complete messages queue and
  // didn't find any messages that needed taking care of
  return false;
}

// takes a message struct and publishes it to MQTT
boolean publishMessagesMQTT(message msg) {
  digitalWrite(LED_BUILTIN, LOW); // turn builtin LED on
  MXINFO_PRINTLLN(F("Sending message to MQTT"));
  #ifdef MXINFO
    Serial.print(F("Collected numPackets of the same contents before sending it: "));
    Serial.println(msg.numPackets);
  #endif //MXINFO
  //MXETHControl/<MAC>/sensor/<SensorID>/
  //get/
  // convert ID to hex string
  char sensorID[7] = {0};
  sprintf(sensorID, "%06X", msg.deviceID); //padding the hex value with leading 0 into 6 characters
  String sensorRoot = (String)mqtt_root + "/sensor/" + sensorID + "/get";

  mqttClient.publish(sensorRoot + "/id", sensorID, true);

  // sensor type
  String deviceType = "";
  if (msg.deviceType == deviceType_t::RemoteControl) {
    deviceType = "RemoteControl";
  } else if (msg.deviceType == deviceType_t::WindowSensor) {
    deviceType = "WindowSensor";
  }
  mqttClient.publish(sensorRoot + "/type", deviceType, true);

  // battery status
  String batteryStatus = "";
  if (msg.deviceType == deviceType_t::WindowSensor) {
    if (msg.batteryStatus == batteryStatus_t::ok) {
      batteryStatus = "ok";
    } else if (msg.batteryStatus == batteryStatus_t::low) {
      batteryStatus = "low";
    }
    mqttClient.publish(sensorRoot + "/battery", batteryStatus, false); // this is a volatile state, do not retain
  }

  // deviceCmd
  String deviceCmd = "";

  if ((msg.deviceType == deviceType_t::WindowSensor) && (msg.deviceCmd == deviceCmd_t::WindowOpened)) {
    deviceCmd = "WindowOpened";
  } else if ((msg.deviceType == deviceType_t::WindowSensor) && (msg.deviceCmd == deviceCmd_t::WindowClosed)) {
    deviceCmd = "WindowClosed";
  } else if ((msg.deviceType == deviceType_t::RemoteControl) && (msg.deviceCmd == deviceCmd_t::DayMode)) {
    deviceCmd = "DayMode";
  } else if ((msg.deviceType == deviceType_t::RemoteControl) && (msg.deviceCmd == deviceCmd_t::NightMode)) {
    deviceCmd = "NightMode";
  } else if ((msg.deviceType == deviceType_t::RemoteControl) && (msg.deviceCmd == deviceCmd_t::SetTemp)) {
    if (msg.tempOffset > 0) {
      // if positive add the + sign as indicator for offset, not an absolute temperature
      deviceCmd = "+" + String(msg.tempOffset,1);
      // we only need one digit precision
    } else {
      // if negative we already have the sign
      deviceCmd = String(msg.tempOffset,1);
    }
  }
  mqttClient.publish(sensorRoot + "/cmd", deviceCmd, false);

  // publish also raw packet
  String rawPacket = "";
  char rawHex[2] = "";
  for (uint8_t i = 0; i < msg.packetSize; i++ ) {
    sprintf(rawHex, "%02X", msg.packet[i]); // padding the hex values with leading 0
    rawPacket = rawPacket + rawHex + ((i < msg.packetSize - 1)? " ":"");
  }
  mqttClient.publish(sensorRoot + "/raw", rawPacket, false);

  // publish RSSI value
  mqttClient.publish(sensorRoot + "/rssi", (String)msg.RSSI, false);

  // publish also a json string which can be used to listen on and have all published values
  // in a single structured message
  String jsonMsg = "";
  jsonMsg = "{\"id\":\"" + (String)sensorID +
            "\",\"type\":\"" + deviceType +
            "\",\"battery\":\"" + batteryStatus +
            "\",\"cmd\":\"" + deviceCmd +
            "\",\"raw\":\"" + rawPacket +
            "\",\"rssi\":" + msg.RSSI +
            "}";
  MXINFO_PRINTLLN("Sending json message to MQTT: ");
  MXINFO_PRINTLN(jsonMsg);
  mqttClient.publish((sensorRoot), jsonMsg); // directly to /get

  digitalWrite(LED_BUILTIN, HIGH); // turn builtin LED off
  return false;
}

// checks if any message time has expired and if yes sends it to MQTT
boolean publishMessages() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < CFG_MESSAGES_SIZE; i++) {
    if (messages[i].hasData) {
      if (now - messages[i].receiveTime > CFG_MESSAGE_DELAY * 1000) {
        MXDEBUG_PRINTLLN(F("Message timer expired, sending it to MQTT"));
        publishMessagesMQTT(messages[i]);
        message tmpMsg; // creating empty message
        messages[i] = tmpMsg; // replacing the message we just published,so removing it from the queue.

        // sending the first message we find, next round will take care about the rest
        return true;
      }
    }
  }
  return false;
}

// initializes the thermostats by publishing once their initial values and
// subscribes to their cmd topic
void initThermostats() {
  MXINFO_PRINTLLN(F("Initializing thermostats and MQTT subscription setup"));
  uint32_t deviceID = 0;
  char thermostatID[7] = {0}; // deviceID as a hex string
  String thermostatRoot = "";
  String rawPacket = "00 00"; // initial raw packet

  for (uint8_t i = 1; i <= CFG_ETH200NUMTHERMOSTATS; i++) {
    // we are pushing the ID number three times into the 3 byte ID value
    // so that we get something like 010101, 020202, 030303
    deviceID = 0;
    deviceID = deviceID << 8 | i;
    deviceID = deviceID << 8 | i;
    deviceID = deviceID << 8 | i;
    sprintf(thermostatID, "%06X", deviceID);

    thermostatRoot = (String)mqtt_root + "/thermostat/" + thermostatID;
    //MXETHControl/<MAC>/thermostat/<ThermostatID>/
    //get/

    mqttClient.publish(thermostatRoot + "/get/id", thermostatID, true);
    //mqttClient.publish(thermostatRoot + "/get/raw", rawPacket, false); //v2.4 we didn't do anything so don't update this value

    // we are subscribing to the "cmd" topic and after that's finished we are "Ready"
    mqttClient.subscribe((thermostatRoot + "/set/cmd").c_str());
    //mqttClient.publish(thermostatRoot + "/set/cmd", "Ready", false); //v2.4 removed sending a Ready update
    // at this point MXETHControl will get the published "Ready" value back because it is already subscribed
    // MQTT Message arrived topic :[MXETHControl/BCDDC2248523/thermostat/010101/set/cmd] MQTT Message: Ready
  }

  for (uint8_t i = 1; i <= CFG_ETH200NUMGROUPS; i++) {
    // we are pushing the ID number three times into the 3 byte ID value
    // so that we get something like 990101, 990202, 990303
    deviceID = 0;
    deviceID = deviceID << 8 | 0x99;
    deviceID = deviceID << 8 | i;
    deviceID = deviceID << 8 | i;
    sprintf(thermostatID, "%06X", deviceID);

    thermostatRoot = (String)mqtt_root + "/thermostat/" + thermostatID;
    //MXETHControl/<MAC>/thermostat/<ThermostatID>/
    //get/

    mqttClient.publish(thermostatRoot + "/get/id", thermostatID, true);
    //mqttClient.publish(thermostatRoot + "/get/raw", rawPacket, false); //v2.4 we didn't do anything so don't update this value

    // we are subscribing to the "cmd" topic and after that's finished we are "Ready"
    mqttClient.subscribe((thermostatRoot + "/set/cmd").c_str());
    //mqttClient.publish(thermostatRoot + "/set/cmd", "Ready", false); //v2.4 removed sending a Ready update
    // at this point MXETHControl will get the published "Ready" value back because it is already subscribed
    // MQTT Message arrived topic :[MXETHControl/BCDDC2248523/thermostat/FF0101/set/cmd] MQTT Message: Ready
  }
  // when we reach this point initialization is finished
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "initialized", false);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // turn builtin LED on when we start doing stuff

  MXDEBUG_PRINTLLN(F("Preparing RMF69 pins."));
  detachInterrupt(CFG_RF69_IRQ_PIN);
  pinMode(CFG_RF69_IRQ_PIN, INPUT);
  digitalWrite(CFG_RF69_IRQ_PIN, LOW);
  pinMode(CFG_RF69_RST_PIN, OUTPUT);
  digitalWrite(CFG_RF69_RST_PIN, LOW);
  delay(10); // wait 10ms to wait for RFM69 ready 

  #if defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)
    //if we don't output anything we don't need serial
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("MXETHControl - Serial output initialized"));
    #if defined(MXDEBUG)
      Serial.println(F("MXDEBUG enabled"));
      MXDEBUG_PRINTLLN(F("MXDEBUG macro output working."));
    #elif defined(MXINFO)
      Serial.println(F("MXINFO enabled"));
      MXINFO_PRINTLN(F("MXINFO macro output working."));
    #elif defined(MXDEBUG_TIME)
      Serial.println(F("MXDEBUG_TIME enabled"));
    #endif //defined(MXDEBUG)
    Serial.println();
    //while(!Serial) {} // Wait
  #endif //defined(MXDEBUG) || defined(MXINFO) || defined(MXDEBUG_TIME)

  MXINFO_PRINTLN(F(""));
  MXTIME_PRINT(F(""));
  splashScreen();
  MXTIME_PRINT(F(""));

  startWiFi();
  MXTIME_PRINT(F(""));

  /* I believe this isn't required.
  // initialize queues
  for (uint8_t i = 0; i < CFG_MESSAGES_SIZE; i++) {
    message tmpMsg;
    messages[i] = tmpMsg;
  }
  for (uint8_t i = 0; i < CFG_MQTTCMDS_SIZE; i++) {
    mqttCmd tmpCmd;
    mqttCmds[i] = tmpCmd;
  }
  */

  #ifdef HTTP_OTA_FW_UPD
    // do the firmware updates at the beginning, if we have a bug in the following code
    // it should always be fixable through a firmware update
    checkForFWUpdates();
    MXINFO_PRINTLN(F(""));
  #endif //HTTP_OTA_FW_UPD
  
  MXTIME_PRINT(F(""));
  MXINFO_PRINTLN(F(""));

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  //since we are sleeping all the time we look for any retained messages
  //in the "set" topic when we wake up and then we act on them
  //mqttClient.subscribe(((String)mqtt_root + CONFIG_MQTT_TOPIC_SET + "/#").c_str());
  mqttClient.loop();
  MXTIME_PRINT(F(""));
  MXINFO_PRINTLN(F(""));
  
  MXTIME_PRINT(F(""));
  MXINFO_PRINT(F("MQTT sending status data."));
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_HARDWARE), MQTT_PRJ_HARDWARE, true);
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_VERSION), MQTT_PRJ_VERSION, true);
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_MAC), WiFi.macAddress(), true);
  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_IP), WiFi.localIP().toString());
  MXINFO_PRINTLN("");
  MXTIME_PRINT("");
  yield();

  MXINFO_PRINTLN(F("Initializing ETH200RFM69 module."));
  radio.initialize();
  radio.setPowerLevel(CFG_RF69_POWERLEVEL);
  MXDEBUG_PRINTLLN(F("Finished initializing the ETH200RFM69 module."));
  MXDEBUG_PRINTLLN(F("ETH200RFM69 register readout:"));
  #ifdef MXDEBUG
    radio.readAllRegsCompact();
    MXINFO_PRINTLN(F(""));
    radio.readAllRegs();
  #endif //MXDEBUG

  initThermostats();

  mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "listening", false);
  digitalWrite(LED_BUILTIN, HIGH); // turn builtin LED off when we stop doing stuff
}

int counter = 0;
int counterLoop = 0;
int counterBreak = ((CFG_ESP_LOOP_DELAY > 0)? (100000 / (CFG_ESP_LOOP_DELAY * 10)): 100000);
uint8_t receivingSomething = 0; // this is set whenever we received a valid packet and
                                // reset if we haven't received a packet in receivingMaxTime
unsigned long receivingLastTime = 0; // haven't received anything yet

void loop() {
  // keep mqtt client connection active
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  // check if we have any message to publish
  yield();
  publishMessages();

  // check if any incoming MQTT cmds need to be handled
  yield();
  runMQTTCmdsQueue();

  #ifdef MXINFO
    /*
    if (counter % 30000 == 0) {
      // turn off every other second
      digitalWrite(LED_BUILTIN, HIGH);
    } else if (counter % 15000 == 0) {
      // turn on
      digitalWrite(LED_BUILTIN, LOW);
    }
    */
    if (counter >= counterBreak) {
      MXINFO_PRINT(F("Listening for packets, loop (x"));
      MXINFO_PRINT(counterBreak);
      MXINFO_PRINT(F("): "));
      MXINFO_PRINTLN(counterLoop);
      counterLoop++;
      counter = 0;
    }
    counter++;
  #endif //MXINFO
  if (radio.receiveDone()) {
    if (receivingSomething == 0) {
      // received the first packet, of several packets
      digitalWrite(LED_BUILTIN, LOW); // turn builtin LED on
      receivingSomething = 1;
      receivingLastTime = millis();
      mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "receiving", false);
    } else {
      // we are already in "receiving" state, got another packet, need to reset timer
      receivingLastTime = millis();
    }
    message msg = convertPacket2Message();
    pushMessages(msg);
    MXDEBUG_PRINTLLN(F("Packet received:"));
    // static printing of sync word
    #ifdef MXDEBUG
      printHexWithZeroPad(Serial, (uint8_t)0x7E); // this is hard coded just for output,
                                                  // otherwise handled directly by RFM69
      Serial.print(" ");
      for (byte i = 0; i < radio.DATALEN; i++) {
        printHexWithZeroPad(Serial, radio.DATA[i]);
        Serial.print(" ");
      }
      Serial.print("   [RX_RSSI:");
      Serial.print(radio.RSSI);
      Serial.println("]");
      Serial.println();
    #endif //MXDEBUG
  }
  if (receivingSomething == 1) {
    if (millis() - receivingLastTime > CFG_STATE_RECEIVING_MAX_TIME) {
      // checking if the last received packet is from a while ago, if yes
      // update status
      receivingSomething = 0;
      receivingLastTime = 0;
      // we are back to listening
      mqttClient.publish(((String)mqtt_root + MQTT_TOPIC_STATUS + MQTT_TOPIC_STATUS_STATE), "listening", false);
      digitalWrite(LED_BUILTIN, HIGH); // turn builtin LED off
    }
  }

  // slow down the loop a bit
  delay(CFG_ESP_LOOP_DELAY); 
}