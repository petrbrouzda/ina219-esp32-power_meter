
#include "ConfigData.h"


// should erase all config data items
ConfigData::ConfigData()
{
    ssid[0]=0;
    pass[0]=0;

    ra_url[0]=0;
    ra_dev_name[0]=0;
    ra_pass[0]=0;

}


// should return FALSE if some items are not filled up
bool ConfigData::isValid()
{
    if( 
        ssid[0]==0 ||
        pass[0]==0 ||
        ra_url[0]==0 ||
        ra_dev_name[0]==0 ||
        ra_pass[0]==0  
    ) 
        return false;
    else
        return true;
}


void loadItem( char * itemName, int maxLen, char * target,  JsonObject& json )
{
    const char * tmp = json[itemName];
    if( tmp!=NULL ) {
       strncpy( target, tmp, maxLen );
       target[maxLen-1] = 0;
    }
}

void ConfigData::readFromJson( JsonObject& json )
{
    loadItem( (char*)"ssid", 32, ssid, json );
    loadItem( (char*)"pass", 32, pass, json );
    loadItem( (char*)"ra_url", 64, ra_url, json );
    loadItem( (char*)"ra_dev_name", 32, ra_dev_name, json );
    loadItem( (char*)"ra_pass", 32, ra_pass, json );
}

void ConfigData::saveToJson( JsonObject& json )
{
    json["ssid"] = ssid;
    json["pass"] = pass;
    json["ra_url"] = ra_url;
    json["ra_dev_name"] = ra_dev_name;
    json["ra_pass"] = ra_pass;
}
