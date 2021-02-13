/****************************************************************************
MXPubSubClientWrapper.h - Simple extension of PubSubClient.

Includes functions which can handle the String class.
 
Copyright 2020 mt-mrx <64284703+mt-mrx@users.noreply.github.com>

Based on:
 PubSubClient - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
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

#ifndef MXPUBSUBCLIENTWRAPPER_H
    #define MXPUBSUBCLIENTWRAPPER_H
  /*
  * #define MQTT_MAX_PACKET_SIZE 256
  * increases MQTT message sizes, needs to be done either in the PubSubClient
  * library or can be done in platformio.ini file.
  * If you would define it here, it has no effect on the library
  * I think since version 2.8 there also exist a method setBufferSize() to
  * override MQTT_MAX_PACKET_SIZE
  */
  #include <PubSubClient.h>      // for MQTT

  class MXPubSubClientWrapper : public PubSubClient {
    private:
    public:
      MXPubSubClientWrapper(Client& espc);
      bool publish(StringSumHelper topic, String str);
      bool publish(StringSumHelper topic, unsigned int num);
      bool publish(const char* topic, String str);
      bool publish(const char* topic, unsigned int num);

      bool publish(StringSumHelper topic, String str, bool retain);
      bool publish(StringSumHelper topic, unsigned int num, bool retain);
      bool publish(const char* topic, String str, bool retain);
      bool publish(const char* topic, unsigned int num, bool retain);
  };

  MXPubSubClientWrapper::MXPubSubClientWrapper(Client& espc) : PubSubClient(espc) {

  }

  bool MXPubSubClientWrapper::publish(StringSumHelper topic, String str) {
    return publish(topic.c_str(), str);
  }

  bool MXPubSubClientWrapper::publish(StringSumHelper topic, unsigned int num) {
    return publish(topic.c_str(), num);
  }

  bool MXPubSubClientWrapper::publish(const char* topic, String str) {
    return publish(topic, str, false);
  }

  bool MXPubSubClientWrapper::publish(const char* topic, unsigned int num) {
    return publish(topic, num, false);
  }

  bool MXPubSubClientWrapper::publish(StringSumHelper topic, String str, bool retain) {
    return publish(topic.c_str(), str, retain);
  }

  bool MXPubSubClientWrapper::publish(StringSumHelper topic, unsigned int num, bool retain) {
    return publish(topic.c_str(), num, retain);
  }

  bool MXPubSubClientWrapper::publish(const char* topic, String str, bool retain) {
    char buf[128];

    if (str.length() >= 128) return false;

    str.toCharArray(buf, 128);
    return PubSubClient::publish(topic, buf, retain);
  }

  bool MXPubSubClientWrapper::publish(const char* topic, unsigned int num, bool retain) {
    char buf[6];

    dtostrf(num, 0, 0, buf);
    return PubSubClient::publish(topic, buf, retain);
  }
#endif //MXPUBSUBCLIENTWRAPPER_H