### window sensor open
DEBUG: 1635932: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
F0       04       80       F2       7A       82       EF       07       7E       F0       
11110000 00000100 10000000 11110010 01111010 10000010 11101111 00000111 01111110 11110000 
DEBUG: 1635948: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
F0       04       80       F2       7A       82       EF       07       7D       
11110000 00000100 10000000 11110010 01111010 10000010 11101111 00000111 01111101 
DEBUG: 1635975: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
0F 20 01 4F 5E 41 F7 E0 BE 
DEBUG: 1635989: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x0F
deviceType -> 0x20
DEBUG: 1636002: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 8
DEBUG: 1636013: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
0F 20 01 4F 5E 41 F7 E0 
DEBUG: 1636027: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1636036: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 6, crcCalculated: BDB7, crcMask: 8408
sync word iteration: -1, crcCalculated: 5B70
iteration: 0, crcCalculated: 8B2B
iteration: 1, crcCalculated: BE58
iteration: 2, crcCalculated: CFFA
iteration: 3, crcCalculated: E2E9
iteration: 4, crcCalculated: C1D6
iteration: 5, crcCalculated: E0F7
Swapped CRC: F7E0
Packet CRC: F7E0, Calculated CRC: F7E0
DEBUG: 1636079: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1636089: void loop() main.cpp:1083 Packet received:
7E 0F 20 01 4F 5E 41 F7 E0    [RX_RSSI:-67]


### window sensor close
DEBUG: 1557478: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
70       04       80       F2       7A       02       AA       AF       7E       70       
01110000 00000100 10000000 11110010 01111010 00000010 10101010 10101111 01111110 01110000 
DEBUG: 1557494: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
70       04       80       F2       7A       02       AA       AF       7C       
01110000 00000100 10000000 11110010 01111010 00000010 10101010 10101111 01111100 
DEBUG: 1557521: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
0E 20 01 4F 5E 40 55 F5 3E 
DEBUG: 1557536: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x0E
deviceType -> 0x20
DEBUG: 1557548: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 8
DEBUG: 1557559: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
0E 20 01 4F 5E 40 55 F5 
DEBUG: 1557573: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1557582: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 6, crcCalculated: BDB7, crcMask: 8408
sync word iteration: -1, crcCalculated: 5B70
iteration: 0, crcCalculated: 9AA2
iteration: 1, crcCalculated: A780
iteration: 2, crcCalculated: 9526
iteration: 3, crcCalculated: FE52
iteration: 4, crcCalculated: CA92
iteration: 5, crcCalculated: F555
Swapped CRC: 55F5
Packet CRC: 55F5, Calculated CRC: 55F5
DEBUG: 1557625: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1557635: void loop() main.cpp:1083 Packet received:
7E 0E 20 01 4F 5E 40 55 F5    [RX_RSSI:-67]

Listening for packets, loop (x100000): 388
Listening for packets, loop (x100000): 389
DEBUG: 1565751: boolean publishMessages() main.cpp:898 Message timer expired, sending it to MQTT
INFO: 1565751: boolean publishMessagesMQTT(message) main.cpp:805 Sending message to MQTT
Collected numPackets of the same contents before sending it: 26
INFO: 1565765: boolean publishMessagesMQTT(message) main.cpp:884 Sending json message to MQTT: 
{"id":"014F5E","type":"WindowSensor","battery":"ok","cmd":"WindowClosed","raw":"0E 20 01 4F 5E 40 55 F5","rssi":-62}


### remote control day mode
DEBUG: 1718199: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
88       08       00       8C       09       42       00       5C       12       7E       
10001000 00001000 00000000 10001100 00001001 01000010 00000000 01011100 00010010 01111110 
DEBUG: 1718215: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
88       08       00       8C       09       42       00       5C       12       
10001000 00001000 00000000 10001100 00001001 01000010 00000000 01011100 00010010 
DEBUG: 1718240: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
11 10 00 31 90 42 00 3A 48 
DEBUG: 1718254: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x11
deviceType -> 0x10
DEBUG: 1718266: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1718278: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
11 10 00 31 90 42 00 3A 48 
DEBUG: 1718292: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1718301: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: AA00
iteration: 1, crcCalculated: 102B
iteration: 2, crcCalculated: 9FC1
iteration: 3, crcCalculated: F710
iteration: 4, crcCalculated: 84FF
iteration: 5, crcCalculated: 6EEA
iteration: 6, crcCalculated: 483A
Swapped CRC: 3A48
Packet CRC: 3A48, Calculated CRC: 3A48
DEBUG: 1718347: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1718357: void loop() main.cpp:1083 Packet received:
7E 11 10 00 31 90 42 00 3A 48    [RX_RSSI:-58]


### remote control night mode
DEBUG: 1740264: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
48       08       00       8C       09       C2       00       31       9F       3F       
01001000 00001000 00000000 10001100 00001001 11000010 00000000 00110001 10011111 00111111 
DEBUG: 1740280: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
48       08       00       8C       09       C2       00       31       9F       
01001000 00001000 00000000 10001100 00001001 11000010 00000000 00110001 10011111 
DEBUG: 1740307: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
12 10 00 31 90 43 00 8C F9 
DEBUG: 1740322: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x12
deviceType -> 0x10
DEBUG: 1740334: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1740345: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
12 10 00 31 90 43 00 8C F9 
DEBUG: 1740360: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1740369: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: 989B
iteration: 1, crcCalculated: 3A43
iteration: 2, crcCalculated: 70A5
iteration: 3, crcCalculated: D2DD
iteration: 4, crcCalculated: 9933
iteration: 5, crcCalculated: 731E
iteration: 6, crcCalculated: F98C
Swapped CRC: 8CF9
Packet CRC: 8CF9, Calculated CRC: 8CF9
DEBUG: 1740414: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1740425: void loop() main.cpp:1083 Packet received:
7E 12 10 00 31 90 43 00 8C F9    [RX_RSSI:-59]


### remote control -0.5
DEBUG: 1794705: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
C8       08       00       8C       09       02       FB       C9       61       3F       
11001000 00001000 00000000 10001100 00001001 00000010 11111011 11001001 01100001 00111111 
DEBUG: 1794720: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
C8       08       00       8C       09       02       FF       92       C2       
11001000 00001000 00000000 10001100 00001001 00000010 11111111 10010010 11000010 
DEBUG: 1794748: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
13 10 00 31 90 40 FF 49 43 
DEBUG: 1794762: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x13
deviceType -> 0x10
DEBUG: 1794774: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1794785: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
13 10 00 31 90 40 FF 49 43 
DEBUG: 1794800: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1794809: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: 8912
iteration: 1, crcCalculated: 239B
iteration: 2, crcCalculated: 2A79
iteration: 3, crcCalculated: CE66
iteration: 4, crcCalculated: 9277
iteration: 5, crcCalculated: 45AE
iteration: 6, crcCalculated: 4349
Swapped CRC: 4943
Packet CRC: 4943, Calculated CRC: 4943
DEBUG: 1794855: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1794865: void loop() main.cpp:1083 Packet received:
7E 13 10 00 31 90 40 FF 49 43    [RX_RSSI:-59]


### remote control -1.0
DEBUG: 1821234: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
28       08       00       8C       09       02       7D       8D       D4       BF       
00101000 00001000 00000000 10001100 00001001 00000010 01111101 10001101 11010100 10111111 
DEBUG: 1821250: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
28       08       00       8C       09       02       7F       1B       A9       
00101000 00001000 00000000 10001100 00001001 00000010 01111111 00011011 10101001 
DEBUG: 1821277: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
14 10 00 31 90 40 FE D8 95 
DEBUG: 1821292: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x14
deviceType -> 0x10
DEBUG: 1821304: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1821315: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
14 10 00 31 90 40 FE D8 95 
DEBUG: 1821329: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1821339: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: FDAD
iteration: 1, crcCalculated: 6E93
iteration: 2, crcCalculated: A67C
iteration: 3, crcCalculated: 9947
iteration: 4, crcCalculated: A2AB
iteration: 5, crcCalculated: 597F
iteration: 6, crcCalculated: 95D8
Swapped CRC: D895
Packet CRC: D895, Calculated CRC: D895
DEBUG: 1821384: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1821395: void loop() main.cpp:1083 Packet received:
7E 14 10 00 31 90 40 FE D8 95    [RX_RSSI:-62]

Listening for packets, loop (x100000): 449
Listening for packets, loop (x100000): 450
DEBUG: 1829322: boolean publishMessages() main.cpp:898 Message timer expired, sending it to MQTT
INFO: 1829322: boolean publishMessagesMQTT(message) main.cpp:805 Sending message to MQTT
Collected numPackets of the same contents before sending it: 26
INFO: 1829336: boolean publishMessagesMQTT(message) main.cpp:884 Sending json message to MQTT: 
{"id":"003190","type":"RemoteControl","battery":"","cmd":"-1.0","raw":"14 10 00 31 90 40 FE D8 95","rssi":-58}


### remote control +0.5
DEBUG: 1860137: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
A8       08       00       8C       09       02       80       AE       A0       7E       
10101000 00001000 00000000 10001100 00001001 00000010 10000000 10101110 10100000 01111110 
DEBUG: 1860153: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
A8       08       00       8C       09       02       80       AE       A0       
10101000 00001000 00000000 10001100 00001001 00000010 10000000 10101110 10100000 
DEBUG: 1860177: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
15 10 00 31 90 40 01 75 05 
DEBUG: 1860192: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x15
deviceType -> 0x10
DEBUG: 1860204: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1860215: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
15 10 00 31 90 40 01 75 05 
DEBUG: 1860229: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1860239: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: EC24
iteration: 1, crcCalculated: 774B
iteration: 2, crcCalculated: FCA0
iteration: 3, crcCalculated: 85FC
iteration: 4, crcCalculated: A9EF
iteration: 5, crcCalculated: 5D54
iteration: 6, crcCalculated: 0575
Swapped CRC: 7505
Packet CRC: 7505, Calculated CRC: 7505
DEBUG: 1860284: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1860295: void loop() main.cpp:1083 Packet received:
7E 15 10 00 31 90 40 01 75 05    [RX_RSSI:-57]

Listening for packets, loop (x100000): 458
Listening for packets, loop (x100000): 459
DEBUG: 1868285: boolean publishMessages() main.cpp:898 Message timer expired, sending it to MQTT
INFO: 1868285: boolean publishMessagesMQTT(message) main.cpp:805 Sending message to MQTT
Collected numPackets of the same contents before sending it: 26
INFO: 1868299: boolean publishMessagesMQTT(message) main.cpp:884 Sending json message to MQTT: 
{"id":"003190","type":"RemoteControl","battery":"","cmd":"+0.5","raw":"15 10 00 31 90 40 01 75 05","rssi":-55}


### remote control +1.0
DEBUG: 1917593: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:395 raw packet(zero stuffed, manchester decoded):
68       08       00       8C       09       02       40       01       F4       BF       
01101000 00001000 00000000 10001100 00001001 00000010 01000000 00000001 11110100 10111111 
DEBUG: 1917609: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:414 packet(zero destuffed, manchester decoded):
packet was stuffed: 1 times
68       08       00       8C       09       02       40       01       F9       
01101000 00001000 00000000 10001100 00001001 00000010 01000000 00000001 11111001 
DEBUG: 1917636: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:434 packet(reversed byte order, zero destuffed, manchester decoded):
16 10 00 31 90 40 02 80 9F 
DEBUG: 1917651: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:451 packet analysis
communicationCounter -> 0x16
deviceType -> 0x10
DEBUG: 1917663: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:479 Payload length based on device type.
PAYLOADLEN -> 9
DEBUG: 1917674: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:494 packet(proper length, reversed, destuffed, manchester decoded):
16 10 00 31 90 40 02 80 9F 
DEBUG: 1917688: void ETH200RFM69::interruptHandler() ETH200RFM69.cpp:503 Calculating and comparing CRC:
DEBUG: 1917697: uint16_t ETH200RFM69::calcPacketCRC16r(uint8_t*, uint8_t, uint16_t, uint16_t) ETH200RFM69.cpp:635 CRC Calc start
length: 7, crcCalculated: C11F, crcMask: 8408
sync word iteration: -1, crcCalculated: 724E
iteration: 0, crcCalculated: DEBF
iteration: 1, crcCalculated: 5D23
iteration: 2, crcCalculated: 13C4
iteration: 3, crcCalculated: A031
iteration: 4, crcCalculated: B423
iteration: 5, crcCalculated: 5129
iteration: 6, crcCalculated: 9F80
Swapped CRC: 809F
Packet CRC: 809F, Calculated CRC: 809F
DEBUG: 1917743: boolean pushMessages(message) main.cpp:745 Message already in messages queue. Incrementing numPackets
DEBUG: 1917754: void loop() main.cpp:1083 Packet received:
7E 16 10 00 31 90 40 02 80 9F    [RX_RSSI:-60]

Listening for packets, loop (x100000): 472
Listening for packets, loop (x100000): 473
DEBUG: 1925681: boolean publishMessages() main.cpp:898 Message timer expired, sending it to MQTT
INFO: 1925681: boolean publishMessagesMQTT(message) main.cpp:805 Sending message to MQTT
Collected numPackets of the same contents before sending it: 26
INFO: 1925695: boolean publishMessagesMQTT(message) main.cpp:884 Sending json message to MQTT: 
{"id":"003190","type":"RemoteControl","battery":"","cmd":"+1.0","raw":"16 10 00 31 90 40 02 80 9F","rssi":-56}

