#include "menu.h"



#define MENU_FONT 4
#define MENU_VELIKOST_RADKU 26
#define DISPLAY_RADKU 4
#define NADPIS_X 5
#define RADEK_X 15
#define ZNACKA_X 2

/**
 * Vykresleni menu
 */ 
void renderMenu()
{
  if( currentMenu==NULL ) return;

  // zpracujeme klavesnici
  while( keyAvail() ) {
    char ch = getKey();
    if( ch=='u' ) {
        if( currentMenu->curPos>0 ) {
          currentMenu->curPos--;
        } else {
          currentMenu->curPos = currentMenu->ct - 1;
        }
    } else if( ch=='d' ) {
        if( currentMenu->curPos < (currentMenu->ct-1) ) {
          currentMenu->curPos++;
        } else {
            currentMenu->curPos = 0;
        }
    } else if( ch=='l' ) {
        currentMenu->state = -1;
    } else if( ch=='r' ) {
        currentMenu->state = currentMenu->codes[currentMenu->curPos];
    }
    currentMenu->redraw = true;
  } // while( keyAvail() ) {

  // renderujeme menu
  if( currentMenu->redraw ) {

    tft.setRotation(3);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM); // kresli se od top-left

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString(currentMenu->nadpis, NADPIS_X, 0, MENU_FONT );

    int od = 0;
    if( currentMenu->curPos >= DISPLAY_RADKU-1 ) {
      od = currentMenu->curPos - 2;
    }
    if( od > currentMenu->ct - 4 ) {
      od = currentMenu->ct - 4;
    }
    if( od<0 ) {
      od = 0;
    }

    for( int i = 0; i < DISPLAY_RADKU; i++ ) {
        int activeRow = i + od;
        if( activeRow==currentMenu->ct ) break;
        
        // pridame k textu z obou stran mezery, aby byla oznacena polozka oznacena hezky
        char buffer[100];
        sprintf( buffer, " %s ", 
            currentMenu->items[activeRow] 
            );

        if( currentMenu->state == currentMenu->codes[activeRow] ) { 
          tft.setTextColor(TFT_RED, TFT_YELLOW);    // barva ZVOLENE polozky (pri kliknuti VPRAVO)
        } else  if( currentMenu->curPos == activeRow ) {    
          tft.setTextColor(TFT_BLACK, TFT_WHITE);   // barva oznacene polozky (kde je kurzor)    
        } else {                                            
          tft.setTextColor(TFT_GREEN, TFT_BLACK);   // bezna polozka
        }
        if( currentMenu->active==currentMenu->codes[activeRow] ) {
          // pro PREDVOLENOU (drive aktivni) polozku se pred ni ukaze znacka ">"
          tft.drawString(">", ZNACKA_X, MENU_VELIKOST_RADKU + i*MENU_VELIKOST_RADKU, MENU_FONT );
        }
        tft.drawString(buffer, RADEK_X, MENU_VELIKOST_RADKU + i*MENU_VELIKOST_RADKU, MENU_FONT );
    } // for( int i = 0; i < DISPLAY_RADKU; i++ )

    currentMenu->redraw = false;
  } // if( currentMenu->redraw )

}


/**
 * Uvodni vykresleni stranky pro limit low/high
 */
void limitPgFullDraw()
{
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM); // kresli se od top-left
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("Limit low/high", NADPIS_X, 0, MENU_FONT );
  tft.setTextColor(TFT_WHITE, TFT_BLACK);   
  tft.drawString("Rozhrani mezi" , NADPIS_X, MENU_VELIKOST_RADKU, MENU_FONT );
  tft.drawString("nizkou a vysokou" , NADPIS_X, 2*MENU_VELIKOST_RADKU, MENU_FONT );
  tft.drawString("spotrebou." , NADPIS_X, 3*MENU_VELIKOST_RADKU, MENU_FONT );
}

/**
 * Vykresli zadavanou hodnotu 
 */ 
void LimitPgNumber( char * value, int cursorPos, int maxPos, char * append )
{
  int pos_x = RADEK_X;
  int pos_y = 4*MENU_VELIKOST_RADKU;
  char bafr[10];

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  for( int i = 0; i<cursorPos; i++ ) {
    sprintf( bafr, "%c", value[i] );
    pos_x += tft.drawString( bafr , pos_x, pos_y, MENU_FONT );
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  sprintf( bafr, "%c", value[cursorPos] );
  pos_x += tft.drawString( bafr , pos_x, pos_y, MENU_FONT );
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  for( int i = cursorPos+1; i<maxPos; i++ ) {
    sprintf( bafr, "%c", value[i] );
    pos_x += tft.drawString( bafr , pos_x, pos_y, MENU_FONT );
  }
  pos_x += tft.drawString( " " , pos_x, pos_y, MENU_FONT );
  pos_x += tft.drawString( append , pos_x, pos_y, MENU_FONT );
  pos_x += tft.drawString( "  " , pos_x, pos_y, MENU_FONT );
}
