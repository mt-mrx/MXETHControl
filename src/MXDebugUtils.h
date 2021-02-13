/****************************************************************************
MXDebugUtils.h - Simple debugging utilities.
 
Copyright 2020 mt-mrx <64284703+mt-mrx@users.noreply.github.com>

Based on ideas taken from:
  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1271517197

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

#ifndef MXDEBUGUTILS_H
  #define MXDEBUGUTILS_H
  #include <config.h>            // project settings file, need to include to use MXDEBUG

  // Debug print functions
  // and why they need to be static:
  // https://stackoverflow.com/questions/34997795/inline-function-multiple-definition
  
  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, const char* text, bool printloc) __attribute__((unused));
  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, const char* text, bool printloc) {
    if (printloc) {
      //print location information of the calling statement
      Serial.print(prefix);
      Serial.print(millis());
      Serial.print(": ");
      Serial.print(func);
      Serial.print(" ");
      Serial.print(file);
      Serial.print(":");
      Serial.print(line);
      Serial.print(" ");
    } 
    Serial.print(text);
  }

  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, int text, bool printloc) __attribute__((unused));
  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, int text, bool printloc) {
    mxDebugPrint(prefix, func, file, line, String(text).c_str(), printloc);
  }

  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, String text, bool printloc) __attribute__((unused));
  static void mxDebugPrint(const char* prefix, const char* func, const char* file,
                           int line, String text, bool printloc) {
    mxDebugPrint(prefix, func, file, line, text.c_str(), printloc);
  }

  #ifdef MXDEBUG
    #define MXDEBUG_PRINT(text) \
      mxDebugPrint("DEBUG: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, false);
      //print text if MXDEBUG enabled without newline
    #define MXDEBUG_PRINTLN(text) \
      mxDebugPrint("DEBUG: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, false); \
      Serial.println();
      //print text if MXDEBUG enabled with newline
    #define MXDEBUG_PRINTL(text) \
      mxDebugPrint("DEBUG: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, true);
      //print text if MXDEBUG enabled without newline, with call location
    #define MXDEBUG_PRINTLLN(text) \
      mxDebugPrint("DEBUG: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, true); \
      Serial.println();
      //print text if MXDEBUG enabled with newline, with call location
  #else
    // The non-debug macro means that the code is validated but never called.
    // See chapter 8 of 'The Practice of Programming', by Kernighan and Pike.
    // source: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
    #define MXDEBUG_PRINT(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXDEBUG_PRINTLN(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXDEBUG_PRINTL(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXDEBUG_PRINTLLN(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
  #endif //MXDEBUG

  #ifdef MXINFO
    #define MXINFO_PRINT(text) \
      mxDebugPrint("INFO: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, false);
      //print text if MXINFO enabled without newline
    #define MXINFO_PRINTLN(text) \
      mxDebugPrint("INFO: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, false); \
      Serial.println();
      //print text if MXINFO enabled with newline
    #define MXINFO_PRINTL(text) \
      mxDebugPrint("INFO: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, true);
      //print text if MXINFO enabled without newline, with call location
    #define MXINFO_PRINTLLN(text) \
      mxDebugPrint("INFO: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text, true); \
      Serial.println();
      //print text if MXINFO enabled with newline, with call location
  #else
    // The non-info macro means that the code is validated but never called.
    // See chapter 8 of 'The Practice of Programming', by Kernighan and Pike.
    // source: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
    #define MXINFO_PRINT(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXINFO_PRINTLN(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXINFO_PRINTL(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
    #define MXINFO_PRINTLLN(text) \
      do { if (0) mxDebugPrint("", "", "", 0, text, true); } while (0);
  #endif //MXINFO

  // timinig code execution parts
  // and why they need to be static:
  // https://stackoverflow.com/questions/34997795/inline-function-multiple-definition
  static unsigned long tsLast;  //millis() value for last MXTIME_PRINT() function call
  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, const char* text) __attribute__((unused));
  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, const char* text) {
    unsigned long tsCurrent = millis();
    unsigned long tsDelta = tsCurrent - tsLast;
    Serial.print(prefix);
    Serial.print(func);
    Serial.print(' ');
    Serial.print(file);
    Serial.print(':');
    Serial.print(line);
    Serial.print(F(" Time Since Start: "));
    Serial.print(tsCurrent);
    Serial.print(F("ms, Time Since Last MXTIME_PRINT(): "));
    Serial.print(tsDelta);
    Serial.println(F("ms "));
    Serial.println(text);
    //setting last call of this function
    tsLast = millis();
  }

  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, int text) __attribute__((unused));
  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, int text) {
    mxDebugTime(prefix, func, file, line, String(text).c_str());
  }

  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, String text) __attribute__((unused));
  static void mxDebugTime(const char* prefix, const char* func, const char* file,
                          int line, String text) {
    mxDebugTime(prefix, func, file, line, text.c_str());
  }

  #ifdef MXDEBUG_TIME
    #define MXTIME_PRINT(text) \
      mxDebugTime("TIME: ", __PRETTY_FUNCTION__, __FILE__, __LINE__, text);
  #else
    // The non-time macro means that the code is validated but never called.
    // See chapter 8 of 'The Practice of Programming', by Kernighan and Pike.
    // source: https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
    #define MXTIME_PRINT(text) \
      do { if (0) mxDebugTime("", "", "", 0, text); } while (0)
  #endif //MXDEBUG_TIME

  /*
    Printing binary and hex values with leading zeros
    Author: septillion
    Source: https://forum.arduino.cc/index.php?topic=464778.0
    and why they need to be static:
    https://stackoverflow.com/questions/34997795/inline-function-multiple-definition
  */
  static void printBinWithZeroPad(Print &out, unsigned long n, byte size) {
    for (byte i = size - 1; ;i--) {
      out.print(bitRead(n, i));

      // need to break at exactly 0 and cannot use condition like i>=0 because
      // if you reach i=0 and substract 1 the result is then 255 and the
      // condition i>=0 would be true again
      if(i == 0) {
        break;
      }
    }
  }
  static inline void printBinWithZeroPad(Print &out, unsigned long n) { printBinWithZeroPad(out, n, sizeof(n) * 8); }
  static inline void printBinWithZeroPad(Print &out, unsigned int n) { printBinWithZeroPad(out, n, sizeof(n) * 8); }
  static inline void printBinWithZeroPad(Print &out, uint16_t n) { printBinWithZeroPad(out, n, sizeof(n) * 8); }
  static inline void printBinWithZeroPad(Print &out, uint8_t n) { printBinWithZeroPad(out, n, sizeof(n) * 8); }

  static void printHexWithZeroPad(Print &out, unsigned long n, byte size) {
    for(byte i = size - 1; ;i--) {
      out.print((n >> (i * 4)) & 0x0F, HEX);
    
      // see printBinWithZeroPad for explanation
      if(i == 0) {
        break;
      }
    }
  }
  static inline void printHexWithZeroPad(Print &out, unsigned long n) { printHexWithZeroPad(out, n, sizeof(n) * 2); }
  static inline void printHexWithZeroPad(Print &out, unsigned int n) { printHexWithZeroPad(out, n, sizeof(n) * 2); }
  static inline void printHexWithZeroPad(Print &out, uint16_t n) { printHexWithZeroPad(out, n, sizeof(n) * 2); }
  static inline void printHexWithZeroPad(Print &out, byte n) { printHexWithZeroPad(out, n, sizeof(n) * 2); }
#endif //MXDEBUGUTILS_H
