#include "ratatoskr.h"
#include "../platform/platform.h"


ratatoskr::ratatoskr( ConfigData * config, int storageSize, int logMode  )
{
    this->logger = new raLogger( logMode );
    this->conn = new raConnection( config, this->logger );
    this->storage = new raStorage( storageSize, this->logger );
    this->telemetry = new raTelemetryPayload( this->storage, RACONN_MAX_DATA, this->logger );
    this->lastErrTime = -2 * RA_PAUSE_AFTER_ERR;
    this->allDataPrepared = false;
    
}

int ratatoskr::defineChannel( int deviceClass, int valueType, char * deviceName, long msgRateSec )
{
    int channel;
    
    channel = this->telemetry->defineChannel( deviceClass, valueType, deviceName, msgRateSec );
    
    return channel;
}

int ratatoskr::postData( int channel, int priority, double value )
{
    return this->telemetry->send( channel, priority, value );
}

bool ratatoskr::hasData()
{
    return this->storage->hasData();
}

void ratatoskr::process()
{
    bool necoPoslano = false;
    
    if( (millis() - this->lastErrTime) < RA_PAUSE_AFTER_ERR ) {
        return;
    }
    
    this->storage->startReading();
    raStorageRecord* rec = storage->readNext();
    while( rec!=NULL ) {
        // je potreba pouzivat time(), protoze ten funguje na ESP32 v deep sleep!
        int nyni = time(NULL);
        
        unsigned char data[256];
        
        data[0] = (unsigned char)( (nyni >> 16) & 0xff );
        data[1] = (unsigned char)( (nyni >> 8) & 0xff );
        data[2] = (unsigned char)( nyni & 0xff );
        memcpy( data+3, rec->data, rec->data_length );
        
        int len = rec->data_length + 3;
        
        if( 0 == conn->send( data, len ) ) {
            rec->markAsDeleted();
            necoPoslano = true; 
        } else {
            this->logger->log( "%s not sent", this->identity );
            this->lastErrTime = millis();
            break;
        }
        rec = this->storage->readNext();
    }
    
    if( necoPoslano ) {
        this->storage->purgeDeleted();
    }
}

void ratatoskr::setAllDataPrepared()
{
    this->allDataPrepared = true;
}

void ratatoskr::clearAllDataPrepared()
{
    this->allDataPrepared = false;
}

bool ratatoskr::isAllDataPrepared()
{
    return this->allDataPrepared;
}

void ratatoskr::raDeepSleep( long usec )
{
    this->logger->log( "deep sleep for %d sec, uptime %d msec", (usec/1000000), millis() );
    ra__DeepSleep( usec );
}

int ratatoskr::sendBlob( unsigned char * blob, int blobSize, int startTime, char * desc, char * extension  )
{
    return this->conn->sendBlob( blob, blobSize, startTime, desc, extension );
}
