#include "Wire.h"

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
 * 
 * Kompilovano s ESP32 arduino core 1.0.4.
 */ 

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

// +++ Tasker +++
  // ne-interrupt reseni planovani uloh
  // https://github.com/joysfera/arduino-tasker
  // import "Tasker" 2.0.0 in lib manager !!!
  
  // maximum number of tasks:
  #define TASKER_MAX_TASKS 7
  #include "Tasker.h"
  Tasker tasker; 
// --- Tasker ---

// komponenty aplikace
#include "menu.h"
#include "values.h"
#include "keyboard.h"
#include "MeteringPageRender.h"
#include "PrintMsg.h"
#include "MenuPageRender.h"


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


// -------- vnitrni stavy aplikace ---------------



void setAppState( int newState ) {
  Serial.printf( "appState = %d\n", newState );
  appState = newState;
}


/** spousteno Taskerem jedenkrat za minutu */
void doComputeMinData()
{
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

  vals.doCompute( coMerim, interval_ms, curTime, lowHighThreshold );
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


// pozice polozek v menu - pro editaci hodnot z podrizenych obrazovek
#define HLAVNI_MENU_COMERIM 1
#define HLAVNI_MENU_ROZLISENI 2
#define HLAVNI_MENU_LIMIT 3


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
  hlavniMenu->addItem( "Vymazat pocitadla", 101 );

  coMerimMenu = new Menu( "Co merim" );
  coMerimMenu->addItem( "Spotrebic", 11 );
  coMerimMenu->addItem( "Zdroj", 12 );
  
  rozliseniMenu = new Menu( "Rozsah" );
  rozliseniMenu->addItem( "32 V / 2 A", 21 );
  rozliseniMenu->addItem( "32 V / 1 A", 22 );
  rozliseniMenu->addItem( "16 V / 400 mA", 23 );
}



void setup() {

  tft.init();
  printMsg( TFT_WHITE, "Meric spotreby 1.0", "... startuji ..." );

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
  Wire.begin(I2C_SDA, I2C_SCL);

  if (! ina219.begin()) {
    printMsg( TFT_RED, "CHYBA", "Nevidim INA219." );
    while (1) { delay(10); }
  }

  // nastavim nejvetsi povoleny range, abychom nebyli prekvapeni
  ina219.setCalibration_32V_2A();

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

    } else if( i==100 ) {
      
      // spoustim mereni
      displayedPage = 1;
      meteringPageChanged = true;
      vals.lastMeteringTime = millis() - 1;
      // rovnou to zmerime a zobrazime na obrazovce, aby uzivatel necekal 1.5 sec na reakci
      doMeter();
      vals.resetMinuteData();
      doPrint();
      tasker.setInterval( doPrint, METERING_DISPLAY_REFRESH );
      tasker.setInterval( doComputeMinData, 60000 );

    } else if( i==101 ) {
      
      // smazat pocitadla a zpet do menu
      setAppState( 1 );
      hlavniMenu->clearState();
      hlavniMenu->setPos(0);
      vals.resetCounters();
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


void appStateMenu()
{
  switch( appState ) {
    case 1: 
      renderMenu();
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
      appStateLimit(); 
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
      appStateMenu();
  }
  
  delay(10);
}

/*
Použití knihovny Wire ve verzi 1.0.1 v adresáři: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\Wire 
Použití knihovny Adafruit_INA219 ve verzi 1.0.9 v adresáři: C:\Users\brouzda\Documents\Arduino\libraries\Adafruit_INA219 
Použití knihovny Adafruit_BusIO ve verzi 1.4.1 v adresáři: C:\Users\brouzda\Documents\Arduino\libraries\Adafruit_BusIO 
Použití knihovny SPI ve verzi 1.0 v adresáři: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\SPI 
Použití knihovny TFT_eSPI ve verzi 2.2.14 v adresáři: C:\Users\brouzda\Documents\Arduino\libraries\TFT_eSPI 
Použití knihovny FS ve verzi 1.0 v adresáři: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\FS 
Použití knihovny SPIFFS ve verzi 1.0 v adresáři: C:\Users\brouzda\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\SPIFFS 
Použití knihovny Tasker ve verzi 2.0 v adresáři: C:\Users\brouzda\Documents\Arduino\libraries\Tasker 
*/
 
