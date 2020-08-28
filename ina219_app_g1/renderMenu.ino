#include "Menu.h"

#define MENU_FONT 4
#define MENU_VELIKOST_RADKU 26
#define DISPLAY_RADKU 4
#define NADPIS_X 5
#define RADEK_X 15
#define ZNACKA_X 2

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
        
        char buffer[100];
        sprintf( buffer, " %s ", 
            currentMenu->items[activeRow] 
            );

        if( currentMenu->state == currentMenu->codes[activeRow] ) {
          tft.setTextColor(TFT_RED, TFT_YELLOW);
        } else  if( currentMenu->curPos == activeRow ) {
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
        }
        if( currentMenu->active==currentMenu->codes[activeRow] ) {
            tft.drawString(">", ZNACKA_X, MENU_VELIKOST_RADKU + i*MENU_VELIKOST_RADKU, MENU_FONT );
        }
        tft.drawString(buffer, RADEK_X, MENU_VELIKOST_RADKU + i*MENU_VELIKOST_RADKU, MENU_FONT );
    }

    currentMenu->redraw = false;
  }

}
