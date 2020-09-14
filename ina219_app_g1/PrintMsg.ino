/**
 * Pomocny nastroj pro tisk zprav.
 */
void printMsg( int color, const char * text1, const char * text2 )
{
  Serial.printf( "msg: '%s' '%s'\n", text1, text2 );
  
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM); // kresli se od top-left
  tft.setTextColor( color , TFT_BLACK);

  tft.setCursor( 0, 40  );
  tft.print( text1 );
  if( text2!=NULL ) {
    tft.setCursor( 0, 90  );
    tft.print( text2 );
  }
}
