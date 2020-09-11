#ifndef RA_CONNECTION_H
#define RA_CONNECTION_H

#if defined(ESP8266)

    #include <ESP8266HTTPClient.h>
    #include <ESP8266WiFi.h>

#elif defined(ESP32)

    #include <WiFi.h>
    #include <HTTPClient.h>

#endif

#include "../../ConfigData.h"
#include "raLogger.h"

#include "../aes-sha/sha256.h"

#define RACONN_MAX_DATA 224
#define RACONN_MSG_LEN 512


class raConnection 
{
  public:
    raConnection( ConfigData * cd, raLogger* logger );
    int send( unsigned char * dataKOdeslani, int dataLen );
    int sendBlob( unsigned char * blob, int blobSize, int startTime, char * desc, char * extension );
    bool isConnected();
    
  private:
    void login();
    void createLoginToken( BYTE *target, long serverTime );
    void createDataPayload( BYTE *target, BYTE *source, int sourceLen );
    long readServerTime();
    
    raLogger* logger;
    ConfigData * cd;
    bool connected;
    char session[32];
    BYTE sessionKey[32];
    Sha256* sha256;
    BYTE msg[RACONN_MSG_LEN];

    char * identity = (char*)"CONN";    
};

#endif

