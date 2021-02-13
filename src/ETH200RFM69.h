/****************************************************************************
ETH200RFM69 modification for ELV ETH200

For use with an RFM69CW module and specific behavior for the ELV ETH200
sensors and actors.  

Copyright 2020 mt-mrx <64284703+mt-mrx@users.noreply.github.com>

Based on:
  RFM69 library:
    https://github.com/LowPowerLab/RFM69
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

#ifndef ETH200RFM69_h
  #define ETH200RFM69_h

  #include <RFM69.h>
  #include <RFM69registers.h>

  class ETH200RFM69: public RFM69 {
    public:
      static uint8_t PAYLOADETH200;
      static uint16_t ETH200CRCStartWindowSensor;
      static uint16_t ETH200CRCStartRemoteControl;
      static uint16_t ETH200CRCMask;
      uint8_t currentPacketCounter = 1; // the current packet counter, will be incremented whenever a packet is sent
      uint8_t *lastSentPacket; // the last raw packet we sent out, pointer to an array which will be initialized during constructor
      uint8_t lastSentPacketSize = 0;
      ETH200RFM69(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false); //override
      bool initialize(); //override
      void readAllRegs(); //override
      virtual bool receiveDone(); //override
      boolean send(uint8_t buffer[], uint8_t bufferSize, uint8_t numStuffedBits = 0); //override and signature change
      boolean sendPacket(uint8_t deviceType, uint32_t address, uint8_t cmd, uint8_t cmds[], uint8_t cmdsSize); // sends a packet
    protected:
      static void isr0(); //override
      void interruptHandler(); //override
      void sendFrame(uint8_t buffer[], uint8_t bufferSize, uint8_t numStuffedBits = 0); //override and signature change
      uint8_t reverseByte(uint8_t b);
      uint16_t calcCRC16r(uint16_t c,uint16_t crc, uint16_t mask);
      uint16_t calcPacketCRC16r(uint8_t packet[], uint8_t length, uint16_t crcStart, uint16_t crcMask);
      uint8_t destuffPayload(uint8_t inBuf[], uint8_t destuffedBuf[], uint8_t length);
      uint8_t stuffPayload(uint8_t inBuf[], uint8_t stuffedBuf[], uint8_t length);
  };
#endif // ETH200RFM69_h