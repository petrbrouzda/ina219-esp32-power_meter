#include <Arduino.h>

#include "../aes-sha/CRC32.h"

#define CBC 1
#define ECB 0
#define CTR 0
extern "C" {
    #include "../aes-sha/aes.h"
}

#include "../platform/platform.h"
#include "../platform/Esp8266RTCUserMemory.h"

#include "raConnection.h"

// konfigurace - mozno nedefinovat
// #define DETAILED_RA_LOG


#ifdef ESP32
    RTC_DATA_ATTR long rtcIndicator;
    RTC_DATA_ATTR unsigned char rtcSessionId[32];
    RTC_DATA_ATTR unsigned char rtcSessionKey[32];    
#endif


raConnection::raConnection( ConfigData * cd, raLogger* logger  )
{
    this->logger = logger;
    this->cd = cd;
    this->connected = false;
    this->sha256=new Sha256();
    

    // pokud mame v RTC memory data o spojeni, nacteme je
    
    #ifdef ESP8266
        struct rtcData data;
        if( readRtcData( &data ) ) {
            this->logger->log( "RTC data OK" );
            memcpy( (void*)this->session, (const void *)(data.data), 32 );
            this->session[31] = 0;
            memcpy( (void*)this->sessionKey, (const void *)(data.data+32), 32 );
            this->connected = true;
        }
    #endif
    
    #ifdef ESP32
        if( rtcIndicator == 0xDEADFACE ) {
            this->logger->log( "RTC data OK" );
            memcpy( (void*)this->session, (const void *)(rtcSessionId), 32 );
            this->session[31] = 0;
            memcpy( (void*)this->sessionKey, (const void *)(rtcSessionKey), 32 );
            this->connected = true;
        }
    #endif
}




#define LOG_BUFFER_LENGTH 100
char log_buffer[LOG_BUFFER_LENGTH+LOG_BUFFER_LENGTH+1];

unsigned char btohexa_high(unsigned char b)
{
    b >>= 4;
    return (b>0x9u) ? b+87 : b+'0';     // 87 = 'a'-10
}

unsigned char btohexa_low(unsigned char b)
{
    b &= 0x0F;
    return (b>9u) ? b+87 : b+'0';   // 87 = 'a'-10
}

void btohexa( unsigned char * bytes, int inLength, char * output, int outLength )
{
    char * out = output;
    for( int i = 0; i<inLength && i<outLength; i++ )
    {
        *out = btohexa_high( bytes[i] );
        out++;
        *out = btohexa_low( bytes[i] );
        out++;
    }
    *out = 0;
}

/**
 * Vygeneruje blok nahodnych bytes; delka musi byt delitelna 4!
 */
void random_block( BYTE * target, int size )
{
    for( int i = 0; i<size;  ) {
        
        // definovano v platform/esp8266-trng.h platform/esp32-trng.h
        long l = trng();
            
        target[i++] = (unsigned char)(l & 0xff);
        target[i++] = (unsigned char)( (l>>8) & 0xff);
        target[i++] = (unsigned char)( (l>>16) & 0xff);
        target[i++] = (unsigned char)( (l>>24) & 0xff);
    }
}


/**
 * payload: 
 *      CRC32 of data (4 byte)
 *      length of data (2 byte, low endian)
 *      data
 */
void raConnection::createDataPayload( BYTE *target, BYTE *source, int sourceLen )
{
    BYTE aes_iv[AES_BLOCKLEN];
    random_block( aes_iv, AES_BLOCKLEN );
    // tohle neni debug, ale uz si konstruuji zpravu!
    btohexa( aes_iv, AES_BLOCKLEN, (char *)target, 130 );
    
    BYTE payload[RACONN_MAX_DATA+1];
    payload[4] = (sourceLen>>8) & 0xff;
    payload[5] = sourceLen & 0xff;
    memcpy( payload+6, source, sourceLen );
    
    uint32_t crc1 = CRC32::calculate((const void*) source, sourceLen);
    payload[0] = (BYTE)( (crc1>>24) & 0xff);
    payload[1] = (BYTE)( (crc1>>16) & 0xff);
    payload[2] = (BYTE)( (crc1>>8) & 0xff);
    payload[3] = (BYTE)(crc1 & 0xff);
    
    int celkovaDelka = sourceLen + 2 + 4;
    int zbyva = AES_BLOCKLEN - (celkovaDelka % AES_BLOCKLEN);
    if( zbyva>0 ) {  
        random_block( payload+celkovaDelka, zbyva );
    }
    
#ifdef DETAILED_RA_LOG
    this->logger->log( "delka %d, doplneni %d", celkovaDelka, zbyva );

    btohexa( this->sessionKey, AES_KEYLEN, log_buffer, LOG_BUFFER_LENGTH );
    this->logger->log( "session key: %s", log_buffer );
    
    this->logger->log( "IV: %s", target );
#endif        

    celkovaDelka = celkovaDelka + zbyva;

    struct AES_ctx ctx;
    
    AES_init_ctx_iv(&ctx, (unsigned char const*)this->sessionKey, (unsigned char const*)aes_iv);
    AES_CBC_encrypt_buffer(&ctx, (unsigned char *)payload, celkovaDelka );

    char * output_ptr = (char *)target + AES_BLOCKLEN+AES_BLOCKLEN;
    output_ptr[0] = ':';
    output_ptr++;
    btohexa( payload, celkovaDelka, output_ptr, RACONN_MAX_DATA+RACONN_MAX_DATA );
    
#ifdef DETAILED_RA_LOG
    //this->logger->log( "out: %s", output_ptr );
#endif    
}


/**
 * 0 = OK
 * 1 = connection problem
 * 2 = invalid session, reconect later 
 */
int raConnection::send( unsigned char * dataKOdeslani, int dataLen )
{
    if( !this->connected ) {
        this->login();
    }
    if( !this->connected ) {
        return 1;           
    }
        
    char url[256];
    sprintf( url, "http://%s/data", this->cd->ra_url );
    
    this->logger->log( "%s DATA", this->identity );
    
    sprintf( (char *)this->msg, "%s\n", 
        this->session
         );

    BYTE* target = this->msg + strlen( (char *)this->msg ); 
    this->createDataPayload( target, (BYTE*)dataKOdeslani, dataLen );
    
    
    HTTPClient http;
    http.begin( url );
    http.addHeader("Content-Type", "application/octet-stream");
    int httpCode = http.POST( (
#ifdef ESP8266    
            const 
#endif            
            uint8_t*)this->msg, strlen((char*)this->msg) );
    if( httpCode!=200 ) {
        // error description
        String payload = http.getString();
        this->logger->log( "%s [%s]", this->identity, payload.c_str() );
    }
    http.end();
    
    // this->logger->log( "%s rc=%d [%s]", this->identity, httpCode, this->session );
    this->logger->log( "%s rc=%d", this->identity, httpCode );

    if( httpCode==403 ) {  
        this->connected = false; 
        return 2; 
    }        
    
    if( httpCode==200 ) {  
        return 0; 
    }        
    
    return 1;
}


long raConnection::readServerTime()
{
    char url[256];
    sprintf( url, "http://%s/time", this->cd->ra_url );
    
    this->logger->log( "%s TIME", this->identity );
    
    HTTPClient http;
    http.begin( url );
    int httpCode = http.GET();
    String payload = http.getString();
    http.end();

    if( httpCode!=200 ) {
        // error description
        this->logger->log( "%s r%d [%s]", this->identity, httpCode, payload.c_str() );
        return 0;
    } else {
        long l = atol( (const char*)payload.c_str() );
        this->logger->log( "%s TIME=%d", this->identity, l );
        return l;
    }
}





// definovano v aes.h
// #define AES_KEYLEN 32
// #define AES_BLOCKLEN 16
 




/**
 * Vygeneruje session key.
 * Z hesla sestavi sestavi sifrovaci klic pro login blok.
 * Vygeneruje cely payload blok pro login a zapise ho na vystupni adresu.
 */
void raConnection::createLoginToken( BYTE *target, long serverTime )
{
    BYTE aes_key[AES_KEYLEN];
    BYTE aes_iv[AES_BLOCKLEN];
    
    this->sha256->init();
    this->sha256->update((const BYTE *)this->cd->ra_pass, strlen((const char *)this->cd->ra_pass));
    this->sha256->final(aes_key);
    
    random_block( (BYTE *)this->sessionKey, AES_KEYLEN );
    random_block( aes_iv, AES_BLOCKLEN );
    // tohle neni debug, ale uz si konstruuji zpravu!
    btohexa( aes_iv, AES_BLOCKLEN, (char *)target, 130 );

#ifdef DETAILED_RA_LOG
    this->logger->log( "password: %s", this->cd->ra_pass );
    btohexa( aes_key, SHA256_BLOCK_SIZE, log_buffer, LOG_BUFFER_LENGTH );
    this->logger->log( "password hash: %s", log_buffer );
    
    btohexa( this->sessionKey, AES_KEYLEN, log_buffer, LOG_BUFFER_LENGTH );
    this->logger->log( "session key: %s", log_buffer );
    
    this->logger->log( "IV: %s", target );
#endif        
    
    int msgLen = AES_KEYLEN+AES_BLOCKLEN; 
    BYTE msg[msgLen] ;
    memcpy( msg, this->sessionKey, AES_KEYLEN );
    
    uint32_t crc1 = CRC32::calculate((const void*) this->sessionKey, AES_BLOCKLEN);
    msg[AES_KEYLEN+0] = (BYTE)( (crc1>>24) & 0xff);
    msg[AES_KEYLEN+1] = (BYTE)( (crc1>>16) & 0xff);
    msg[AES_KEYLEN+2] = (BYTE)( (crc1>>8) & 0xff);
    msg[AES_KEYLEN+3] = (BYTE)(crc1 & 0xff);
    
    uint32_t crc2 = CRC32::calculate((const void*) (this->sessionKey + AES_BLOCKLEN) , AES_BLOCKLEN );
    msg[AES_KEYLEN+4] = (BYTE)( (crc2>>24) & 0xff);
    msg[AES_KEYLEN+5] = (BYTE)( (crc2>>16) & 0xff);
    msg[AES_KEYLEN+6] = (BYTE)( (crc2>>8) & 0xff);
    msg[AES_KEYLEN+7] = (BYTE)(crc2 & 0xff);

#ifdef DETAILED_RA_LOG
    this->logger->log( "key CRC: %08.8x %08.8x", crc1, crc2 );
#endif

    msg[AES_KEYLEN+8] = (BYTE)( (serverTime>>24) & 0xff );
    msg[AES_KEYLEN+9] = (BYTE)( (serverTime>>16) & 0xff );
    msg[AES_KEYLEN+10] = (BYTE)( (serverTime>>8) & 0xff );
    msg[AES_KEYLEN+11] = (BYTE)( (serverTime) & 0xff );
    
    random_block( msg+AES_KEYLEN+12, 4 ); 
    
    struct AES_ctx ctx;
    
    AES_init_ctx_iv(&ctx, (unsigned char const*)aes_key, (unsigned char const*)aes_iv);
    AES_CBC_encrypt_buffer(&ctx, (unsigned char *)msg, msgLen );

    char * output_ptr = (char *)target + AES_BLOCKLEN+AES_BLOCKLEN;
    output_ptr[0] = ':';
    output_ptr++;
    btohexa( msg, msgLen, output_ptr, 100 );  // delka je // AES_KEYLEN+AES_KEYLEN+AES_BLOCKLEN+AES_BLOCKLEN +1;
    
#ifdef DETAILED_RA_LOG
    this->logger->log( "out: %s", output_ptr );
#endif    
}


//TODO: misto random bytes poslat time_t, ktere vrati server predeslym dotazem; tim se to ochrani proti replay utoku
/**
 * Format login zpravy:
 *      <net_name>:<device_name>\n
 *      <login_paylod_block>\n
 *      
 * Format login_payload_block:
 *      <AES IV>:<AES_encrypted_data>
 * oboji zapsane jako retezec hexitu
 * 
 * Obsah AES_encrypted_data:
 *      <AES session key 32 byte><CRC32 z prvnich 16 byte klice><CRC32 z druhych 16 byte klice><server time 4 byte high endian><random data 4 byte>
 *      
*/
void raConnection::login() 
{
    char url[256];
    sprintf( url, "http://%s/login", this->cd->ra_url );
    
    long serverTime = this->readServerTime();
    if( serverTime==0 ) {
        this->connected = false;
        return;
    }
    
    this->logger->log( "%s LOGIN", this->identity );
    
    sprintf( (char *)this->msg, "%s\n", 
        this->cd->ra_dev_name );

    BYTE* target = this->msg + strlen( (char *)this->msg ); 
    this->createLoginToken( target, serverTime );
        
    HTTPClient http;
    http.begin( url );
    http.addHeader("Content-Type", "application/octet-stream");
    int httpCode = http.POST( (
#ifdef ESP8266    
            const 
#endif            
            uint8_t*)this->msg, strlen((char*)this->msg) );
    String payload = http.getString();
    if( httpCode!=200 ) {
        // error description
        this->logger->log( "%s [%s]", this->identity, payload.c_str() );
    }
    http.end();
    
    if( httpCode!=200 ) {  
        this->connected = false; 
        return;
    }

    strncpy( this->session, payload.c_str(), 31 );
    this->session[31] = 0;
    char *p = strchr( this->session, '\n' );
    if( p!=NULL ) { 
        p[0]=0; 
    }

    this->logger->log( "%s rc=%d [%s]", this->identity, httpCode, this->session );

    // mame session v this->session a klic v this->sessionKey
    this->connected = true;
    
    
    // Ulozime si data o spojeni do RTC memory
    
    #ifdef ESP8266
        struct rtcData data;
        memcpy( (void*)(data.data), (const void *)this->session, 32 );
        memcpy( (void*)(data.data+32), (const void *)this->sessionKey, 32 );
        saveRtcData( &data );
    #endif
    
    #ifdef ESP32
        memcpy( (void*)(rtcSessionId), (const void *)this->session, 32 );
        memcpy( (void*)(rtcSessionKey), (const void *)this->sessionKey, 32 );
        rtcIndicator = 0xDEADFACE;
    #endif    
}


// #define BLOB_DETAIL_LOG

/**
 * Odesle jednu radku hlavicky na server
 */ 
void sendText( WiFiClient * client, char * data )
{
    #ifdef BLOB_DETAIL_LOG
        Serial.printf( "> %s\n", data  );
    #endif
    client->println( data );
}

int sentLen;

/**
 * Odesle jednu radku payloadu na server
 */ 
void sendBin( WiFiClient * client, uint8_t * data, size_t len )
{
    sentLen += len;
    #ifdef BLOB_DETAIL_LOG
        Serial.printf( "> %s\n", data );
        Serial.printf( "> %d .. %d\n", len , sentLen );
    #endif
    client->write( data, len );
}

#define MAX_LINE_LEN 256
char receivedLine[MAX_LINE_LEN+2];
int pos = 0;
bool lineComplete = false;

/**
 * Zpracovani odpovedi ze serveru - slozeni jednotlivych radek z prijatych znaku.
 */ 
void addChar( char c )
{
    if( c=='\n' ) {
        lineComplete = true;
    } else if( c=='\r' ) {
        receivedLine[pos] = 0;
    } else {
        if( pos < MAX_LINE_LEN ) {
            receivedLine[pos++] = c;
            receivedLine[pos] = 0;
        }
    }
}

/**
 * Vrati nactenou radku ze serveru
 */ 
char * getLine()
{
    pos = 0;
    lineComplete = false;
    //D/Serial.printf( "< [%s]\n", receivedLine  );
    return receivedLine;
}

#define BLOCKSIZE_PLAIN 512
#define BLOCKSIZE_CIPHER 1024

/**
 * Parsuje radku s http statusem
 */ 
int parseStatusLine( char * statusLine )
{
    char * p = strchr( statusLine, ' ' );
    if( p==NULL ) {
        return -1;
    } 
    int status = atoi( p );
    Serial.printf( "\nstatus=%d\n", status );
    if( status==200 ) {
        return 0;
    }
    return status;
}

/**
 * Zpracuje radku s aplikacni odpovedi z RA
 */ 
int parseDataLine( char * dataLine )
{
    if( strcmp(dataLine,"OK") == 0 ) {
        return 0;
    } else {
        Serial.printf( "ERR: %s\n", dataLine );
        return -1;
    }
}

/**
 * Zaloguje sifrovaci klic a IV
 */ 
void raConnection::log_keys( unsigned char * key, unsigned char * iv )
{
    char buff[256];
    btohexa( key, AES_KEYLEN, (char*)buff, 100 );
    int pos = AES_KEYLEN+AES_KEYLEN; 
    buff[pos] = ' ';
    btohexa( iv, AES_BLOCKLEN, (char*)buff+pos+1, 100 );
    pos += 1 + AES_BLOCKLEN + AES_BLOCKLEN;
    buff[pos] = 0;
    this->logger->log( "key/iv %s", buff );      
}

/**
 * Vlastni odeslani blobu; predpoklada, ze klient je prihlaseny.
 */ 
int raConnection::sendBlobInt( unsigned char * blob, int blobSize, int startTime, char * desc, char * extension )
{
    char server[200];
    char url[200];
    strcpy( server, this->cd->ra_url  );
    char * p = strchr( server, '/' );
    if( p==NULL ) { return 1; }
    strcpy(url, p);
    strcat( url, "/blob" );
    *p = 0;
    
    this->logger->log( "%s server=[%s] url=[%s]", this->identity, server, url );
    
    unsigned char begin[256];
    sprintf( (char*)begin, "%s\n", this->session );
    
    BYTE aes_iv[AES_BLOCKLEN];
    random_block( aes_iv, AES_BLOCKLEN );
    btohexa( aes_iv, AES_BLOCKLEN, (char *)(begin+strlen((const char*)begin)), 130 );
    strcat( (char*)begin, "\n");

    unsigned char header[512];
    sprintf( (char*)header, "%d\n%s\n%s\n%d\n",
        time(NULL)-startTime,
        desc,
        extension,
        blobSize
    );

    int len = strlen((const char*)header) ;
    if( (len % AES_BLOCKLEN) != 0 ) {
        len = ((len/AES_BLOCKLEN)+1)*AES_BLOCKLEN;
    } 
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (unsigned char const*)this->sessionKey, (unsigned char const*)aes_iv);
    AES_CBC_encrypt_buffer(&ctx, (unsigned char *)header, len );

    unsigned char textBuff[BLOCKSIZE_CIPHER+10];

    btohexa( header, len, (char*)textBuff, BLOCKSIZE_PLAIN );
    int headerTextLen = strlen( (const char*)textBuff );
    textBuff[headerTextLen++] = '\n';
    textBuff[headerTextLen++] = 0;

    int beginSize = strlen( (const char*)begin );
    int blobBlocks = blobSize / BLOCKSIZE_PLAIN;
    if( (blobSize % BLOCKSIZE_PLAIN)!=0 ) { blobBlocks++; }
    int blobOutSize = (BLOCKSIZE_CIPHER+1)*blobBlocks;
    int totalLen = blobOutSize + beginSize + headerTextLen ;
    sentLen = 0;

    WiFiClient client;

    if (! client.connect(server, 80)) {
        Serial.println("Connect failed.");
        return 2;
    }

    char buf[256];
    sprintf( buf, "POST %s HTTP/1.1", url );
    sendText( &client, buf );

    sprintf( buf, "Host: %s", server );
    sendText( &client, buf );
    sprintf( buf, "Content-Length: %d", totalLen );
    sendText( &client, buf );
    sendText( &client, (char *)"Content-Type: application/octet-stream" );
    sendText( &client, (char *)"" );

    totalLen = 0;
    sendBin( &client, begin, beginSize );
    sendBin( &client, textBuff, headerTextLen );

    unsigned char binData[BLOCKSIZE_PLAIN+2];

    uint8_t *fbBuf = blob;
    for (size_t n=0; n<blobSize; n=n+BLOCKSIZE_PLAIN) {
        #ifdef BLOB_DETAIL_LOG
            Serial.printf( "n=%d, size=%d\n",n, blobSize );
            log_keys( (unsigned char*)this->sessionKey, (unsigned char*)aes_iv );
        #endif
        if (n+BLOCKSIZE_PLAIN < blobSize) {
            #ifdef BLOB_DETAIL_LOG
                Serial.println( "v1" );
            #endif
            memcpy( binData, fbBuf, BLOCKSIZE_PLAIN );
            AES_init_ctx_iv(&ctx, (unsigned char const*)this->sessionKey, (unsigned char const*)aes_iv);
            AES_CBC_encrypt_buffer(&ctx, binData, BLOCKSIZE_PLAIN );
            btohexa( binData, BLOCKSIZE_PLAIN, (char*)textBuff, 1025 );
            textBuff[1024] = '\n';
            textBuff[1025] = 0;
            sendBin( &client,textBuff, 1025 );
            fbBuf += BLOCKSIZE_PLAIN;
        }
        else if (blobSize % BLOCKSIZE_PLAIN > 0) {
            size_t remainder = blobSize % BLOCKSIZE_PLAIN;
            #ifdef BLOB_DETAIL_LOG
                Serial.printf( "v2 remainder=%d\n", remainder );
            #endif
            memcpy( binData, fbBuf, remainder );
            AES_init_ctx_iv(&ctx, (unsigned char const*)this->sessionKey, (unsigned char const*)aes_iv);
            AES_CBC_encrypt_buffer(&ctx, binData, BLOCKSIZE_PLAIN );
            btohexa( binData, BLOCKSIZE_PLAIN, (char*)textBuff, 1025 );
            textBuff[1024] = '\n';
            textBuff[1025] = 0;
            sendBin( &client,textBuff, 1025 );
        }
    }   

    int timeoutTimer = 30000;
    long startTimer = millis();

    /**
     * Stavovy automat pro zpracovani vysledku. Vnitrni stavy:
     * 0 = cekam na http status line
     * 1 = cekam na konec hlavicky
     * 2 = cekam na prvni radek dat
     * 3 = hotovo
     */
    int status = 0;
    int rc = 3;

    while( (status!=3) && (startTimer + timeoutTimer) > millis() ) {
        Serial.print(".");
        delay(100);      
        while (client.available()) {
            addChar( client.read() );
            startTimer = millis();
            if( lineComplete ) {
                char * line = getLine();
                if( status==0 ) {
                    if( 0!=parseStatusLine( line ) ) {
                        status = 3;
                        break;
                    }
                    status = 1;
                } else if( status==1 ) {
                    if( strlen(line)==0 ) {
                        // prazdna radka = konec hlavicky
                        status = 2;
                    }
                } else if( status==2 ) {
                    if( 0==parseDataLine( line ) ) {
                        rc = 0;
                    } else {
                        rc = 4;
                    }
                    status = 3;
                    break;
                }
            }
        } // while (client.available()) {
    }
    client.stop();
    
    return rc;
}

/**
 * Odesle blob. Pokud neni prihlaseno, prihlasi.
 * 0 = OK, jina hodnota = chyba; >=300 http result code.
 */ 
int raConnection::sendBlob( unsigned char * blob, int blobSize, int startTime, char * desc, char * extension )
{
    this->logger->log( "%s BLOB+", this->identity );

    if( !this->connected ) {
        this->login();
    }
    if( !this->connected ) {
        this->logger->log( "%s BLOB-ERR no login, no sendin'", this->identity );
        return 1;           
    }
    
    int rc = this->sendBlobInt( blob, blobSize, startTime, desc, extension );
    if( rc>=400 ) {
        this->logger->log( "%s BLOB ERR %d, reconnect", this->identity, rc );
        this->login();
        if( this->connected ) {
            rc = this->sendBlobInt( blob, blobSize, startTime, desc, extension );               
        }
    }

    if( rc==0 ) {    
        this->logger->log( "%s BLOB-OK", this->identity );
    } else {
        this->logger->log( "%s BLOB-ERR %d", this->identity, rc );
    } 
    return rc;
}
    

/**
 * Pokud neni spojene, prihlasi se
 * Vraci true = prihlaseno; false = neprihlaseno
 */ 
bool raConnection::isConnected()
{
    if( !this->connected ) {
        this->login();
    }
    if( !this->connected ) {
        return false;           
    }
    return true;
}
