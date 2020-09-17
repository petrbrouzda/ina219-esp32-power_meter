/*
 * I2C scanner from https://diyi0t.com/i2c-tutorial-for-arduino-and-esp8266/
 */

#include "Wire.h"

#define I2C_SDA 25
#define I2C_SCL 26


void setup(){
  Serial.begin(115200); 
  while(!Serial){} // Waiting for serial connection
 
  Serial.println();
  Serial.println("Start I2C scanner ...");
  Serial.print("\r\n");
  byte count = 0;
  
    // https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
  Wire.begin(I2C_SDA, I2C_SCL);

  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0)
      {
      Serial.print("Found I2C Device: ");
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      delay(1);
      }
  }
  Serial.print("\r\n");
  Serial.println("Finish I2C scanner");
  Serial.print("Found ");
  Serial.print(count, HEX);
  Serial.println(" Device(s).");
}


void loop() {
  // put your main code here, to run repeatedly:

}
