
#include "Menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Menu::Menu( const char * nadpis )
{
    this->nadpis = (char*)nadpis;
    this->ct = 0;
    this->curPos = 0;
    this->state = 0;
    this->redraw = true;
    this->active = -1;
}

void Menu::updateText( int pos, const char * text )
{
    if( pos>=MAX_MENU_ITEMS) return;
    strcpy( this->items[pos], (const char*)text );
}

void Menu::addItem( const char * text, int code )
{
    if( this->ct==MAX_MENU_ITEMS) return;
    
    strcpy( this->items[this->ct], (const char*)text );
    this->codes[this->ct] = code;
    this->ct++;
}

void Menu::setPos( int pos )
{
    this->curPos = pos;
}

int Menu::getResult()
{
    return this->state;
}

void Menu::clearState()
{
  this->state = 0;
  this->redraw = true;
}

void Menu::setActive( int code )
{
    this->active = code;
    for( int i = 0; i<this->ct; i++ ) {
      if( this->codes[i] == code ) {
        this->curPos = i;
      }
    }
}
