
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

/**
 * Zmeni text menu. Predava se POZICE, ne kod polozky. 
 */ 
void Menu::updateText( int pos, const char * text )
{
  if( pos>=MAX_MENU_ITEMS) return;
  strcpy( this->items[pos], (const char*)text );
}

/**
 * Prida k menu novou polozku.
 * Kod polozky musi byt vetsi nez nula.
 */ 
void Menu::addItem( const char * text, int code )
{
  if( this->ct==MAX_MENU_ITEMS) return;
  
  strcpy( this->items[this->ct], (const char*)text );
  this->codes[this->ct] = code;
  this->ct++;
}

/**
 * Nastavi pozici kurzoru. Predava se POZICE, ne kod.
 */ 
void Menu::setPos( int pos )
{
  this->curPos = pos;
}

/**
 * Vrati stav menu:
 * - 0 = zatim nic nezvoleno
 * - -1 = zvoleno BACK (sipka vlevo)
 * - jine = kod zvolene volby
 */ 
int Menu::getResult()
{
  return this->state;
}

/**
 * Smaze zvolenou pozici, vynuti vykresleni menu.
 */ 
void Menu::clearState()
{
  this->state = 0;
  this->redraw = true;
}

/**
 * Oznaci PREDVOLENOU (drive aktivni) polozku.
 * Jako parametr se posila kod, ne poradi.
 */ 
void Menu::setActive( int code )
{
  this->active = code;
  for( int i = 0; i<this->ct; i++ ) {
    if( this->codes[i] == code ) {
      this->curPos = i;
    }
  }
}
