### Building/Flashing Preparation
$ pip3 install esptool

# Detect Flash size
$ esptool.py --port /dev/ttyUSB0 flash_id | grep "flash size"
Detected flash size: 4MB

### Uploading/Flashing firmware
$ esptool.py --port /dev/ttyUSB0 --baud 460800 erase_flash
$ esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash -fs 4MB -fm dout 0x0 workspace/firmware.bin 


### MQTT structure
MXETHControl/<MAC>/
  set/reset                      # publish "1" restarts the device
  set/update                     # publish "1" then the device checks for OTA update
                                 # if set to "" then no OTA update is tried
  set/ping                       # publish "1", device should respond with a pong "1"
  set/pong
  status/hardware                # name of hardware, e.g. "MXETHControl"
  status/ip                      # current IP address of this device
  status/mac                     # mac address of ESP8266
  status/online                  # 0 or 1, is the LWT/last will and testament topic
  status/version                 # version of code
  status/state                   # state can be: "initialized", "listening", "receiving", "sending", "restarting", "checkingOTA"
  FriendlyName                   # retain? Will be manually set via external MQTT command
MXETHControl/<MAC>/thermostat/<ThermostatID>/
  get/id                         # ID used to control thermostat, needs to be set to random desired ID before cmd = Learn
                                 # basically the MXEthControl uses this ID, 3 byte, to simulate a remote control for each thermostat
                                 # Currently 0x"010101", 0x"020202", 0x"030303" ... 0x"070707" are enabled (CFG_ETH200NUMTHERMOSTATS).
                                 #
                                 # There are also groups of thermostat, they are exactly handled the same but start with 0x"99...."
                                 # just so that they are listed after the "normal" thermostats. (CFG_ETH200NUMGROUPS)
                                 # Group topics are intended to reduce the used duty cycle, which is 1% in the 868 MHz band.
                                 # 1% of 1 hour = 36 seconds.
                                 # Sending either one command to 6 thermostats, results in ~36 seconds sending, or sending one command
                                 # to one group topic, results in ~6 seconds sending, makes a huge difference.
                                 # All thermostats need to listen to that group topic as well,
                                 # So all my thermostats "listen" to:
                                 #   their individual IDs, 0x"010101" ... 0x"070707"
                                 #   an "all" ID, 0x"990101"
                                 #   Wohnzimmer thermostats listen to 0x"990202"
                                 # Thermostats can listen to up to 4 "senders"
                                 #
  get/raw                        # When sending a package to a thermostat MXETHControl publishes the raw package (without sync
                                 # word) to this topic before sending. Mostly used for debugging
  set/cmd                        # "Learn" - Sends a Learn package, the thermostat need to be in the 30 second learning mode to receive it
                                 # "WindowOpened", "WindowClosed"
                                 # "DayMode", "NightMode"
                                 # "<absolute temperature>" -  5.0 - 29.5 in 0.5 steps
                                 # "<relative temperature>" - -9.5 - +9.5 in 0.5 steps, if a prefix - or + is added we are treating
                                 #                                               the temperature as a relative decrease/increase
                                 #                                               (like done by the remote control)
  FriendlyName                   # retain? Will be manually set via external MQTT command
MXETHControl/<MAC>/sensor/<SensorID>/
  get/id                         # ID of sensor
  get/type                       # Can be "WindowSensor", "RemoteControl"
  get/battery                    # battery status, can be "ok" or "low"
  get/cmd                        # window sensor : "WindowOpened", "WindowClosed"
                                 # remote control: "DayMode", "NightMode" or "<temperature offset>"
                                 # or an unknown "<raw value>"
  get/raw                        # raw message
  get/rssi                       # Received Signal Strength Indication
  FriendlyName                   # retain? Will be manually set via external MQTT command

### sample MQTT messages of ELV RemoteControl and WindowSensor
12:30:01.272103 MXETHControl/BCDDC2247927/sensor/003190/get/id 003190
12:30:01.327492 MXETHControl/BCDDC2247927/sensor/003190/get/type RemoteControl
12:30:01.333897 MXETHControl/BCDDC2247927/sensor/003190/get/cmd DayMode
12:30:01.338577 MXETHControl/BCDDC2247927/sensor/003190/get/raw 17 10 00 31 90 42 00 F7 10
12:30:01.342767 MXETHControl/BCDDC2247927/sensor/003190/get/rssi -59
12:30:01.347624 MXETHControl/BCDDC2247927/sensor/003190/get {"id":"003190","type":"RemoteControl","battery":"","cmd":"DayMode","raw":"17 10 00 31 90 42 00 F7 10","rssi":-59}

12:32:21.973208 MXETHControl/BCDDC2247927/sensor/003190/get/id 003190
12:32:22.035330 MXETHControl/BCDDC2247927/sensor/003190/get/type RemoteControl
12:32:22.041602 MXETHControl/BCDDC2247927/sensor/003190/get/cmd +1.5
12:32:22.045872 MXETHControl/BCDDC2247927/sensor/003190/get/raw 18 10 00 31 90 40 03 28 08
12:32:22.050796 MXETHControl/BCDDC2247927/sensor/003190/get/rssi -67
12:32:22.054207 MXETHControl/BCDDC2247927/sensor/003190/get {"id":"003190","type":"RemoteControl","battery":"","cmd":"+1.5","raw":"18 10 00 31 90 40 03 28 08","rssi":-67}

12:31:25.855687 MXETHControl/BCDDC2247927/sensor/014F5E/get/id 014F5E
12:31:25.904764 MXETHControl/BCDDC2247927/sensor/014F5E/get/type WindowSensor
12:31:25.912406 MXETHControl/BCDDC2247927/sensor/014F5E/get/battery ok
12:31:25.917048 MXETHControl/BCDDC2247927/sensor/014F5E/get/cmd WindowOpened
12:31:25.921675 MXETHControl/BCDDC2247927/sensor/014F5E/get/raw 15 20 01 4F 5E 41 49 8B
12:31:25.926211 MXETHControl/BCDDC2247927/sensor/014F5E/get/rssi -52
12:31:25.929760 MXETHControl/BCDDC2247927/sensor/014F5E/get {"id":"014F5E","type":"WindowSensor","battery":"ok","cmd":"WindowOpened","raw":"15 20 01 4F 5E 41 49 8B","rssi":-52}

### sample MQTT messages when using MXETHControl
12:33:55.837888 MXETHControl/BCDDC2247927/thermostat/050505/set/cmd WindowOpened
12:34:01.490440 MXETHControl/BCDDC2247927/thermostat/050505/get/raw 02 20 05 05 05 41 97 E5

12:34:19.205300 MXETHControl/BCDDC2247927/thermostat/050505/set/cmd WindowClosed
12:34:24.941206 MXETHControl/BCDDC2247927/thermostat/050505/get/raw 03 20 05 05 05 40 35 F0

12:34:35.337779 MXETHControl/BCDDC2247927/thermostat/050505/set/cmd +1.5
12:34:41.383882 MXETHControl/BCDDC2247927/thermostat/050505/get/raw 04 10 05 05 05 40 03 F5 A4

12:34:47.816562 MXETHControl/BCDDC2247927/thermostat/050505/set/cmd 17.0
12:35:00.706884 MXETHControl/BCDDC2247927/thermostat/050505/get/raw 06 10 05 05 05 40 18 1C 3D

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
# not connected:
#                              DIO1/DCLK 11        # In continuous mode clock signal of data
#                              DIO2/DATA 10        # In continuous mode data signal



### raw samples
# explanation source: https://www.mikrocontroller.net/topic/172034

0x7e zz ll aa aa aa cmd0 cmd1...cmdn crc crc
  |  |  |  |        |    |            \ crc 2 byte - 16 bit checksum
  |  |  |  |        |    \ cmd1...cmdn, depends on device type
  |  |  |  |        \ cmd0 1 byte - command
  |  |  |  \ aa 3 byte - address
  |  |  \ ll 1 byte - device type
  |  |                0x10 = remote control,
  |  |                0x20 = window sensor, 
  |  |                0x30 = wall thermostat, 
  |  |                0x31 - 0x33 USB-program stick 
  |  |                  (0x33 = learn, 0x32 = time sync, 0x33 = week program)
  |  \ zz 1 byte - is a counter that differentiates communication streams
  \ 7e - sync word

### a raw packet stream, still manchester decoded starts with and looks like this
0x 55 55 AA AA 6A A9 ...packet... 6A A9 ...packet... 6A A9 ...packet... 6A A9 ...
   |           |     |            \ the next sync word directly after the last bit of the CRC of the previous packet, without a new preamble
   |           |     |              when the bits were shifted because of bit stuffing the next sync word starts immediately after
   |           |     |              the last used data bit, so not on the next Byte boundary.
   |           |     \ e.g. WindowSensor 128 bits packet payload. (if it wasn't stuffed with additional 0)      
   |           \ 16 bits sync word
   \ 32 bits preamble, where the first nibble is usally some kind of garbage

### a raw packet stream, already manchester decoded
# this packet has no stuffed 0 in it
# the bytes are still in transmitted order and not reversed
0x  0 ff 7e 50 04 80 f2 7a 82 0e 2f 7e500480f27a820e2f 7e500480f27a820e2f 7e50...
   |     |  |                       \ the next sync word directly after the CRC of the previous packet, without a new preamble
   |     |  \ e.g. WindowSensor 64 bits packet payload. (if it wasn't stuffed with additional 0)      
   |     \ 8 bits sync word
   \ 16 bits preamble, in this case the first nibble had only 3 bits and needs to be deleted, otherwise all other bits are shifted wrongly

## window sensor, cmd0, 8 byte payload
packets are sent 168 times for window sensor
40 (window close)
41 (window open) 
C0 (window close, battery low, <= 2.2V)
C1 (window open, battery low, <= 2.2V)

##remote control cmds, cmd0, 9 byte payload
packets are sent 151 times for remote control
42 (day mode), cmd1= 0x0
43 (night mode), cmd1 = 0x0
40 (temperature setting), cmd1:
  FF  -0.5
  FE  -1.0
  FC  -1.5
  01  +0.5
  02  +1.0
  03  +1.5

### bit stuffing tests
# source: https://github.com/nospam2000/urh-ETH_Comfort_decode_plugin
# the results are also manchester encoded, but that doesn't matter
#1, just the initial sync word
0x 7   E  
0b 01111110
$ ./urh-ETH_Comfort_decode_plugin.py e 01111110
01101010 10101001

#2, no stuffing needed
0x 7   E    1   0
0b 01111110 00010000
$ ./urh-ETH_Comfort_decode_plugin.py e 0111111000010000
01101010 10101001 01010101 10010101

#3, stuffing needed
0x 7   E    F   C
0b 01111110 11111100
$ ./urh-ETH_Comfort_decode_plugin.py e 0111111011111100
01101010 10101001 01011010 10101001 10

$ ./urh-ETH_Comfort_decode_plugin.py e 0111111011111000
01101010 10101001 01010110 10101010 01
