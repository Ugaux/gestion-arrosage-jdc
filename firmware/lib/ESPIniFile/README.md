# ESPIniFile

ESPIniFile is an ESP8266/ESP32 library for opening and parsing '.ini' files via SPIFFS or LittleFS, with the added functionnality of understanding sub-sections.
ESPIniFile is designed to use minimal memory requirements, and the only buffer used is one supplied by the user, thus the user remains in charge of memory usage.

This is a modification of the library: [SPIFFSIniFile](https://github.com/yurilopes/SPIFFSIniFile). All credits go to Steve Marple and Yuri Lopes. All I did was adapt the source code to also support LittleFS and sub-section parsing.

Licensing: GNU LGPL v3
