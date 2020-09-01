
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
 * formatNumber( 10.223456, "mWh", "Wh" ) -> "10.2 mWh "
 * formatNumber( 512.223456, "mWh", "Wh" ) -> "512 mWh "
 * formatNumber( 5912.223456, "mWh", "Wh" ) -> "5.91 Wh "
 * formatNumber( 5912.223456, "mWh", "Wh", false ) -> "5.91 Wh"
 * 
 * Vraci odkaz do statickeho bufferu.
 */
char * formatNumber( double number, const char * jednotkaBase, const char * jednotkaK, bool addSpace=true )
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
    sprintf( numBuffer, "%.0f %s", (number/1000.0), jednotkaK ); // >100 000 je 123 Wh
  } 
  if( addSpace ) {
    strcat( numBuffer, " " );
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
    Serial.printf( "total %f mWh per %d s \t ", vals.total_uWh/1000.0), vals.total_ms/1000 );
    Serial.printf( "avg %f mW \t", vals.avg_power ); 
    Serial.printf( "total %f mAh \t", (getCal(vals.total_uAh)/1000.0) ); 
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
            (char*)"avg power:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.avg_power, "mW", "W" )
            );            
}


void print_page1_half2()
{
  char buff[32];

  tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"time:" );

    sprintf( buff, "%d s", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            buff );
  
  tftPrint( TFT_WHITE, 1,
            X_COL_2,
            1, 1,
            (char*)"max current {min):" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            2, 1,
            formatNumber( vals.maxCurrentMin, "mA", "A" )
            );


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            2, 2,
            (char*)"max current {total):" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.maxCurrentTotal, "mA", "A" )
            );


   tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"capacity:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.total_uAh/1000.0, "mAh", "Ah" )
            );        
   
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

    char bafr[32];
    sprintf( bafr, "%d s   ", time(NULL)-vals.minVoltageTime );
    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            3, 3,
            (char*)"min voltage time:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            4, 3,
            bafr
            );    

}


void print_page2_half2()
{
  
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"time:" );

    char buff[32];
    sprintf( buff, "%d s ", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            buff );
  

    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            1, 1,
            (char*)"min voltage {total):" );

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
            formatNumber( vals.maxVoltageTotal*1000.0, "mV", "V" )
            );


    sprintf( buff, "%d s   ", time(NULL)-vals.maxVoltageTime );
    
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"max voltage time:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            buff
            );   
    
}


void print_page3_half1()
{
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            0, 0,
            (char*)"current:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.current_mA, "mA", "A" )
            );

    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            2, 2,
            (char*)"max current {min):" );

    tftPrint( TFT_RED, 2,
            X_COL_1+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.maxCurrentMin, "mA", "A" )
            ); 


}


void print_page3_half2()
{
  tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"time:" );

    char buff[32];
    sprintf( buff, "%d s ", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            buff );
  

    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            2, 2,
            (char*)"max current {total):" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.maxCurrentTotal, "mA", "A" )
            );

    sprintf( buff, "%d s   ", time(NULL)-vals.maxCurrentTime );
    
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"max current time:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            buff
            );   
   
}



void print_page4_half1()
{
    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            0, 0,
            (char*)"current:" );

    tftPrint( 
            vals.current_mA>lowHighThreshold ? TFT_RED : TFT_YELLOW,
            2,
            X_COL_1+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.current_mA, "mA", "A" )
            );

    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            1, 1,
            (char*)"low power time:" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            2, 1,
            formatNumber( (float)(vals.lowPowerTime), "ms", "s" )
            );


    
    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            2, 2,
            (char*)"low power capa:" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.lowPower_uAh/1000.0, "mAh", "Ah"  )
            );    


    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            3, 3,
            (char*)"low power:" );

    tftPrint( TFT_YELLOW, 2,
            X_COL_1+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.lowPower_uWh/1000.0, "mWh", "Wh"  )
            ); 
   
}


void print_page4_half2()
{
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"time:" );

    char buff[32];
    sprintf( buff, "%d s ", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            buff );


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            1, 1,
            (char*)"high power time:" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            2, 1,
            formatNumber( (float)(vals.highPowerTime), "ms", "s" )
            );

    
    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            2, 2,
            (char*)"high power capa:" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            3, 2,
            formatNumber( vals.highPower_uAh/1000.0, "mAh", "Ah"  )
            );  


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"high power:" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.highPower_uWh/1000.0, "mWh", "Wh"  )
            );             
}

void print_page5_half1()
{

    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            0, 0,
            (char*)"current:" );

    tftPrint( 
            vals.current_mA>lowHighThreshold ? TFT_RED : TFT_YELLOW, 
            2,
            X_COL_1+X_OFF_NADPIS,
            1, 0,
            formatNumber( vals.current_mA, "mA", "A" )
            );    


    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            1, 1,
            (char*)"high power events:" );

    char bafr[32];
    sprintf( bafr, "%d", vals.highPowerEvents );
    
    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            2, 1,
            bafr
            );


    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            2, 2,
            (char*)"high power time:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            3, 2,
            formatNumber( (float)(vals.highPowerTime), "ms", "s" )
            );     


    tftPrint( TFT_WHITE, 1,
            X_COL_1,
            3, 3,
            (char*)"last HPE length:" );

    tftPrint( TFT_GREEN, 2,
            X_COL_1+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.lastHighPowerEventLengthMsec, "ms", "s"  )
            );               
}


void print_page5_half2()
{

    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            0, 0,
            (char*)"time:" );

    char buff[32];
    sprintf( buff, "%d s ", vals.meteringTime/1000 );
          
    tftPrint( TFT_GREEN, 2,
            X_COL_2+X_OFF_NADPIS,
            1, 0,
            buff );


    if( vals.highPowerEvents>0 ) {
      tftPrint( TFT_WHITE, 1,
              X_COL_2,
              1, 1,
              (char*)"last HPE age:" );
  
      sprintf( buff, "%d s ", (millis()-vals.lastHighPowerEventStartMsec)/1000 );
      
      tftPrint( TFT_GREEN, 2,
              X_COL_2+X_OFF_NADPIS,
              2, 1,
              buff
              );
    }
    

    if( vals.highPowerEvents>0 ) {
      tftPrint( TFT_WHITE, 1,
              X_COL_2,
              2, 2,
              (char*)"avg HPE length:" );
  
      tftPrint( TFT_GREEN, 2,
              X_COL_2+X_OFF_NADPIS,
              3, 2,
              formatNumber( ((double)vals.highPowerTime)/((double)vals.highPowerEvents), "ms", "s" )
              );     
    }


    tftPrint( TFT_WHITE, 1,
            X_COL_2,
            3, 3,
            (char*)"max HPE length:" );

    tftPrint( TFT_RED, 2,
            X_COL_2+X_OFF_NADPIS,
            4, 3,
            formatNumber( vals.maxHighPowerEventLengthMsec, "ms", "s"  )
            );               
}


#define X_FOOTER_COL_1 0
#define X_FOOTER_COL_2 60
#define X_FOOTER_COL_3 130
#define X_FOOTER_COL_4 190


void print_page_footer()
{
    if( coMerim==11 ) {
      tftPrint( TFT_GREEN, 1,
            X_FOOTER_COL_1,
            4, 4,
            (char*)"[load]" );
    } else {
      tftPrint( TFT_YELLOW, 1,
            X_FOOTER_COL_1,
            4, 4,
            (char*)"[source]" );
    }

    if( rozliseni==21 ) {
      tftPrint( TFT_GREEN, 1,
            X_FOOTER_COL_2,
            4, 4,
            (char*)"[32V-2A]" );
    } else if( rozliseni==22 ) {
      tftPrint( TFT_WHITE, 1,
            X_FOOTER_COL_2,
            4, 4,
            (char*)"[32V-1A]" );
    } else {
      tftPrint( TFT_YELLOW, 1,
            X_FOOTER_COL_2,
            4, 4,
            (char*)"[16V-0.4A]" );
    }

    char bafr[32];
    sprintf( bafr, "[%.0f mA]", lowHighThreshold );
    tftPrint( TFT_GREEN, 1,
            X_FOOTER_COL_3,
            4, 4,
            bafr );

    sprintf( bafr, "[%d]", displayedPage );
    tftPrint( TFT_GREEN, 1,
            X_FOOTER_COL_4,
            4, 4,
            bafr );            
    
}
