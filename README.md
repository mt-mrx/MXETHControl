# MXETHControl
ELV/EQ3 ETH components to MQTT Bridge

Runs on a Wemos D1 mini/ESP8266, and uses an RFM69HCW module to capture
packages from ELV ETH 200 comfort "window sensors" and "remote controls"
and publishes them via MQTT.
It also subscribes to MQTT topics to control thermostat devices by
simulating one "remote control" per thermostat.

It also includes firmware OTA updates either checked at every boot or
triggered via MQTT.

For more details see the readme.txt file.
