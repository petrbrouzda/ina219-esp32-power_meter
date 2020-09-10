/**
 * Obsluha klavesnice. 
 * Klavesy se sbiraji a debouncuji v ISR,
 * ukladaji se do 32znakoveho bufferu, ze ktereho je muze aplikace po jedne klavese odebirat.
 */

#include "keyboard.h"


/*
 * !!! Vsechny veci volane v ISR by mely mit tag IRAM_ATTR !!!
 */


#define KEYBUFF_SIZE 32
char keyBuffer[KEYBUFF_SIZE+1];
int keyPos = 0;

// debounce cas pro tlacitka, msec
#define DEBOUNCE_TIME 100


/**
 * Ceka nejaky stisk klavesy?
 */ 
bool keyAvail() 
{
    return keyPos!=0;
}

/**
 * Dej mi prvni cekajici klavesu.
 */ 
char getKey()
{
    if( ! keyAvail() ) return '*';
    noInterrupts();
        char rc = keyBuffer[0];
        strcpy( keyBuffer, keyBuffer+1 );
        keyPos--;
    interrupts();
    return rc;
}

void IRAM_ATTR addKeyPress( char c )
{
    if( keyPos >= KEYBUFF_SIZE ) return;
    noInterrupts();
        keyBuffer[keyPos++] = c;
        keyBuffer[keyPos] = 0;
    interrupts();
}

void IRAM_ATTR buttonChange( Button * button )
{
    long tnow = millis();
    if( (tnow - button->debounceTimeStarted) < DEBOUNCE_TIME ) {
        // nedelame nic 
        return;
    }
    button->debounceTimeStarted = tnow;
    button->currentState = button->currentState==HIGH ? LOW : HIGH;
    if( button->currentState==LOW ) {
        addKeyPress( button->bChar );
    }
}

/* ISR pro jednotliva tlacitka */

void IRAM_ATTR isrButtonR() {
    buttonChange( &buttonR );
}

void IRAM_ATTR isrButtonL() {
    buttonChange( &buttonL );
}

void IRAM_ATTR isrButtonU() {
    buttonChange( &buttonU );
}

void IRAM_ATTR isrButtonD() {
    buttonChange( &buttonD );
}
