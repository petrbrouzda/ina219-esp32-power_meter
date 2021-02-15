/**
 * Meric spotreby pro 
 * - ESP32 s OLED displejem LilyGO TTGO T-Display 
 *      - https://www.banggood.com/TTGO-T-Display-ESP32-CP2104-WiFi-bluetooth-Module-1_14-Inch-LCD-Development-Board-LILYGO-for-Arduino-products-that-work-with-official-Arduino-boards-p-1522925.html?p=FY1402881924201411VQ&zf=881924
 * - merici modul INA-219. 
 *      - https://www.banggood.com/GY-INA219-High-Precision-I2C-Digital-Current-Sensor-Module-p-1200612.html?p=FY1402881924201411VQ&zf=881924
 * 
 * Vyzaduje knihovny v library manageru:
 * - TFT_eSPI 2.2.14 (s upravou User_Config dle https://github.com/Xinyuan-LilyGO/TTGO-T-Display ! ) 
 * - Tasker 2.0.0 
 * - Adafruit_INA219 1.0.9
 * - ArduinoJSON 5.13.5 (POZOR! Starsi nez aktualni verze!)
 * 
 * Kompilovano s ESP32 arduino core 1.0.4.
 * 
 * V Arduino IDE MUSI byt nastaveno rozdeleni flash tak, aby bylo alespon 1 M filesystemu SPIFS !
*/

//+++++ RatatoskrIoT +++++

  #include "AppConfig.h"

  // RA objects
  ratatoskr* ra;
  raLogger* logger;
  raConfig config;
  Tasker tasker;

  // stav WiFi - je pripojena?
  bool wifiOK = false;
  // cas, kdy byla nastartovana wifi
  long wifiStartTime = 0;
  // duvod posledniho startu, naplni se automaticky
  int wakeupReason;
  // je aktualni start probuzenim z deep sleep?
  bool deepSleepStart;

  #ifdef RA_STORAGE_RTC_RAM 
    RTC_DATA_ATTR unsigned char ra_storage[RA_STORAGE_SIZE];
  #endif

  #ifdef USE_BLINKER
    raBlinker blinker( BLINKER_PIN );
    int blinkerPortal[] = BLINKER_MODE_PORTAL;
    int blinkerSearching[]  = BLINKER_MODE_SEARCHING;
    int blinkerRunning[] = BLINKER_MODE_RUNNING;
    int blinkerRunningWifi[] = BLINKER_MODE_RUNNING_WIFI;;
  #endif  

//----- RatatoskrIoT ----


// +++ INA229 +++
  // Adafruit_INA219  1.0.9 v lib manageru
  #include "Adafruit_INA219.h"
  // https://diyi0t.com/ina219-tutorial-for-arduino-and-esp/
  // https://github.com/adafruit/Adafruit_INA219/blob/master/Adafruit_INA219.cpp
  // https://github.com/adafruit/Adafruit_INA219/blob/master/Adafruit_INA219.h

  // datasheet: https://www.ti.com/lit/ds/symlink/ina219.pdf
  
  Adafruit_INA219 ina219;
// --- INA219 ---

// +++ displej +++
  #include <TFT_eSPI.h>
  // vyzaduje knihovnu TFT_eSPI 2.2.14 v library manageru
  // plus upravu User_Config dle https://github.com/Xinyuan-LilyGO/TTGO-T-Display
  #include <SPI.h>
  // API: https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.h
  
  #ifndef TFT_DISPOFF
    #define TFT_DISPOFF 0x28
  #endif
  
  #ifndef TFT_SLPIN
    #define TFT_SLPIN   0x10
  #endif
  
  #define TFT_MOSI            19
  #define TFT_SCLK            18
  #define TFT_CS              5
  #define TFT_DC              16
  #define TFT_RST             23
  
  #define TFT_BL          4  // Display backlight control pin
  
  TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
// --- displej ---


// komponenty aplikace
#include "menu.h"
#include "values.h"
#include "keyboard.h"
#include "MeteringPageRender.h"
#include "PrintMsg.h"
#include "MenuPageRender.h"
#include "Csv.h"

#define I2C_SDA 25
#define I2C_SCL 26

/*
 * Definice jednotlivych tlacitek
 */
#define KEY_RIGHT 12
#define KEY_DOWN 13
#define KEY_LEFT 15
#define KEY_UP 2
#define CORE_BUTTON_TOP  0
#define CORE_BUTTON_DOWN 35

Button buttonR = { KEY_RIGHT, HIGH, 0, 'r' };
Button buttonL = { KEY_LEFT, HIGH, 0, 'l' };
Button buttonU = { KEY_UP, HIGH, 0, 'u' };
Button buttonD = { KEY_DOWN, HIGH, 0, 'd' };

// ++++++++++ vnitrni stavy aplikace ++++++++++++++

/** 11=spotrebic, 12=zdroj */
int coMerim = 11;

/** rozliseni 21 = 32V 2A, 22 = 32V 1A, 23 = 16V 0.4A*/
int rozliseni = 21;

/** namerene hodnoty */
Values vals;

/** Zobrazovane stranky v rezimu mereni:
 * - 1 = vsechny meraky
 * - 2 = historie napeti
 * - 3 = historie proudu
 * - 4 = low pover 1/2
 * - 5 = low power 2/2
 */
int displayedPage = 1;

/** tato promenna ukazuje na prave zobrazovane menu */
Menu * currentMenu;

/**
 * Aktualni rezim prace aplikace:
 * 1 = hlavniMenu
 * 10 = coMerimMenu
 * 20 = rozliseniMenu
 * 30 = limit low/high
 * 40 = zapisMenu
 * 100 = mereni
 * Nemenit primo, ale pres setAppState()!
 */
int appState = 1;

/**
 * Rozhrani mezi nizkou a vysokou spotrebou.
 */
float lowHighThreshold = 5;

/**
 * Stranku s vysledky mereni je treba celou prekreslit (vcetne smazani pozadi)
 */
bool meteringPageChanged = true;

/**
 * Maji se posilat data na server?
 */
bool saveData = true;

/**
 * Zaznamenat samostatne prechody do high power / low power?
 */
bool saveEvents = true;

/**
 * CSV data
 */
Csv * csv;

/**
 * Cas startu mereni
 */
int meteringStart;

/**
 * Od jakeho obsazeni CSV se to snazi ulozit data na server?
 */
#define CSV_USAGE_THRESHOLD 90

// -------- vnitrni stavy aplikace ---------------


char tempName[64];

char * createFileName( const char * append )
{
  const char * coMerimStr = coMerim==12 ? "source" : "load";
  const char * rozliseniStr;
  if( rozliseni==21 ) {
    rozliseniStr = "32V-2A";
  } else if( rozliseni==22 ) {
    rozliseniStr = "32V-1A";
  } else {
    rozliseniStr = "16V-400mA";
  }
  
  sprintf( tempName, "%s_%s_%.0fmA_%s", 
    coMerimStr,
    rozliseniStr,
    lowHighThreshold,
    saveEvents ? "Y" : "N" );

  if( append!=NULL )
  {
    sprintf( tempName + strlen(tempName), "_%s", append );
  }

  return tempName;
}


void setAppState( int newState ) {
  Serial.printf( "appState = %d\n", newState );
  appState = newState;
}

void saveCsv()
{
  // je to tady a ne v loop(), protoze pri mereni se loop nevola
  wifiOK = checkWifiStatus();
  if( ! csv->hasData() ) {
    return;
  }
  if( wifiOK ) {
    meteringPageChanged = true;
    // pokud se nepodari, zkusime to celkem 4x
    for( int i = 0; i<4; i++ ) {
      printMsg( TFT_WHITE, "Odesilam...", NULL, NULL );    
      if( 0 == ra->sendBlob( (unsigned char*)csv->getContent(), csv->getSize(), meteringStart, (char*)csv->getName(), (char*)"csv" ) ) {
        Serial.println( "CSV sent" );
        csv->rewind();
        csv->setName( createFileName("cont") );
        meteringStart = time(NULL);
        break;
      } else {
        Serial.println( "CSV not sent" );
        printMsg( TFT_RED, "Chyba", "pri", "odeslani" );    
        delay( 250 );
      }
    }
  }  
}


/** spousteno Taskerem jedenkrat za minutu */
void doComputeMinData()
{
  if( saveData ) {
    vals.write( csv, " " );
    if( csv->getUsage() > CSV_USAGE_THRESHOLD ) {
      saveCsv();
    }
  }

  vals.resetMinuteData(); 
}

/** spousteno z loopu cca kazdych 10 msec */
void doMeter()
{
  long curTime = millis();

  vals.current_mA = ina219.getCurrent_mA();
  vals.shuntvoltage = ina219.getShuntVoltage_mV();
  vals.busvoltage = ina219.getBusVoltage_V();
  vals.power_mW = ina219.getPower_mW();
  
  long interval_ms = curTime - vals.lastMeteringTime;

  vals.doCompute( coMerim, interval_ms, curTime, lowHighThreshold, csv, saveEvents );
}




/** Vykresleni displeje (a pripadne debug na serial port).
   Spousteno Taskerem 1x za 1,5 sec.
   V prubehu vykresleni se vola mereni hodnot doMeter(), aby byly intervaly cca 10 msec a neprisli jsme o peaky spotreby.
*/
void doPrint()
{
  //T/ long t1 = millis();
  print_dumpValuesToSerial();

  //T/ long t2 = millis();

  if( meteringPageChanged ) {
    // smazani obrazovky trva 14 msec
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM); // kresli se od top-left 
  }
  
  // tedy po 14 msec zmerime hodnoty
  doMeter();

  //T/ long t2b = millis();

  /* vykresleni cele obrazovky trva 22 msec, takze lepe v pulce zmerit hodnoty - proto je vykresleni 
    rozdeleno na dve casti, mezi kterymi se provede mereni */
  if( displayedPage==1 ) {
    
    print_page1_half1();
    doMeter();
    print_page1_half2();
    
  } else if( displayedPage==2 ) {
    
    print_page2_half1();
    doMeter();
    print_page2_half2();
    
  } else if( displayedPage==3 ) {
    
    print_page3_half1();
    doMeter();
    print_page3_half2();
    
  } else if( displayedPage==4 ) {
    
    print_page4_half1();
    doMeter();
    print_page4_half2();
    
  } else if( displayedPage==5 ) {
    
    print_page5_half1();
    doMeter();
    print_page5_half2();
    
  }

  if( meteringPageChanged ) {
    print_page_footer();  
    meteringPageChanged = false;
  }

  print_page_footer2( csv->getUsage() );
  
  // na konci obrazovky (po dalsich cca 10 msec vykreslovani) zmerime hodnoty,
  // protoze po navratu do loop bude zase 10 msec pauza
  doMeter();            

  //T/ long t3 = millis();
  //T/ Serial.printf( "timing: serial %d ms, display clear %d, draw %d ms\n", (t2-t1), (t2b-t2), (t3-t2b) );
}






/**
 * Obsluha vnitrniho stavu aplikace - definice menu, stavoveho automatu atd.
 */

// definice jednotlivych menu
Menu * hlavniMenu;
Menu * coMerimMenu;
Menu * rozliseniMenu;
Menu * zapisMenu;


// pozice polozek v menu - pro editaci hodnot z podrizenych obrazovek
#define HLAVNI_MENU_COMERIM 1
#define HLAVNI_MENU_ROZLISENI 2
#define HLAVNI_MENU_LIMIT 3
#define HLAVNI_MENU_SAVE 4


/**
 * volano ze setup() - inicializace menu.
 */
void definujMenu()
{
  hlavniMenu = new Menu( "Hlavni menu" );
  hlavniMenu->addItem( "Spustit mereni", 100 );
  hlavniMenu->addItem( "Merim: Spotrebic", 10 );
  hlavniMenu->addItem( "Rozsah: 32V 2A", 20 );
  hlavniMenu->addItem( "Low/high: 5 mA", 30 );
  hlavniMenu->addItem( "Zapis: Vse", 40 );
  hlavniMenu->addItem( "Vymazat pocitadla", 101 );

  coMerimMenu = new Menu( "Co merim" );
  coMerimMenu->addItem( "Spotrebic", 11 );
  coMerimMenu->addItem( "Zdroj", 12 );
  
  rozliseniMenu = new Menu( "Rozsah" );
  rozliseniMenu->addItem( "32 V / 2 A", 21 );
  rozliseniMenu->addItem( "32 V / 1 A", 22 );
  rozliseniMenu->addItem( "16 V / 400 mA", 23 );

  zapisMenu = new Menu( "Zapisovat" );
  zapisMenu->addItem( "Vse", 31 );
  zapisMenu->addItem( "1x za minutu", 32 );
  zapisMenu->addItem( "Nezapisovat", 33 );
}


void raAllWasSent()
{
  // v teto aplikaci se nepouziva
}

/**
 * Vraci jmeno aplikace a jeji verzi do alokovaneho bufferu.
 */
void raGetAppName( char * target, int size )
{
  snprintf( target, size, "%s - %s %s", __FILE__, __DATE__, __TIME__ );
  target[size-1] = 0;
}



void setup() {

  Serial.begin(115200);
  while(!Serial) {}

  tft.init();
  printMsg( TFT_WHITE, "Meric spotreby", "1.1+wifi", "... startuji ..." );
  delay(300);

  //+++++ RatatoskrIoT +++++
    // Mel by byt jako prvni v setup().
    // Pokud je parametr true, zapne Serial pro rychlost 115200. Jinak musi byt Serial zapnuty uz drive, nez je tohle zavolano.
    ratatoskr_startup( false );
  //----- RatatoskrIoT ----

  //++++++ Custom code +++++

  csv = new Csv( 100000, ',', ";" );
  csv->setName( createFileName(NULL) );
  csv->beginHeader();
  vals.writeHeader( csv );
  csv->endHeader();

  // https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
  Wire.begin(I2C_SDA, I2C_SCL);

  if (! ina219.begin()) {
    printMsg( TFT_RED, "CHYBA", "Nevidim INA219.", NULL );
    while (1) { delay(10); }
  }
  Serial.println( "INA-219 found" );

  // nastavim nejvetsi povoleny range, abychom nebyli prekvapeni
  ina219.setCalibration_32V_2A();

  Serial.println( "INA-219 set" );

  pinMode( KEY_RIGHT, INPUT_PULLUP );
  attachInterrupt(buttonR.pin, isrButtonR, CHANGE);
  
  pinMode( KEY_DOWN, INPUT_PULLUP );
  attachInterrupt(buttonD.pin, isrButtonD, CHANGE);
  
  pinMode( KEY_LEFT, INPUT_PULLUP );
  attachInterrupt(buttonL.pin, isrButtonL, CHANGE);
  
  pinMode( KEY_UP, INPUT_PULLUP );
  attachInterrupt(buttonU.pin, isrButtonU, CHANGE);
  
  // Dve tlacitka, co jsou primo na boardu. Ale ted pro ne nemam pouziti.
    // pinMode( CORE_BUTTON_TOP, INPUT_PULLUP );
    // pinMode( CORE_BUTTON_DOWN, INPUT_PULLUP );

  definujMenu();
  currentMenu = hlavniMenu;
  setAppState(1);

  vals.resetCounters();
  meteringStart = 0;

  //------ Custom code ------

}



bool drawLimitPg = true;
bool redrawLimitPg = false;
char limitBuff[10];
int cursorPos;
#define LIMIT_MAX_LEN 4

void appStateLimit()
{
  if( drawLimitPg ) {
    sprintf( limitBuff, "%04.0f", lowHighThreshold );
    limitPgFullDraw();
    cursorPos = 0;
    drawLimitPg = false;
    redrawLimitPg = true;
  }

  while( keyAvail() ) {
    char ch = getKey();
    if( ch=='l' ) {

      cursorPos--;
      if( cursorPos<0 ) {
        // zmacknuto ZPET na zacatku
        setAppState( 1 );
        hlavniMenu->clearState();
        return;
      }

    } else if( ch=='r' ) {
      
      cursorPos++;
      if( cursorPos==LIMIT_MAX_LEN ) {
        // zmacknuto DAL na konci
        setAppState( 1 );
        hlavniMenu->clearState();
        lowHighThreshold = atof( limitBuff );
        char bafr[32];
        sprintf( bafr, "Low/high: %.0f mA", lowHighThreshold );
        hlavniMenu->updateText( HLAVNI_MENU_LIMIT, bafr );
        return;
      }

    } else if( ch=='u' ) {

      if( limitBuff[cursorPos]=='9' ) {
        limitBuff[cursorPos]='0';
      } else {
        limitBuff[cursorPos]++;
      }
      
    } else if( ch=='d' ) {

      if( limitBuff[cursorPos]=='0' ) {
        limitBuff[cursorPos]='9';
      } else {
        limitBuff[cursorPos]--;
      }

    }

    redrawLimitPg = true;
  } // while( keyAvail() )

  if( redrawLimitPg ) {
    LimitPgNumber( limitBuff, cursorPos, LIMIT_MAX_LEN, (char*)"mA" );
    redrawLimitPg = false;
  }

}


#define MENU_CHANGE_DELAY 50
#define METERING_DISPLAY_REFRESH 1000

void appStateMainMenu()
{
  int i = hlavniMenu->getResult();
  if( i!=0 ) {
    // doslo k volbe!
    
    // tlacitko zpet => spustit mereni
    if( i==-1 ) { i=100; }
    
    setAppState( i );
    if( i==10 ) {
      
      currentMenu = coMerimMenu;
      coMerimMenu->clearState();
      coMerimMenu->setActive( coMerim );
      delay( MENU_CHANGE_DELAY );

    } else if( i==20 ) {
      
      currentMenu = rozliseniMenu;
      rozliseniMenu->clearState();
      rozliseniMenu->setActive( rozliseni );
      delay( MENU_CHANGE_DELAY );

    } else if( i==30 ) {

      drawLimitPg = true;
      delay( MENU_CHANGE_DELAY );

    } else if( i==40 ) {
      
      currentMenu = zapisMenu;
      zapisMenu->clearState();
      int stav = 33;
      if( saveData && saveEvents ) {
        stav = 31;
      } else if( saveData && !saveEvents ) {
        stav = 32;
      }
      zapisMenu->setActive( stav );
      delay( MENU_CHANGE_DELAY );

    } else if( i==100 ) {
      
      // spoustim mereni
      displayedPage = 1;
      meteringPageChanged = true;
      vals.lastMeteringTime = millis() - 1;
      // rovnou to zmerime a zobrazime na obrazovce, aby uzivatel necekal 1.5 sec na reakci
      doMeter();
      vals.resetMinuteData();
      vals.write( csv, " " );
      if( meteringStart == 0 ) {
        meteringStart = time(NULL);
        csv->setName( createFileName(NULL) );
      }
      doPrint();
      tasker.setInterval( doPrint, METERING_DISPLAY_REFRESH );
      tasker.setInterval( doComputeMinData, 60000 );

    } else if( i==101 ) {
      
      // smazat pocitadla a zpet do menu
      setAppState( 1 );
      hlavniMenu->clearState();
      hlavniMenu->setPos(0);
      vals.resetCounters();
      meteringStart = time(NULL);
      csv->setName( createFileName(NULL) );
      delay( MENU_CHANGE_DELAY );

    } 
  } // doslo k volbe!
}


void appStateCoMerimMenu()
{
  int i = coMerimMenu->getResult();
  if( i!=0 ) {
    // doslo k volbe!
    
    if( i>0 ) {
      coMerim = i;
      switch( coMerim ) {
        case 11: hlavniMenu->updateText( HLAVNI_MENU_COMERIM, "Merim: Spotrebic" ); break;
        case 12: hlavniMenu->updateText( HLAVNI_MENU_COMERIM, "Merim: Zdroj" ); break;
      }
    } // if( i>0 ) 

    // navrat do hlavniho menu 
    setAppState( 1 );
    currentMenu = hlavniMenu;
    hlavniMenu->setPos( HLAVNI_MENU_COMERIM );
    hlavniMenu->clearState();
    delay( MENU_CHANGE_DELAY );
  } // doslo k volbe!  
}

void appStateRozliseniMenu()
{
  int i = rozliseniMenu->getResult();
  if( i!=0 ) {
    // doslo k volbe!

    if( i>0 ) {
      rozliseni = i;
      switch( rozliseni ) {
        case 21: 
            hlavniMenu->updateText( HLAVNI_MENU_ROZLISENI, "Rozsah: 32V 2A" ); 
            ina219.setCalibration_32V_2A();
            break;
        case 22: 
            hlavniMenu->updateText( HLAVNI_MENU_ROZLISENI, "Rozsah: 32V 1A" ); 
            ina219.setCalibration_32V_1A();
            break;
        case 23: 
            hlavniMenu->updateText( HLAVNI_MENU_ROZLISENI, "Rozsah: 16V 0.4A" ); 
            ina219.setCalibration_16V_400mA();
            break;
      } // switch( rozliseni )
    } // if( i>0 ) 

    // navrat do hlavniho menu 
    setAppState( 1 );
    currentMenu = hlavniMenu;
    hlavniMenu->setPos( HLAVNI_MENU_ROZLISENI );
    hlavniMenu->clearState();
    delay( MENU_CHANGE_DELAY );
  } // doslo k volbe!  
}
      
      
void appStateZapisMenu()
{
  int i = zapisMenu->getResult();
  if( i!=0 ) {
    // doslo k volbe!

    switch( i ) {
      case 31: 
          hlavniMenu->updateText( HLAVNI_MENU_SAVE, "Zapis: Vse" ); 
          saveData = true;
          saveEvents = true;
          break;
      case 32: 
          hlavniMenu->updateText( HLAVNI_MENU_SAVE, "Zapis: Jen 1/min" ); 
          saveData = true;
          saveEvents = false;
          break;
      case 33: 
          hlavniMenu->updateText( HLAVNI_MENU_SAVE, "Zapis: Vypnuto" ); 
          saveData = false;
          saveEvents = false;
          break;
    } // switch( i )

    // navrat do hlavniho menu 
    setAppState( 1 );
    currentMenu = hlavniMenu;
    hlavniMenu->setPos( HLAVNI_MENU_SAVE );
    hlavniMenu->clearState();
    delay( MENU_CHANGE_DELAY );
  } // doslo k volbe!  
}


void appStateMenu()
{
  switch( appState ) {
    case 1: 
      // render vykresluje aktualni stav menu
      renderMenu();
      // a obsluzna procedura pro state vyhodnocuje zmeny stavu (akce z klavesnice)
      appStateMainMenu(); 
      break;
    case 10: 
      renderMenu();
      appStateCoMerimMenu(); 
      break;
    case 20: 
      renderMenu();
      appStateRozliseniMenu(); 
      break;
    case 30: 
      // appStateLimit obsahuje i render, nevola se samostatne
      appStateLimit(); 
      break;
    case 40: 
      renderMenu();
      appStateZapisMenu(); 
      break;
  }
}


#define DISPLAY_PAGE_MIN 1
#define DISPLAY_PAGE_MAX 5

void appStateMereni()
{
    // obsluha klavesnice
    while( keyAvail() ) {
      char ch = getKey();
      if( ch=='l' ) {

          // zmacknuto ZPET, konec mereni, zastaveni periodickych tasku
         setAppState( 1 );
         hlavniMenu->clearState();
         tasker.setInterval( doComputeMinData, 0 );
         tasker.setInterval( doPrint, 0 );
         if( saveData ) {
            saveCsv();
         }
         
      } else if( ch=='r' ) {

          // odesleme na server pracovni verzi dat
          if( csv->hasData() ) {
              wifiOK = checkWifiStatus();
              if( wifiOK ) {
                meteringPageChanged = true;
                printMsg( TFT_WHITE, "Odesilam...", NULL, NULL );    
                if( 0 != ra->sendBlob( (unsigned char*)csv->getContent(), csv->getSize(), meteringStart, (char*)createFileName("temp"), (char*)"csv" ) ) {         
                    printMsg( TFT_RED, "Chyba", "pri", "odeslani" );    
                    delay( 250 );
                }
              }
          }

      } else if( ch=='u' ) {
        
        displayedPage--;
        if( displayedPage<DISPLAY_PAGE_MIN ) displayedPage=DISPLAY_PAGE_MAX;
        meteringPageChanged = true;
        // hned prekreslime, aby to bylo plynule
        doPrint();

      } else if( ch=='d' ) {
        
        displayedPage++;
        if( displayedPage>DISPLAY_PAGE_MAX ) displayedPage=DISPLAY_PAGE_MIN;
        meteringPageChanged = true;
        // hned prekreslime, aby to bylo plynule
        doPrint();

      }
    } // while( keyAvail() )

   // mereni
   doMeter();
   
   // do scheduled tasks
   tasker.loop();
}

void loop() 
{
  if( appState==100 ) {
      appStateMereni();
  } else {
      // obsahuje call checkWifiStatus()
      saveCsv();
      appStateMenu();
  }
  
  delay(10);
}







//------------------------------------ callbacks from WiFi +++


void wifiStatus_StartingConfigPortal(  char * apName, char *apPassword, char * ipAddress   )
{
  // +++ user code here +++
  logger->log( "Starting AP [%s], password [%s]. Server IP [%s].", apName, apPassword, ipAddress );
  char line1[50], line2[50], line3[50];
  sprintf( line1, "AP %s", apName );
  sprintf( line2, "pwd %s", apPassword );
  sprintf( line3, "IP %s", ipAddress );
  printMsg( TFT_WHITE, line1, line2, line3 );  
  // --- user code here ---
}

void wifiStatus_Connected(  int status, int waitTime, char * ipAddress  )
{
  // +++ user code here +++
  logger->log("* wifi [%s], %d dBm, %d s", ipAddress, WiFi.RSSI(), waitTime );
  // --- user code here ---
}

void wifiStatus_NotConnected( int status, long msecNoConnTime )
{
  // +++ user code here +++
    char desc[32];
    getWifiStatusText( status, desc );
    logger->log("* no wifi (%s), %d s", desc, (msecNoConnTime / 1000L) );
  // --- user code here ---
}

void wifiStatus_Starting()
{
  // +++ user code here +++
  // --- user code here ---
}

/**
 * Je zavolano pri startu, pokud jsou v poradku konfiguracni data.
 * Pokud vrati TRUE, startuje se config portal.
 * Pokud FALSE, pripojuje se wifi.
 */
bool wifiStatus_ShouldStartConfig()
{
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);

  // Pokud se pouziva FLASH button, je treba dat pauzu, aby ho uzivatel stihl zmacknout,
  // jinak je mozne usporit cas a energii tim, ze se rovnou pojede.
  // delay(1500);

  return digitalRead(CONFIG_BUTTON) == CONFIG_BUTTON_START_CONFIG;
}
//------------------------------------ callbacks from WiFi ---


/*
 */
 
