
#include "values.h"

// @config vypisovat vsechny namerene hodnoty do serial portu?
// #define PRINT_VALUES_SERIAL

#define X_COL_1 0
#define X_COL_2 120
#define X_OFF_NADPIS 5

#define STATUS2_OFFSET 80

#define NAPIS_MALY_VYSKA 12
#define NAPIS_VELKY_VYSKA 20


/**
 * Nastroje pro tisk jednotlivych mericich obrazovek.
 */

void tftPrint( int color, int textSize,
        int pos_x,
        int pos_y_malych_napisu, int pos_y_velkych_napisu,
        char * buff )
{
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextSize(textSize);
        int pos_y = pos_y_malych_napisu*NAPIS_MALY_VYSKA + pos_y_velkych_napisu*NAPIS_VELKY_VYSKA;
        tft.setCursor( pos_x, pos_y );
        tft.print( buff );
}


char numBuffer[32];

/**          
 * Formatuje cislo s jednotkou na 2-3 platna mista.
 * Pokud je cislo vetsi nez 100, zmensi ho 1000x a pouzije jednotku K.
 * 
 * formatNumber( 10.223456, "mWh", "Wh" ) -> "10.2 mWh"
 * formatNumber( 512.223456, "mWh", "Wh" ) -> "512 mWh"
 * formatNumber( 5912.223456, "mWh", "Wh" ) -> "5.91 Wh"
 * 
 * Vraci odkaz do statickeho bufferu.
 */
char * formatNumber( double number, const char * jednotkaBase, const char * jednotkaK )
{
  if( number < 1.0 ) {
    sprintf( numBuffer, "%.02f %s", number, jednotkaBase );   // do 1.0 je to 0.12 mWh
  } else if( number < 10.0 ) {
    sprintf( numBuffer, "%.02f %s", number, jednotkaBase );   // do 10.0 je to 1.23 mWh
  } else if( number < 100.0 ) {
    sprintf( numBuffer, "%.01f %s", number, jednotkaBase );   // do 100.0 je to 12.3 mWh
  } else if( number < 1000.0 ) {
    sprintf( numBuffer, "%.0f %s", number, jednotkaBase ); // 100-1000 je 123 mWh
  } else if( number < 10000.0 ) {
    sprintf( numBuffer, "%.02f %s", (number/1000.0), jednotkaK ); // 1 000-10 000 je 1.23 Wh
  } else if( number < 100000.0 ) { 
    sprintf( numBuffer, "%.01f %s", (number/1000.0), jednotkaK ); // 10 000 - 100 000 je 12.3 Wh
  } else {
    sprintf( numBuffer, "%.0f %s", number, jednotkaK ); // >100 000 je 123 Wh
  } 
  return numBuffer;
}

void print_dumpValuesToSerial()
{
  #ifdef PRINT_VALUES_SERIAL
    // POZOR! Vypis vseho na serial trva 7 msec, tim se omezuje schopnost zmerit drobne fluktuace!
    Serial.printf( "current %.02f mA \t", vals.current_mA); 
    Serial.printf( "source %.02f V  %.02f mW \t", vals.source, vals.source_mW); 
    Serial.printf( "load %.02f V  %.02f mW  %.02f %% \t", vals.busvoltage, vals.power_mW, (100.0-vals.shunt_pomer) ); 
    Serial.printf( "shunt %.02f mV  %.02f mW  %.02f %% \n", vals.shuntvoltage, vals.shunt_power_mW, vals.shunt_pomer); 
    Serial.printf( "    energy %f uWh per %d ms \t ", vals.energy_uWh, vals.interval_ms );
    Serial.printf( "total %f mWh per %d s \t ", (vals.total_uWh/1000.0), vals.total_ms/1000 );
    Serial.printf( "avg %f mW \t", vals.avg_power ); 
    Serial.printf( "total %f mAh \t", (vals.total_uAh/1000.0) ); 
    Serial.println("");
    // uz to 7 msec bezelo, znovu zmerime hodnoty
    doMeter();
  #endif
}


void print_page1_half1()
{
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            0, 0,
            (char*)"current:" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.current_mA, "mA", "A" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            1, 1,
            (char*)"voltage:" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            2, 1,
            formatNumber( vals.outVoltage*1000.0, "mV", "V" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            2, 2,
            (char*)"power:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.outPower, "mW", "W" )
            );    



    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            3, 3,
            (char*)"capacity:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            4, 3,
            formatNumber( (vals.total_uAh/1000.0), "mAh", "Ah" )
            );        
}


void print_page1_half2()
{
  char buff[32];
  tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"max current {min):" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.maxCurrentMin, "mA", "A" )
            );


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            1, 1,
            (char*)"max current {total):" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            2, 1,
            formatNumber( vals.maxCurrentTotal, "mA", "A" )
            );


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            2, 2,
            (char*)"avg power:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.avg_power, "mW", "W" )
            );


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"time:" );

    sprintf( buff, "%d s", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            buff );
}



void print_page2_half1()
{
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            0, 0,
            (char*)"voltage (actual):" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.outVoltage*1000.0, "mV", "V" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            1, 1,
            (char*)"min voltage (min):" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            2, 1,
            formatNumber( vals.minVoltageMin*1000.0, "mV", "V" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            2, 2,
            (char*)"max voltage (min):" );

    tftPrint( TFT_RED, 2,
            X_COL_1+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.maxVoltageMin*1000.0, "mV", "V"  )
            );    


   
}


void print_page2_half2()
{
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            1, 1,
            (char*)"min voltage (total):" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_2+X_OFF_NADPIS,
            2, 1,
            formatNumber( vals.minVoltageTotal*1000.0, "mV", "V" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            2, 2,
            (char*)"max voltage (total):" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.maxVoltageTotal*1000.0, "mV", "V"  )
            );  
}


void print_page_footer()
{

    if( coMerim==11 ) {
      tftPrint( TFT_GREEN, 1,
            X_COL_1,
            4, 4,
            (char*)"[spotrebic]" );
    } else {
      tftPrint( TFT_YELLOW, 1,
            X_COL_1,
            4, 4,
            (char*)"[zdroj]" );
    }

    if( rozliseni==21 ) {
      tftPrint( TFT_GREEN, 1,
            STATUS2_OFFSET+X_COL_1,
            4, 4,
            (char*)"[32V-2A]" );
    } else if( rozliseni==22 ) {
      tftPrint( TFT_WHITE, 1,
            STATUS2_OFFSET+X_COL_1,
            4, 4,
            (char*)"[32V-1A]" );
    } else {
      tftPrint( TFT_YELLOW, 1,
            STATUS2_OFFSET+X_COL_1,
            4, 4,
            (char*)"[16V-0.4A]" );
    }
}
