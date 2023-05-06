//#include <LiquidCrystal.h>
//
//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
//
//void setup() {
//
//  lcd.begin(16, 2);
//  lcd.print("I am!");
//  
//}
//
//void loop() {
//  lcd.setCursor(0, 1);
//  lcd.print(millis()/1000);
//
//}
#include <SimpleDHT.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 2;
SimpleDHT11 dht11(pinDHT11);

void setup() {
  lcd.begin(16, 2);
  //lcd.print("hello world");
  Serial.begin(9600);
}

void loop() {
  // start working...
  Serial.println("=================================");
  Serial.println("Sample DHT11...");
  
  lcd.setCursor(0, 1);
//  lcd.print(millis()/1000);
  
  
  // read without samples.
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
    Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    //lcd.print("Read DHT11 failed, err="); lcd.print(SimpleDHTErrCode(err));
    //Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
    
    return;
    
  }
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" H");
  
  lcd.print((int)temperature); lcd.print(" C*, "); 
  lcd.print((int)humidity); lcd.print("% H");

  
  // DHT11 sampling rate is 1HZ.
  delay(1500);
}
