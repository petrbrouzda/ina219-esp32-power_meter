#ifndef RA_CORE_H
#define RA_CORE_H

#include "raLogger.h"
#include "raConnection.h"
#include "raStorage.h"
#include "raTelemetryPayload.h"

// po chybe odesilani se nezkousi komunikovat po urcitou dobu
#define RA_PAUSE_AFTER_ERR 15000

class ratatoskr 
{
  public:
    raLogger* logger;
    raConnection* conn;
    raTelemetryPayload * telemetry;
    raStorage * storage;

    ratatoskr( ConfigData * config, int storageSize, int logMode );
    int defineChannel( int deviceClass, int valueType, char * deviceName, long msgRateSec ); 
    int postData( int channel, int priority, double value );
    int sendBlob( unsigned char * blob, int blobSize,  int startTime, char * desc, char * extension );
    void process();
    bool hasData();
    void setAllDataPrepared();
    void clearAllDataPrepared();
    bool isAllDataPrepared();
    void raDeepSleep( long usec );
    
  private:
    char * identity = (char*)"RA"; 
    long lastErrTime;
    bool allDataPrepared;   
};

#endif

