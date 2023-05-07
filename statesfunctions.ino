void state0() {
  //disabled
  lcd.setCursor(0, 0);
  lcd.print("machine off");

  //turn off fan

}

void state1() {
  //idle
  //turn on green light and turn off fan
  //display temp and humidity to LCD
}

void state2() {
  //running
  //turn on fan and blue light
  //display temp and humidity to LCD
}

void state3() {
  //error
  //turn everything off and turn on red light
  //display water level 
}