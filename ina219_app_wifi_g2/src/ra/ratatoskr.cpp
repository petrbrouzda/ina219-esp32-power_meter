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

bool ratatoskr::process()
{
    bool necoPoslano = false;
    
    if( (millis() - this->lastErrTime) < RA_PAUSE_AFTER_ERR ) {
        return false;
    }
    
    this->storage->startReading();
    raStorageRecord* rec = this->storage->readNext();
    if( rec!=NULL) {

        // je potreba pouzivat time(), protoze ten funguje na ESP32 v deep sleep!
        int nyni = time(NULL);
        
        unsigned char data[RACONN_MAX_DATA+10];
        int maxLen = RACONN_MAX_DATA;
        int pos = 0;
        int len = 3;
        int recs = 0;

        data[0] = (unsigned char)( (nyni >> 16) & 0xff );
        data[1] = (unsigned char)( (nyni >> 8) & 0xff );
        data[2] = (unsigned char)( nyni & 0xff );
        pos = 3;
        
        while( rec!=NULL ) {
            if( pos + rec->data_length + 2 >= maxLen ) {
                break;
            }
            data[pos++] = rec->data_length;
            memcpy( data+pos, rec->data, rec->data_length );
            len += rec->data_length + 1;
            pos += rec->data_length;
            recs++;

            rec->markAsDeleted();
            rec = this->storage->readNext();
        }
        data[pos++] = 0;

        this->logger->log( "%s recs:%d len:%d", this->identity, recs, len );
        if( 0 == conn->send( data, len ) ) {
            necoPoslano = true; 
            this->storage->purgeDeleted();
        } else {
            this->logger->log( "%s not sent", this->identity );
            this->lastErrTime = millis();
            this->storage->undeleteAll();
        }
    }
    return necoPoslano;
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

bool ratatoskr::isConnected()
{
    return this->conn->isConnected();
}

int ratatoskr::getStorageUsage()
{
    return this->storage->getUsage();
}