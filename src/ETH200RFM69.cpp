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

#include <ETH200RFM69.h>
#include <MXDebugUtils.h>      // for debugging function support

uint8_t ETH200RFM69::PAYLOADETH200;
uint16_t ETH200RFM69::ETH200CRCStartWindowSensor;
uint16_t ETH200RFM69::ETH200CRCStartRemoteControl;
uint16_t ETH200RFM69::ETH200CRCMask;

// for ETH200 packet analysis
enum deviceType_t {
  deviceTypeUnknown,
  RemoteControl = 0x10,
  WindowSensor = 0x20,
};

ETH200RFM69::ETH200RFM69(uint8_t slaveSelectPin, uint8_t interruptPin, bool isRFM69HW): RFM69(slaveSelectPin, interruptPin, isRFM69HW) {

}

bool ETH200RFM69::initialize() {
  _interruptNum = digitalPinToInterrupt(_interruptPin);
  if (_interruptNum == NOT_AN_INTERRUPT) return false;
  #ifdef RF69_ATTACHINTERRUPT_TAKES_PIN_NUMBER
      _interruptNum = _interruptPin;
  #endif

  // packet size/payload length (without sync word) in byte
  // I think variable length packet mode cannot be used because the RFM69 expects the first payload byte to contain the length
  // which is not the case for the ETH200 system
  PAYLOADETH200 = CFG_ETH200MAXPACKETSIZE; // max of 9 bytes of payload per packet, this should be true for
                      // "windows sensor"(8 byte) and "remote control"(9 byte)
                      // but if there were 5 consecutive ones, "11111", included in the bit string the string was stuffed with a "0"
                      // after the 5 ones, and this one "0" bit shifts all the others bits to the right and we have another byte added.
                      // so in the end, we have a max of 10 byte

  uint8_t initLastSentPacket[CFG_ETH200MAXPACKETSIZE] = {0}; // pointer to empty initialized proper sized array
  lastSentPacket = initLastSentPacket;
  // start and mask values used for CRC caculation
  // Dietmar Weisser - https://www.mikrocontroller.net/topic/172034
  // I have no clue how he figured them out
  ETH200CRCStartWindowSensor = 0xBDB7;
  ETH200CRCStartRemoteControl = 0xC11F;
  ETH200CRCMask = 0x8408;

  const uint8_t CONFIG[][2] = {
    /* 0x01 */ { REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY },
    /* 0x02 */ { REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET |
                                RF_DATAMODUL_MODULATIONTYPE_FSK |
                                RF_DATAMODUL_MODULATIONSHAPING_00 }, // no shaping   
    /* some datarates documented in the datasheet
      MSB LSM     Decimal value     Bitrate
      1a 0b       6667              4.8
      0d 05       3333              9.6
      0a 00       2560              12.5
      06 83       1667              19.2
      03 41       833               38.4
      02 80       640               50.0

      formula: 640 * 50 / (desired bitrate) = Decimal value
        e.g.   640 * 50 / 38.4 = 833.3333 -> rounded 833 = 0x03 0x41
      
      my calculated values
      640 * 50 / 10 kbit/s = 3200 = 0x0C 0x80
    */   
    // ELV200 9.6 kbit/s, sync word 0x7E is transmitted in manchester 0x6AA9 in 1.69ms, measured through RTL-SDR and WindowSensor
    /* 0x03 */ { REG_BITRATEMSB, RF_BITRATEMSB_9600},
    /* 0x04 */ { REG_BITRATELSB, RF_BITRATELSB_9600},
    // 10 kbit/s
    ///* 0x03 */ { REG_BITRATEMSB, 0x0C},
    ///* 0x04 */ { REG_BITRATELSB, 0x80},
    ///* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_5000}, // default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
    ///* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_5000},
    ///* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_15000}, // 15kHz, according to forum post "Hub: 15kHz"
    ///* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_15000},
    /* 0x05 */ { REG_FDEVMSB, RF_FDEVMSB_25000}, // 25kHz, RTL-SDR diagram looks better (no idea if that's a valid assumption)
    /* 0x06 */ { REG_FDEVLSB, RF_FDEVLSB_25000},

    // Frequency setting:
    // 868.0 MHz = 0xD9 00 00 = 0b 1101 1001  0000 0000  0000 0000 = 0d 14221312
    // 868.3 MHz = 0xD9 13 33 = 0b 1101 1001  0001 0011  0011 0011 = 0d 14226227
    // 915.0 MHz = 0xE4 C0 00 = 0b 1110 0100  1100 0000  0000 0000 = 0d 14991360
    //ELV200: 868.3MHz # source: https://forum.fhem.de/index.php?topic=49300.0
    //                   source: https://www.mikrocontroller.net/articles/RFM69
    ///* 0x07 */ { REG_FRFMSB, 0xD9 },
    ///* 0x08 */ { REG_FRFMID, 0x13 },
    ///* 0x09 */ { REG_FRFLSB, 0x33 },
    // self calculated: FRF = 14221312 * x / 868    // x = desired frequency
    // 868.304 MHz    // 14221312 * 868.304 / 868 = 0d 14226292 = 0xD9 13 74
    // 868.304 MHz    // 14221312 * 868.308 / 868 = 0d 14226358 = 0xD9 13 B6
    /* 0x07 */ { REG_FRFMSB, 0xD9 },
    /* 0x08 */ { REG_FRFMID, 0x13 },
    /* 0x09 */ { REG_FRFLSB, 0xB6 },

    // looks like PA1 and PA2 are not implemented on RFM69W/CW, hence the max output power is 13dBm
    // +17dBm and +20dBm are possible on RFM69HW
    // +13dBm formula: Pout = -18 + OutputPower (with PA0 or PA1**)
    // +17dBm formula: Pout = -14 + OutputPower (with PA1 and PA2)**
    // +20dBm formula: Pout = -11 + OutputPower (with PA1 and PA2)** and high power PA settings (section 3.3.7 in datasheet)
    ///* 0x11 */ { REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111},
    ///* 0x13 */ { REG_OCP, RF_OCP_ON | RF_OCP_TRIM_95 }, // over current protection (default is 95mA)

    // RXBW defaults are { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_5} (RxBw: 10.4KHz)
    /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2 }, // (BitRate < 2 * RxBw)
    //for BR-19200: /* 0x19 */ { REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_24 | RF_RXBW_EXP_3 },
    /* 0x25 */ { REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01 }, // DIO0 is the only IRQ we're using
    /* 0x26 */ { REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF }, // DIO5 ClkOut disable for power saving
    /* 0x28 */ { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
    
    /* RSSI Threshhold
      220 - works fine, often detects noise
    */
    /* 0x29 */ { REG_RSSITHRESH, 220 }, // must be set to dBm = (-Sensitivity / 2), default is 0xE4 = 228 so -114dBm
    
    ///* 0x2D */ { REG_PREAMBLELSB, RF_PREAMBLESIZE_LSB_VALUE } // default 3 preamble bytes 0xAAAAAA
    /* 0x2D */ { REG_PREAMBLELSB, 0x03 }, // ETH200 expects 4 byte preamble (manchester encoded) and I think it should be 0x55 55 AA AA
                                          // (what I'm seeing from the Window Sensor is 0x15 55 AA AA)
                                          // but we cannot send that using RFM69 integrated functions, you can only
                                          // specify the number of 0xAA it will send. Need to do it ugly/manually

    /* ETH200: sync word config
    
      sync word for ELV ETH200 raw value (0x7E in manchester) is: 01101010 10101001
      MSB set to 01101010 = 0x6A for ETH200
      LSB set to 10101001 = 0xA9 for ETH200
      this needs to be the raw bitstream, it doesn't matter if the integrated manchester
      decoder is used or not, the decoder is not applied to the sync word.

      These syncvalues will also be used by the RFM69 module when sending packets and they will be automatically prepended.
      Since I moved to sending the complete packages bit stream by myself no automatic prepending of sync word is used.

      The number of errors tolerated in the Sync word recognition can be set from 0 to 7 bits to via SyncTol.
    */
    /* 0x2E */ { REG_SYNCCONFIG, RF_SYNC_ON |
                                 RF_SYNC_SIZE_2 |
                                 RF_SYNC_FIFOFILL_AUTO |
                                 RF_SYNC_TOL_0 },
    /* 0x2F */ { REG_SYNCVALUE1, 0x6A },
    /* 0x30 */ { REG_SYNCVALUE2, 0xA9 },

    // ETH200: packet config
    // use the internal Manchester decoding/encoding
    /* 0x37 */ { REG_PACKETCONFIG1, RF_PACKET1_FORMAT_FIXED |
                                    RF_PACKET1_DCFREE_MANCHESTER |
                                    RF_PACKET1_CRC_OFF |
                                    RF_PACKET1_CRCAUTOCLEAR_OFF |
                                    RF_PACKET1_ADRSFILTERING_OFF },
    // packet size/payload length (without sync word) in byte
    /* 0x38 */ { REG_PAYLOADLENGTH, PAYLOADETH200 }, // in variable length mode: the max frame size, not used in TX

    ///* 0x39 */ { REG_NODEADRS, nodeID }, // turned off because we're not using address filtering
    /* 0x3C */ { REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE }, // TX on FIFO not empty
    ///* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    //for BR-19200: /* 0x3D */ { REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_NONE | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF }, // RXRESTARTDELAY must match transmitter PA ramp-down time (bitrate dependent)
    /* 0x6F */ { REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0 }, // run DAGC continuously in RX mode for Fading Margin Improvement, recommended default for AfcLowBetaOn=0
    {255, 0}
  };

  digitalWrite(_slaveSelectPin, HIGH);
  pinMode(_slaveSelectPin, OUTPUT);
  if(_spi == nullptr){
    _spi = &SPI;
  }
  #if defined(ESP32)
    _spi->begin(18,19,23,5); //SPI3  (SCK,MISO,MOSI,CS)
    //_spi->begin(14,12,13,15); //SPI2  (SCK,MISO,MOSI,CS) 
  #else
    _spi->begin();
  #endif  

  #ifdef SPI_HAS_TRANSACTION
    _settings = SPISettings(8000000, MSBFIRST, SPI_MODE0);
  #endif

  uint32_t start = millis();
  uint8_t timeout = 50;
  // check if registers can be written and read back, to verify chip communication works
  do writeReg(REG_SYNCVALUE1, 0xAA); while (readReg(REG_SYNCVALUE1) != 0xaa && millis()-start < timeout);
  start = millis();
  do writeReg(REG_SYNCVALUE1, 0x55); while (readReg(REG_SYNCVALUE1) != 0x55 && millis()-start < timeout);

  for (uint8_t i = 0; CONFIG[i][0] != 255; i++)
    writeReg(CONFIG[i][0], CONFIG[i][1]);

  // Encryption is persistent between resets and can trip you up during debugging.
  // Disable it during initialization so we always start from a known state.
  encrypt(0);

  setHighPower(_isRFM69HW); // called regardless if it's a RFM69W or RFM69HW
  setMode(RF69_MODE_STANDBY);
  start = millis();
  while (((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) && millis()-start < timeout); // wait for ModeReady
  if (millis()-start >= timeout)
    return false;
  attachInterrupt(_interruptNum, ETH200RFM69::isr0, RISING);

  //ELV200: don't need nodeID
  //_address = nodeID;
  return true;
}

/*internal function
  This function removes the stuffed "0" after 5x "1". That means
  "|01111101|01..." will become "|01111110|1..." for the whole bit string.

  inBuf[]        - pointer to the inBuf[] array
  destuffedBuf[] - returns the destuffed array as destuffedBuf[]
  length         - buf[] length, sizeof(destuffedBuf[]) is length - 1
  boolean        - return value is true if destuffing happened
*/
uint8_t ETH200RFM69::destuffPayload(uint8_t inBuf[], uint8_t destuffedBuf[], uint8_t length) {
  uint8_t wasStuffed = 0;    // returns the number of time buf[] needed to be destuffed
  uint8_t oneCounter = 0;    // counts numbers of "1" will be reset after 5 found
  uint8_t i = 0;             // the Byte # in buf[] we are currently itterating over
  int8_t j = 0;              // the Bit # in the Byte we are currently itterating over
  uint8_t curByte = 0;       // the Byte of buf[] we are currently itterating over
  uint8_t curBit = 0;        // the current Bit we are looking at
  uint8_t outArrPos = 0;     // position in destuffedBuf[] where we are writing to
  uint8_t outBitCounter = 0; // the number of Bits already written to our destuffedBuf[outArrPos]
                             // needs to roll after 8 written Bits

  for (i = 0; i < length; i++) {
    curByte = inBuf[i];
    for (j = 7; j >= 0; j--) {
      curBit = bitRead(curByte, j);
      if (oneCounter == 5) {
        // found 5x "1" we need to delete the "0", that means just don't write it to the out array and go to next bit
        //    "|01111101|01..." will become "|01111110|1..."
        // or "|01111100|11..." will become "|01111101|1..."
        wasStuffed++;
        oneCounter = 0;
      } else if (curBit == 1) {
        // read a "1" but haven't yet reached our 5x "1" limit
        // shift the outByte to the left and push a 1 into it
        destuffedBuf[outArrPos] = destuffedBuf[outArrPos] << 1 | 1;
        outBitCounter++;
        oneCounter++;
      } else if (curBit == 0) { // just to make it a bit more readable
        // shift the outByte to the left and push a 0 into it
        destuffedBuf[outArrPos] = destuffedBuf[outArrPos] << 1 | 0;
        outBitCounter++; 
        oneCounter = 0; // whenever we find a "0" we reset the number
                        // of consecutive "1" we've seen so far
      }
      if (outBitCounter == 8) {
        // we have written 8 Bits to the Byte in the destuffedBuf array,
        // go to next Byte
        outArrPos++;
        outBitCounter = 0;
      }
      if (outArrPos == length - 1) {
        // we reached the end of the destuffedBuf array and filled all its Bits
        // no need to look at the remaining Bits in inBuf
        break;
      }
    }
  }
  return wasStuffed;
}

/* internal function
  This function adds a stuffed "0" after 5x "1". That means
  "|01111110|1..." will become "|01111101|01..." for the whole bit string.

  inBuf[]        - pointer to the inBuf[] array
  stuffedBuf[]   - returns the destuffed array as destuffedBuf[]
  length         - buf[] length, sizeof(stuffedBuf[]) is length + 1
  boolean        - return value is true if destuffing happened
*/
uint8_t ETH200RFM69::stuffPayload(uint8_t inBuf[], uint8_t stuffedBuf[], uint8_t length) {
  uint8_t wasStuffed = 0;    // returns the number of time buf[] needed to be stuffed
  uint8_t oneCounter = 0;    // counts numbers of "1" will be reset after 5 found
  uint8_t i = 0;             // the Byte # in buf[] we are currently itterating over
  int8_t j = 0;              // the Bit # in the Byte we are currently itterating over
  uint8_t curByte = 0;       // the Byte of buf[] we are currently itterating over
  uint8_t curBit = 0;        // the current Bit we are looking at
  uint8_t outArrPos = 0;     // position in destuffedBuf[] where we are writing to
  uint8_t outBitCounter = 0; // the number of Bits already written to our stuffedBuf[outArrPos]
                             // needs to roll after 8 written Bits

  for (i = 0; i < length; i++) {
    curByte = inBuf[i];
    for (j = 7; j >= 0; j--) {
      curBit = bitRead(curByte, j);
      if (oneCounter == 5) {
        // found 5x "1" we need to add a "0"
        //    "|01111110|1..." will become "|01111101|01..."
        // or "|01111101|1..." will become "|01111100|11..."
        // first, add the additional 0
        stuffedBuf[outArrPos] = stuffedBuf[outArrPos] << 1 | 0;
        outBitCounter++; 
        // second, check for byte roll over
        if (outBitCounter == 8) {
          // we have written 8 Bits to the Byte in the stuffedBuf array,
          // go to next Byte
          outArrPos++;
          outBitCounter = 0;
        }
        wasStuffed++;
        oneCounter = 0;
      }
      // each bit we read we need to write to the output
      if (curBit == 1) {
        // read a "1"
        // shift the outByte to the left and push a 1 into it
        stuffedBuf[outArrPos] = stuffedBuf[outArrPos] << 1 | 1;
        outBitCounter++;
        oneCounter++;
      } else if (curBit == 0) { // just to make it a bit more readable
        // shift the outByte to the left and push a 0 into it
        stuffedBuf[outArrPos] = stuffedBuf[outArrPos] << 1 | 0;
        outBitCounter++; 
        oneCounter = 0; // whenever we find a "0" we reset the number
                        // of consecutive "1" we've seen so far
      }
      if (outBitCounter == 8) {
        // we have written 8 Bits to the Byte in the stuffedBuf array,
        // go to next Byte
        outArrPos++;
        outBitCounter = 0;
      }
    }
    if (i == length - 1) {
      // we finished with the last Byte of inBuff[]
      // need to move the bits of the last Byte in stuffedBuf[] to the correct position
      // 0000 0000    0 stuffed, shift (8 - 0), irrelevant
      // 0000 000x    1 stuffed, shift (8 - 1)
      // 0000 00xx    2 stuffed, shift (8 - 2)
      // xxxx xxxx    8 stuffed, shift (8 - 8)
      // e.g. "01101110|01111111|bitStringEnd" will become 
      //      "01101110|01111101|10000000"
      stuffedBuf[outArrPos] = stuffedBuf[outArrPos] << (8 - wasStuffed) | 0;
    }
  }
  return wasStuffed;
}

// internal function
 ISR_PREFIX void ETH200RFM69::isr0() {
   _haveData = true;
 }

// copied from RFM69, just for adding debug output
bool ETH200RFM69::receiveDone() {
  if (_haveData) {
    //MXDEBUG_PRINTLLN(F("IRQ received, have data"));
  	_haveData = false;
  	interruptHandler();
  }
  if (_mode == RF69_MODE_RX && PAYLOADLEN > 0) {
    setMode(RF69_MODE_STANDBY); // enables interrupts
    return true;
  } else if (_mode == RF69_MODE_RX) { 
    // already in RX no payload yet
    return false;
  }
  receiveBegin();
  return false;
}

// internal function - interrupt gets called when a packet is received
void ETH200RFM69::interruptHandler() {
  //MXDEBUG_PRINTLLN(F("IRQ triggered."));
  if (_mode == RF69_MODE_RX && (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)) {
    setMode(RF69_MODE_STANDBY);
    select();
    _spi->transfer(REG_FIFO & 0x7F);


    // 1. step: destuff the bit string
    // to be able to do that we need to read the complete number of PAYLOADETH200 bytes.
    
    uint8_t buf[PAYLOADETH200]; // temporary buffer to read the fifo into
    for (uint8_t i = 0; i < PAYLOADETH200; i++) {
      buf[i] = _spi->transfer(0);
    }
    #ifdef MXDEBUG
      MXDEBUG_PRINTLLN(F("raw packet(zero stuffed, manchester decoded):"));
      for (uint8_t i = 0; i < PAYLOADETH200; i++) {
        printHexWithZeroPad(Serial, buf[i]);
        //Serial.print(F(" "));
        Serial.print(F("       "));
      }
      Serial.println();
      for (uint8_t i = 0; i < PAYLOADETH200; i++) {
        printBinWithZeroPad(Serial, buf[i]);
        Serial.print(F(" "));
      }
      Serial.println();
    #endif //MXDEBUG

    uint8_t destuffedBufLength = PAYLOADETH200 - 1; // we are always one byte shorter than the stuffed array
    uint8_t destuffedBuf[destuffedBufLength];
    uint8_t wasStuffed __attribute__((unused));
    wasStuffed = destuffPayload(buf, destuffedBuf, PAYLOADETH200);
    #ifdef MXDEBUG
      MXDEBUG_PRINTLLN(F("packet(zero destuffed, manchester decoded):"));
      if (wasStuffed > 0) {
        Serial.print("packet was stuffed: ");
        Serial.print(wasStuffed);
        Serial.println(" times");
      }
      for (uint8_t i = 0; i < destuffedBufLength; i++) {
        printHexWithZeroPad(Serial, destuffedBuf[i]);
        //Serial.print(F(" "));
        Serial.print(F("       "));
      }
      Serial.println();
      for (uint8_t i = 0; i < destuffedBufLength; i++) {
        printBinWithZeroPad(Serial, destuffedBuf[i]);
        Serial.print(F(" "));
      }
      Serial.println();
    #endif //MXDEBUG

    // 2. step: reverse the byte order
    MXDEBUG_PRINTLLN(F("packet(reversed byte order, zero destuffed, manchester decoded):"));
    for (uint8_t i = 0; i < destuffedBufLength; i++) {
      destuffedBuf[i] = reverseByte(destuffedBuf[i]);
      #ifdef MXDEBUG
        printHexWithZeroPad(Serial, destuffedBuf[i]);
        Serial.print(F(" "));
      #endif //MXDEBUG
    }
    MXDEBUG_PRINTLN(F(""));

    // 3. determine device type
    // we need this to determine where the CRC is located and how long the packet is
    // byte 1 - ETH200 communications counter
    uint8_t communicationCounter __attribute__((unused));
    communicationCounter = destuffedBuf[0];
    // byte 2 - ETH200 device type
    uint8_t deviceType = destuffedBuf[1];
    #ifdef MXDEBUG
      MXDEBUG_PRINTLLN(F("packet analysis"));
      Serial.print(F("communicationCounter -> 0x"));
      printHexWithZeroPad(Serial, communicationCounter);
      Serial.println();
      Serial.print(F("deviceType -> 0x"));
      printHexWithZeroPad(Serial, deviceType);
      Serial.println();
    #endif

    // 4. step: check if packet is of a known device type
    uint16_t crcStart = 0;
    if (deviceType == deviceType_t::RemoteControl) {
      // packet length = 10 bytes (incl sync word)
      PAYLOADLEN = 9;
      crcStart = ETH200CRCStartRemoteControl;
    } else if (deviceType == deviceType_t::WindowSensor) {
      // packet length = 9 bytes (incl sync word)
      PAYLOADLEN = 8;
      crcStart = ETH200CRCStartWindowSensor;
    } else {
      // 0x30 = wall thermostat, 
      // 0x31 - 0x33 USB-program stick 
      //  (0x33 = learn, 0x32 = time sync, 0x33 = week program)
      MXDEBUG_PRINTLLN(F("Received deviceType which is not implemented. Ignoring packet."));
      PAYLOADLEN = 0;
    }
    PAYLOADLEN = PAYLOADLEN > 66 ? 0 : PAYLOADLEN; // precaution
    #ifdef MXDEBUG
      MXDEBUG_PRINTLLN(F("Payload length based on device type."));
      Serial.print(F("PAYLOADLEN -> "));
      Serial.println(PAYLOADLEN);
    #endif //MXDEBUG

    if ((PAYLOADLEN > 9) || (PAYLOADLEN == 0)) {
      // reset, packet isn't for us
      MXDEBUG_PRINTLLN(F("Packet received but packet length too long or too short, discarded."));
      PAYLOADLEN = 0;
      unselect();
      receiveBegin();
      return;
    }

    #ifdef MXDEBUG
      MXDEBUG_PRINTLLN(F("packet(proper length, reversed, destuffed, manchester decoded):"));
      for (uint8_t i = 0; i < PAYLOADLEN; i++) {
        printHexWithZeroPad(Serial, destuffedBuf[i]);
        Serial.print(F(" "));
      }
      Serial.println(F(""));
    #endif //MXDEBUG

    // 5. step: calculate the CRC
    MXDEBUG_PRINTLLN(F("Calculating and comparing CRC:"));
    uint16_t crcCalculated = 0; // the CRC we calculated from Byte #1 to Byte #PAYLOADLEN - 2
    uint16_t crcPacket = 0;     // the CRC extracted from the packet
    crcCalculated = calcPacketCRC16r(destuffedBuf, PAYLOADLEN - 2, crcStart, ETH200CRCMask);
    crcPacket = crcPacket << 8 | destuffedBuf[PAYLOADLEN - 2]; // we are shifting the next to last Byte int the two Byte CRC
    crcPacket = crcPacket << 8 | destuffedBuf[PAYLOADLEN - 1]; // we are shifting the last Byte int the two Byte CRC
    #ifdef MXDEBUG
      Serial.print(F("Packet CRC: "));
      printHexWithZeroPad(Serial, crcPacket);
      Serial.print(F(", Calculated CRC: "));
      printHexWithZeroPad(Serial, crcCalculated);
      Serial.println();
    #endif //MXDEBUG

    // 6. step: check that CRC matches packet
    if (crcPacket != crcCalculated) {
      // reset, packet isn't for us
      // we are more verbose because the length of the packet matches already
      // so if the CRC is wrong, maybe we have a bug?
      MXDEBUG_PRINTLLN(F("Packet CRC does not match, discarding."));
      #ifdef MXDEBUG
        MXDEBUG_PRINTLLN(F("packet(proper length, reversed, destuffed, manchester decoded):"));
        for (uint8_t i = 0; i < PAYLOADLEN; i++) {
          printHexWithZeroPad(Serial, destuffedBuf[i]);
          Serial.print(F(" "));
        }
        Serial.println();
        Serial.print(F("Packet CRC: "));
        printHexWithZeroPad(Serial, crcPacket);
        Serial.print(F(", Calculated CRC: "));
        printHexWithZeroPad(Serial, crcCalculated);
        Serial.println();
      #endif //MXDEBUG
      PAYLOADLEN = 0;
      unselect();
      receiveBegin();
      return;
    }

    // packet is correct, fill the DATA array
    DATALEN = PAYLOADLEN;
    for (uint8_t i = 0; i < DATALEN; i++) {
      // just copy it one byte at a time
      DATA[i] = destuffedBuf[i];
    }

    DATA[DATALEN] = 0; // add null at end of string // add null at end of string
    unselect();
    setMode(RF69_MODE_RX);
  }
  RSSI = readRSSI();
}

void ETH200RFM69::readAllRegs() {
  uint8_t regVal;
  
  Serial.println(F("Address - HEX - BIN"));
  for (uint8_t regAddr = 1; regAddr <= 0x4F; regAddr++) {
    select();
    _spi->transfer(regAddr & 0x7F); // send address + r/w bit
    regVal = _spi->transfer(0);
    unselect();

    printHexWithZeroPad(Serial, regAddr);
    Serial.print(F(" - "));
    printHexWithZeroPad(Serial, regVal);
    Serial.print(F(" - "));
    printBinWithZeroPad(Serial, regVal);
    Serial.println(F(""));
  }
}

/*
  CRC Berechnung für ETH comfort 200
    Authors: Dietmar Weisser, Michael Walischewski (michel72)
    https://www.mikrocontroller.net/topic/172034

  Reverse CRC
  uint16_t c      - two bytes, CRC will be calculated and added to crc
  uint16_t crc    - previous crc value
  uint16_t mask   - 
*/
uint16_t ETH200RFM69::calcCRC16r( uint16_t c, uint16_t crc, uint16_t mask) {
  uint8_t i;
  for (i = 0; i < 8; i++) {
    if ((crc ^ c) & 1) {
      crc = (crc >> 1) ^ mask;
    } else {
      crc >>= 1;
    }
    c >>= 1;
  };
  return(crc);
}

/* internal function
  This function removes the stuffed "0" between 5x "1" and 6x "1". That means
  "|01111101|01..." will become "|01111110|1..." for the whole bit string.

  packet[]       - pointer to the packet[] array which contains the data
  length         - number of bytes the CRC should be calculated over
  crcStart       - CRC start value
  crcMask        - CRC Mask
  uint16_t       - return value is the two byte CRC
*/
uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t packet[], uint8_t length, uint16_t crcStart, uint16_t crcMask) {
  uint16_t crcResult = 0;
  uint16_t crcCalculated = crcStart;
  #ifdef MXDEBUG
    MXDEBUG_PRINTLLN(F("CRC Calc start"));
    Serial.print(F("length: "));
    Serial.print(length);
    Serial.print(F(", crcCalculated: "));
    printHexWithZeroPad(Serial, crcCalculated);
    Serial.print(F(", crcMask: "));
    printHexWithZeroPad(Serial, crcMask);
    Serial.println();
  #endif //MXDEBUG

  // in the CRC calculation the sync word needs to be included, since it's not
  // included in our packet[] payload, do it hard coded as the first calculation step
  crcCalculated = calcCRC16r(0x7E, crcCalculated, crcMask);
  #ifdef MXDEBUG
    Serial.print(F("sync word iteration: -1"));
    Serial.print(F(", crcCalculated: "));
    printHexWithZeroPad(Serial, crcCalculated);
    Serial.println();
  #endif //MXDEBUG

  for (uint8_t i = 0; i < length; i++) {
    crcCalculated = calcCRC16r(packet[i], crcCalculated, crcMask);
    #ifdef MXDEBUG
      Serial.print(F("iteration: "));
      Serial.print(i);
      Serial.print(F(", crcCalculated: "));
      printHexWithZeroPad(Serial, crcCalculated);
      Serial.println();
    #endif //MXDEBUG
  }
  // at the end we need to swap the CRC bytes
  uint8_t hiByte = (crcCalculated & 0xFF00) >> 8;
  uint8_t loByte = (crcCalculated & 0x00FF);
  crcResult = loByte << 8 | hiByte; // shift the hiByte left into the result
  #ifdef MXDEBUG
    Serial.print(F("Swapped CRC: "));
    printHexWithZeroPad(Serial, crcResult);
    Serial.println();
  #endif //MXDEBUG

  return crcResult;
}

// reverses the bit order inside a byte
uint8_t ETH200RFM69::reverseByte(uint8_t b) {
  uint8_t result = 0;

	for (uint8_t i = 0; i<8; i++) {
		if (b & (1 << i)) {
			result = result << 1 | 1;
		}
		else {
			result = result << 1;
		}
	}
  return result;
}

// Makes some sanity check and waits for radio module readiness to send a package.
// compared to RFM69::send we just need the buffer and the buffer size for sending
// numStuffedBits - Number of Bits which were stuffed into the last byte, we need that
//                  to squeeze the following sync word and data directly after those bits 
boolean ETH200RFM69::send(uint8_t buffer[], uint8_t bufferSize, uint8_t numStuffedBits) {
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  uint32_t now = millis();
  while (!canSend() && millis() - now < RF69_CSMA_LIMIT_MS) {
    receiveDone();
  }
  MXDEBUG_PRINTLLN(F("Radio is ready to send data, sending the frame."));
  MXDEBUG_PRINT(F("Sending it "));
  MXDEBUG_PRINT(CFG_ETH200NUMPACKETSENDREPEATS);
  MXDEBUG_PRINTLN(F(" times."));
  
  yield();
  MXTIME_PRINT(F("Timer just before sending."));
  sendFrame(buffer, bufferSize, numStuffedBits);
  MXTIME_PRINT(F("Timer just after sending."));

  // we sent something update the last packet variables
  // copy the packet into the lastSentPacket array
  for (uint8_t i = 0; i < bufferSize; i++) {
    lastSentPacket[i] = buffer[i];
    #ifdef MXDEBUG
      printHexWithZeroPad(Serial, buffer[i]);
      Serial.print(F(" "));
    #endif //MXDEBUG
  }
  lastSentPacketSize = bufferSize;
  
  MXDEBUG_PRINTLN(F(""));
  MXDEBUG_PRINTLLN("Finished sending all packets.");

  return true;
}

/* internal function
 actually takes the buffer and pushes it into the radio module for sending.
 compare to RFM69::sendFrame we need the buffer, buffer size and number of stuffed bits for sending
 numStuffedBits - Number of Bits which were stuffed into the last byte, we need that
                  to squeeze the following sync word and data directly after those bits
*/
void ETH200RFM69::sendFrame(uint8_t buffer[], uint8_t bufferSize, uint8_t numStuffedBits) {
  setMode(RF69_MODE_STANDBY); // turn off receiver to prevent reception while filling fifo
  while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) {
    // wait for ModeReady
  }
  MXDEBUG_PRINTLLN(F("RFM69 signaled STANDBY ModeReady."))

  /*
    We need to change the RFM69 fixed packet length to the length of the payload
    we are actually sending, because after that amount of bytes in the FIFO, which are
    sent out, the RFM69 module automatically sends out the sync word again and then
    continuous with more bytes which are pushed into the FIFO.
  */
  MXDEBUG_PRINTLLN(F("Disabling RFM69 fixed packet payload length, because we are handling sync words ourselves."));
  MXDEBUG_PRINT(F("New packet length: "));
  MXDEBUG_PRINTLN(0xFF);
  writeReg(REG_PAYLOADLENGTH, 0xFF);

  // set fifo threshold to be able to at least get one more (sync word + packet) pushed into it
  uint8_t fifoThreshold = RF69_MAX_DATA_LEN - (1 + bufferSize) + 5; // +5 byte spare space, you never know ;-)
  MXDEBUG_PRINT(F("New fifoThreshold: "));
  MXDEBUG_PRINTLN(fifoThreshold);
  writeReg(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY |
                           fifoThreshold);

  /*
    When the buffer was stuffed with additional bits it means that the last byte is not "full"
    only numStuffedBits from the "left/bit 7..." include valid data. But the thermostats expect
    the sync word following directly those bits, so we need to shift the complete payload.
    The first packet starts with a proper 0x7E sync word, but its last byte is squeeze of the
    stuffed Bits and the next 0x7E
  */
  // we need the sync word ate the beginning of every payload so create a new packet array
  // consisting of sync word followed by the actual payload
  uint8_t packet[bufferSize + 1];
  packet[0] = 0x7E;
  for (uint8_t i = 0; i < bufferSize; i++) {
    packet[i + 1] = buffer[i];
  }

  uint8_t numBitsInPacket = 0;
  if (numStuffedBits == 0) {
    // sync word + number of full bytes
    numBitsInPacket = 8 + (8 * bufferSize);
  } else {
    // sync word + number of full bytes + carry over stuffed bits
    numBitsInPacket = 8 + (8 * (bufferSize - 1)) + numStuffedBits;
  }

  uint8_t byteForFIFO = 0;           // the byte we are going to push into the FIFO
  uint8_t byteForFIFOBitCounter = 0; // number of used bits inside byteForFIFO

  //MXDEBUG_PRINTLN(F("Will print for every 10 frames a . during sending."));
  // any debugging output inside this loop is problematic since the FIFO must not run
  // empty and serial output takes a lot of time.
  for (uint16_t i = 0; i < CFG_ETH200NUMPACKETSENDREPEATS; i++) {
    /*
    if (i % 10 == 0) {
      MXDEBUG_PRINT(F("."));
      MXDEBUG_PRINT(F("."));
      yield(); //after sending a couple of packages we need to give the ESP some time to do other things
               //if we don't do this, the watchdog will reset the ESP because it thinks it has crashed.
    }
    */
    if ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOLEVEL) == 0x00) {
      // fifoLevel bit is "Set when the number of bytes in the FIFO strictly exceeds FifoThreshold, else cleared.""
      // so only write to fifo if it has not yet exceeded the threshold level because we know at least one full
      // packet + sync word still fits into it.

      // write to FIFO
      select();  
      _spi->transfer(REG_FIFO | 0x80);
      if (i == 0) {
        /*
          ETH200 expects 4 byte preamble (manchester encoded) and I think it should be 0x55 55 AA AA
          (what I'm seeing from the Window Sensor is 0x15 55 AA AA)
          but we cannot send that using RFM69 integrated functions, you can only
          specify the number of 0xAA it will send. Need to do it ugly/manually
          non manchester encoded it will be 0x00 FF

          the very first "packet" we are sending we are prefixing with our custom preamble
          there will be some garbage + exactly one sync word before our custom preamble
        */  
        //MXDEBUG_PRINTLLN(F("Pushing preamble into FIFO:"));
        _spi->transfer(0x00);
        _spi->transfer(0xFF);
      }

      //MXDEBUG_PRINTLLN(F("Pushing packet into FIFO:"));
      //MXDEBUG_PRINT(F("Bytes: "));
      for (uint8_t curBitPos = 0; curBitPos < numBitsInPacket; curBitPos++) {
        /*
          need to construct the byte first before we can push it into the FIFO

          All this construction is needed because a stuffed payload/bit stream does not
          end on a byte boundary and the thermostats expects payload bits are followed
          directly by sync word bits.
        */
        /*
        current position of our cursor inside the packet
        curBitPos = 0, packet[0], bit 7,  pPos = curBitPos / 8, bPos = 7 - (curBitPos % 8)
        curBitPos = 1, packet[0], bit 6
        curBitPos = 2, packet[0], bit 5
        ...
        curBitPos = 7, packet[0], bit 0
        curBitPos = 8, packet[1], bit 7
        curBitPos = 9, packet[1], bit 6
        ...
        curBitPos = numBitsInPacket - 1, packet[buffersize - 1], bit X
        */            
        uint8_t curBit = bitRead(packet[curBitPos / 8], 7 - (curBitPos % 8)); // current Bit at curBitPos in packet
        if (curBit == 0) {
          byteForFIFO = byteForFIFO << 1 | 0;
        } else if (curBit == 1) {
          byteForFIFO = byteForFIFO << 1 | 1;
        }
        byteForFIFOBitCounter++;  // we have added one bit

        if (byteForFIFOBitCounter == 8) {
          // we have one full byte, push it into the FIFO
          /*
          #ifdef MXDEBUG
            printBinWithZeroPad(Serial, byteForFIFO);
            Serial.print(F(" "));
          #endif //MXDEBUG
          */
          _spi->transfer(byteForFIFO);
          byteForFIFOBitCounter = 0;
        } // else keep filling the byteForFIFO until it's full
      }
      // when we reach this position and the byteForFIFO is not full yet, we'll
      // use it for the next packet loop. And the last packets last byte we just ignore.
      //MXDEBUG_PRINTLN(F(""));
      unselect();
    } else {
      if (_mode != RF69_MODE_TX) {
        // wait until FIFO is prefilled before enabling transmit.
        MXDEBUG_PRINTLLN(F("Fifo threshold reached, TX not enabled yet."));
        MXDEBUG_PRINTLLN(F("Starting RFM69 TX."));
        setMode(RF69_MODE_TX);
      }
      //MXDEBUG_PRINTLN(F("Fifo threshold reached, wait."));
      // since we ran through the loop without sending a packet, reduce the loop counter again.
      i--;

      // if the fifo is full, give the microcontroller a chance to do other stuff. need to see
      // if that's a good idea
      yield();
    }
    yield(); 
  }

  while ((readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0x00) {
    // wait for last PacketSent
    yield(); 
  }
  MXDEBUG_PRINTLLN(F("Last packet sent, going into STANDBY mode."));
  setMode(RF69_MODE_STANDBY);
  MXDEBUG_PRINTLLN(F("Resetting RFM69 fixed packet payload length to default value."));
  MXDEBUG_PRINTL(F("New packet length: "));
  MXDEBUG_PRINTLN(PAYLOADETH200);
  writeReg(REG_PAYLOADLENGTH, PAYLOADETH200);
}


// sends a package
boolean ETH200RFM69::sendPacket(uint8_t deviceType, uint32_t address, uint8_t cmd, uint8_t cmds[], uint8_t cmdsSize) {
  uint16_t crcStart = 0; // crc start value, depends on device type
  // 0. step is construct the raw packet
  //    without the sync word, that's added by the RFM69 module automatically
  uint8_t payloadLength = 0;
  if (deviceType == deviceType_t::RemoteControl) {
    // Remote Control
    payloadLength = 9;
    crcStart = ETH200CRCStartRemoteControl;
  } else if (deviceType == deviceType_t::WindowSensor) {
    // Window Sensor
    payloadLength = 8;
    crcStart = ETH200CRCStartWindowSensor;
  } else {
    MXINFO_PRINTLLN(F("Was instructed to send package for unknown device type, aborting"));
    return false;
  }
  uint8_t payload[payloadLength];
  for (uint8_t i = 0; i < payloadLength; i++) {
    // init payload
    payload[i] = 0;
  }
  payload[0] = currentPacketCounter;
  payload[1] = deviceType;
  // we are shifting the Byte we want to the right most position, that will be cast/assigned
  // to the variable
  payload[2] = address >> 16; // byte #2 from the address, byte #1 is empty
  payload[3] = address >> 8;  // byte #3 from the address
  payload[4] = address;       // byte #4 from the address
  payload[5] = cmd;
  uint8_t payloadPos = 6;
  for (uint8_t i = 0; i < cmdsSize; i++) {
    // if cmdsSize > 0 
    payload[payloadPos] = cmds[i];
    payloadPos++;
  }
  // 1. step, calculate CRC of the payload and add it to the packet
  uint16_t crcCalculated = 0;
  crcCalculated = ETH200RFM69::calcPacketCRC16r(payload, payloadPos, crcStart, ETH200CRCMask);
  payload[payloadPos] = crcCalculated >> 8;   // byte #1 from CRC
  payloadPos++;
  payload[payloadPos] = crcCalculated;        // byte #2 from CRC
  payloadPos++;
  #ifdef MXDEBUG
    Serial.print(F("Packet CRC: "));
    printHexWithZeroPad(Serial, crcCalculated);
    Serial.println();
  #endif //MXDEBUG

  MXDEBUG_PRINTLLN(F("Packet constructed, including CRC:"));
  // copy the packet into the lastSentPacket array
  for (uint8_t i = 0; i < payloadPos; i++) {
    lastSentPacket[i] = payload[i];
    #ifdef MXDEBUG
      printHexWithZeroPad(Serial, payload[i]);
      Serial.print(F(" "));
    #endif //MXDEBUG
  }
  MXDEBUG_PRINTLN(F(""));
  lastSentPacketSize = payloadPos;

  // 2. step: reverse the byte order
  MXDEBUG_PRINTLLN(F("packet(crc calculated, reversed byte order):"));
  uint8_t reversedPayload[payloadLength];
  for (uint8_t i = 0; i < payloadLength; i++) {
    // init reversedPayload
    reversedPayload[i] = 0;
  }
  for (uint8_t i= 0; i < payloadLength; i++) {
    reversedPayload[i] = reverseByte(payload[i]);
    #ifdef MXDEBUG
      printHexWithZeroPad(Serial, reversedPayload[i]);
      Serial.print(F(" "));
    #endif //MXDEBUG
  }
  MXDEBUG_PRINTLN(F(""));

  // 3. step: stuffing the bit string with additional 0 Bits if needed
  #ifdef MXDEBUG
    MXDEBUG_PRINTLLN(F("packet with bit details(crc calculated, reversed byte order):"));
    for (uint8_t i = 0; i < payloadLength; i++) {
      printHexWithZeroPad(Serial, reversedPayload[i]);
      //Serial.print(F(" "));
      Serial.print(F("       "));
    }
    Serial.println();
    for (uint8_t i = 0; i < payloadLength; i++) {
      printBinWithZeroPad(Serial, reversedPayload[i]);
      Serial.print(F(" "));
    }
    Serial.println();
  #endif //MXDEBUG

  uint8_t stuffedPayloadLength = payloadLength + 1; // we are always one byte longer than the original array
                                                    // because we may need to add some overflow bits
  uint8_t stuffedPayload[stuffedPayloadLength];
  uint8_t wasStuffed = 0;
  wasStuffed = stuffPayload(reversedPayload, stuffedPayload, payloadLength);

  if (wasStuffed == 0) {
    // if the payload wasn't stuffed the last byte contains no information and
    // we can ignore it
    stuffedPayloadLength--;
  }

  #ifdef MXDEBUG
    MXDEBUG_PRINTLLN(F("packet(crc calculated, reversed byte order, zero stuffed):"));
    if (wasStuffed > 0) {
      Serial.print("packet was stuffed: ");
      Serial.print(wasStuffed);
      Serial.println(" times");
    } else {
      Serial.println("packet was not stuffed");
    }
    for (uint8_t i = 0; i < stuffedPayloadLength; i++) {
      printHexWithZeroPad(Serial, stuffedPayload[i]);
      Serial.print(F("       "));
    }
    Serial.println();
    for (uint8_t i = 0; i < stuffedPayloadLength; i++) {
      printBinWithZeroPad(Serial, stuffedPayload[i]);
      Serial.print(F(" "));
    }
    Serial.println();
  #endif //MXDEBUG

  // packet (stuffedPayload) is prepared for sending
  // manchester encoding and sync word prefix will be added by RFM69
  yield(); // before we starting the transmit give the microcontroller time to do other stuff
  MXDEBUG_PRINTLLN(F("Handing packet over to RFM69 module."));
  MXTIME_PRINT(F("Start sending packets."));
  send(stuffedPayload, stuffedPayloadLength, wasStuffed);
  MXTIME_PRINT(F("Finished sending packets."));
  // we sent a packet so we need to increase the counter for the next packet
  if (currentPacketCounter < 255) {
    currentPacketCounter++;
  } else {
    // rollover
    currentPacketCounter = 1;
  }
  // if we reach this point the packet was sent successfully
  return true;
}