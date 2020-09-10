
#ifndef CONFIGDATA_H
#define CONFIGDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <ArduinoJson.h>                    // https://github.com/bblanchon/ArduinoJson

class ConfigData
{
  public:

    // configuration data for wifi
    char ssid[32];
    char pass[32];

    // custom config data
    char ra_url[64];
    char ra_dev_name[32];
    char ra_pass[32];

    // feel free to add another configuration items here


    //---- services 
    ConfigData();
    bool isValid();
    void readFromJson( JsonObject& json );
    void saveToJson( JsonObject& json );
};

#endif   // CONFIGDATA_H
